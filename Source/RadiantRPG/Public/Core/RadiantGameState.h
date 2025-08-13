// Public/Core/RadiantGameState.h
// Game state implementation for RadiantRPG - manages replicated world state (updated for simplified time)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GameplayTagContainer.h"
#include "Types/RadiantTypes.h"
#include "Types/TimeTypes.h"
#include "Net/UnrealNetwork.h"
#include "Types/GlobalVariablesTypes.h"
#include "RadiantGameState.generated.h"

// Forward declarations
class URadiantGameManager;
class URadiantWorldManager;
class ARadiantPlayerState;

/**
 * RadiantGameState - Replicated world state manager for RadiantRPG
 * 
 * Responsibilities:
 * - Replicated world time systems (delegated to WorldManager)
 * - Global faction relationships and warfare status
 * - Active world events and their parameters
 * - Global gameplay flags and variables
 * - Player state management and coordination
 * - World simulation state synchronization
 * 
 * This class ensures all clients stay synchronized with the authoritative
 * world state and provides a central point for world-level queries.
 * 
 * TIME MANAGEMENT NOTE: Time logic is now handled by WorldManager.
 * GameState focuses on replication and network synchronization.
 */
UCLASS(BlueprintType)
class RADIANTRPG_API ARadiantGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    ARadiantGameState();

    // AActor interface
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

    // Replication
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    // === REPLICATED STATE ===
    
    /** Replicated world time (synchronized from WorldManager on authority) */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "World Time", ReplicatedUsing = OnRep_WorldTime)
    FSimpleWorldTime WorldTime;

    /** Whether time progression is enabled */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "World Time")
    bool bTimeProgressionEnabled;

    /** Active world events (replicated) */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "World Events", ReplicatedUsing = OnRep_ActiveWorldEvents)
    TArray<FWorldEventData> ActiveWorldEvents;

    /** Global gameplay flags (replicated) */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Global State", ReplicatedUsing = OnRep_GlobalFlags)
    FGameplayTagContainer GlobalFlags;

    /** Faction relationships and wars (replicated) */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Factions", ReplicatedUsing = OnRep_ActiveWars)
    TArray<FGameplayTag> ActiveWars;

    /** Global numeric variables (replicated) */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Global State")
    FGlobalVariablesContainer GlobalVariables;

    /** Global string variables (replicated) */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Global State")
    FGlobalStringsContainer  GlobalStrings;

    // === CONFIGURATION ===
    
    /** Maximum number of concurrent world events */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Events")
    int32 MaxConcurrentEvents;

    // === CACHED REFERENCES ===
    
    /** Cached reference to game manager */
    UPROPERTY()
    TObjectPtr<URadiantGameManager> GameManager;

    /** Cached reference to world manager (for time delegation) */
    UPROPERTY()
    TObjectPtr<URadiantWorldManager> WorldManager;

    // === INTERNAL STATE ===
    
    /** Last time events were updated */
    float LastEventUpdateTime;

    /** Next event ID counter */
    int32 NextEventID;

    /** Whether game state has been initialized */
    bool bGameStateInitialized;

public:
    // === EVENTS ===
    
    UPROPERTY(BlueprintAssignable, Category = "World Events")
    FOnSimpleTimeChanged OnWorldTimeChanged;

    UPROPERTY(BlueprintAssignable, Category = "World Events")
    FOnWorldEventStarted OnWorldEventStarted;

    UPROPERTY(BlueprintAssignable, Category = "World Events")
    FOnWorldEventEnded OnWorldEventEnded;

    UPROPERTY(BlueprintAssignable, Category = "Faction Events")
    FOnFactionRelationshipChanged OnFactionRelationshipChanged;

    UPROPERTY(BlueprintAssignable, Category = "Global Events")
    FOnGlobalFlagChanged OnGlobalFlagChanged;

    

    // === WORLD TIME INTERFACE (DELEGATED TO WORLDMANAGER) ===
    
    /** Get current world time data (from replicated state) */
    UFUNCTION(BlueprintPure, Category = "World Time")
    const FSimpleWorldTime& GetWorldTime() const { return WorldTime; }

    /** Set world time directly (Authority only) - delegates to WorldManager */
    UFUNCTION(BlueprintCallable, Category = "World Time", CallInEditor)
    void SetWorldTime(const FSimpleWorldTime& NewTimeData);

    /** Add time to world clock - delegates to WorldManager */
    UFUNCTION(BlueprintCallable, Category = "World Time")
    void AddWorldTime(float SecondsToAdd);

    /** Set time scale - delegates to WorldManager */
    UFUNCTION(BlueprintCallable, Category = "World Time")
    void SetTimeScale(float NewTimeScale);

    /** Pause/unpause time - delegates to WorldManager */
    UFUNCTION(BlueprintCallable, Category = "World Time")
    void SetTimePaused(bool bPaused);

    /** Get current time as hours (0-23) - uses direct property */
    UFUNCTION(BlueprintPure, Category = "World Time")
    int32 GetCurrentHour() const { return WorldTime.Hour; }

    /** Get current time as minutes (0-59) - uses direct property */
    UFUNCTION(BlueprintPure, Category = "World Time")
    int32 GetCurrentMinutes() const { return WorldTime.Minute; }

    /** Get current day */
    UFUNCTION(BlueprintPure, Category = "World Time")
    int32 GetCurrentDay() const { return WorldTime.Day; }

    /** Get current season */
    UFUNCTION(BlueprintPure, Category = "World Time")
    int32 GetCurrentSeason() const { return WorldTime.Season; }

    /** Get time of day enum */
    UFUNCTION(BlueprintPure, Category = "World Time")
    ETimeOfDay GetTimeOfDay() const { return WorldTime.GetTimeOfDay(); }

    /** Check if it's currently day time */
    UFUNCTION(BlueprintPure, Category = "World Time")
    bool IsDaytime() const { return WorldTime.IsDaytime(); }

    /** Check if it's currently night time */
    UFUNCTION(BlueprintPure, Category = "World Time")
    bool IsNighttime() const { return WorldTime.IsNighttime(); }

    /** Get formatted time string */
    UFUNCTION(BlueprintPure, Category = "World Time")
    FString GetFormattedTimeString() const { return WorldTime.GetTimeString(); }

    /** Get formatted date string */
    UFUNCTION(BlueprintPure, Category = "World Time")
    FString GetFormattedDateString() const { return WorldTime.GetDateString(); }

    // === WORLD EVENTS INTERFACE ===
    
    /** Add world event (Authority only) */
    UFUNCTION(BlueprintCallable, Category = "World Events")
    void AddWorldEvent(const FWorldEventData& EventData);

    /** Remove world event by ID (Authority only) */
    UFUNCTION(BlueprintCallable, Category = "World Events")
    bool RemoveWorldEvent(int32 EventID);

    /** Get active world events */
    UFUNCTION(BlueprintPure, Category = "World Events")
    TArray<FWorldEventData> GetActiveWorldEvents() const { return ActiveWorldEvents; }

    /** Check if a specific event is active */
    UFUNCTION(BlueprintPure, Category = "World Events")
    bool IsEventActive(int32 EventID) const;

    // === FACTION INTERFACE ===
    UFUNCTION(BlueprintPure, Category = "Factions")
    bool AreFactionsAtWar(FGameplayTag FactionA, FGameplayTag FactionB) const;

    /** Get faction relationship value (-1.0 = hostile, 0.0 = neutral, 1.0 = allied) */
    UFUNCTION(BlueprintPure, Category = "Factions")
    float GetFactionRelationship(FGameplayTag FactionA, FGameplayTag FactionB) const;

    /** Get all active wars */
    UFUNCTION(BlueprintPure, Category = "Factions")
    TArray<FGameplayTag> GetActiveWars() const { return ActiveWars; }
    
    /** Start a war between factions (Authority only) */
    UFUNCTION(BlueprintCallable, Category = "Factions")
    void StartWar(FGameplayTag FactionA, FGameplayTag FactionB);

    /** End a war between factions (Authority only) */
    UFUNCTION(BlueprintCallable, Category = "Factions")
    void EndWar(FGameplayTag FactionA, FGameplayTag FactionB);

    // === GLOBAL STATE INTERFACE ===
    
    /** Set global flag (Authority only) */
    UFUNCTION(BlueprintCallable, Category = "Global State")
    void SetGlobalFlag(FGameplayTag Flag, bool bValue);

    /** Check if global flag is set */
    UFUNCTION(BlueprintPure, Category = "Global State")
    bool HasGlobalFlag(FGameplayTag Flag) const { return GlobalFlags.HasTag(Flag); }

    /** Get all global flags */
    UFUNCTION(BlueprintPure, Category = "Global State")
    FGameplayTagContainer GetGlobalFlags() const { return GlobalFlags; }

    /** Set global variable (Authority only) */
    UFUNCTION(BlueprintCallable, Category = "Global State")
    void SetGlobalVariable(const FString& VariableName, float Value);

    /** Get global variable */
    UFUNCTION(BlueprintPure, Category = "Global State")
    float GetGlobalVariable(const FString& VariableName, float DefaultValue = 0.0f) const;

    /** Set global string (Authority only) */
    UFUNCTION(BlueprintCallable, Category = "Global State")
    void SetGlobalString(const FString& VariableName, const FString& Value);

    /** Get global string */
    UFUNCTION(BlueprintPure, Category = "Global State")
    FString GetGlobalString(const FString& VariableName, const FString& DefaultValue = "") const;

    // === UTILITY FUNCTIONS ===
    
    /** Get world manager reference */
    UFUNCTION(BlueprintPure, Category = "Systems")
    URadiantWorldManager* GetWorldManager() const { return WorldManager; }

    /** Force sync time from WorldManager (Authority only) */
    UFUNCTION(BlueprintCallable, Category = "World Time")
    void SyncTimeFromWorldManager();

protected:
    // === INTERNAL FUNCTIONS ===
    
    /** Initialize world state on authority */
    void InitializeWorldState();

    /** Update world state (called on authority) */
    void UpdateWorldState(float DeltaTime);

    /** Update world events */
    void UpdateWorldEvents(float DeltaTime);

    /** Validate world event data */
    bool ValidateWorldEventData(const FWorldEventData& EventData) const;

    /** Generate faction relationship key for local storage */
    FString GetFactionRelationshipKey(FGameplayTag FactionA, FGameplayTag FactionB) const;

    // === REPLICATION CALLBACKS ===
    
    UFUNCTION()
    void OnRep_WorldTime();

    UFUNCTION()
    void OnRep_ActiveWorldEvents();

    UFUNCTION()
    void OnRep_GlobalFlags();

    UFUNCTION()
    void OnRep_ActiveWars();

    // === SERVER RPC IMPLEMENTATIONS ===
    
    UFUNCTION(Server, Reliable)
    void ServerSetGlobalVariable(const FString& VariableName, float Value);

    UFUNCTION(Server, Reliable)
    void ServerSetGlobalString(const FString& VariableName, const FString& Value);

    // === BLUEPRINT EVENTS ===
    
    UFUNCTION(BlueprintImplementableEvent, Category = "World Events")
    void OnWorldTimeChangedBP(const FSimpleWorldTime& NewTimeData);

    UFUNCTION(BlueprintImplementableEvent, Category = "World Events")
    void OnWorldEventStartedBP(const FWorldEventData& EventData);

    UFUNCTION(BlueprintImplementableEvent, Category = "World Events")
    void OnWorldEventEndedBP(const FWorldEventData& EventData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Faction Events")
    void OnFactionRelationshipChangedBP(FGameplayTag FactionA, FGameplayTag FactionB, float NewRelationshipValue);

    UFUNCTION(BlueprintImplementableEvent, Category = "Global Events")  
    void OnGlobalFlagChangedBP(FGameplayTag Flag, bool bValue);

private:
    void ServerSetGlobalVariable_Implementation(const FString& VariableName, float Value);
    void ServerSetGlobalString_Implementation(const FString& VariableName, const FString& Value);
};