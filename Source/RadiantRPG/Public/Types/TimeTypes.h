// Public/Types/TimeTypes.h
// Simplified time management system for RadiantRPG

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "TimeTypes.generated.h"



/**
 * Simple time of day periods for AI behavior and world systems
 * Keep it balanced between simple and engaging
 */

UENUM(BlueprintType)
enum class ETimeOfDay : uint8
{
    Dawn            UMETA(DisplayName = "Morning"),        // 6-12 (6 AM - Noon)
    Morning         UMETA(DisplayName = "Morning"),        // 6-12 (6 AM - Noon)
    Noon            UMETA(DisplayName = "Noon"),      // 12-17 (Noon - 5 PM) 
    Afternoon       UMETA(DisplayName = "Afternoon"),      // 12-17 (Noon - 5 PM) 
    Evening         UMETA(DisplayName = "Evening"),        // 17-21 (5 PM - 9 PM)
    Dusk            UMETA(DisplayName = "Dusk"),        // 17-21 (5 PM - 9 PM)
    Night           UMETA(DisplayName = "Night"),          // 21-2 (9 PM - 2 AM)
    Midnight        UMETA(DisplayName = "Midnight"),          // 21-2 (9 PM - 2 AM)
    LateNight       UMETA(DisplayName = "Late Night"),     // 2-6 (2 AM - 6 AM)
    
    MAX             UMETA(Hidden)
};

/**
 * Season enumeration - simple seasonal progression
 * 1 Season = 1 Month = 30 Days
 */
UENUM(BlueprintType)
enum class ESeason : uint8
{
    Spring          UMETA(DisplayName = "Spring"),
    Summer          UMETA(DisplayName = "Summer"),
    Autumn          UMETA(DisplayName = "Autumn"),
    Winter          UMETA(DisplayName = "Winter"),
    
    MAX             UMETA(Hidden)
};

/**
 * Simplified world time data structure
 * Simple calendar: 1 Season = 1 Month = 30 Days, 24 Hours, 60 Minutes
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FSimpleWorldTime
{
    GENERATED_BODY()

    /** Total elapsed game time in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    float TotalGameSeconds;

    /** Current season (1 season = 1 month) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    int32 Season;

    /** Current day of season (1-30) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    int32 Day;

    /** Current hour (0-23) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    int32 Hour;

    /** Current minute (0-59) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    int32 Minute;

    /** Time scale multiplier (1.0 = real time) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    float TimeScale;

    /** Whether time progression is paused */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    bool bTimePaused;

    FSimpleWorldTime()
    {
        TotalGameSeconds = 0.0f;
        Season = 1;
        Day = 1;
        Hour = 6; // Start at dawn
        Minute = 0;
        TimeScale = 60.0f; // 60x speed by default (1 real minute = 1 game hour)
        bTimePaused = false;
    }

    /** Get current time of day enum */
    ETimeOfDay GetTimeOfDay() const
    {
        if (Hour >= 6 && Hour < 12)
            return ETimeOfDay::Morning;
        else if (Hour >= 12 && Hour < 17)
            return ETimeOfDay::Afternoon;
        else if (Hour >= 17 && Hour < 21)
            return ETimeOfDay::Evening;
        else if (Hour >= 21 || Hour < 2)
            return ETimeOfDay::Night;
        else // 2-6
            return ETimeOfDay::LateNight;
    }

    /** Get current season enum */
    ESeason GetSeason() const
    {
        int32 SeasonIndex = ((Season - 1) % 4);
        return static_cast<ESeason>(SeasonIndex);
    }

    /** Check if it's currently daytime (Morning or Afternoon) */
    bool IsDaytime() const
    {
        ETimeOfDay CurrentTime = GetTimeOfDay();
        return CurrentTime == ETimeOfDay::Morning || CurrentTime == ETimeOfDay::Afternoon;
    }

    /** Check if it's currently nighttime (Night or LateNight) */
    bool IsNighttime() const
    {
        ETimeOfDay CurrentTime = GetTimeOfDay();
        return CurrentTime == ETimeOfDay::Night || CurrentTime == ETimeOfDay::LateNight;
    }

    /** Get formatted time string (HH:MM) */
    FString GetTimeString() const
    {
        return FString::Printf(TEXT("%02d:%02d"), Hour, Minute);
    }

    /** Get formatted date string */
    FString GetDateString() const
    {
        FString SeasonName = UEnum::GetValueAsString(GetSeason());
        SeasonName = SeasonName.Replace(TEXT("ESeason::"), TEXT(""));
        return FString::Printf(TEXT("Season %d (%s), Day %d"), Season, *SeasonName, Day);
    }

    /** Get full date and time string */
    FString GetFullTimeString() const
    {
        return FString::Printf(TEXT("%s - %s"), *GetDateString(), *GetTimeString());
    }

    /** Convert to total minutes for easy calculations */
    int32 GetTotalMinutes() const
    {
        return ((Season - 1) * 30 * 24 * 60) + // Minutes from previous seasons
               ((Day - 1) * 24 * 60) +          // Minutes from previous days this season
               (Hour * 60) +                     // Minutes from hours today
               Minute;                           // Minutes this hour
    }

    /** Set time from total minutes */
    void SetFromTotalMinutes(int32 TotalMinutes)
    {
        Season = (TotalMinutes / (30 * 24 * 60)) + 1;
        int32 RemainingMinutes = TotalMinutes % (30 * 24 * 60);
        
        Day = (RemainingMinutes / (24 * 60)) + 1;
        RemainingMinutes = RemainingMinutes % (24 * 60);
        
        Hour = RemainingMinutes / 60;
        Minute = RemainingMinutes % 60;
    }
};

/**
 * Time management settings for world simulation
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FSimpleTimeSettings
{
    GENERATED_BODY()

    /** Default time scale (60.0 = 1 real minute = 1 game hour) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Settings", meta = (ClampMin = "0.1", ClampMax = "1000.0"))
    float DefaultTimeScale;

    /** Whether to automatically progress time */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Settings")
    bool bAutoTimeProgression;

    /** Starting season (for new games) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Settings", meta = (ClampMin = "1"))
    int32 StartingSeason;

    /** Starting day (1-30) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Settings", meta = (ClampMin = "1", ClampMax = "30"))
    int32 StartingDay;

    /** Starting hour (0-23) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Settings", meta = (ClampMin = "0", ClampMax = "23"))
    int32 StartingHour;

    FSimpleTimeSettings()
    {
        DefaultTimeScale = 60.0f;
        bAutoTimeProgression = true;
        StartingSeason = 1;
        StartingDay = 1;
        StartingHour = 6; // Start at dawn
    }
};


// Events for time changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSimpleTimeChanged, const FSimpleWorldTime&, NewTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeOfDayChanged, ETimeOfDay, NewTimeOfDay);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSeasonChanged, ESeason, NewSeason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNewDay, int32, Season, int32, Day);