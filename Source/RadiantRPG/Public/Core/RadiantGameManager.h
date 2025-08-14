// Public/Core/RadiantGameManager.h
// Game manager for RadiantRPG - central coordinator for all major game systems (simplified)

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "Types/RadiantTypes.h"
#include "Types/SystemTypes.h"
#include "World/ISimpleTimeManager.h"
#include "RadiantGameManager.generated.h"

// Forward declarations
class URadiantWorldManager;
class ARadiantGameState;

/** Simplified game state data structure - TIME REMOVED (handled by WorldManager) */
USTRUCT(BlueprintType)
struct FGameStateData
{
    GENERATED_BODY()

    /** Global gameplay flags */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game State")
    FGameplayTagContainer GlobalFlags;

    /** Global numeric variables */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game State")
    TMap<FString, float> GlobalVariables;

    /** Global string variables */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game State")
    TMap<FString, FString> GlobalStrings;

    FGameStateData()
    {
        // No more time data - WorldManager handles this
    }
};

/**
 * GameManager - Central coordinator for all major game systems
 * 
 * Responsibilities:
 * - Game state management (menu, loading, playing, paused)
 * - System initialization and coordination  
 * - Global game settings and difficulty
 * - Cross-system communication hub
 * - Save/Load game state coordination
 * - Performance and system health monitoring
 * 
 * TIME MANAGEMENT REMOVED: All time operations delegated to WorldManager
 * This is the primary entry point for game-wide operations
 * and ensures proper system initialization order.
 */
UCLASS(BlueprintType, Blueprintable)
class RADIANTRPG_API URadiantGameManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    URadiantGameManager();

    // USubsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    void OnAIManagerReady();
    virtual void Deinitialize() override;

    // UTickableWorldSubsystem interface
    virtual void Tick(float DeltaTime);
    virtual bool IsTickable() const;
    virtual TStatId GetStatId() const;

protected:
    // === CORE GAME STATE ===
    
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    EGameState CurrentGameState;

    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    EGameState PreviousGameState;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Settings")
    FGameSettings CurrentGameSettings;

    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    FGameStateData GameStateData;

    // === SYSTEM MANAGERS ===
    
    /** World simulation manager (also handles time) */
    UPROPERTY()
    TObjectPtr<URadiantWorldManager> WorldManager;

    /** Future system managers (commented until implemented) */
    //UPROPERTY()
    //TObjectPtr<URadiantFactionManager> FactionManager;

    //UPROPERTY()
    //TObjectPtr<URadiantEconomyManager> EconomyManager;

    //UPROPERTY()
    //TObjectPtr<URadiantStoryManager> StoryManager;

    // === INTERNAL STATE ===
    
    /** Whether the manager is initialized */
    bool bIsInitialized;

    /** Whether the game is shutting down */
    bool bIsShuttingDown;
    
    /** Auto-save timer */
    FTimerHandle AutoSaveTimerHandle;
    
    /** System initialization order */
    TArray<TSubclassOf<UGameInstanceSubsystem>> SystemInitializationOrder;

public:

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game Manager")
    bool IsInitialized() const { return bIsInitialized; }
    
    // === EVENTS ===
    
    UPROPERTY(BlueprintAssignable, Category = "Game Events")
    FOnGameStateChanged OnGameStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Game Events")
    FOnDifficultyChanged OnDifficultyChanged;

    UPROPERTY(BlueprintAssignable, Category = "Game Events")
    FOnGameSettingsChanged OnGameSettingsChanged;

    UPROPERTY(BlueprintAssignable, Category = "Game Events")
    FOnGlobalFlagChanged OnGlobalFlagChanged;

    // === GAME STATE INTERFACE ===
    
    /** Get current game state */
    UFUNCTION(BlueprintPure, Category = "Game State")
    EGameState GetGameState() const { return CurrentGameState; }

    /** Set game state with proper transition handling */
    UFUNCTION(BlueprintCallable, Category = "Game State")
    void SetGameState(EGameState NewState);

    /** Check if game is currently in a playable state */
    UFUNCTION(BlueprintPure, Category = "Game State")
    bool IsInPlayableState() const;
    void OnGameStateTransition(EGameState FromState, EGameState ToState);

    // === TIME INTERFACE (DELEGATED TO WORLDMANAGER) ===
    
    /** Get time manager interface from WorldManager */
    UFUNCTION(BlueprintPure, Category = "Time System")
    URadiantWorldManager* GetTimeManager() const { return WorldManager; }

    /** Get current game time (delegated) */
    UFUNCTION(BlueprintPure, Category = "Time System")
    FString GetCurrentTimeString() const;

    /** Get current game day (delegated) */
    UFUNCTION(BlueprintPure, Category = "Time System")
    int32 GetCurrentGameDay() const;

    /** Get current season (delegated) */
    UFUNCTION(BlueprintPure, Category = "Time System")
    int32 GetCurrentSeason() const;
    bool IsDaytime() const;

    /** Check if it's daytime (delegated) */
    UFUNCTION(BlueprintPure, Category = "Time System")
    bool IsCurrentlyDaytime() const;

    // === DIFFICULTY AND SETTINGS ===
    
    /** Get current difficulty level */
    UFUNCTION(BlueprintPure, Category = "Game Settings")
    EDifficultyLevel GetDifficultyLevel() const { return CurrentGameSettings.DifficultyLevel; }

    /** Set difficulty level and apply settings */
    UFUNCTION(BlueprintCallable, Category = "Game Settings")
    void SetDifficultyLevel(EDifficultyLevel NewDifficulty);

    /** Get current game settings */
    UFUNCTION(BlueprintPure, Category = "Game Settings")
    const FGameSettings& GetGameSettings() const { return CurrentGameSettings; }

    /** Apply new game settings */
    UFUNCTION(BlueprintCallable, Category = "Game Settings")
    void ApplyGameSettings(const FGameSettings& NewSettings);

    // === GLOBAL STATE MANAGEMENT ===
    
    /** Set a global gameplay flag */
    UFUNCTION(BlueprintCallable, Category = "Global State")
    void SetGlobalFlag(FGameplayTag Flag, bool bValue);

    /** Check if a global flag is set */
    UFUNCTION(BlueprintPure, Category = "Global State")
    bool HasGlobalFlag(FGameplayTag Flag) const;

    /** Remove a global flag */
    UFUNCTION(BlueprintCallable, Category = "Global State")
    void RemoveGlobalFlag(FGameplayTag Flag);

    /** Get all global flags */
    UFUNCTION(BlueprintPure, Category = "Global State")
    FGameplayTagContainer GetGlobalFlags() const { return GameStateData.GlobalFlags; }

    /** Set a global numeric variable */
    UFUNCTION(BlueprintCallable, Category = "Global State")
    void SetGlobalVariable(const FString& VariableName, float Value);

    /** Get a global numeric variable */
    UFUNCTION(BlueprintPure, Category = "Global State")
    float GetGlobalVariable(const FString& VariableName, float DefaultValue = 0.0f) const;

    /** Set a global string variable */
    UFUNCTION(BlueprintCallable, Category = "Global State")
    void SetGlobalString(const FString& VariableName, const FString& Value);

    /** Get a global string variable */
    UFUNCTION(BlueprintPure, Category = "Global State")
    FString GetGlobalString(const FString& VariableName, const FString& DefaultValue = "") const;

    // === SYSTEM ACCESSORS ===
    
    /** Get world manager subsystem */
    UFUNCTION(BlueprintPure, Category = "Systems")
    URadiantWorldManager* GetWorldManager() const { return WorldManager; }

    // === SAVE/LOAD INTERFACE ===
    
    /** Save current game state */
    UFUNCTION(BlueprintCallable, Category = "Save System")
    bool SaveGame(const FString& SaveSlotName);

    /** Load game state */
    UFUNCTION(BlueprintCallable, Category = "Save System")
    bool LoadGame(const FString& SaveSlotName);
    bool DoesSaveExist(const FString& SaveSlotName) const;

    /** Check if save slot exists */
    UFUNCTION(BlueprintPure, Category = "Save System")
    bool DoesSaveSlotExist(const FString& SaveSlotName) const;

    /** Get available save slots */
    UFUNCTION(BlueprintPure, Category = "Save System")
    TArray<FString> GetAvailableSaveSlots() const;

    /** Delete save slot */
    UFUNCTION(BlueprintCallable, Category = "Save System")
    bool DeleteSaveSlot(const FString& SaveSlotName);

    // === UTILITY FUNCTIONS ===
    
    /** Force garbage collection */
    UFUNCTION(BlueprintCallable, Category = "Performance", CallInEditor)
    void ForceGarbageCollection();

    /** Get memory usage statistics */
    UFUNCTION(BlueprintPure, Category = "Performance")
    float GetMemoryUsageMB() const;

    /** Reset all game systems to default state */
    UFUNCTION(BlueprintCallable, Category = "Debug", CallInEditor)
    void ResetAllSystems();

    /** Check if all game systems are healthy */
    UFUNCTION(BlueprintPure, Category = "System Health")
    bool AreAllSystemsHealthy() const;

    /** Get detailed system status report */
    UFUNCTION(BlueprintPure, Category = "System Health")
    TMap<FString, FString> GetSystemStatusReport() const;
    void OnOtherSystemInitialized(const FSystemInitializationEvent& Event);
    void OnWorldManagerReady();

protected:
    // === BLUEPRINT EVENTS ===
    
    UFUNCTION(BlueprintImplementableEvent, Category = "Game Events")
    void OnGameStateChangedBP(EGameState OldState, EGameState NewState);

    UFUNCTION(BlueprintImplementableEvent, Category = "Game Events")
    void OnDifficultyChangedBP(EDifficultyLevel NewDifficulty);

    UFUNCTION(BlueprintImplementableEvent, Category = "Game Events")
    void OnGameSettingsChangedBP(const FGameSettings& NewSettings);

    UFUNCTION(BlueprintImplementableEvent, Category = "Game Events")
    void OnGlobalFlagChangedBP(FGameplayTag Flag, bool bValue);

    // === INTERNAL FUNCTIONS ===
    
    /** Initialize game systems in proper order */
    bool InitializeGameSystems();

    /** Shutdown game systems */
    void ShutdownGameSystems();

    /** Update system health monitoring */
    void UpdateSystemHealth(float DeltaTime);
    void UpdateGlobalSystems(float DeltaTime);

    /** Setup auto-save timer */
    void SetupAutoSaveTimer();

    /** Perform automatic save */
    void PerformAutoSave();

    /** Check system health by name */
    bool IsSystemHealthy(const FString& SystemName) const;

    /** Get enum value as string for debugging */
    FString GetEnumValueAsString(const FString& EnumName, int32 EnumValue) const;

    /** Memory cleanup when usage is high */
    void PerformLightMemoryCleanup();

private:
    /** Last memory warning time for throttling */
    float LastMemoryWarningTime = 0.0f;
};