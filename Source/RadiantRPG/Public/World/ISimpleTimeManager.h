// Public/World/ISimpleTimeManager.h
// Simple time management interface for RadiantRPG

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Types/TimeTypes.h"
#include "ISimpleTimeManager.generated.h"

/**
 * Interface for simple time management
 * All time-related queries should go through this interface
 */
UINTERFACE(MinimalAPI, Blueprintable)
class USimpleTimeManagerInterface : public UInterface
{
    GENERATED_BODY()
};

class RADIANTRPG_API ISimpleTimeManagerInterface
{
    GENERATED_BODY()

public:
    // === CORE TIME INTERFACE ===

    /** Get current world time */
    virtual const FSimpleWorldTime& GetWorldTime() const = 0;

    /** Set world time directly (use with caution) */
    virtual void SetWorldTime(const FSimpleWorldTime& NewTime) = 0;

    /** Advance time by specified real seconds */
    virtual void AdvanceTime(float RealSecondsToAdvance) = 0;

    /** Advance time by specified game minutes */
    virtual void AdvanceGameMinutes(int32 GameMinutesToAdvance) = 0;

    // === TIME SCALE CONTROL ===

    /** Set time scale multiplier */
    virtual void SetTimeScale(float NewTimeScale) = 0;

    /** Get current time scale */
    virtual float GetTimeScale() const = 0;

    /** Pause/unpause time progression */
    virtual void SetTimePaused(bool bPaused) = 0;

    /** Check if time is paused */
    virtual bool IsTimePaused() const = 0;

    // === TIME QUERIES ===

    /** Get current season */
    virtual int32 GetCurrentSeason() const = 0;

    /** Get current day of season (1-30) */
    virtual int32 GetCurrentDay() const = 0;

    /** Get current hour (0-23) */
    virtual int32 GetCurrentHour() const = 0;

    /** Get current minute (0-59) */
    virtual int32 GetCurrentMinute() const = 0;

    /** Get current time of day */
    virtual ETimeOfDay GetTimeOfDay() const = 0;

    /** Get current season enum */
    virtual ESeason GetSeasonType() const = 0;

    /** Check if it's currently daytime */
    virtual bool IsDaytime() const = 0;

    /** Check if it's currently nighttime */
    virtual bool IsNighttime() const = 0;

    // === FORMATTED OUTPUT ===

    /** Get current time as formatted string (HH:MM) */
    virtual FString GetTimeString() const = 0;

    /** Get current date as formatted string */
    virtual FString GetDateString() const = 0;

    /** Get full date and time string */
    virtual FString GetFullTimeString() const = 0;
};