// Public/World/RadiantWorldManager.h
// World simulation manager for RadiantRPG - handles global world state, time, weather, and simulation

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "Types/WorldTypes.h"
#include "Types/RadiantTypes.h"
#include "Types/TimeTypes.h"
#include "World/ISimpleTimeManager.h"
#include "Engine/World.h"
#include "Tickable.h"
#include "Types/WorldManagerTypes.h"
#include "RadiantWorldManager.generated.h"

struct FActiveWorldEvent;
// Forward declarations
class ARadiantZoneManager;
class UWorldEventManager;

/**
 * Interface for world simulation management
 */
UINTERFACE(MinimalAPI, Blueprintable)
class URadiantWorldManagerInterface : public UInterface
{
    GENERATED_BODY()
};

class IRadiantWorldManagerInterface
{
    GENERATED_BODY()

public:
    /** Initialize the world simulation */
    virtual void InitializeWorldSimulation() = 0;

    /** Shutdown the world simulation */
    virtual void ShutdownWorldSimulation() = 0;

    /** Update world simulation state */
    virtual void UpdateWorldSimulation(float DeltaTime) = 0;

    /** Check if world simulation is active */
    virtual bool IsWorldSimulationActive() const = 0;

    /** Get current world state snapshot */
    virtual FWorldState GetWorldStateSnapshot() const = 0;

    /** Apply world state from snapshot */
    virtual void ApplyWorldStateSnapshot(const FWorldState& WorldState) = 0;
};

/**
 * RadiantWorldManager - Core world simulation subsystem
 * 
 * Responsibilities:
 * - Simple world time management and synchronization
 * - Weather system coordination across zones
 * - World state simulation and persistence
 * - Zone lifecycle management
 * - Environmental effect coordination
 * - World event generation and propagation
 * - Global simulation parameters and scaling
 */
UCLASS(BlueprintType)
class RADIANTRPG_API URadiantWorldManager : public UGameInstanceSubsystem, 
                                           public IRadiantWorldManagerInterface, 
                                           public ISimpleTimeManagerInterface,
                                           public FTickableGameObject
{
    GENERATED_BODY()

public:
    URadiantWorldManager();

    // USubsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    // FTickableGameObject interface
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual TStatId GetStatId() const override;

    // IRadiantWorldManagerInterface implementation
    virtual void InitializeWorldSimulation() override;
    virtual void ShutdownWorldSimulation() override;
    virtual void UpdateWorldSimulation(float DeltaTime) override;
    virtual bool IsWorldSimulationActive() const override { return bSimulationActive; }
    virtual FWorldState GetWorldStateSnapshot() const override;
    virtual void ApplyWorldStateSnapshot(const FWorldState& WorldState) override;

    // ISimpleTimeManagerInterface implementation
    virtual const FSimpleWorldTime& GetWorldTime() const override { return CurrentWorldTime; }
    virtual void SetWorldTime(const FSimpleWorldTime& NewTime) override;
    virtual void AdvanceTime(float RealSecondsToAdvance) override;
    virtual void AdvanceGameMinutes(int32 GameMinutesToAdvance) override;
    virtual void SetTimeScale(float NewTimeScale) override;
    virtual float GetTimeScale() const override { return TimeSettings.DefaultTimeScale; }
    virtual void SetTimePaused(bool bPaused) override;
    virtual bool IsTimePaused() const override { return CurrentWorldTime.bTimePaused; }
    virtual int32 GetCurrentSeason() const override { return CurrentWorldTime.Season; }
    virtual int32 GetCurrentDay() const override { return CurrentWorldTime.Day; }
    virtual int32 GetCurrentHour() const override { return CurrentWorldTime.Hour; }
    virtual int32 GetCurrentMinute() const override { return CurrentWorldTime.Minute; }
    virtual ETimeOfDay GetTimeOfDay() const override { return CurrentWorldTime.GetTimeOfDay(); }
    virtual ESeason GetSeasonType() const override { return CurrentWorldTime.GetSeason(); }
    virtual bool IsDaytime() const override { return CurrentWorldTime.IsDaytime(); }
    virtual bool IsNighttime() const override { return CurrentWorldTime.IsNighttime(); }
    virtual FString GetTimeString() const override { return CurrentWorldTime.GetTimeString(); }
    virtual FString GetDateString() const override { return CurrentWorldTime.GetDateString(); }
    virtual FString GetFullTimeString() const override { return CurrentWorldTime.GetFullTimeString(); }

protected:
    // === CONFIGURATION ===
    
    /** World simulation settings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Simulation")
    FWorldSimulationSettings SimulationSettings;

    /** Simple time system settings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time System")
    FSimpleTimeSettings TimeSettings;

    /** Weather system configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather System")
    FWeatherSystemSettings WeatherSettings;

    // === RUNTIME STATE ===
    
    /** Current simplified world time */
    UPROPERTY(BlueprintReadOnly, Category = "World State")
    FSimpleWorldTime CurrentWorldTime;

    /** Current global weather state */
    UPROPERTY(BlueprintReadOnly, Category = "Weather System")
    FWorldWeatherData CurrentGlobalWeather;

    /** Registered zones in the world */
    UPROPERTY(BlueprintReadOnly, Category = "Zone Management")
    TMap<FGameplayTag, TObjectPtr<ARadiantZoneManager>> RegisteredZones;

    /** Active world events */
    UPROPERTY(BlueprintReadOnly, Category = "World Events")
    TArray<FActiveWorldEvent> ActiveWorldEvents;

    /** World simulation state flags */
    UPROPERTY(BlueprintReadOnly, Category = "Simulation State")
    FWorldSimulationState SimulationState;

    // === INTERNAL STATE ===
    
    /** Whether simulation is currently active */
    bool bSimulationActive;

    /** Whether manager is initialized */
    bool bIsInitialized;

    /** Time since last major simulation update */
    float TimeSinceLastMajorUpdate;

    /** Cached world event manager reference */
    UPROPERTY()
    TObjectPtr<UWorldEventManager> WorldEventManager;

    /** Timer handles for various systems */
    FTimerHandle WeatherUpdateTimer;
    FTimerHandle WorldEventTimer;
    FTimerHandle SimulationSaveTimer;

    /** Cached previous time of day for change detection */
    ETimeOfDay PreviousTimeOfDay;

    /** Cached previous season for change detection */
    ESeason PreviousSeason;

public:
    // === EVENTS ===
    
    UPROPERTY(BlueprintAssignable, Category = "Time Events")
    FOnSimpleTimeChanged OnTimeChanged;

    UPROPERTY(BlueprintAssignable, Category = "Time Events")
    FOnTimeOfDayChanged OnTimeOfDayChanged;

    UPROPERTY(BlueprintAssignable, Category = "Time Events")
    FOnSeasonChanged OnSeasonChanged;

    UPROPERTY(BlueprintAssignable, Category = "Time Events")
    FOnNewDay OnNewDay;

    // === BLUEPRINT TIME INTERFACE ===
    
    /** Get current world time (Blueprint accessible) */
    UFUNCTION(BlueprintPure, Category = "World Time")
    const FSimpleWorldTime& GetSimpleWorldTime() const { return CurrentWorldTime; }

    /** Set world time directly (use with caution) */
    UFUNCTION(BlueprintCallable, Category = "World Time", CallInEditor)
    void SetSimpleWorldTime(const FSimpleWorldTime& NewTime);

    /** Advance time by specified real seconds */
    UFUNCTION(BlueprintCallable, Category = "World Time")
    void AdvanceTimeBySeconds(float RealSecondsToAdvance);

    /** Advance time by specified game minutes */
    UFUNCTION(BlueprintCallable, Category = "World Time")
    void AdvanceTimeByGameMinutes(int32 GameMinutesToAdvance);

    /** Set global time scale multiplier */
    UFUNCTION(BlueprintCallable, Category = "World Time")
    void SetGlobalTimeScale(float NewTimeScale);

    /** Get current time scale */
    UFUNCTION(BlueprintPure, Category = "World Time")
    float GetGlobalTimeScale() const { return GetTimeScale(); }

    /** Pause/unpause world time progression */
    UFUNCTION(BlueprintCallable, Category = "World Time")
    void SetTimeProgression(bool bPaused);

    /** Check if time is paused */
    UFUNCTION(BlueprintPure, Category = "World Time")
    bool IsTimeProgressionPaused() const { return IsTimePaused(); }

    /** Get current season number */
    UFUNCTION(BlueprintPure, Category = "World Time")
    int32 GetSeasonNumber() const { return GetCurrentSeason(); }

    /** Get current day of season (1-30) */
    UFUNCTION(BlueprintPure, Category = "World Time")
    int32 GetDayOfSeason() const { return GetCurrentDay(); }

    /** Get current hour (0-23) */
    UFUNCTION(BlueprintPure, Category = "World Time")
    int32 GetHourOfDay() const { return GetCurrentHour(); }

    /** Get current minute (0-59) */
    UFUNCTION(BlueprintPure, Category = "World Time")
    int32 GetMinuteOfHour() const { return GetCurrentMinute(); }

    /** Get current time of day */
    UFUNCTION(BlueprintPure, Category = "World Time")
    ETimeOfDay GetCurrentTimeOfDay() const { return GetTimeOfDay(); }

    /** Get current season type */
    UFUNCTION(BlueprintPure, Category = "World Time")
    ESeason GetCurrentSeasonType() const { return GetSeasonType(); }

    /** Check if it's currently daytime */
    UFUNCTION(BlueprintPure, Category = "World Time")
    bool IsCurrentlyDaytime() const { return IsDaytime(); }

    /** Check if it's currently nighttime */
    UFUNCTION(BlueprintPure, Category = "World Time")
    bool IsCurrentlyNighttime() const { return IsNighttime(); }

    /** Get formatted time string (HH:MM) */
    UFUNCTION(BlueprintPure, Category = "World Time")
    FString GetFormattedTimeString() const { return GetTimeString(); }

    /** Get formatted date string */
    UFUNCTION(BlueprintPure, Category = "World Time")
    FString GetFormattedDateString() const { return GetDateString(); }

    /** Get full formatted date and time string */
    UFUNCTION(BlueprintPure, Category = "World Time")
    FString GetFormattedFullTimeString() const { return GetFullTimeString(); }

    // === WEATHER SYSTEM INTERFACE ===
    
    /** Get current global weather */
    UFUNCTION(BlueprintPure, Category = "Weather System")
    const FWorldWeatherData& GetGlobalWeather() const { return CurrentGlobalWeather; }

    /** Set global weather type */
    UFUNCTION(BlueprintCallable, Category = "Weather System")
    void SetGlobalWeather(EWeatherType NewWeather, float Intensity = 1.0f, float TransitionTime = 30.0f);

    /** Trigger weather change in specific zone */
    UFUNCTION(BlueprintCallable, Category = "Weather System")
    void SetZoneWeather(FGameplayTag ZoneTag, EWeatherType NewWeather, float Intensity = 1.0f);

    /** Update weather patterns based on time and conditions */
    UFUNCTION(BlueprintCallable, Category = "Weather System")
    void UpdateWeatherSystem();

    /** Check if weather affects visibility */
    UFUNCTION(BlueprintPure, Category = "Weather System")
    bool DoesWeatherAffectVisibility() const;

    // === ZONE MANAGEMENT INTERFACE ===
    
    /** Register a zone with the world manager */
    UFUNCTION(BlueprintCallable, Category = "Zone Management")
    void RegisterZone(ARadiantZoneManager* Zone);

    /** Unregister a zone from the world manager */
    UFUNCTION(BlueprintCallable, Category = "Zone Management")
    void UnregisterZone(ARadiantZoneManager* Zone);

    /** Get zone by tag */
    UFUNCTION(BlueprintPure, Category = "Zone Management")
    ARadiantZoneManager* GetZoneByTag(FGameplayTag ZoneTag) const;

    /** Get all zones of a specific type */
    UFUNCTION(BlueprintPure, Category = "Zone Management")
    TArray<ARadiantZoneManager*> GetZonesByType(EZoneType ZoneType) const;

    /** Get zone at world location */
    UFUNCTION(BlueprintPure, Category = "Zone Management")
    ARadiantZoneManager* GetZoneAtLocation(FVector WorldLocation) const;

    /** Get all registered zones */
    UFUNCTION(BlueprintPure, Category = "Zone Management")
    TArray<ARadiantZoneManager*> GetAllZones() const;

    // === WORLD EVENT INTERFACE ===
    
    /** Trigger a global world event */
    UFUNCTION(BlueprintCallable, Category = "World Events")
    void TriggerGlobalEvent(FGameplayTag EventTag, float Duration = 0.0f, AActor* Instigator = nullptr);

    /** Get active world events */
    UFUNCTION(BlueprintPure, Category = "World Events")
    TArray<FActiveWorldEvent> GetActiveWorldEvents() const { return ActiveWorldEvents; }

    /** Stop a world event by tag */
    UFUNCTION(BlueprintCallable, Category = "World Events")
    bool StopWorldEvent(FGameplayTag EventTag);

    // === DEBUG AND MONITORING ===
    
    /** Get detailed debug string */
    UFUNCTION(BlueprintPure, Category = "Debug")
    FString GetDebugString() const;

    /** Validate world state consistency */
    UFUNCTION(BlueprintCallable, Category = "Debug")
    bool ValidateWorldState() const;

protected:
    // === INTERNAL FUNCTIONS ===
    
    /** Initialize simplified time system */
    void InitializeSimpleTimeSystem();

    /** Initialize weather system */
    void InitializeWeatherSystem();

    /** Initialize event system */
    void InitializeEventSystem();

    /** Initialize default world state */
    void InitializeDefaultWorldState();

    /** Update time progression */
    void UpdateTimeProgression(float DeltaTime);

    /** Process time change events */
    void ProcessTimeChangeEvents(ETimeOfDay OldTimeOfDay, ESeason OldSeason);

    /** Calculate weather transition */
    void CalculateWeatherTransition(float DeltaTime);

    /** Process scheduled events */
    void ProcessScheduledEvents();

    /** Validate zone registrations */
    void ValidateZoneRegistrations();

    /** Update simulation metrics */
    void UpdateSimulationMetrics(float DeltaTime);

    /** Event handlers */
    UFUNCTION()
    void OnGlobalWeatherChanged(const FWorldWeatherData& NewWeather);

    UFUNCTION()
    void OnGlobalTimeChanged(const FSimpleWorldTime& NewTime);
};