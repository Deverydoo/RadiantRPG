// Private/Core/RadiantGameManager.cpp
// Game manager implementation for RadiantRPG - simplified without time management

#include "Core/RadiantGameManager.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameplayTagsManager.h"
#include "RenderTargetPool.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/DateTime.h"
#include "GenericPlatform/GenericPlatformMemory.h"
#include "Stats/StatsHierarchical.h"
#include "Types/SystemTypes.h"
#include "World/RadiantWorldManager.h"

URadiantGameManager::URadiantGameManager()
{
    // Initialize core state
    CurrentGameState = EGameState::None;
    PreviousGameState = EGameState::None;
    
    // Initialize settings with defaults
    CurrentGameSettings = FGameSettings();
    
    // Initialize game state data (no more time data)
    GameStateData = FGameStateData();
    
    // Initialize system references
    WorldManager = nullptr;
    
    // Initialize internal state
    bIsInitialized = false;
    bIsShuttingDown = false;
    LastMemoryWarningTime = 0.0f;
    
    UE_LOG(LogTemp, Log, TEXT("RadiantGameManager constructed (simplified - no time management)"));
}

void URadiantGameManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    UE_LOG(LogTemp, Log, TEXT("RadiantGameManager initializing..."));
    
    // Set initial game state
    SetGameState(EGameState::Initializing);
    
    // Initialize our systems first
    bool bInitSuccess = InitializeGameSystems();
    
    // Set up auto-save timer
    SetupAutoSaveTimer();
    
    // Mark as initialized
    bIsInitialized = bInitSuccess;
    
    // Broadcast initialization event
    FSystemEventCoordinator::Get().BroadcastSystemInitialized(
        ESystemType::GameManager, 
        bInitSuccess, 
        TEXT("RadiantGameManager")
    );
    
    if (bInitSuccess)
    {
        // Transition to menu state
        SetGameState(EGameState::MainMenu);
        UE_LOG(LogTemp, Log, TEXT("RadiantGameManager initialization complete"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("RadiantGameManager initialization failed"));
    }
}

void URadiantGameManager::Deinitialize()
{
    UE_LOG(LogTemp, Log, TEXT("RadiantGameManager shutting down..."));
    
    bIsShuttingDown = true;
    
    // Broadcast shutdown event
    FSystemEventCoordinator::Get().BroadcastSystemShutdown(
        ESystemType::GameManager, 
        TEXT("RadiantGameManager")
    );
    
    // Clear auto-save timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AutoSaveTimerHandle);
    }
    
    // Shutdown systems
    ShutdownGameSystems();
    
    // Mark as uninitialized
    bIsInitialized = false;
    
    Super::Deinitialize();
    
    UE_LOG(LogTemp, Log, TEXT("RadiantGameManager shutdown complete"));
}

void URadiantGameManager::Tick(float DeltaTime)
{
    if (!bIsInitialized || bIsShuttingDown)
    {
        return;
    }
    
    // Update system health monitoring
    UpdateSystemHealth(DeltaTime);
}

bool URadiantGameManager::IsTickable() const
{
    return bIsInitialized && !bIsShuttingDown && !IsTemplate();
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
    
    UE_LOG(LogTemp, Log, TEXT("GameManager: State changed from %s to %s"), 
           *GetEnumValueAsString(TEXT("EGameState"), static_cast<int32>(OldState)),
           *GetEnumValueAsString(TEXT("EGameState"), static_cast<int32>(NewState)));
    
    // Broadcast state change
    OnGameStateChanged.Broadcast(OldState, NewState);
    OnGameStateChangedBP(OldState, NewState);
    
    // Handle state-specific logic
    switch (NewState)
    {
        case EGameState::Playing:
            // Ensure world simulation is active
            if (WorldManager && !WorldManager->IsWorldSimulationActive())
            {
                WorldManager->InitializeWorldSimulation();
            }
            break;
            
        case EGameState::Paused:
            // Pause time progression
            if (WorldManager)
            {
                WorldManager->SetTimePaused(true);
            }
            break;
            
        case EGameState::Loading:
        case EGameState::Saving:
            // Keep systems running but prevent new operations
            break;
            
        default:
            break;
    }
}

bool URadiantGameManager::IsInPlayableState() const
{
    return CurrentGameState == EGameState::Playing || 
           CurrentGameState == EGameState::Paused;
}

// === TIME INTERFACE (DELEGATED TO WORLDMANAGER) ===

FString URadiantGameManager::GetCurrentTimeString() const
{
    if (WorldManager)
    {
        return WorldManager->GetTimeString();
    }
    return TEXT("No Time Manager");
}

int32 URadiantGameManager::GetCurrentGameDay() const
{
    if (WorldManager)
    {
        return WorldManager->GetCurrentDay();
    }
    return 1;
}

int32 URadiantGameManager::GetCurrentSeason() const
{
    if (WorldManager)
    {
        return WorldManager->GetCurrentSeason();
    }
    return 1;
}

bool URadiantGameManager::IsCurrentlyDaytime() const
{
    if (WorldManager)
    {
        return WorldManager->IsDaytime();
    }
    return true; // Default to daytime
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
    
    UE_LOG(LogTemp, Log, TEXT("GameManager: Difficulty changed from %s to %s"),
           *GetEnumValueAsString(TEXT("EDifficultyLevel"), static_cast<int32>(OldDifficulty)),
           *GetEnumValueAsString(TEXT("EDifficultyLevel"), static_cast<int32>(NewDifficulty)));
    
    // Broadcast difficulty change
    OnDifficultyChanged.Broadcast(NewDifficulty);
    OnDifficultyChangedBP(NewDifficulty);
    
    // Apply difficulty-specific settings
    // TODO: Implement difficulty-specific modifications when systems are available
}

void URadiantGameManager::ApplyGameSettings(const FGameSettings& NewSettings)
{
    FGameSettings OldSettings = CurrentGameSettings;
    CurrentGameSettings = NewSettings;
    
    UE_LOG(LogTemp, Log, TEXT("GameManager: Game settings applied"));
    
    // Apply auto-save timer changes
    if (OldSettings.AutoSaveInterval != NewSettings.AutoSaveInterval)
    {
        SetupAutoSaveTimer();
    }
    
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
    
    if (!IsInPlayableState())
    {
        UE_LOG(LogTemp, Warning, TEXT("GameManager: Cannot save - not in playable state"));
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
    // Force immediate garbage collection
    CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, true);
    
    // Wait a frame for cleanup to complete
    GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
    {
        float PostCleanupMemory = GetMemoryUsageMB();
        UE_LOG(LogTemp, Log, TEXT("GameManager: Post-cleanup memory usage: %.2f MB"), PostCleanupMemory);
    });
}

float URadiantGameManager::GetMemoryUsageMB() const
{
    FPlatformMemoryStats MemStats = FPlatformMemory::GetStats();
    return MemStats.UsedPhysical / (1024.0f * 1024.0f);
}

void URadiantGameManager::ResetAllSystems()
{
    UE_LOG(LogTemp, Log, TEXT("GameManager: Resetting all systems"));
    
    // Reset game state data (no more time data)
    GameStateData = FGameStateData();
    
    // Reset settings to defaults
    CurrentGameSettings = FGameSettings();
    
    // Reset WorldManager if available
    if (WorldManager)
    {
        // Reset to default time
        WorldManager->SetWorldTime(FSimpleWorldTime());
    }
    
    UE_LOG(LogTemp, Log, TEXT("GameManager: All systems reset complete"));
}

bool URadiantGameManager::AreAllSystemsHealthy() const
{
    // Check core systems first
    if (CurrentGameState == EGameState::None || CurrentGameState == EGameState::Error)
    {
        return false;
    }

    // Progressive memory thresholds instead of hard cutoff
    float MemoryUsageMB = GetMemoryUsageMB();
    if (MemoryUsageMB > 7168.0f) // 7GB critical threshold
    {
        UE_LOG(LogTemp, Error, TEXT("GameManager: Critical memory usage detected: %.2f MB"), MemoryUsageMB);
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

    // Check WorldManager health
    if (WorldManager && !WorldManager->IsWorldSimulationActive())
    {
        UE_LOG(LogTemp, Warning, TEXT("GameManager: WorldManager simulation is not active"));
        return false;
    }

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

    // WorldManager status
    if (WorldManager)
    {
        bool bWorldHealthy = WorldManager->IsWorldSimulationActive();
        SystemStatus.Add(TEXT("WorldManager"), bWorldHealthy ? TEXT("Healthy") : TEXT("Unhealthy"));
        
        // Add time system status
        SystemStatus.Add(TEXT("TimeSystem"), WorldManager->IsTimePaused() ? TEXT("Paused") : TEXT("Running"));
        SystemStatus.Add(TEXT("CurrentTime"), WorldManager->GetFullTimeString());
    }
    else
    {
        SystemStatus.Add(TEXT("WorldManager"), TEXT("Not Initialized"));
    }

    return SystemStatus;
}

// === INTERNAL FUNCTIONS ===
void URadiantGameManager::OnOtherSystemInitialized(const FSystemInitializationEvent& Event)
{
    UE_LOG(LogTemp, Log, TEXT("GameManager: Received system initialization event for %s"), *Event.SystemName);
    
    // React to other systems coming online
    switch (Event.SystemType)
    {
    case ESystemType::WorldManager:
        if (Event.bInitializationSuccessful)
        {
            OnWorldManagerReady();
        }
        break;
            
    case ESystemType::AIManager:
        if (Event.bInitializationSuccessful)
        {
            OnAIManagerReady();
        }
        break;
            
    default:
        break;
    }
}

void URadiantGameManager::OnWorldManagerReady()
{
    UE_LOG(LogTemp, Log, TEXT("GameManager: WorldManager is ready - can start world simulation"));
    
    // Now we can safely interact with WorldManager
    if (WorldManager)
    {
        // Request world simulation start if needed
        if (!WorldManager->IsWorldSimulationActive())
        {
            WorldManager->InitializeWorldSimulation();
        }
    }
}

void URadiantGameManager::OnAIManagerReady()
{
    UE_LOG(LogTemp, Log, TEXT("GameManager: AIManager is ready - can coordinate with AI systems"));
    
    // Set up AI coordination when AI systems are ready
}

bool URadiantGameManager::InitializeGameSystems()
{
    UE_LOG(LogTemp, Log, TEXT("GameManager: Initializing game systems..."));
    
    bool bAllSystemsInitialized = true;
    
    // Get subsystem references
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        WorldManager = GameInstance->GetSubsystem<URadiantWorldManager>();
    }

    // Initialize systems in dependency order
    if (WorldManager)
    {
        // Don't call InitializeWorldSimulation directly - let WorldManager handle its own initialization
        UE_LOG(LogTemp, Log, TEXT("GameManager: WorldManager reference acquired"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("GameManager: WorldManager subsystem not available"));
        bAllSystemsInitialized = false;
    }
    
    // Subscribe to other system events to coordinate
    FSystemEventCoordinator::Get().OnSystemInitialized.AddUObject(this, &URadiantGameManager::OnOtherSystemInitialized);
    
    UE_LOG(LogTemp, Log, TEXT("GameManager: Game systems initialization complete"));
    return bAllSystemsInitialized;
}


void URadiantGameManager::ShutdownGameSystems()
{
    UE_LOG(LogTemp, Log, TEXT("GameManager: Shutting down game systems..."));
    
    // Shutdown systems in reverse dependency order
    if (WorldManager)
    {
        WorldManager->ShutdownWorldSimulation();
        WorldManager = nullptr;
        UE_LOG(LogTemp, Log, TEXT("GameManager: WorldManager shutdown"));
    }
    
    UE_LOG(LogTemp, Log, TEXT("GameManager: Game systems shutdown complete"));
}

void URadiantGameManager::UpdateSystemHealth(float DeltaTime)
{
    // Monitor memory usage
    float MemoryUsageMB = GetMemoryUsageMB();
    if (MemoryUsageMB > 5120.0f) // 5GB warning threshold  
    {
        // Log warning but don't fail health check immediately
        float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
        
        if (CurrentTime - LastMemoryWarningTime > 10.0f) // Throttle warnings to every 10 seconds
        {
            UE_LOG(LogTemp, Warning, TEXT("GameManager: High memory usage detected: %.2f MB - performing light cleanup"), MemoryUsageMB);
            PerformLightMemoryCleanup();
            LastMemoryWarningTime = CurrentTime;
        }
    }
}

void URadiantGameManager::SetupAutoSaveTimer()
{
    // Clear existing timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AutoSaveTimerHandle);
    }
    
    // Set up new timer if auto-save is enabled
    if (CurrentGameSettings.AutoSaveInterval > 0.0f)
    {
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

bool URadiantGameManager::IsSystemHealthy(const FString& SystemName) const
{
    // TODO: Implement specific system health checks when systems are available
    if (SystemName == TEXT("WorldManager"))
    {
        return WorldManager && WorldManager->IsWorldSimulationActive();
    }
    
    return true;
}

FString URadiantGameManager::GetEnumValueAsString(const FString& EnumName, int32 EnumValue) const
{
    // Simple enum to string conversion for debugging
    return FString::Printf(TEXT("%s_%d"), *EnumName, EnumValue);
}

void URadiantGameManager::PerformLightMemoryCleanup()
{
    // Light memory cleanup without full garbage collection
    UE_LOG(LogTemp, Log, TEXT("GameManager: Elevated memory usage: %.2f MB - performing light cleanup"), GetMemoryUsageMB());
    
    // TODO: Add light cleanup operations
    // For now, just recommend garbage collection
    if (GetMemoryUsageMB() > 6144.0f) // 6GB threshold
    {
        ForceGarbageCollection();
    }
}