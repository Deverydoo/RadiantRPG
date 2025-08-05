// Public/Core/RadiantGameState.h
// Game state implementation for RadiantRPG - manages replicated world state

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GameplayTagContainer.h"
#include "Types/RadiantTypes.h"
#include "Net/UnrealNetwork.h"
#include "RadiantGameState.generated.h"

// Forward declarations
class URadiantGameManager;
class ARadiantPlayerState;

/**
 * RadiantGameState - Replicated world state manager for RadiantRPG
 * 
 * Responsibilities:
 * - Replicated world time systems  
 * - Global faction relationships and warfare status
 * - Active world events and their parameters
 * - Global gameplay flags and variables
 * - Player state management and coordination
 * - World simulation state synchronization
 * 
 * This class ensures all clients stay synchronized with the authoritative
 * world state and provides a central point for world-level queries.
 */
UCLASS(BlueprintType, Blueprintable)
class RADIANTRPG_API ARadiantGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    ARadiantGameState();

    // === GAMESTATE BASE OVERRIDES ===
    
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    void InitializeWorldState();
    virtual void PostInitializeComponents() override;

protected:
    // === REPLICATED WORLD STATE ===
    
    /** Current world time data */
    UPROPERTY(ReplicatedUsing = OnRep_WorldTime, BlueprintReadOnly, Category = "World Time")
    FWorldTimeData WorldTime;

    // TODO: Weather system placeholder - uncomment when weather system is implemented
    // /** Current world weather data */
    // UPROPERTY(ReplicatedUsing = OnRep_WorldWeather, BlueprintReadOnly, Category = "World Weather")
    // FWorldWeatherData WorldWeather;

    /** Faction relationships - using TArray for replication compatibility */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Factions")
    TArray<FString> KnownFactions;

    /** Currently active world events */
    UPROPERTY(ReplicatedUsing = OnRep_ActiveWorldEvents, BlueprintReadOnly, Category = "World Events")
    TArray<FWorldEventData> ActiveWorldEvents;

    /** Global gameplay flags */
    UPROPERTY(ReplicatedUsing = OnRep_GlobalFlags, BlueprintReadOnly, Category = "Global State") 
    FGameplayTagContainer GlobalFlags;

    /** Active faction wars */
    UPROPERTY(ReplicatedUsing = OnRep_ActiveWars, BlueprintReadOnly, Category = "Factions")
    TArray<FGameplayTag> ActiveWars;

    // === NON-REPLICATED LOCAL DATA ===
    
    /** Faction relationships stored locally (non-replicated to avoid TMap issues) */
    UPROPERTY(BlueprintReadOnly, Category = "Factions")
    TMap<FString, FFactionRelationshipData> FactionRelationships;

    /** Global numeric variables (server-managed via RPCs) */
    UPROPERTY(BlueprintReadOnly, Category = "Global State")
    TMap<FString, float> GlobalVariables;

    /** Global string variables (server-managed via RPCs) */
    UPROPERTY(BlueprintReadOnly, Category = "Global State")
    TMap<FString, FString> GlobalStrings;

    // === CONFIGURATION ===
    
    /** Seconds per game day (real time) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Time")
    float SecondsPerGameDay;

    /** Whether to automatically update time of day */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Time")
    bool bAutoUpdateTimeOfDay;

    // TODO: Weather system placeholders - uncomment when implemented
    // /** Whether dynamic weather is enabled */
    // UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Weather")
    // bool bDynamicWeather;
    //
    // /** Chance per minute for weather to change (0-1) */
    // UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Weather")
    // float WeatherChangeChance;

    /** Maximum number of concurrent world events */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Events")
    int32 MaxConcurrentEvents;

    // === CACHED REFERENCES ===
    
    /** Cached reference to game manager */
    UPROPERTY()
    TObjectPtr<URadiantGameManager> GameManager;

    // === INTERNAL STATE ===
    
    // TODO: Weather system placeholders
    // /** Last time weather was updated */
    // float LastWeatherUpdateTime;

    /** Last time events were updated */
    float LastEventUpdateTime;

    /** Next event ID counter */
    int32 NextEventID;

    /** Whether game state has been initialized */
    bool bGameStateInitialized;

public:
    // === EVENTS ===
    
    UPROPERTY(BlueprintAssignable, Category = "World Events")
    FOnWorldTimeChanged OnWorldTimeChanged;

    // TODO: Weather events - uncomment when weather system implemented
    // UPROPERTY(BlueprintAssignable, Category = "World Events")
    // FOnWorldWeatherChanged OnWorldWeatherChanged;

    UPROPERTY(BlueprintAssignable, Category = "World Events")
    FOnWorldEventStarted OnWorldEventStarted;

    UPROPERTY(BlueprintAssignable, Category = "World Events")
    FOnWorldEventEnded OnWorldEventEnded;

    UPROPERTY(BlueprintAssignable, Category = "Faction Events")
    FOnFactionRelationshipChanged OnFactionRelationshipChanged;

    UPROPERTY(BlueprintAssignable, Category = "Global Events")
    FOnGlobalFlagChanged OnGlobalFlagChanged;

    // === WORLD TIME INTERFACE ===
    
    /** Get current world time data */
    UFUNCTION(BlueprintPure, Category = "World Time")
    const FWorldTimeData& GetWorldTime() const { return WorldTime; }

    /** Set world time directly (Authority only) */
    UFUNCTION(BlueprintCallable, Category = "World Time", CallInEditor)
    void SetWorldTime(const FWorldTimeData& NewTimeData);

    /** Add time to world clock */
    UFUNCTION(BlueprintCallable, Category = "World Time")
    void AddWorldTime(float SecondsToAdd);

    /** Set time scale */
    UFUNCTION(BlueprintCallable, Category = "World Time")
    void SetTimeScale(float NewTimeScale);

    /** Pause/unpause time */
    UFUNCTION(BlueprintCallable, Category = "World Time")
    void SetTimePaused(bool bPaused);

    /** Get current time as hours (0-24) */
    UFUNCTION(BlueprintPure, Category = "World Time")
    float GetCurrentHour() const { return WorldTime.GetHours(); }

    /** Get current time as minutes (0-59) */
    UFUNCTION(BlueprintPure, Category = "World Time")
    float GetCurrentMinutes() const { return WorldTime.GetMinutes(); }

    /** Get time of day as float (0-1) */
    UFUNCTION(BlueprintPure, Category = "World Time")
    float GetTimeOfDayFloat() const;

    /** Check if it's currently day time */
    UFUNCTION(BlueprintPure, Category = "World Time")
    bool IsDaytime() const;

    /** Check if it's currently night time */
    UFUNCTION(BlueprintPure, Category = "World Time")
    bool IsNighttime() const;

    // TODO: Weather interface - uncomment when weather system implemented
    // // === WORLD WEATHER INTERFACE ===
    // 
    // /** Get current weather data */
    // UFUNCTION(BlueprintPure, Category = "World Weather")
    // const FWorldWeatherData& GetWorldWeather() const { return WorldWeather; }
    //
    // /** Set weather directly (Authority only) */
    // UFUNCTION(BlueprintCallable, Category = "World Weather", CallInEditor)
    // void SetWorldWeather(const FWorldWeatherData& NewWeatherData);
    //
    // /** Start weather transition */
    // UFUNCTION(BlueprintCallable, Category = "World Weather")
    // void ChangeWeather(EWeatherType NewWeather, float Intensity, float TransitionTime);
    //
    // /** Check if weather has precipitation */
    // UFUNCTION(BlueprintPure, Category = "World Weather")
    // bool IsWeatherPrecipitation() const;
    //
    // /** Check if weather is stormy */
    // UFUNCTION(BlueprintPure, Category = "World Weather")
    // bool IsWeatherStormy() const;

    // === FACTION INTERFACE ===
    
    /** Get relationship value between two factions */
    UFUNCTION(BlueprintPure, Category = "Factions")
    float GetFactionRelationship(FGameplayTag FactionA, FGameplayTag FactionB) const;

    /** Set faction relationship */
    UFUNCTION(BlueprintCallable, Category = "Factions")
    void SetFactionRelationship(FGameplayTag FactionA, FGameplayTag FactionB, float RelationshipValue);

    /** Find faction relationship data */
    UFUNCTION(BlueprintPure, Category = "Factions")
    const FFactionRelationshipData& FindFactionRelationship(FGameplayTag FactionA, FGameplayTag FactionB) const;

    /** Declare war between factions */
    UFUNCTION(BlueprintCallable, Category = "Factions")
    void DeclareFactionWar(FGameplayTag FactionA, FGameplayTag FactionB);

    /** End war between factions */
    UFUNCTION(BlueprintCallable, Category = "Factions")
    void EndFactionWar(FGameplayTag FactionA, FGameplayTag FactionB);

    /** Get list of factions currently at war */
    UFUNCTION(BlueprintPure, Category = "Factions")
    TArray<FGameplayTag> GetFactionsAtWar() const;

    // === WORLD EVENTS INTERFACE ===
     
    /** Start a new world event */
    UFUNCTION(BlueprintCallable, Category = "World Events")
    bool StartWorldEvent(const FWorldEventData& EventData);

    /** End a world event by ID */
    UFUNCTION(BlueprintCallable, Category = "World Events")
    bool EndWorldEvent(const FString& EventID);

    /** Get all active world events */
    UFUNCTION(BlueprintPure, Category = "World Events")
    const TArray<FWorldEventData>& GetActiveWorldEvents() const { return ActiveWorldEvents; }

    /** Find world event by ID */
    UFUNCTION(BlueprintPure, Category = "World Events")
    const FWorldEventData& FindWorldEvent(const FString& EventID) const;

    /** Check if world event is active */
    UFUNCTION(BlueprintPure, Category = "World Events")
    bool IsWorldEventActive(const FString& EventID) const;

    // === GLOBAL FLAGS INTERFACE ===
    
    /** Set global flag state */
    UFUNCTION(BlueprintCallable, Category = "Global State")
    void SetGlobalFlag(FGameplayTag Flag, bool bValue);

    /** Get global flag state */
    UFUNCTION(BlueprintPure, Category = "Global State")
    bool GetGlobalFlag(FGameplayTag Flag) const;

    /** Toggle global flag */
    UFUNCTION(BlueprintCallable, Category = "Global State")
    void ToggleGlobalFlag(FGameplayTag Flag);

    /** Get all active global flags */
    UFUNCTION(BlueprintPure, Category = "Global State")
    FGameplayTagContainer GetGlobalFlags() const { return GlobalFlags; }

    // === GLOBAL VARIABLES INTERFACE ===
    
    /** Set global variable (Server RPC) */
    UFUNCTION(BlueprintCallable, Category = "Global State", Server, Reliable)
    void ServerSetGlobalVariable(const FString& VariableName, float Value);

    /** Get global variable */
    UFUNCTION(BlueprintPure, Category = "Global State")
    float GetGlobalVariable(const FString& VariableName, float DefaultValue = 0.0f) const;

    /** Set global string (Server RPC) */
    UFUNCTION(BlueprintCallable, Category = "Global State", Server, Reliable)
    void ServerSetGlobalString(const FString& VariableName, const FString& Value);

    /** Get global string */
    UFUNCTION(BlueprintPure, Category = "Global State")
    FString GetGlobalString(const FString& VariableName, const FString& DefaultValue = TEXT("")) const;

    // === UTILITY FUNCTIONS ===
    
    /** Generate formatted time string */
    UFUNCTION(BlueprintPure, Category = "World Time")
    FString GetFormattedTimeString() const;

    /** Generate formatted date string */
    UFUNCTION(BlueprintPure, Category = "World Time")
    FString GetFormattedDateString() const;

    /** Generate unique event ID */
    UFUNCTION(BlueprintPure, Category = "World Events")
    FString GenerateEventID();

    /** Initialize known factions from data tables */
    UFUNCTION(BlueprintCallable, Category = "Factions")
    void InitializeKnownFactions();

protected:
    // === INTERNAL UPDATE FUNCTIONS ===
    
    /** Update world time progression */
    void UpdateWorldTime(float DeltaTime);

    /** Update time of day enum based on current time */
    void UpdateTimeOfDay();

    // TODO: Weather updates - uncomment when weather system implemented
    // /** Update world weather system */
    // void UpdateWorldWeather(float DeltaTime);
    // 
    // /** Process weather transitions */
    // void ProcessWeatherTransition(float DeltaTime);
    //
    // /** Get random weather transition */
    // EWeatherType GetRandomWeatherTransition(EWeatherType CurrentWeather) const;

    /** Update active world events */
    void UpdateWorldEvents(float DeltaTime);

    /** Validate world event data */
    bool ValidateWorldEventData(const FWorldEventData& EventData) const;

    /** Generate faction relationship key for local storage */
    FString GetFactionRelationshipKey(FGameplayTag FactionA, FGameplayTag FactionB) const;

    /** Calculate time of day enum from current game time */
    ETimeOfDay CalculateTimeOfDay(float GameTimeSeconds) const;

    // === REPLICATION CALLBACKS ===
    
    UFUNCTION()
    void OnRep_WorldTime();

    // TODO: Weather replication - uncomment when weather system implemented
    // UFUNCTION()
    // void OnRep_WorldWeather();

    UFUNCTION()
    void OnRep_ActiveWorldEvents();

    UFUNCTION()
    void OnRep_GlobalFlags();

    UFUNCTION()
    void OnRep_ActiveWars();

    // === SERVER RPC IMPLEMENTATIONS ===
    
    void ServerSetGlobalVariable_Implementation(const FString& VariableName, float Value);
    void ServerSetGlobalString_Implementation(const FString& VariableName, const FString& Value);

    // === BLUEPRINT EVENTS ===
    
    UFUNCTION(BlueprintImplementableEvent, Category = "World Events")
    void OnWorldTimeChangedBP(const FWorldTimeData& NewTimeData);

    // TODO: Weather blueprint events - uncomment when weather system implemented
    // UFUNCTION(BlueprintImplementableEvent, Category = "World Events")
    // void OnWorldWeatherChangedBP(const FWorldWeatherData& NewWeatherData);

    UFUNCTION(BlueprintImplementableEvent, Category = "World Events")
    void OnWorldEventStartedBP(const FWorldEventData& EventData);

    UFUNCTION(BlueprintImplementableEvent, Category = "World Events")
    void OnWorldEventEndedBP(const FWorldEventData& EventData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Faction Events")
    void OnFactionRelationshipChangedBP(FGameplayTag FactionA, FGameplayTag FactionB, float NewRelationshipValue);

    UFUNCTION(BlueprintImplementableEvent, Category = "Global Events")  
    void OnGlobalFlagChangedBP(FGameplayTag Flag, bool bValue);
};