// Public/Core/RadiantGameManager.h
// Game manager for RadiantRPG - central coordinator for all major game systems

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "Types/RadiantTypes.h"
#include "RadiantGameManager.generated.h"

// Forward declarations
class URadiantWorldManager;
class URadiantZoneManager;
class URadiantFactionManager;
class URadiantEconomyManager;
class URadiantStoryManager;
class ARadiantGameState;

/** Game state data structure for internal management */
USTRUCT(BlueprintType)
struct FGameStateData
{
    GENERATED_BODY()

    /** Current game time in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game State")
    float GameTime;

    /** Current game day */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game State")
    int32 GameDay;

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
        GameTime = 0.0f;
        GameDay = 1;
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
    
    /** World simulation manager */
    //UPROPERTY()
    //TObjectPtr<URadiantWorldManager> WorldManager;

    /** Zone and location manager */
    //UPROPERTY()
    //TObjectPtr<URadiantZoneManager> ZoneManager;

    /** Faction relationship manager */
    //UPROPERTY()
    //TObjectPtr<URadiantFactionManager> FactionManager;

    /** Economy simulation manager */
    //UPROPERTY()
    //TObjectPtr<URadiantEconomyManager> EconomyManager;

    /** Dynamic story and event manager */
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

    /** Get game time in seconds since game start */
    UFUNCTION(BlueprintPure, Category = "Game State")
    float GetGameTime() const { return GameStateData.GameTime; }

    /** Get current game day */
    UFUNCTION(BlueprintPure, Category = "Game State")
    int32 GetGameDay() const { return GameStateData.GameDay; }

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
    //UFUNCTION(BlueprintPure, Category = "Systems")
    //URadiantWorldManager* GetWorldManager() const { return WorldManager; }

    /** Get zone manager subsystem */
    //UFUNCTION(BlueprintPure, Category = "Systems")
    //URadiantZoneManager* GetZoneManager() const { return ZoneManager; }

    /** Get faction manager subsystem */
    //UFUNCTION(BlueprintPure, Category = "Systems")
    //URadiantFactionManager* GetFactionManager() const { return FactionManager; }

    /** Get economy manager subsystem */
    //UFUNCTION(BlueprintPure, Category = "Systems")
    //URadiantEconomyManager* GetEconomyManager() const { return EconomyManager; }

    /** Get story manager subsystem */
    //UFUNCTION(BlueprintPure, Category = "Systems")
    //URadiantStoryManager* GetStoryManager() const { return StoryManager; }

    // === SAVE/LOAD INTERFACE ===
    
    /** Save current game state */
    UFUNCTION(BlueprintCallable, Category = "Save System")
    bool SaveGame(const FString& SaveSlotName);

    /** Load game state */
    UFUNCTION(BlueprintCallable, Category = "Save System")
    bool LoadGame(const FString& SaveSlotName);

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

    /** Check if specific system is healthy */
    UFUNCTION(BlueprintPure, Category = "System Health")
    bool IsSystemHealthy(const FString& SystemName) const;
    void PerformLightMemoryCleanup();
    void TriggerAIMemoryCleanup();

    void PerformMemoryCleanup();
    void PerformAggressiveMemoryCleanup();
    void ReduceTextureQuality();
    void RestoreTextureQuality();
    /** Force refresh system health status */
    UFUNCTION(BlueprintCallable, Category = "System Health")
    void RefreshSystemHealth();
    FString GetEnumValueAsString(const FString& EnumName, int32 EnumValue) const;

protected:
    // === INTERNAL FUNCTIONS ===
    
    /** Initialize all game systems in proper order */
    void InitializeGameSystems();

    /** Shutdown all game systems */
    void ShutdownGameSystems();

    /** Handle game state transitions */
    void HandleGameStateChange(EGameState OldState, EGameState NewState);

    /** Setup auto-save timer */
    void SetupAutoSave();

    /** Perform auto-save */
    UFUNCTION()
    void PerformAutoSave();

    /** Update game time */
    void UpdateGameTime(float DeltaTime);

    // === BLUEPRINT EVENTS ===
    
    UFUNCTION(BlueprintImplementableEvent, Category = "Game Events")
    void OnGameStateChangedBP(EGameState OldState, EGameState NewState);

    UFUNCTION(BlueprintImplementableEvent, Category = "Game Events")
    void OnDifficultyChangedBP(EDifficultyLevel NewDifficulty);

    UFUNCTION(BlueprintImplementableEvent, Category = "Game Events")
    void OnGameSettingsChangedBP(const FGameSettings& NewSettings);

    UFUNCTION(BlueprintImplementableEvent, Category = "Game Events")
    void OnGlobalFlagChangedBP(FGameplayTag Flag, bool bValue);

private:
    FTimerHandle TextureQualityRestoreHandle;
};