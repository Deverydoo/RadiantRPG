// Public/Core/RadiantGameMode.h
// Main game mode for RadiantRPG - handles player spawning, world initialization, and game flow

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "RadiantGameMode.generated.h"

// Forward declarations
class APlayerCharacter;
class ARadiantPlayerController;
class URadiantGameManager;
class ARadiantGameState;
class ARadiantPlayerState;

UENUM(BlueprintType)
enum class EWorldInitializationPhase : uint8
{
    PreInit UMETA(DisplayName = "Pre-Initialization"),
    SystemsStartup UMETA(DisplayName = "Systems Startup"),
    WorldGeneration UMETA(DisplayName = "World Generation"),
    AIInitialization UMETA(DisplayName = "AI Initialization"),
    PlayerSpawn UMETA(DisplayName = "Player Spawn"),
    PostInit UMETA(DisplayName = "Post-Initialization"),
    Complete UMETA(DisplayName = "Complete")
};

UENUM(BlueprintType)
enum class EGameSessionState : uint8
{
    WaitingToStart UMETA(DisplayName = "Waiting to Start"),
    InProgress UMETA(DisplayName = "In Progress"),
    WaitingPostMatch UMETA(DisplayName = "Waiting Post Match"),
    LeavingMap UMETA(DisplayName = "Leaving Map"),
    Aborted UMETA(DisplayName = "Aborted")
};

// Game mode events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldInitializationPhaseChanged, EWorldInitializationPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameSessionStateChanged, EGameSessionState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerJoined, APlayerController*, JoinedPlayer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerLeft, APlayerController*, LeftPlayer);

/**
 * RadiantGameMode - Main game mode for RadiantRPG
 * 
 * Responsibilities:
 * - World initialization and system startup coordination
 * - Player spawning and controller assignment
 * - Game session management (start, pause, end)
 * - Player persistence and save/load coordination
 * - AI population and world event initialization
 * - Zone transition management
 * - Multiplayer session handling (future expansion)
 * 
 * The GameMode acts as the authoritative coordinator for world state
 * and ensures proper initialization order for all game systems.
 */
UCLASS(Blueprintable)
class RADIANTRPG_API ARadiantGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ARadiantGameMode();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

    // === WORLD INITIALIZATION ===
    
    /** Current world initialization phase */
    UPROPERTY(BlueprintReadOnly, Category = "Initialization")
    EWorldInitializationPhase CurrentInitPhase;

    /** Current game session state */
    UPROPERTY(BlueprintReadOnly, Category = "Game Session")
    EGameSessionState CurrentSessionState;

    /** Whether world initialization is complete */
    UPROPERTY(BlueprintReadOnly, Category = "Initialization")
    bool bWorldInitializationComplete;

    /** Whether AI systems should be active */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Settings")
    bool bEnableAISystems;

    /** Whether world simulation should run */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings")
    bool bEnableWorldSimulation;

    /** Whether dynamic events should be generated */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings")
    bool bEnableDynamicEvents;

    // === PLAYER MANAGEMENT ===
    
    /** Default player character class */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
    TSubclassOf<APlayerCharacter> DefaultPlayerCharacterClass;

    /** Default player controller class */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
    TSubclassOf<ARadiantPlayerController> DefaultPlayerControllerClass;

    /** Player spawn locations data table */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
    UDataTable* PlayerSpawnLocationsTable;

    /** Currently active players */
    UPROPERTY(BlueprintReadOnly, Category = "Player")
    TArray<APlayerController*> ActivePlayers;

    // === CACHED REFERENCES ===
    
    /** Cached reference to game manager */
    UPROPERTY(BlueprintReadOnly, Category = "Systems")
    URadiantGameManager* GameManager;

    /** Cached reference to game state */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    ARadiantGameState* RadiantGameState;

    // === CONFIGURATION ===
    
    /** World initialization timeout in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    float WorldInitializationTimeout;

    /** Whether to auto-save on player join/leave */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    bool bAutoSaveOnPlayerEvents;

    /** Delay before starting world simulation after player spawn */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    float WorldSimulationStartDelay;

public:
    // === EVENTS ===
    
    UPROPERTY(BlueprintAssignable, Category = "Game Mode Events")
    FOnWorldInitializationPhaseChanged OnWorldInitializationPhaseChanged;

    UPROPERTY(BlueprintAssignable, Category = "Game Mode Events")
    FOnGameSessionStateChanged OnGameSessionStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Game Mode Events")
    FOnPlayerJoined OnPlayerJoined;

    UPROPERTY(BlueprintAssignable, Category = "Game Mode Events")
    FOnPlayerLeft OnPlayerLeft;

    // === GAMEMODE BASE OVERRIDES ===
    
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
    virtual void RestartPlayer(AController* NewPlayer) override;
    virtual bool ShouldSpawnAtStartSpot(AController* Player) override;
    virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName = TEXT("")) override;

    // === PUBLIC INTERFACE ===
    
    /** Get current world initialization phase */
    UFUNCTION(BlueprintPure, Category = "Game Mode")
    EWorldInitializationPhase GetCurrentInitPhase() const { return CurrentInitPhase; }

    /** Get current game session state */
    UFUNCTION(BlueprintPure, Category = "Game Session")
    EGameSessionState GetCurrentSessionState() const { return CurrentSessionState; }

    /** Check if world initialization is complete */
    UFUNCTION(BlueprintPure, Category = "Game Mode")
    bool IsWorldInitializationComplete() const { return bWorldInitializationComplete; }

    /** Get number of active players */
    UFUNCTION(BlueprintPure, Category = "Player")
    int32 GetActivePlayerCount() const { return ActivePlayers.Num(); }

    /** Get all active players */
    UFUNCTION(BlueprintPure, Category = "Player")
    TArray<APlayerController*> GetActivePlayers() const { return ActivePlayers; }

    /** Start world simulation systems */
    UFUNCTION(BlueprintCallable, Category = "World")
    void StartWorldSimulation();

    /** Stop world simulation systems */
    UFUNCTION(BlueprintCallable, Category = "World")
    void StopWorldSimulation();

    /** Restart world simulation */
    UFUNCTION(BlueprintCallable, Category = "World")
    void RestartWorldSimulation();

    /** Force complete world initialization */
    UFUNCTION(BlueprintCallable, Category = "Game Mode")
    void ForceCompleteWorldInitialization();

protected:
    // === INITIALIZATION PROCESS ===
    
    /** Start world initialization sequence */
    void StartWorldInitialization();

    /** Update world initialization phase */
    void SetWorldInitializationPhase(EWorldInitializationPhase NewPhase);

    /** Initialize game systems */
    void InitializeGameSystems();

    /** Initialize world generation */
    void InitializeWorldGeneration();

    /** Initialize AI systems */
    void InitializeAISystems();

    /** Complete world initialization */
    void CompleteWorldInitialization();

    // === PLAYER MANAGEMENT ===
    
    /** Initialize player character */
    void InitializePlayerCharacter(APlayerCharacter* PlayerCharacter, APlayerController* PlayerController);

    /** Handle player joining */
    void HandlePlayerJoined(APlayerController* NewPlayer);

    /** Handle player leaving */
    void HandlePlayerLeft(APlayerController* ExitingPlayer);

    /** Find appropriate spawn location for player */
    AActor* FindPlayerSpawnLocation(AController* Player, const FString& SpawnTag = TEXT(""));

    /** Get spawn transform for player */
    FTransform GetPlayerSpawnTransform(AController* Player, const FString& SpawnTag = TEXT(""));

    // === SESSION MANAGEMENT ===
    
    /** Set game session state */
    void SetGameSessionState(EGameSessionState NewState);

    /** Handle session state transitions */
    void HandleSessionStateChanged(EGameSessionState OldState, EGameSessionState NewState);

    /** Start game session */
    void StartGameSession();

    /** End game session */
    void EndGameSession();

    // === WORLD SIMULATION ===
    
    /** Initialize world events system */
    void InitializeWorldEvents();

    /** Initialize zone systems */
    void InitializeZoneSystems();

    /** Initialize faction systems */
    void InitializeFactionSystems();

    /** Initialize economy systems */
    void InitializeEconomySystems();

    // === SAVE/LOAD COORDINATION ===
    
    /** Save world state */
    UFUNCTION(BlueprintCallable, Category = "Save/Load")
    bool SaveWorldState(const FString& SaveSlotName = TEXT(""));

    /** Load world state */
    UFUNCTION(BlueprintCallable, Category = "Save/Load")
    bool LoadWorldState(const FString& SaveSlotName = TEXT(""));

    /** Auto-save world state */
    void TriggerAutoSave();

    // === SYSTEM HEALTH MONITORING ===
    
    /** Check system health */
    void UpdateSystemHealthCheck();

    /** Handle system failure */
    void HandleSystemFailure(const FString& SystemName);

    /** Attempt system recovery */
    bool AttemptSystemRecovery(const FString& SystemName);

    // === TIMER CALLBACKS ===
    
    /** World initialization timeout callback */
    UFUNCTION()
    void OnWorldInitializationTimeout();

    /** World simulation startup timer */
    UFUNCTION()
    void OnWorldSimulationStartup();

    // === BLUEPRINT EVENTS ===
    
    /** Called when world initialization phase changes */
    UFUNCTION(BlueprintImplementableEvent, Category = "Game Mode Events")
    void OnWorldInitializationPhaseChangedBP(EWorldInitializationPhase OldPhase, EWorldInitializationPhase NewPhase);

    /** Called when game session state changes */
    UFUNCTION(BlueprintImplementableEvent, Category = "Game Mode Events")
    void OnGameSessionStateChangedBP(EGameSessionState OldState, EGameSessionState NewState);

    /** Called when player joins the game */
    UFUNCTION(BlueprintImplementableEvent, Category = "Game Mode Events")
    void OnPlayerJoinedBP(APlayerController* NewPlayer);

    /** Called when player leaves the game */
    UFUNCTION(BlueprintImplementableEvent, Category = "Game Mode Events")
    void OnPlayerLeftBP(APlayerController* ExitingPlayer);

    /** Called when world initialization completes */
    UFUNCTION(BlueprintImplementableEvent, Category = "Game Mode Events")
    void OnWorldInitializationCompleteBP();

    /** Called when world simulation starts */
    UFUNCTION(BlueprintImplementableEvent, Category = "World Events")
    void OnWorldSimulationStartedBP();

    /** Called when world simulation stops */
    UFUNCTION(BlueprintImplementableEvent, Category = "World Events")
    void OnWorldSimulationStoppedBP();

private:
    // === INTERNAL STATE ===
    
    /** Initialization timer handle */
    FTimerHandle InitializationTimeoutHandle;

    /** World simulation startup timer handle */
    FTimerHandle WorldSimulationStartupHandle;

    /** Auto-save timer handle */
    FTimerHandle AutoSaveTimerHandle;

    /** Whether game mode is shutting down */
    bool bIsShuttingDown;

    /** World initialization start time for metrics */
    double WorldInitializationStartTime;

    /** System health check interval */
    static constexpr float SystemHealthCheckInterval = 30.0f;

    /** Last system health check time */
    float LastSystemHealthCheckTime;

    // === HELPER FUNCTIONS ===
    
    /** Log game mode status */
    void LogGameModeStatus(const FString& Status, bool bIsError = false) const;

    /** Get default spawn transform */
    FTransform GetDefaultSpawnTransform() const;

    /** Validate game mode configuration */
    bool ValidateGameModeConfiguration() const;

    /** Calculate initialization metrics */
    void CalculateInitializationMetrics();
};