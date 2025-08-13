// Public/Types/WorldManagerTypes.h
// Type definitions for RadiantWorldManager - simulation settings and state structures

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Types/WorldTypes.h"
#include "WorldManagerTypes.generated.h"

/**
 * World simulation configuration settings
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FWorldSimulationSettings
{
    GENERATED_BODY()

    /** How often to perform major simulation updates (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float MajorUpdateInterval = 5.0f;

    /** How often to process scheduled events (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float EventProcessingInterval = 1.0f;

    /** Auto-save interval for simulation state (seconds, 0 = disabled) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Persistence")
    float AutoSaveInterval = 300.0f; // 5 minutes

    /** Maximum number of active world events */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Events")
    int32 MaxActiveEvents = 50;

    /** Whether to enable performance monitoring */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bEnablePerformanceMonitoring = true;

    /** Whether to enable debug logging */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bEnableDebugLogging = false;

    FWorldSimulationSettings()
    {
    }
};

/**
 * Time system configuration
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FWorldTimeSettings
{
    GENERATED_BODY()

    /** Real seconds per game day */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    float SecondsPerGameDay = 3600.0f; // 1 hour real = 1 game day

    /** Global time scale multiplier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time", meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float TimeScale = 1.0f;

    /** Whether time progression is paused */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    bool bTimePaused = false;

    /** Whether to automatically advance time */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    bool bAutoTimeProgression = true;

    /** Hour when dawn begins (0-23) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time", meta = (ClampMin = "0", ClampMax = "23"))
    int32 DawnHour = 6;

    /** Hour when day begins (0-23) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time", meta = (ClampMin = "0", ClampMax = "23"))
    int32 DayHour = 8;

    /** Hour when dusk begins (0-23) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time", meta = (ClampMin = "0", ClampMax = "23"))
    int32 DuskHour = 18;

    /** Hour when night begins (0-23) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time", meta = (ClampMin = "0", ClampMax = "23"))
    int32 NightHour = 20;

    FWorldTimeSettings()
    {
    }
};

/**
 * Weather system configuration
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FWeatherSystemSettings
{
    GENERATED_BODY()

    /** How often to update weather patterns (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
    float UpdateInterval = 30.0f;

    /** Whether dynamic weather changes are enabled */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
    bool bDynamicWeather = true;

    /** Chance per update for weather to change (0-1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WeatherChangeChance = 0.1f;

    /** Minimum time between weather changes (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
    float MinWeatherDuration = 600.0f; // 10 minutes

    /** Maximum time between weather changes (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
    float MaxWeatherDuration = 3600.0f; // 1 hour

    /** Default weather transition time (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
    float DefaultTransitionTime = 60.0f;

    /** Whether weather affects AI behavior */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
    bool bWeatherAffectsAI = true;

    /** Whether weather affects player visibility */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
    bool bWeatherAffectsVisibility = true;

    FWeatherSystemSettings()
    {
    }
};

/**
 * Current simulation state
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FWorldSimulationState
{
    GENERATED_BODY()

    /** Whether simulation is currently paused */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bIsPaused = false;

    /** Total time simulation has been running (real seconds) */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    float TotalSimulationTime = 0.0f;

    /** Last time simulation was updated */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    float LastUpdateTime = 0.0f;

    /** Current simulation performance rating (0-1) */
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float PerformanceRating = 1.0f;

    /** Number of simulation updates performed */
    UPROPERTY(BlueprintReadOnly, Category = "Statistics")
    int32 UpdateCount = 0;

    /** Number of events processed */
    UPROPERTY(BlueprintReadOnly, Category = "Statistics")
    int32 EventsProcessed = 0;
    bool bWeatherSystemActive;
    bool bEventSystemActive;
    bool bZoneSystemActive;

    FWorldSimulationState()
    {
    }
};

/**
 * Active world event data
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FActiveWorldEvent
{
    GENERATED_BODY()

    /** Event identifier tag */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    FGameplayTag EventTag;

    /** When the event started (world time) */
    UPROPERTY(BlueprintReadOnly, Category = "Event")
    float StartTime = 0.0f;

    /** How long the event lasts (0 = permanent) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    float Duration = 0.0f;

    /** Event intensity/strength */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Intensity = 1.0f;

    /** Actor that triggered this event */
    UPROPERTY(BlueprintReadOnly, Category = "Event")
    TObjectPtr<AActor> Instigator = nullptr;

    /** Zone where event is active (empty = global) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    FGameplayTag ZoneTag;

    /** Whether this is a global event */
    UPROPERTY(BlueprintReadOnly, Category = "Event")
    bool bIsGlobal = false;

    /** Whether this event can be cancelled */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    bool bCanBeCancelled = true;

    /** Event-specific metadata */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    TMap<FString, FString> EventData;

    FActiveWorldEvent()
    {
    }

    /** Check if event is still active */
    bool IsActive(float CurrentTime) const
    {
        return Duration <= 0.0f || (CurrentTime - StartTime) < Duration;
    }

    /** Get remaining time for event */
    float GetRemainingTime(float CurrentTime) const
    {
        if (Duration <= 0.0f)
        {
            return -1.0f; // Permanent
        }
        return FMath::Max(0.0f, Duration - (CurrentTime - StartTime));
    }
};

/**
 * Simulation performance metrics
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FWorldSimulationMetrics
{
    GENERATED_BODY()

    /** Number of zones currently managed */
    UPROPERTY(BlueprintReadOnly, Category = "Metrics")
    int32 ActiveZoneCount = 0;

    /** Number of active world events */
    UPROPERTY(BlueprintReadOnly, Category = "Metrics")
    int32 ActiveEventCount = 0;

    /** Total simulation uptime in seconds */
    UPROPERTY(BlueprintReadOnly, Category = "Metrics")
    float SimulationUptime = 0.0f;

    /** Current time scale */
    UPROPERTY(BlueprintReadOnly, Category = "Metrics")
    float CurrentTimeScale = 1.0f;

    /** Whether simulation is currently active */
    UPROPERTY(BlueprintReadOnly, Category = "Metrics")
    bool bIsSimulationActive = false;

    /** Average tick time in milliseconds */
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float AverageTickTime = 0.0f;

    /** Peak memory usage in MB */
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float PeakMemoryUsage = 0.0f;

    /** Current performance score (0-1) */
    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float PerformanceScore = 1.0f;

    FWorldSimulationMetrics()
    {
    }
};

/**
 * Scheduled event data
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FScheduledWorldEvent
{
    GENERATED_BODY()

    /** Event to trigger */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    FGameplayTag EventTag;

    /** When to trigger the event (world time) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    float TriggerTime = 0.0f;

    /** Event duration once triggered */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    float Duration = 0.0f;

    /** Event intensity */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Intensity = 1.0f;

    /** Zone to trigger event in (empty = global) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    FGameplayTag TargetZone;

    /** Whether event repeats */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    bool bRepeating = false;

    /** Repeat interval if repeating */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    float RepeatInterval = 0.0f;

    /** Maximum number of repeats (0 = infinite) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    int32 MaxRepeats = 0;

    /** Current repeat count */
    UPROPERTY(BlueprintReadOnly, Category = "Event")
    int32 CurrentRepeats = 0;

    FScheduledWorldEvent()
    {
    }

    /** Check if this event should trigger now */
    bool ShouldTrigger(float CurrentTime) const
    {
        return CurrentTime >= TriggerTime;
    }

    /** Check if this event can repeat */
    bool CanRepeat() const
    {
        return bRepeating && (MaxRepeats == 0 || CurrentRepeats < MaxRepeats);
    }
};

/**
 * Environmental condition data
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FEnvironmentalCondition
{
    GENERATED_BODY()

    /** Type of environmental condition */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    FGameplayTag ConditionType;

    /** Intensity of the condition (0-1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Intensity = 1.0f;

    /** Area of effect radius */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    float AreaRadius = 1000.0f;

    /** Center location of the condition */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    FVector CenterLocation = FVector::ZeroVector;

    /** How long the condition lasts */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    float Duration = 0.0f;

    /** When the condition started */
    UPROPERTY(BlueprintReadOnly, Category = "Environment")
    float StartTime = 0.0f;

    /** Tags for affected actors */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    FGameplayTagContainer AffectedActorTags;

    FEnvironmentalCondition()
    {
    }

    /** Check if condition is still active */
    bool IsActive(float CurrentTime) const
    {
        return Duration <= 0.0f || (CurrentTime - StartTime) < Duration;
    }

    /** Check if location is within area of effect */
    bool IsLocationAffected(const FVector& Location) const
    {
        return FVector::Dist(Location, CenterLocation) <= AreaRadius;
    }
};