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
    
    // Initialize our systems - always succeed for core initialization
    bool bInitSuccess = InitializeGameSystems();
    
    // Set up auto-save timer
    SetupAutoSaveTimer();
    
    // Always mark as initialized - missing subsystem references can be resolved later
    bIsInitialized = true;
    
    // Broadcast initialization event
    FSystemEventCoordinator::Get().BroadcastSystemInitialized(
        ESystemType::GameManager, 
        true, // Always report success for GameManager itself
        TEXT("RadiantGameManager")
    );
    
    // Transition to menu state
    SetGameState(EGameState::MainMenu);
    UE_LOG(LogTemp, Log, TEXT("RadiantGameManager initialization complete - subsystem references will be resolved when available"));
}

void URadiantGameManager::Deinitialize()
{
    UE_LOG(LogTemp, Log, TEXT("RadiantGameManager shutting down..."));
    
    bIsShuttingDown = true;
    
    // Clear auto-save timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AutoSaveTimerHandle);
    }
    
    // Shutdown our systems
    ShutdownGameSystems();
    
    // Clear system references
    WorldManager = nullptr;
    
    // Reset state
    bIsInitialized = false;
    SetGameState(EGameState::None);
    
    UE_LOG(LogTemp, Log, TEXT("RadiantGameManager shutdown complete"));
    
    Super::Deinitialize();
}

void URadiantGameManager::Tick(float DeltaTime)
{
    // Skip ticking if not initialized or shutting down
    if (!bIsInitialized || bIsShuttingDown)
    {
        return;
    }
    
    // Update system health monitoring
    UpdateSystemHealth(DeltaTime);
    
    // Update global game systems
    UpdateGlobalSystems(DeltaTime);
}

bool URadiantGameManager::IsTickable() const
{
    return bIsInitialized && !bIsShuttingDown;
}

TStatId URadiantGameManager::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(URadiantGameManager, STATGROUP_Tickables);
}

void URadiantGameManager::SetGameState(EGameState NewState)
{
    if (NewState == CurrentGameState)
    {
        return;
    }
    
    EGameState OldState = CurrentGameState;
    PreviousGameState = OldState;
    CurrentGameState = NewState;
    
    UE_LOG(LogTemp, Log, TEXT("GameManager: Game state changed from %s to %s"), 
           *UEnum::GetValueAsString(OldState),
           *UEnum::GetValueAsString(NewState));
    
    // Handle state transitions
    OnGameStateTransition(OldState, NewState);
    
    // Broadcast the change
    OnGameStateChanged.Broadcast(OldState, NewState);
}

bool URadiantGameManager::IsInPlayableState() const
{
    return CurrentGameState == EGameState::Playing || 
           CurrentGameState == EGameState::Paused;
}

void URadiantGameManager::OnGameStateTransition(EGameState FromState, EGameState ToState)
{
    // Handle specific state transitions
    switch (ToState)
    {
    case EGameState::Playing:
        if (WorldManager)
        {
            WorldManager->SetTimePaused(false);
        }
        break;
        
    case EGameState::Paused:
        if (WorldManager)
        {
            WorldManager->SetTimePaused(true);
        }
        break;
        
    case EGameState::MainMenu:
        // Pause world simulation when in menu
        if (WorldManager)
        {
            WorldManager->SetTimePaused(true);
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
    
    // Try to get subsystem references, but don't fail if they're not ready yet
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        WorldManager = GameInstance->GetSubsystem<URadiantWorldManager>();
    }

    // Log what we found, but don't fail initialization
    if (WorldManager)
    {
        UE_LOG(LogTemp, Log, TEXT("GameManager: WorldManager reference acquired"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("GameManager: WorldManager subsystem not yet available - will resolve later"));
    }
    
    // Subscribe to other system events to coordinate
    FSystemEventCoordinator::Get().OnSystemInitialized.AddUObject(this, &URadiantGameManager::OnOtherSystemInitialized);
    
    UE_LOG(LogTemp, Log, TEXT("GameManager: Game systems initialization complete"));
    
    // Always return true - we can resolve missing references later
    return true;
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
        
        // Only log memory warnings every 30 seconds to avoid spam
        if (CurrentTime - LastMemoryWarningTime > 30.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("GameManager: High memory usage detected: %.2f MB"), MemoryUsageMB);
            LastMemoryWarningTime = CurrentTime;
        }
    }
    
    // Try to resolve missing subsystem references
    if (!WorldManager && GetGameInstance())
    {
        WorldManager = GetGameInstance()->GetSubsystem<URadiantWorldManager>();
        if (WorldManager)
        {
            UE_LOG(LogTemp, Log, TEXT("GameManager: WorldManager reference resolved"));
            OnWorldManagerReady();
        }
    }
}

void URadiantGameManager::UpdateGlobalSystems(float DeltaTime)
{
    // Update global game systems that don't have their own subsystems
    
    // Example: Update global cooldowns, effects, etc.
    // This is where we'd put game-wide logic that needs to run every frame
    
    // For now, this is a placeholder for future global systems
}

void URadiantGameManager::SetupAutoSaveTimer()
{
    // Set up periodic auto-save
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(AutoSaveTimerHandle,
                                         this, &URadiantGameManager::PerformAutoSave,
                                         300.0f, // 5 minutes
                                         true);
        
        UE_LOG(LogTemp, Log, TEXT("GameManager: Auto-save timer initialized"));
    }
}

void URadiantGameManager::PerformAutoSave()
{
    if (CurrentGameState != EGameState::Playing)
    {
        return; // Only auto-save during gameplay
    }
    
    UE_LOG(LogTemp, Log, TEXT("GameManager: Performing auto-save..."));
    
    // TODO: Implement actual save system
    // For now, just log that we would save
    
    UE_LOG(LogTemp, Log, TEXT("GameManager: Auto-save completed (placeholder)"));
}

// === TIME INTERFACE (DELEGATED TO WORLDMANAGER) ===

FString URadiantGameManager::GetCurrentTimeString() const
{
    if (WorldManager)
    {
        return WorldManager->GetFullTimeString();
    }
    return TEXT("Time System Not Available");
}

int32 URadiantGameManager::GetCurrentGameDay() const
{
    if (WorldManager)
    {
        return WorldManager->GetCurrentDay();
    }
    return 1; // Default to day 1
}

int32 URadiantGameManager::GetCurrentSeason() const
{
    if (WorldManager)
    {
        return WorldManager->GetCurrentSeason();
    }
    return 0; // Default to first season
}

bool URadiantGameManager::IsDaytime() const
{
    if (WorldManager)
    {
        return WorldManager->IsDaytime();
    }
    return true; // Default to daytime
}

bool URadiantGameManager::IsCurrentlyDaytime() const
{
    return IsDaytime();
}

// === SAVE/LOAD INTERFACE ===

bool URadiantGameManager::SaveGame(const FString& SaveSlotName)
{
    FString SlotName = SaveSlotName.IsEmpty() ? 
        FString::Printf(TEXT("GameSave_%s"), *FDateTime::Now().ToString()) : SaveSlotName;
    
    UE_LOG(LogTemp, Log, TEXT("GameManager: Saving game to slot: %s"), *SlotName);
    
    // TODO: Implement save system
    // 1. Collect data from WorldManager (time, world state)
    // 2. Collect data from other systems
    // 3. Save to file
    
    UE_LOG(LogTemp, Log, TEXT("GameManager: Game save completed (placeholder)"));
    return true;
}

bool URadiantGameManager::LoadGame(const FString& SaveSlotName)
{
    UE_LOG(LogTemp, Log, TEXT("GameManager: Loading game from slot: %s"), *SaveSlotName);
    
    // TODO: Implement load system
    // 1. Load data from file
    // 2. Restore WorldManager state (time, world state)
    // 3. Restore other system states
    
    UE_LOG(LogTemp, Log, TEXT("GameManager: Game load completed (placeholder)"));
    return true;
}

bool URadiantGameManager::DoesSaveExist(const FString& SaveSlotName) const
{
    // TODO: Check if save file exists
    UE_LOG(LogTemp, Log, TEXT("GameManager: Checking save existence for slot: %s"), *SaveSlotName);
    
    return false;
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
}

// === GLOBAL STATE MANAGEMENT ===

void URadiantGameManager::SetGlobalFlag(FGameplayTag Flag, bool bValue)
{
    bool bPreviousValue = GameStateData.GlobalFlags.HasTag(Flag);
    
    if (bValue)
    {
        GameStateData.GlobalFlags.AddTag(Flag);
    }
    else
    {
        GameStateData.GlobalFlags.RemoveTag(Flag);
    }
    
    // Only broadcast if the value actually changed
    if (bPreviousValue != bValue)
    {
        OnGlobalFlagChanged.Broadcast(Flag);
        UE_LOG(LogTemp, Log, TEXT("GameManager: Global flag '%s' set to %s"), 
               *Flag.ToString(), bValue ? TEXT("True") : TEXT("False"));
    }
}

bool URadiantGameManager::HasGlobalFlag(FGameplayTag Flag) const
{
    return GameStateData.GlobalFlags.HasTag(Flag);
}

void URadiantGameManager::RemoveGlobalFlag(FGameplayTag Flag)
{
    if (GameStateData.GlobalFlags.HasTag(Flag))
    {
        GameStateData.GlobalFlags.RemoveTag(Flag);
        OnGlobalFlagChanged.Broadcast(Flag);
        UE_LOG(LogTemp, Log, TEXT("GameManager: Global flag '%s' removed"), *Flag.ToString());
    }
}

void URadiantGameManager::SetGlobalVariable(const FString& VariableName, float Value)
{
    float* ExistingValue = GameStateData.GlobalVariables.Find(VariableName);
    float OldValue = ExistingValue ? *ExistingValue : 0.0f;
    
    GameStateData.GlobalVariables.Add(VariableName, Value);
    
    UE_LOG(LogTemp, Log, TEXT("GameManager: Global variable '%s' set to %f (was %f)"), 
           *VariableName, Value, OldValue);
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
    FString* ExistingValue = GameStateData.GlobalStrings.Find(VariableName);
    FString OldValue = ExistingValue ? *ExistingValue : TEXT("");
    
    GameStateData.GlobalStrings.Add(VariableName, Value);
    
    UE_LOG(LogTemp, Log, TEXT("GameManager: Global string '%s' set to '%s' (was '%s')"), 
           *VariableName, *Value, *OldValue);
}

FString URadiantGameManager::GetGlobalString(const FString& VariableName, const FString& DefaultValue) const
{
    if (const FString* Value = GameStateData.GlobalStrings.Find(VariableName))
    {
        return *Value;
    }
    return DefaultValue;
}

// === SAVE/LOAD UTILITIES ===

bool URadiantGameManager::DoesSaveSlotExist(const FString& SaveSlotName) const
{
    // TODO: Check if save file exists
    UE_LOG(LogTemp, Log, TEXT("GameManager: Checking save existence for slot: %s"), *SaveSlotName);
    
    return false;
}

TArray<FString> URadiantGameManager::GetAvailableSaveSlots() const
{
    TArray<FString> SaveSlots;
    
    // TODO: Scan save directory for available slots
    UE_LOG(LogTemp, Log, TEXT("GameManager: Getting available save slots"));
    
    return SaveSlots;
}

bool URadiantGameManager::DeleteSaveSlot(const FString& SaveSlotName)
{
    // TODO: Delete save file
    UE_LOG(LogTemp, Log, TEXT("GameManager: Deleting save slot: %s"), *SaveSlotName);
    
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

    // Check WorldManager health - but don't fail if not available yet
    if (WorldManager && !WorldManager->IsWorldSimulationActive())
    {
        UE_LOG(LogTemp, Warning, TEXT("GameManager: WorldManager simulation is not active"));
        // Don't return false here - the simulation might still be starting up
    }

    return true;
}

TMap<FString, FString> URadiantGameManager::GetSystemStatusReport() const
{
    TMap<FString, FString> SystemStatus;

    // Core system status
    SystemStatus.Add(TEXT("GameManager"), bIsInitialized ? 
        TEXT("Healthy") : TEXT("Unhealthy - Not Initialized"));
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
        SystemStatus.Add(TEXT("WorldManager"), TEXT("Not Available"));
    }

    return SystemStatus;
}

// === UTILITY FUNCTIONS ===

FString URadiantGameManager::GetEnumValueAsString(const FString& EnumName, int32 EnumValue) const
{
    // Helper function to convert enum values to strings
    if (EnumName == TEXT("EGameState"))
    {
        UEnum* GameStateEnum = StaticEnum<EGameState>();
        if (GameStateEnum)
        {
            return GameStateEnum->GetNameStringByValue(EnumValue);
        }
    }
    else if (EnumName == TEXT("EDifficultyLevel"))
    {
        UEnum* DifficultyEnum = StaticEnum<EDifficultyLevel>();
        if (DifficultyEnum)
        {
            return DifficultyEnum->GetNameStringByValue(EnumValue);
        }
    }
    
    return FString::Printf(TEXT("Unknown(%d)"), EnumValue);
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
            // Try to get the reference now that it's initialized
            if (!WorldManager && GetGameInstance())
            {
                WorldManager = GetGameInstance()->GetSubsystem<URadiantWorldManager>();
            }
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
