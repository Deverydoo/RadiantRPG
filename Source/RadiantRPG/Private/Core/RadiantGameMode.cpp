// Private/Core/RadiantGameMode.cpp
// Main game mode implementation for RadiantRPG

#include "Core/RadiantGameMode.h"

#include "EngineUtils.h"
#include "Core/RadiantGameManager.h"
#include "Controllers/RadiantPlayerController.h"
#include "Characters/PlayerCharacter.h"
#include "Engine/World.h"
#include "Engine/PlayerStartPIE.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "GameplayTagsManager.h"

ARadiantGameMode::ARadiantGameMode()
{
    // Set default classes - MAKE SURE THESE PATHS MATCH YOUR ACTUAL BLUEPRINT CLASSES
    static ConstructorHelpers::FClassFinder<APawn> PlayerCharacterBPClass(TEXT("/Game/Blueprints/Characters/BP_Player"));
    if (PlayerCharacterBPClass.Class != nullptr)
    {
        DefaultPawnClass = PlayerCharacterBPClass.Class;
        DefaultPlayerCharacterClass = PlayerCharacterBPClass.Class;
        UE_LOG(LogTemp, Warning, TEXT("RadiantGameMode: Set player character class to %s"), *DefaultPawnClass->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("RadiantGameMode: Failed to find BP_Player class! Check the path."));
    }

    static ConstructorHelpers::FClassFinder<APlayerController> PlayerControllerBPClass(TEXT("/Game/Blueprints/Controllers/BP_RadiantPlayerController"));
    if (PlayerControllerBPClass.Class != nullptr)
    {
        PlayerControllerClass = PlayerControllerBPClass.Class;
        DefaultPlayerControllerClass = PlayerControllerBPClass.Class;
        UE_LOG(LogTemp, Warning, TEXT("RadiantGameMode: Set player controller class to %s"), *PlayerControllerClass->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("RadiantGameMode: Failed to find BP_RadiantPlayerController class! Check the path."));
    }

    // Set other defaults
    bUseSeamlessTravel = false;
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    // Initialize world settings
    WorldInitializationTimeout = 30.0f;
    bAutoSaveOnPlayerEvents = false;
    WorldSimulationStartDelay = 2.0f;

    // Initialize world state
    CurrentInitPhase = EWorldInitializationPhase::PreInit;
    CurrentSessionState = EGameSessionState::WaitingToStart;
    bWorldInitializationComplete = false;

    // Initialize systems flags
    bEnableAISystems = true;
    bEnableWorldSimulation = true; 
    bEnableDynamicEvents = true;

    UE_LOG(LogTemp, Warning, TEXT("RadiantGameMode constructor completed"));
}

void ARadiantGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    LogGameModeStatus(TEXT("RadiantGameMode starting initialization"));
    
    // Cache important references
    GameManager = GetGameInstance()->GetSubsystem<URadiantGameManager>();
    if (!GameManager)
    {
        LogGameModeStatus(TEXT("Failed to get RadiantGameManager - game systems may not function properly"), true);
    }
    
    // Validate configuration
    if (!ValidateGameModeConfiguration())
    {
        LogGameModeStatus(TEXT("Game mode configuration validation failed"), true);
    }
    
    // Start world initialization sequence
    StartWorldInitialization();
}

void ARadiantGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    LogGameModeStatus(TEXT("RadiantGameMode shutting down"));
    
    bIsShuttingDown = true;
    
    // Clear timers
    GetWorldTimerManager().ClearTimer(InitializationTimeoutHandle);
    GetWorldTimerManager().ClearTimer(WorldSimulationStartupHandle);
    GetWorldTimerManager().ClearTimer(AutoSaveTimerHandle);
    
    // Stop world simulation
    StopWorldSimulation();
    
    // Auto-save if enabled
    if (bAutoSaveOnPlayerEvents && GetActivePlayerCount() > 0)
    {
        TriggerAutoSave();
    }
    
    Super::EndPlay(EndPlayReason);
}

void ARadiantGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Periodic system health check
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastSystemHealthCheckTime > SystemHealthCheckInterval)
    {
        UpdateSystemHealthCheck();
        LastSystemHealthCheckTime = CurrentTime;
    }
}

void ARadiantGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    
    LogGameModeStatus(FString::Printf(TEXT("Initializing game for map: %s"), *MapName));
    
    // Parse game options
    // TODO: Parse custom game options from URL parameters
    
    // Set initial session state
    SetGameSessionState(EGameSessionState::WaitingToStart);
}

void ARadiantGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    
    if (!NewPlayer)
    {
        LogGameModeStatus(TEXT("PostLogin called with null player controller"), true);
        return;
    }
    
    // Add to active players list
    ActivePlayers.AddUnique(NewPlayer);
    
    // Handle player joining
    HandlePlayerJoined(NewPlayer);
    
    // Start game session if this is the first player
    if (ActivePlayers.Num() == 1 && CurrentSessionState == EGameSessionState::WaitingToStart)
    {
        StartGameSession();
    }
    
    LogGameModeStatus(FString::Printf(TEXT("Player joined: %s (Total players: %d)"), 
                      *NewPlayer->GetName(), ActivePlayers.Num()));
}

void ARadiantGameMode::Logout(AController* Exiting)
{
    if (APlayerController* ExitingPlayer = Cast<APlayerController>(Exiting))
    {
        // Remove from active players
        ActivePlayers.Remove(ExitingPlayer);
        
        // Handle player leaving
        HandlePlayerLeft(ExitingPlayer);
        
        LogGameModeStatus(FString::Printf(TEXT("Player left: %s (Remaining players: %d)"), 
                          *ExitingPlayer->GetName(), ActivePlayers.Num()));
        
        // End session if no players remain
        if (ActivePlayers.Num() == 0 && CurrentSessionState == EGameSessionState::InProgress)
        {
            EndGameSession();
        }
    }
    
    Super::Logout(Exiting);
}

UClass* ARadiantGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
    if (APlayerController* PC = Cast<APlayerController>(InController))
    {
        UE_LOG(LogTemp, Warning, TEXT("Getting pawn class for PlayerController"));
        
        UClass* PawnClass = nullptr;
        
        if (IsValid(DefaultPlayerCharacterClass))
        {
            PawnClass = DefaultPlayerCharacterClass.Get();
            UE_LOG(LogTemp, Warning, TEXT("Using DefaultPlayerCharacterClass: %s"), *PawnClass->GetName());
        }
        else if (DefaultPawnClass)
        {
            PawnClass = DefaultPawnClass.Get();
            UE_LOG(LogTemp, Warning, TEXT("Using DefaultPawnClass: %s"), *PawnClass->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("No pawn class configured!"));
            return Super::GetDefaultPawnClassForController_Implementation(InController);
        }
        
        return PawnClass;
    }
    
    return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void ARadiantGameMode::RestartPlayer(AController* NewPlayer)
{
    // BUG FIX: Original code had wrong condition (!NewPlayer->IsPendingKillPending())
    // This should be checking if NewPlayer exists AND is NOT pending kill
    if (!NewPlayer || NewPlayer->IsPendingKillPending())
    {
        UE_LOG(LogTemp, Error, TEXT("RadiantGameMode::RestartPlayer - Player is null or pending kill!"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("RadiantGameMode::RestartPlayer - Attempting to spawn player: %s"), *NewPlayer->GetName());

    // Find appropriate spawn location
    AActor* StartSpot = FindPlayerStart(NewPlayer);
    if (StartSpot)
    {
        UE_LOG(LogTemp, Warning, TEXT("Found PlayerStart: %s at location: %s"), 
               *StartSpot->GetName(), *StartSpot->GetActorLocation().ToString());

        // Get spawn transform
        FTransform SpawnTransform = StartSpot->GetActorTransform();
        
        // Spawn player character
        RestartPlayerAtTransform(NewPlayer, SpawnTransform);
        
        // Initialize player character if it's a player controller
        if (APlayerController* PC = Cast<APlayerController>(NewPlayer))
        {
            if (APlayerCharacter* PlayerChar = Cast<APlayerCharacter>(PC->GetPawn()))
            {
                InitializePlayerCharacter(PlayerChar, PC);
                UE_LOG(LogTemp, Warning, TEXT("Player character spawned successfully: %s at %s"), 
                       *PlayerChar->GetName(), *PlayerChar->GetActorLocation().ToString());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("RestartPlayer - Failed to get PlayerCharacter from spawned pawn!"));
                
                // Debug: Check what pawn was actually spawned
                if (APawn* SpawnedPawn = PC->GetPawn())
                {
                    UE_LOG(LogTemp, Error, TEXT("Spawned pawn is of class: %s"), *SpawnedPawn->GetClass()->GetName());
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("No pawn was spawned at all!"));
                }
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find spawn location for player: %s"), *NewPlayer->GetName());
        
        // Use default spawn as fallback
        UE_LOG(LogTemp, Warning, TEXT("Using emergency spawn at world origin"));
        FTransform DefaultTransform = GetDefaultSpawnTransform();
        RestartPlayerAtTransform(NewPlayer, DefaultTransform);
        
        // Initialize player character if it's a player controller
        if (APlayerController* PC = Cast<APlayerController>(NewPlayer))
        {
            if (APlayerCharacter* PlayerChar = Cast<APlayerCharacter>(PC->GetPawn()))
            {
                InitializePlayerCharacter(PlayerChar, PC);
                UE_LOG(LogTemp, Warning, TEXT("Emergency spawn successful: %s"), *PlayerChar->GetName());
            }
        }
    }
}

bool ARadiantGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
    // Always use our custom spawn logic
    return false;
}

AActor* ARadiantGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
    UE_LOG(LogTemp, Warning, TEXT("FindPlayerStart called for player: %s"), Player ? *Player->GetName() : TEXT("NULL"));
    
    if (!GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("FindPlayerStart - World is null!"));
        return nullptr;
    }

    // First try our custom spawn location logic
    AActor* CustomStart = FindPlayerSpawnLocation(Player, IncomingName);
    if (CustomStart)
    {
        UE_LOG(LogTemp, Warning, TEXT("Found custom spawn location: %s"), *CustomStart->GetName());
        return CustomStart;
    }

    // Fallback: Search for any PlayerStart in the world
    UE_LOG(LogTemp, Warning, TEXT("No custom spawn found, searching for any PlayerStart"));
    
    TArray<AActor*> PlayerStarts;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
    
    UE_LOG(LogTemp, Warning, TEXT("Found %d PlayerStart actors in the world"), PlayerStarts.Num());
    
    for (int32 i = 0; i < PlayerStarts.Num(); i++)
    {
        if (PlayerStarts[i])
        {
            UE_LOG(LogTemp, Warning, TEXT("PlayerStart %d: %s at %s"), 
                   i, *PlayerStarts[i]->GetName(), *PlayerStarts[i]->GetActorLocation().ToString());
        }
    }
    
    if (PlayerStarts.Num() > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Returning first PlayerStart: %s"), *PlayerStarts[0]->GetName());
        return PlayerStarts[0];
    }

    UE_LOG(LogTemp, Error, TEXT("No PlayerStart actors found in the world!"));
    return nullptr;
}

// === INITIALIZATION PROCESS ===

void ARadiantGameMode::StartWorldInitialization()
{
    WorldInitializationStartTime = FPlatformTime::Seconds();
    
    LogGameModeStatus(TEXT("Starting world initialization sequence"));
    
    // Start with pre-initialization phase
    SetWorldInitializationPhase(EWorldInitializationPhase::PreInit);
    
    // Set timeout timer
    GetWorldTimerManager().SetTimer(InitializationTimeoutHandle, 
                                   this, &ARadiantGameMode::OnWorldInitializationTimeout,
                                   WorldInitializationTimeout, false);
    
    // Begin initialization steps
    InitializeGameSystems();
}

void ARadiantGameMode::SetWorldInitializationPhase(EWorldInitializationPhase NewPhase)
{
    if (CurrentInitPhase == NewPhase)
        return;
    
    EWorldInitializationPhase OldPhase = CurrentInitPhase;
    CurrentInitPhase = NewPhase;
    
    LogGameModeStatus(FString::Printf(TEXT("World initialization phase: %s -> %s"),
                      *UEnum::GetValueAsString(OldPhase),
                      *UEnum::GetValueAsString(NewPhase)));
    
    // Broadcast phase change
    OnWorldInitializationPhaseChanged.Broadcast(NewPhase);
    OnWorldInitializationPhaseChangedBP(OldPhase, NewPhase);
    
    // Handle phase-specific logic
    switch (NewPhase)
    {
        case EWorldInitializationPhase::SystemsStartup:
            InitializeGameSystems();
            break;
            
        case EWorldInitializationPhase::WorldGeneration:
            InitializeWorldGeneration();
            break;
            
        case EWorldInitializationPhase::AIInitialization:
            InitializeAISystems();
            break;
            
        case EWorldInitializationPhase::PlayerSpawn:
            // Player spawn is handled by PostLogin
            break;
            
        case EWorldInitializationPhase::PostInit:
            CompleteWorldInitialization();
            break;
            
        case EWorldInitializationPhase::Complete:
            CompleteWorldInitialization();
            break;
    }
}

void ARadiantGameMode::InitializeGameSystems()
{
    LogGameModeStatus(TEXT("Initializing game systems"));
    
    if (GameManager)
    {
        // Game manager handles system initialization
        LogGameModeStatus(TEXT("Game systems initialized successfully"));
        SetWorldInitializationPhase(EWorldInitializationPhase::WorldGeneration);
    }
    else
    {
        LogGameModeStatus(TEXT("Failed to initialize game systems - GameManager not available"), true);
        SetWorldInitializationPhase(EWorldInitializationPhase::Complete); // Skip to complete with errors
    }
}

void ARadiantGameMode::InitializeWorldGeneration()
{
    LogGameModeStatus(TEXT("Initializing world generation"));
    
    // TODO: Initialize procedural world generation systems
    // if (WorldGenerationManager)
    // {
    //     WorldGenerationManager->InitializeWorldGeneration();
    // }
    
    // For now, skip to AI initialization
    SetWorldInitializationPhase(EWorldInitializationPhase::AIInitialization);
}

void ARadiantGameMode::InitializeAISystems()
{
    LogGameModeStatus(TEXT("Initializing AI systems"));
    
    if (bEnableAISystems)
    {
        // TODO: Initialize AI systems when implemented
        // if (AIManager)
        // {
        //     AIManager->InitializeAISystems();
        // }
        
        LogGameModeStatus(TEXT("AI systems initialization placeholder completed"));
    }
    else
    {
        LogGameModeStatus(TEXT("AI systems disabled - skipping initialization"));
    }
    
    SetWorldInitializationPhase(EWorldInitializationPhase::PlayerSpawn);
}

void ARadiantGameMode::CompleteWorldInitialization()
{
    // Clear timeout timer
    GetWorldTimerManager().ClearTimer(InitializationTimeoutHandle);
    
    bWorldInitializationComplete = true;
    CalculateInitializationMetrics();
    
    LogGameModeStatus(TEXT("World initialization completed successfully"));
    OnWorldInitializationCompleteBP();
    
    // Schedule world simulation startup
    if (bEnableWorldSimulation)
    {
        GetWorldTimerManager().SetTimer(WorldSimulationStartupHandle,
                                       this, &ARadiantGameMode::OnWorldSimulationStartup,
                                       WorldSimulationStartDelay, false);
    }
}

// === PLAYER MANAGEMENT ===

void ARadiantGameMode::InitializePlayerCharacter(const APlayerCharacter* PlayerCharacter, const APlayerController* PlayerController) const
{
    if (!PlayerCharacter || !PlayerController)
    {
        LogGameModeStatus(TEXT("Failed to initialize player character - null parameters"), true);
        return;
    }
    
    // TODO: Load player data from save system
    // if (SaveGameManager)
    // {
    //     SaveGameManager->LoadPlayerData(PlayerCharacter, PlayerController);
    // }
    
    LogGameModeStatus(FString::Printf(TEXT("Player character initialized: %s"), *PlayerCharacter->GetName()));
}

void ARadiantGameMode::HandlePlayerJoined(APlayerController* NewPlayer)
{
    if (!NewPlayer)
        return;
    
    // Auto-save on player join if enabled
    if (bAutoSaveOnPlayerEvents)
    {
        TriggerAutoSave();
    }
    
    // Broadcast player joined event
    OnPlayerJoined.Broadcast(NewPlayer);
    OnPlayerJoinedBP(NewPlayer);
}

void ARadiantGameMode::HandlePlayerLeft(APlayerController* ExitingPlayer)
{
    if (!ExitingPlayer)
        return;
    
    // Auto-save on player leave if enabled
    if (bAutoSaveOnPlayerEvents)
    {
        TriggerAutoSave();
    }
    
    // Broadcast player left event
    OnPlayerLeft.Broadcast(ExitingPlayer);
    OnPlayerLeftBP(ExitingPlayer);
}

AActor* ARadiantGameMode::FindPlayerSpawnLocation(AController* Player, const FString& SpawnTag)
{
    if (!GetWorld())
        return nullptr;
    
    // TODO: Use PlayerSpawnLocationsTable when implemented
    
    // Find player starts with matching tag
    TArray<AActor*> PlayerStarts;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
    
    // Filter by tag if specified
    if (!SpawnTag.IsEmpty())
    {
        PlayerStarts = PlayerStarts.FilterByPredicate([&SpawnTag](const AActor* Start)
        {
            return Start && Start->Tags.Contains(FName(*SpawnTag));
        });
    }
    
    // Return first available start
    if (PlayerStarts.Num() > 0)
    {
        return PlayerStarts[0];
    }
    
    // Fallback to PIE start if available
    for (TActorIterator<APlayerStartPIE> It(GetWorld()); It; ++It)
    {
        if (APlayerStartPIE* PIEStart = *It)
        {
            return PIEStart;
        }
    }
    
    LogGameModeStatus(TEXT("No suitable player start found"), true);
    return nullptr;
}

FTransform ARadiantGameMode::GetPlayerSpawnTransform(AController* Player, const FString& SpawnTag)
{
    if (AActor* SpawnActor = FindPlayerSpawnLocation(Player, SpawnTag))
    {
        return SpawnActor->GetActorTransform();
    }
    
    return GetDefaultSpawnTransform();
}

// === SESSION MANAGEMENT ===

void ARadiantGameMode::SetGameSessionState(EGameSessionState NewState)
{
    if (CurrentSessionState == NewState)
        return;
    
    EGameSessionState OldState = CurrentSessionState;
    CurrentSessionState = NewState;
    
    LogGameModeStatus(FString::Printf(TEXT("Game session state: %s -> %s"),
                      *UEnum::GetValueAsString(OldState),
                      *UEnum::GetValueAsString(NewState)));
    
    // Handle state transition
    HandleSessionStateChanged(OldState, NewState);
    
    // Broadcast state change
    OnGameSessionStateChanged.Broadcast(NewState);
    OnGameSessionStateChangedBP(OldState, NewState);
}

void ARadiantGameMode::HandleSessionStateChanged(EGameSessionState OldState, EGameSessionState NewState)
{
    switch (NewState)
    {
        case EGameSessionState::WaitingToStart:
            break;
            
        case EGameSessionState::InProgress:
            // Start world simulation if not already started
            if (bEnableWorldSimulation && bWorldInitializationComplete)
            {
                StartWorldSimulation();
            }
            break;
            
        case EGameSessionState::WaitingPostMatch:
            // Prepare for session end
            StopWorldSimulation();
            break;
            
        case EGameSessionState::LeavingMap:
            // Save world state before leaving
            TriggerAutoSave();
            break;
            
        case EGameSessionState::Aborted:
            // Handle emergency shutdown
            LogGameModeStatus(TEXT("Game session aborted"), true);
            break;
    }
}

void ARadiantGameMode::StartGameSession()
{
    LogGameModeStatus(TEXT("Starting game session"));
    SetGameSessionState(EGameSessionState::InProgress);
}

void ARadiantGameMode::EndGameSession()
{
    LogGameModeStatus(TEXT("Ending game session"));
    SetGameSessionState(EGameSessionState::WaitingPostMatch);
}

// === WORLD SIMULATION ===

void ARadiantGameMode::StartWorldSimulation()
{
    if (!bEnableWorldSimulation)
    {
        LogGameModeStatus(TEXT("World simulation disabled - not starting"));
        return;
    }
    
    LogGameModeStatus(TEXT("Starting world simulation systems"));
    
    // Initialize world events
    InitializeWorldEvents();
    
    // Initialize zone systems
    InitializeZoneSystems();
    
    // Initialize faction systems
    InitializeFactionSystems();
    
    // Initialize economy systems
    InitializeEconomySystems();
    
    OnWorldSimulationStartedBP();
    LogGameModeStatus(TEXT("World simulation started successfully"));
}

void ARadiantGameMode::StopWorldSimulation()
{
    LogGameModeStatus(TEXT("Stopping world simulation systems"));
    
    // TODO: Stop all world simulation systems
    // if (WorldEventManager) WorldEventManager->StopEventGeneration();
    // if (ZoneManager) ZoneManager->StopZoneSimulation();
    // if (FactionsManager) FactionsManager->StopFactionSimulation();
    // if (EconomyManager) EconomyManager->StopEconomicSimulation();
    
    OnWorldSimulationStoppedBP();
    LogGameModeStatus(TEXT("World simulation stopped"));
}

void ARadiantGameMode::RestartWorldSimulation()
{
    LogGameModeStatus(TEXT("Restarting world simulation"));
    StopWorldSimulation();
    
    // Delay restart to allow cleanup
    GetWorldTimerManager().SetTimer(WorldSimulationStartupHandle,
                                   this, &ARadiantGameMode::OnWorldSimulationStartup,
                                   2.0f, false);
}

void ARadiantGameMode::InitializeWorldEvents()
{
    if (!bEnableDynamicEvents)
    {
        LogGameModeStatus(TEXT("Dynamic events disabled - skipping world events initialization"));
        return;
    }

    /*
    if (WorldEventManager)
    {
        WorldEventManager->InitializeEventGeneration();
        WorldEventManager->StartEventGeneration();
    }
    */
    LogGameModeStatus(TEXT("World events system initialization placeholder completed"));
}

void ARadiantGameMode::InitializeZoneSystems()
{
    // TODO: Initialize zone management system
    // if (ZoneManager)
    // {
    //     ZoneManager->InitializeZones();
    //     ZoneManager->StartZoneSimulation();
    // }
    
    LogGameModeStatus(TEXT("Zone systems initialization placeholder completed"));
}

void ARadiantGameMode::InitializeFactionSystems()
{
    // TODO: Initialize faction management system
    // if (FactionsManager)
    // {
    //     FactionsManager->InitializeFactions();
    //     FactionsManager->StartFactionSimulation();
    // }
    
    LogGameModeStatus(TEXT("Faction systems initialization placeholder completed"));
}

void ARadiantGameMode::InitializeEconomySystems()
{
    // TODO: Initialize economy management system
    // if (EconomyManager)
    // {
    //     EconomyManager->InitializeEconomy();
    //     EconomyManager->StartEconomicSimulation();
    // }
    
    LogGameModeStatus(TEXT("Economy systems initialization placeholder completed"));
}

void ARadiantGameMode::ForceCompleteWorldInitialization()
{
    LogGameModeStatus(TEXT("Force completing world initialization"));
    
    // Skip to completion regardless of current phase
    SetWorldInitializationPhase(EWorldInitializationPhase::Complete);
}

// === SAVE/LOAD COORDINATION ===

bool ARadiantGameMode::SaveWorldState(const FString& SaveSlotName)
{
    if (bIsShuttingDown)
    {
        LogGameModeStatus(TEXT("Cannot save world state - game mode is shutting down"));
        return false;
    }
    
    FString SlotName = SaveSlotName.IsEmpty() ? TEXT("WorldState_Auto") : SaveSlotName;
    
    LogGameModeStatus(FString::Printf(TEXT("Saving world state to slot: %s"), *SlotName));
    
    // TODO: Coordinate save across all systems
    // if (GameManager)
    // {
    //     return GameManager->SaveGame(SlotName);
    // }
    
    // Placeholder - assume success
    LogGameModeStatus(TEXT("World state save completed (placeholder)"));
    return true;
}

bool ARadiantGameMode::LoadWorldState(const FString& SaveSlotName)
{
    FString SlotName = SaveSlotName.IsEmpty() ? TEXT("WorldState_Auto") : SaveSlotName;
    
    LogGameModeStatus(FString::Printf(TEXT("Loading world state from slot: %s"), *SlotName));
    
    // TODO: Coordinate load across all systems
    // if (GameManager)
    // {
    //     return GameManager->LoadGame(SlotName);
    // }
    
    // Placeholder - assume success
    LogGameModeStatus(TEXT("World state load completed (placeholder)"));
    return true;
}

void ARadiantGameMode::TriggerAutoSave()
{
    if (!bAutoSaveOnPlayerEvents)
        return;
    
    // Generate auto-save slot name with timestamp
    FDateTime CurrentTime = FDateTime::Now();
    FString AutoSaveSlot = FString::Printf(TEXT("AutoSave_GameMode_%s"), 
                                          *CurrentTime.ToString(TEXT("%Y%m%d_%H%M%S")));
    
    SaveWorldState(AutoSaveSlot);
}

// === SYSTEM HEALTH MONITORING ===

void ARadiantGameMode::UpdateSystemHealthCheck()
{
    if (!GameManager)
        return;
    
    // Get current memory status before health check
    float MemoryUsageMB = 0.0f;
    if (URadiantGameManager* GM = Cast<URadiantGameManager>(GameManager))
    {
        MemoryUsageMB = GM->GetMemoryUsageMB();
    }
    
    // Check if all systems are healthy
    bool bAllSystemsHealthy = GameManager->AreAllSystemsHealthy();
    
    if (!bAllSystemsHealthy)
    {
        // Only log detailed errors in development builds or when memory is critically high
        bool bShouldLogDetailed = false;
        
        #if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
            bShouldLogDetailed = true;
        #else
            if (MemoryUsageMB > 7168.0f) // Only log in shipping if memory is critical (7GB+)
            {
                bShouldLogDetailed = true;
            }
        #endif
        
        if (bShouldLogDetailed)
        {
            LogGameModeStatus(TEXT("System health check detected unhealthy systems"), true);
            
            // Get detailed system status for debugging
            TMap<FString, FString> SystemStatus = GameManager->GetSystemStatusReport();
            for (const auto& StatusPair : SystemStatus)
            {
                if (StatusPair.Value.Contains("HIGH") || StatusPair.Value.Contains("Unhealthy"))
                {
                    UE_LOG(LogTemp, Warning, TEXT("Unhealthy System - %s: %s"), *StatusPair.Key, *StatusPair.Value);
                }
            }
        }
        
        // Attempt automatic recovery for memory issues
        if (MemoryUsageMB > 5120.0f) // 5GB threshold for automatic cleanup
        {
            AttemptSystemRecovery(TEXT("Memory"));
        }
    }
    else
    {
        // Log healthy status occasionally for monitoring
        static int32 HealthyCheckCounter = 0;
        HealthyCheckCounter++;
        
        if (HealthyCheckCounter >= 10) // Every 10th health check (5 minutes)
        {
            UE_LOG(LogTemp, Verbose, TEXT("System health check: All systems healthy (Memory: %.1f MB)"), MemoryUsageMB);
            HealthyCheckCounter = 0;
        }
    }
}

void ARadiantGameMode::HandleSystemFailure(const FString& SystemName)
{
    LogGameModeStatus(FString::Printf(TEXT("System failure detected: %s"), *SystemName), true);
    
    // Attempt recovery
    if (AttemptSystemRecovery(SystemName))
    {
        LogGameModeStatus(FString::Printf(TEXT("System recovery successful: %s"), *SystemName));
    }
    else
    {
        LogGameModeStatus(FString::Printf(TEXT("System recovery failed: %s"), *SystemName), true);
        
        // TODO: Implement graceful degradation or emergency measures
    }
}

void ARadiantGameMode::CheckMemoryRecoveryStatus()
{
    if (URadiantGameManager* GM = Cast<URadiantGameManager>(GameManager))
    {
        float PostRecoveryMemory = GM->GetMemoryUsageMB();
        
        if (PostRecoveryMemory < 4096.0f) // 4GB threshold
        {
            UE_LOG(LogTemp, Log, TEXT("Memory recovery successful: %.2f MB"), PostRecoveryMemory);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Memory recovery insufficient: %.2f MB - may need manual intervention"), PostRecoveryMemory);
        }
    }
    
    GetWorldTimerManager().ClearTimer(MemoryRecoveryCheckHandle);
}

bool ARadiantGameMode::AttemptSystemRecovery(const FString& SystemName)
{
    UE_LOG(LogTemp, Warning, TEXT("Attempting recovery for system: %s"), *SystemName);
    
    // TODO: Add recovery logic for other systems when implemented
    /*
    else if (SystemName == TEXT("WorldManager"))
    {
        if (WorldManager)
        {
            WorldManager->AttemptRecovery();
        }
    }
    else if (SystemName == TEXT("ZoneManager"))
    {
        if (ZoneManager)
        {
            ZoneManager->RestartZoneSimulation();
        }
    }
    */
    return true;
}

// === TIMER CALLBACKS ===
void ARadiantGameMode::OnWorldInitializationTimeout()
{
    LogGameModeStatus(TEXT("World initialization timeout reached - forcing completion"), true);
    ForceCompleteWorldInitialization();
}

void ARadiantGameMode::OnWorldSimulationStartup()
{
    StartWorldSimulation();
}

// === HELPER FUNCTIONS ===

void ARadiantGameMode::LogGameModeStatus(const FString& Status, bool bIsError) const
{
    if (bIsError)
    {
        UE_LOG(LogTemp, Error, TEXT("RadiantGameMode: %s"), *Status);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("RadiantGameMode: %s"), *Status);
    }
}

FTransform ARadiantGameMode::GetDefaultSpawnTransform() const
{
    // Default spawn at world origin with slight elevation
    FVector DefaultLocation = FVector(0.0f, 0.0f, 200.0f);
    FRotator DefaultRotation = FRotator::ZeroRotator;
    FVector DefaultScale = FVector::OneVector;
    
    return FTransform(DefaultRotation, DefaultLocation, DefaultScale);
}

bool ARadiantGameMode::ValidateGameModeConfiguration() const
{
    bool bConfigurationValid = true;
    
    // Validate required classes
    if (!DefaultPlayerCharacterClass)
    {
        LogGameModeStatus(TEXT("DefaultPlayerCharacterClass not set"), true);
        bConfigurationValid = false;
    }
    
    if (!DefaultPlayerControllerClass)
    {
        LogGameModeStatus(TEXT("DefaultPlayerControllerClass not set"), true);
        bConfigurationValid = false;
    }
    
    // Validate timeout settings
    if (WorldInitializationTimeout <= 0.0f)
    {
        LogGameModeStatus(TEXT("WorldInitializationTimeout must be positive"), true);
        bConfigurationValid = false;
    }
    
    if (WorldSimulationStartDelay < 0.0f)
    {
        LogGameModeStatus(TEXT("WorldSimulationStartDelay cannot be negative"), true);
        bConfigurationValid = false;
    }
    
    return bConfigurationValid;
}

void ARadiantGameMode::CalculateInitializationMetrics()
{
    if (WorldInitializationStartTime > 0.0f)
    {
        double InitializationDuration = FPlatformTime::Seconds() - WorldInitializationStartTime;
        LogGameModeStatus(FString::Printf(TEXT("World initialization completed in %.3f seconds"), InitializationDuration));
    }
}