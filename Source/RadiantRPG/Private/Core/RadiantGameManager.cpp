// Private/Core/RadiantGameManager.cpp
// Game manager implementation for RadiantRPG - central coordinator for all major game systems

#include "Core/RadiantGameManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameplayTagsManager.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/DateTime.h"
#include "GenericPlatform/GenericPlatformMemory.h"
#include "Stats/StatsHierarchical.h"

// Forward declared system includes - uncomment when systems are implemented
// #include "World/RadiantWorldManager.h"
// #include "World/RadiantZoneManager.h"
// #include "Factions/RadiantFactionManager.h"
// #include "Economy/RadiantEconomyManager.h"
// #include "Story/RadiantStoryManager.h"

URadiantGameManager::URadiantGameManager()
{
    // Initialize core state
    CurrentGameState = EGameState::None;
    PreviousGameState = EGameState::None;
    
    // Initialize settings with defaults
    CurrentGameSettings = FGameSettings();
    
    // Initialize game state data
    GameStateData = FGameStateData();
    
    // Initialize system references
    //WorldManager = nullptr;
    //ZoneManager = nullptr;
    //FactionManager = nullptr;
    //EconomyManager = nullptr;
    //StoryManager = nullptr;
    
    // Initialize internal state
    bIsInitialized = false;
    bIsShuttingDown = false;
    
    UE_LOG(LogTemp, Log, TEXT("RadiantGameManager constructed"));
}

void URadiantGameManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    UE_LOG(LogTemp, Log, TEXT("RadiantGameManager initializing..."));
    
    // Set initial game state
    SetGameState(EGameState::MainMenu);
    
    // Initialize game systems
    InitializeGameSystems();
    
    // Setup auto-save
    SetupAutoSave();
    
    bIsInitialized = true;
    
    UE_LOG(LogTemp, Log, TEXT("RadiantGameManager initialization complete"));
}

void URadiantGameManager::Deinitialize()
{
    UE_LOG(LogTemp, Log, TEXT("RadiantGameManager deinitializing..."));
    
    bIsShuttingDown = true;
    
    // Clear auto-save timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AutoSaveTimerHandle);
    }
    
    // Shutdown game systems
    ShutdownGameSystems();
    
    Super::Deinitialize();
    
    UE_LOG(LogTemp, Log, TEXT("RadiantGameManager deinitialized"));
}

void URadiantGameManager::Tick(float DeltaTime)
{
    if (!bIsInitialized || bIsShuttingDown)
    {
        return;
    }
    
    // Update game time if in playable state
    if (IsInPlayableState())
    {
        UpdateGameTime(DeltaTime);
    }
}

bool URadiantGameManager::IsTickable() const
{
    return bIsInitialized && !bIsShuttingDown;
}

TStatId URadiantGameManager::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(URadiantGameManager, STATGROUP_Tickables);
}

// === GAME STATE INTERFACE ===

void URadiantGameManager::SetGameState(EGameState NewState)
{
    if (CurrentGameState == NewState)
    {
        return;
    }
    
    EGameState OldState = CurrentGameState;
    PreviousGameState = CurrentGameState;
    CurrentGameState = NewState;
    
    UE_LOG(LogTemp, Log, TEXT("Game state changed from %s to %s"), 
           *UEnum::GetValueAsString(OldState), 
           *UEnum::GetValueAsString(NewState));
    
    // Handle state transition
    HandleGameStateChange(OldState, NewState);
    
    // Broadcast events
    OnGameStateChanged.Broadcast(OldState, NewState);
    OnGameStateChangedBP(OldState, NewState);
}

bool URadiantGameManager::IsInPlayableState() const
{
    return CurrentGameState == EGameState::InGame;
}

// === DIFFICULTY AND SETTINGS ===

void URadiantGameManager::SetDifficultyLevel(EDifficultyLevel NewDifficulty)
{
    if (CurrentGameSettings.DifficultyLevel == NewDifficulty)
    {
        return;
    }
    
    EDifficultyLevel OldDifficulty = CurrentGameSettings.DifficultyLevel;
    CurrentGameSettings.DifficultyLevel = NewDifficulty;
    
    UE_LOG(LogTemp, Log, TEXT("Difficulty changed from %s to %s"), 
           *UEnum::GetValueAsString(OldDifficulty), 
           *UEnum::GetValueAsString(NewDifficulty));
    
    // Broadcast events
    OnDifficultyChanged.Broadcast(NewDifficulty);
    OnDifficultyChangedBP(NewDifficulty);
}

void URadiantGameManager::ApplyGameSettings(const FGameSettings& NewSettings)
{
    FGameSettings OldSettings = CurrentGameSettings;
    CurrentGameSettings = NewSettings;
    
    UE_LOG(LogTemp, Log, TEXT("Game settings updated"));
    
    // Apply settings to various systems
    // TODO: Apply settings to world manager, audio system, etc.
    
    // Broadcast events
    OnGameSettingsChanged.Broadcast(NewSettings);
    OnGameSettingsChangedBP(NewSettings);
}

// === GLOBAL STATE MANAGEMENT ===

void URadiantGameManager::SetGlobalFlag(FGameplayTag Flag, bool bValue)
{
    if (!Flag.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("GameManager: Invalid flag provided"));
        return;
    }
    
    bool bPreviousValue = GameStateData.GlobalFlags.HasTag(Flag);
    
    if (bValue)
    {
        if (!bPreviousValue)
        {
            GameStateData.GlobalFlags.AddTag(Flag);
            OnGlobalFlagChanged.Broadcast(Flag);
            OnGlobalFlagChangedBP(Flag, bValue);
            
            UE_LOG(LogTemp, Log, TEXT("GameManager: Global flag set: %s"), *Flag.ToString());
        }
    }
    else
    {
        if (bPreviousValue)
        {
            GameStateData.GlobalFlags.RemoveTag(Flag);
            OnGlobalFlagChanged.Broadcast(Flag);
            OnGlobalFlagChangedBP(Flag, bValue);
            
            UE_LOG(LogTemp, Log, TEXT("GameManager: Global flag removed: %s"), *Flag.ToString());
        }
    }
}

bool URadiantGameManager::HasGlobalFlag(FGameplayTag Flag) const
{
    return Flag.IsValid() && GameStateData.GlobalFlags.HasTag(Flag);
}

void URadiantGameManager::RemoveGlobalFlag(FGameplayTag Flag)
{
    SetGlobalFlag(Flag, false);
}

void URadiantGameManager::SetGlobalVariable(const FString& VariableName, float Value)
{
    if (VariableName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("GameManager: Empty variable name provided"));
        return;
    }
    
    float* ExistingValue = GameStateData.GlobalVariables.Find(VariableName);
    if (!ExistingValue || !FMath::IsNearlyEqual(*ExistingValue, Value))
    {
        GameStateData.GlobalVariables.Add(VariableName, Value);
        
        UE_LOG(LogTemp, Verbose, TEXT("GameManager: Global variable '%s' set to %f"), 
               *VariableName, Value);
    }
}

float URadiantGameManager::GetGlobalVariable(const FString& VariableName, float DefaultValue) const
{
    if (const float* Value = GameStateData.GlobalVariables.Find(VariableName))
    {
        return *Value;
    }
    
    return DefaultValue;
}

void URadiantGameManager::SetGlobalString(const FString& VariableName, const FString& Value)
{
    if (VariableName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("GameManager: Empty string variable name provided"));
        return;
    }
    
    const FString* ExistingValue = GameStateData.GlobalStrings.Find(VariableName);
    if (!ExistingValue || !ExistingValue->Equals(Value))
    {
        GameStateData.GlobalStrings.Add(VariableName, Value);
        
        UE_LOG(LogTemp, Verbose, TEXT("GameManager: Global string '%s' set to '%s'"), 
               *VariableName, *Value);
    }
}

FString URadiantGameManager::GetGlobalString(const FString& VariableName, const FString& DefaultValue) const
{
    if (const FString* Value = GameStateData.GlobalStrings.Find(VariableName))
    {
        return *Value;
    }
    
    return DefaultValue;
}

// === SAVE/LOAD INTERFACE ===

bool URadiantGameManager::SaveGame(const FString& SaveSlotName)
{
    if (SaveSlotName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("GameManager: Empty save slot name provided"));
        return false;
    }
    
    // TODO: Implement save system when available
    UE_LOG(LogTemp, Log, TEXT("GameManager: Save game requested for slot '%s' (not implemented yet)"), *SaveSlotName);
    
    return false;
}

bool URadiantGameManager::LoadGame(const FString& SaveSlotName)
{
    if (SaveSlotName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("GameManager: Empty save slot name provided"));
        return false;
    }
    
    // TODO: Implement load system when available
    UE_LOG(LogTemp, Log, TEXT("GameManager: Load game requested for slot '%s' (not implemented yet)"), *SaveSlotName);
    
    return false;
}

bool URadiantGameManager::DoesSaveSlotExist(const FString& SaveSlotName) const
{
    if (SaveSlotName.IsEmpty())
    {
        return false;
    }
    
    // TODO: Check if save file exists
    return UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0);
}

TArray<FString> URadiantGameManager::GetAvailableSaveSlots() const
{
    TArray<FString> SaveSlots;
    
    // TODO: Enumerate save files in save directory
    UE_LOG(LogTemp, Log, TEXT("GameManager: Get available save slots (not implemented yet)"));
    
    return SaveSlots;
}

bool URadiantGameManager::DeleteSaveSlot(const FString& SaveSlotName)
{
    if (SaveSlotName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("GameManager: Empty save slot name provided"));
        return false;
    }
    
    // TODO: Delete save file
    UE_LOG(LogTemp, Log, TEXT("GameManager: Delete save slot '%s' (not implemented yet)"), *SaveSlotName);
    
    return false;
}

// === UTILITY FUNCTIONS ===

void URadiantGameManager::ForceGarbageCollection()
{
    UE_LOG(LogTemp, Log, TEXT("GameManager: Forcing garbage collection"));
    
    GEngine->ForceGarbageCollection(true);
}

float URadiantGameManager::GetMemoryUsageMB() const
{
    FPlatformMemoryStats MemStats = FPlatformMemory::GetStats();
    return MemStats.UsedPhysical / (1024.0f * 1024.0f);
}

void URadiantGameManager::ResetAllSystems()
{
    UE_LOG(LogTemp, Log, TEXT("GameManager: Resetting all systems"));
    
    // Reset game state data
    GameStateData = FGameStateData();
    
    // Reset settings to defaults
    CurrentGameSettings = FGameSettings();
    
    // TODO: Reset all subsystems when implemented
    /*
    if (WorldManager)
    {
        // WorldManager->ResetToDefaults();
    }
    
    if (ZoneManager)
    {
        // ZoneManager->ResetToDefaults();
    }
    
    if (FactionManager)
    {
        // FactionManager->ResetToDefaults();
    }
    
    if (EconomyManager)
    {
        // EconomyManager->ResetToDefaults();
    }
    
    if (StoryManager)
    {
        // StoryManager->ResetToDefaults();
    }
    */
    UE_LOG(LogTemp, Log, TEXT("GameManager: All systems reset complete"));
}

// === INTERNAL FUNCTIONS ===

void URadiantGameManager::InitializeGameSystems()
{
    UE_LOG(LogTemp, Log, TEXT("GameManager: Initializing game systems..."));
    
    // Get subsystem references - uncomment when systems are implemented
    /*
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        WorldManager = GameInstance->GetSubsystem<URadiantWorldManager>();
        ZoneManager = GameInstance->GetSubsystem<URadiantZoneManager>();
        FactionManager = GameInstance->GetSubsystem<URadiantFactionManager>();
        EconomyManager = GameInstance->GetSubsystem<URadiantEconomyManager>();
        StoryManager = GameInstance->GetSubsystem<URadiantStoryManager>();
    }
    */
    
    // Initialize systems in dependency order
    /*
    if (WorldManager)
    {
        WorldManager->Initialize();
    }
    
    if (ZoneManager)
    {
        ZoneManager->Initialize();
    }
    
    if (FactionManager)
    {
        FactionManager->Initialize();
    }
    
    if (EconomyManager)
    {
        EconomyManager->Initialize();
    }
    
    if (StoryManager)
    {
        StoryManager->Initialize();
    }
    */
    
    UE_LOG(LogTemp, Log, TEXT("GameManager: Game systems initialization complete"));
}

void URadiantGameManager::ShutdownGameSystems()
{
    UE_LOG(LogTemp, Log, TEXT("GameManager: Shutting down game systems..."));
    
    // Shutdown systems in reverse dependency order
    /*
    if (StoryManager)
    {
        StoryManager->Shutdown();
        StoryManager = nullptr;
    }
    
    if (EconomyManager)
    {
        EconomyManager->Shutdown();
        EconomyManager = nullptr;
    }
    
    if (FactionManager)
    {
        FactionManager->Shutdown();
        FactionManager = nullptr;
    }
    
    if (ZoneManager)
    {
        ZoneManager->Shutdown();
        ZoneManager = nullptr;
    }
    
    if (WorldManager)
    {
        WorldManager->Shutdown();
        WorldManager = nullptr;
    }
    */
    
    UE_LOG(LogTemp, Log, TEXT("GameManager: Game systems shutdown complete"));
}

void URadiantGameManager::HandleGameStateChange(EGameState OldState, EGameState NewState)
{
    UE_LOG(LogTemp, Log, TEXT("GameManager: Handling state transition from %s to %s"), 
           *UEnum::GetValueAsString(OldState), 
           *UEnum::GetValueAsString(NewState));
    
    // Handle state-specific logic
    switch (NewState)
    {
        case EGameState::MainMenu:
            // Stop world simulation
            // Pause background processing
            break;
            
        case EGameState::Loading:
            // Start loading screen
            // Prepare systems for level transition
            break;
            
        case EGameState::InGame:
            // Start world simulation
            // Enable full system processing
            GameStateData.GameTime = 0.0f;
            GameStateData.GameDay = 1;
            break;
            
        case EGameState::Paused:
            // Pause world simulation
            // Reduce system processing
            break;
            
        case EGameState::GameOver:
            // Stop world simulation
            // Prepare for restart or exit
            break;
            
        case EGameState::Credits:
            // Clean up game state
            break;
            
        case EGameState::Options:
            // No special handling needed
            break;
            
        default:
            break;
    }
}

void URadiantGameManager::SetupAutoSave()
{
    if (CurrentGameSettings.AutoSaveInterval <= 0.0f)
    {
        UE_LOG(LogTemp, Log, TEXT("GameManager: Auto-save disabled"));
        return;
    }
    
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            AutoSaveTimerHandle,
            this,
            &URadiantGameManager::PerformAutoSave,
            CurrentGameSettings.AutoSaveInterval * 60.0f, // Convert minutes to seconds
            true // Loop
        );
        
        UE_LOG(LogTemp, Log, TEXT("GameManager: Auto-save timer set for %.1f minutes"), 
               CurrentGameSettings.AutoSaveInterval);
    }
}

void URadiantGameManager::PerformAutoSave()
{
    if (!IsInPlayableState())
    {
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("GameManager: Performing auto-save"));
    
    // Generate auto-save slot name with timestamp
    FDateTime Now = FDateTime::Now();
    FString AutoSaveSlot = FString::Printf(TEXT("AutoSave_%s"), 
                                          *Now.ToString(TEXT("%Y-%m-%d_%H-%M-%S")));
    
    if (SaveGame(AutoSaveSlot))
    {
        UE_LOG(LogTemp, Log, TEXT("GameManager: Auto-save successful: %s"), *AutoSaveSlot);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("GameManager: Auto-save failed"));
    }
}

void URadiantGameManager::UpdateGameTime(float DeltaTime)
{
    GameStateData.GameTime += DeltaTime;
    
    // Update game day based on configured day length
    static const float SecondsPerDay = 1440.0f; // 24 minutes default
    int32 NewGameDay = FMath::FloorToInt(GameStateData.GameTime / SecondsPerDay) + 1;
    
    if (NewGameDay != GameStateData.GameDay)
    {
        GameStateData.GameDay = NewGameDay;
        UE_LOG(LogTemp, Log, TEXT("GameManager: New game day: %d"), GameStateData.GameDay);
    }
}

bool URadiantGameManager::AreAllSystemsHealthy() const
{
    // Check core systems first
    if (CurrentGameState == EGameState::None || CurrentGameState == EGameState::Error)
    {
        return false;
    }

    // Memory check - if we're using too much memory, flag as unhealthy
    float MemoryUsageMB = GetMemoryUsageMB();
    if (MemoryUsageMB > 4096.0f) // 4GB threshold
    {
        UE_LOG(LogTemp, Warning, TEXT("GameManager: High memory usage detected: %.2f MB"), MemoryUsageMB);
        return false;
    }

    // Check if we're initialized
    if (!bIsInitialized)
    {
        return false;
    }

    // Check if we're shutting down
    if (bIsShuttingDown)
    {
        return false;
    }

    // TODO: Check individual system health when systems are implemented
    /*
    if (WorldManager && !IsSystemHealthy("WorldManager"))
    {
        return false;
    }

    if (ZoneManager && !IsSystemHealthy("ZoneManager"))
    {
        return false;
    }

    if (FactionManager && !IsSystemHealthy("FactionManager"))
    {
        return false;
    }

    if (EconomyManager && !IsSystemHealthy("EconomyManager"))
    {
        return false;
    }

    if (StoryManager && !IsSystemHealthy("StoryManager"))
    {
        return false;
    }
    */

    return true;
}

TMap<FString, FString> URadiantGameManager::GetSystemStatusReport() const
{
    TMap<FString, FString> SystemStatus;

    // Core system status
    SystemStatus.Add(TEXT("GameManager"), bIsInitialized ? TEXT("Healthy") : TEXT("Unhealthy - Not Initialized"));
    SystemStatus.Add(TEXT("GameState"), GetEnumValueAsString(TEXT("EGameState"), static_cast<int32>(CurrentGameState)));

    // Memory status
    float MemoryUsageMB = GetMemoryUsageMB();
    FString MemoryStatus = FString::Printf(TEXT("%.2f MB"), MemoryUsageMB);
    if (MemoryUsageMB > 4096.0f)
    {
        MemoryStatus += TEXT(" - HIGH USAGE WARNING");
    }
    SystemStatus.Add(TEXT("Memory"), MemoryStatus);

    // Shutdown status
    SystemStatus.Add(TEXT("ShutdownState"), bIsShuttingDown ? TEXT("Shutting Down") : TEXT("Running"));

    // TODO: Add individual system status when systems are implemented
    /*
    if (WorldManager)
    {
        SystemStatus.Add(TEXT("WorldManager"), IsSystemHealthy("WorldManager") ? TEXT("Healthy") : TEXT("Unhealthy"));
    }
    else
    {
        SystemStatus.Add(TEXT("WorldManager"), TEXT("Not Initialized"));
    }

    if (ZoneManager)
    {
        SystemStatus.Add(TEXT("ZoneManager"), IsSystemHealthy("ZoneManager") ? TEXT("Healthy") : TEXT("Unhealthy"));
    }
    else
    {
        SystemStatus.Add(TEXT("ZoneManager"), TEXT("Not Initialized"));
    }

    if (FactionManager)
    {
        SystemStatus.Add(TEXT("FactionManager"), IsSystemHealthy("FactionManager") ? TEXT("Healthy") : TEXT("Unhealthy"));
    }
    else
    {
        SystemStatus.Add(TEXT("FactionManager"), TEXT("Not Initialized"));
    }

    if (EconomyManager)
    {
        SystemStatus.Add(TEXT("EconomyManager"), IsSystemHealthy("EconomyManager") ? TEXT("Healthy") : TEXT("Unhealthy"));
    }
    else
    {
        SystemStatus.Add(TEXT("EconomyManager"), TEXT("Not Initialized"));
    }

    if (StoryManager)
    {
        SystemStatus.Add(TEXT("StoryManager"), IsSystemHealthy("StoryManager") ? TEXT("Healthy") : TEXT("Unhealthy"));
    }
    else
    {
        SystemStatus.Add(TEXT("StoryManager"), TEXT("Not Initialized"));
    }
    */

    return SystemStatus;
}

bool URadiantGameManager::IsSystemHealthy(const FString& SystemName) const
{
    if (SystemName.IsEmpty())
    {
        return false;
    }

    // Check core systems
    if (SystemName == TEXT("GameManager"))
    {
        return bIsInitialized && !bIsShuttingDown && CurrentGameState != EGameState::Error;
    }

    if (SystemName == TEXT("Memory"))
    {
        return GetMemoryUsageMB() < 4096.0f;
    }

    // TODO: Add specific system health checks when systems are implemented
    /*
    if (SystemName == TEXT("WorldManager"))
    {
        return WorldManager && WorldManager->IsHealthy();
    }

    if (SystemName == TEXT("ZoneManager"))
    {
        return ZoneManager && ZoneManager->IsHealthy();
    }

    if (SystemName == TEXT("FactionManager"))
    {
        return FactionManager && FactionManager->IsHealthy();
    }

    if (SystemName == TEXT("EconomyManager"))
    {
        return EconomyManager && EconomyManager->IsHealthy();
    }

    if (SystemName == TEXT("StoryManager"))
    {
        return StoryManager && StoryManager->IsHealthy();
    }
    */

    // Unknown system
    UE_LOG(LogTemp, Warning, TEXT("GameManager: Unknown system name for health check: %s"), *SystemName);
    return false;
}

void URadiantGameManager::RefreshSystemHealth()
{
    UE_LOG(LogTemp, Log, TEXT("GameManager: Refreshing system health status"));

    // Force memory cleanup
    if (GetMemoryUsageMB() > 3072.0f) // 3GB threshold for cleanup
    {
        ForceGarbageCollection();
    }

    // TODO: Refresh individual system health when systems are implemented
    /*
    if (WorldManager)
    {
        WorldManager->RefreshHealth();
    }

    if (ZoneManager)
    {
        ZoneManager->RefreshHealth();
    }

    if (FactionManager)
    {
        FactionManager->RefreshHealth();
    }

    if (EconomyManager)
    {
        EconomyManager->RefreshHealth();
    }

    if (StoryManager)
    {
        StoryManager->RefreshHealth();
    }
    */

    // Log system status after refresh
    TMap<FString, FString> SystemStatus = GetSystemStatusReport();
    for (const auto& StatusPair : SystemStatus)
    {
        UE_LOG(LogTemp, Log, TEXT("System Status - %s: %s"), *StatusPair.Key, *StatusPair.Value);
    }
}

// Helper function for enum to string conversion
FString URadiantGameManager::GetEnumValueAsString(const FString& EnumName, int32 EnumValue) const
{
    const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, *EnumName, true);
    if (!EnumPtr)
    {
        return FString::Printf(TEXT("Unknown(%d)"), EnumValue);
    }
    
    return EnumPtr->GetNameStringByValue(EnumValue);
}