// Public/Types/RadiantTypes.h
// Core type definitions and delegates for RadiantRPG to prevent circular dependencies

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "RadiantTypes.generated.h"

// ================================================================================
// ENUMERATIONS
// ================================================================================

/** Game states for managing major application flow */
UENUM(BlueprintType)
enum class EGameState : uint8
{
    None            UMETA(DisplayName = "None"),
    MainMenu        UMETA(DisplayName = "Main Menu"),
    Loading         UMETA(DisplayName = "Loading"),
    InGame          UMETA(DisplayName = "In Game"),
    Paused          UMETA(DisplayName = "Paused"),
    GameOver        UMETA(DisplayName = "Game Over"),
    Credits         UMETA(DisplayName = "Credits"),
    Options         UMETA(DisplayName = "Options"),
    Error           UMETA(DisplayName = "Error")
};

/** Difficulty levels affecting game balance */
UENUM(BlueprintType)
enum class EDifficultyLevel : uint8
{
    VeryEasy        UMETA(DisplayName = "Very Easy"),
    Easy            UMETA(DisplayName = "Easy"),
    Normal          UMETA(DisplayName = "Normal"),
    Hard            UMETA(DisplayName = "Hard"),
    VeryHard        UMETA(DisplayName = "Very Hard"),
    Nightmare       UMETA(DisplayName = "Nightmare")
};

/** Time of day categories for behavior and spawning */
UENUM(BlueprintType)
enum class ETimeOfDay : uint8
{
    Dawn            UMETA(DisplayName = "Dawn"),
    Morning         UMETA(DisplayName = "Morning"),
    Noon            UMETA(DisplayName = "Noon"),
    Afternoon       UMETA(DisplayName = "Afternoon"),
    Dusk            UMETA(DisplayName = "Dusk"),
    Evening         UMETA(DisplayName = "Evening"),
    Night           UMETA(DisplayName = "Night"),
    Midnight        UMETA(DisplayName = "Midnight"),
    Midday          UMETA(DisplayName = "Midday") 
};

/** Weather types for environmental simulation */
UENUM(BlueprintType)
enum class EWeatherType : uint8
{
    Clear           UMETA(DisplayName = "Clear"),
    Cloudy          UMETA(DisplayName = "Cloudy"),
    Overcast        UMETA(DisplayName = "Overcast"),
    Fog             UMETA(DisplayName = "Fog"),
    LightRain       UMETA(DisplayName = "Light Rain"),
    HeavyRain       UMETA(DisplayName = "Heavy Rain"),
    Thunderstorm    UMETA(DisplayName = "Thunderstorm"),
    Snow            UMETA(DisplayName = "Snow"),
    Blizzard        UMETA(DisplayName = "Blizzard"),
    Sandstorm       UMETA(DisplayName = "Sandstorm")
};

/** Faction relationship types */
UENUM(BlueprintType)
enum class EFactionRelationship : uint8
{
    Enemy           UMETA(DisplayName = "Enemy"),
    Hostile         UMETA(DisplayName = "Hostile"),
    Unfriendly      UMETA(DisplayName = "Unfriendly"),
    Neutral         UMETA(DisplayName = "Neutral"),
    Friendly        UMETA(DisplayName = "Friendly"),
    Allied          UMETA(DisplayName = "Allied"),
    Unknown          UMETA(DisplayName = "Unknown")
};

/** Skill categories for the Ultima Online-inspired skill system */
UENUM(BlueprintType)
enum class ESkillCategory : uint8
{
    Combat          UMETA(DisplayName = "Combat"),
    Magic           UMETA(DisplayName = "Magic"),
    Crafting        UMETA(DisplayName = "Crafting"),
    Survival        UMETA(DisplayName = "Survival"),
    Social          UMETA(DisplayName = "Social"),
    Movement        UMETA(DisplayName = "Movement"),
    Knowledge       UMETA(DisplayName = "Knowledge")
};

// ================================================================================
// CORE DATA STRUCTURES
// ================================================================================

/** Skill data structure for character progression */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FSkillData
{
    GENERATED_BODY()

    /** Current skill level (0-100) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    float CurrentValue;

    /** Maximum skill cap for this character */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    float MaxValue;

    /** Total experience gained in this skill */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    float TotalExperience;

    /** Whether this skill is currently locked */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    bool bIsLocked;

    /** Temporary modifier from items/effects */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    float TemporaryModifier;

    /** Skill category for grouping */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    ESkillCategory SkillCategory;
    float CurrentExperience;

    FSkillData()
    {
        CurrentValue = 0.0f;
        MaxValue = 100.0f;
        TotalExperience = 0.0f;
        bIsLocked = false;
        TemporaryModifier = 0.0f;
        SkillCategory = ESkillCategory::Combat;
    }

    /** Get effective skill value including temporary modifiers */
    float GetEffectiveValue() const
    {
        return FMath::Clamp(CurrentValue + TemporaryModifier, 0.0f, MaxValue);
    }
};

/** World time data for synchronized time of day */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FWorldTimeData
{
    GENERATED_BODY()

    /** Game time in seconds since start */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    float GameTimeSeconds;

    /** Current day number */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    int32 GameDay;

    /** Current time of day category */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    ETimeOfDay TimeOfDay;

    /** Time scale multiplier (1.0 = real time) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    float TimeScale;
    int CurrentDay;
    bool bTimePaused;

    FWorldTimeData()
    {
        GameTimeSeconds = 0.0f;
        GameDay = 1;
        TimeOfDay = ETimeOfDay::Dawn;
        TimeScale = 60.0f; // 60x speed by default
    }

    /** Get time as hours (0-24) */
    float GetHours() const
    {
        const float SecondsInDay = 86400.0f / TimeScale;
        float DayProgress = FMath::Fmod(GameTimeSeconds, SecondsInDay) / SecondsInDay;
        return DayProgress * 24.0f;
    }

    /** Get time as minutes (0-59) */
    float GetMinutes() const
    {
        float Hours = GetHours();
        return FMath::Fmod(Hours * 60.0f, 60.0f);
    }
};

/** Weather data for environmental systems */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FWorldWeatherData
{
    GENERATED_BODY()

    /** Current weather type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
    EWeatherType CurrentWeather;

    /** Weather transition progress (0-1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
    float TransitionProgress;

    /** Target weather we're transitioning to */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
    EWeatherType TargetWeather;

    /** Weather intensity (0-1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
    float Intensity;

    /** Wind strength (0-1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
    float WindStrength;

    /** Temperature in Celsius */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
    float Temperature;
    float WeatherIntensity;
    float TimeToWeatherChange;

    FWorldWeatherData()
    {
        CurrentWeather = EWeatherType::Clear;
        TransitionProgress = 1.0f;
        TargetWeather = EWeatherType::Clear;
        Intensity = 0.5f;
        WindStrength = 0.3f;
        Temperature = 20.0f;
    }
};

/** Faction relationship data */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FFactionRelationshipData
{
    GENERATED_BODY()

    /** How this faction views the other */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relationship")
    EFactionRelationship Relationship;

    /** Numeric reputation value (-100 to 100) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relationship")
    float ReputationValue;

    /** Whether this relationship is locked from change */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relationship")
    bool bIsLocked;

    FFactionRelationshipData()
    {
        Relationship = EFactionRelationship::Neutral;
        ReputationValue = 0.0f;
        bIsLocked = false;
    }
};

/** World event data for dynamic events */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FWorldEventData
{
    GENERATED_BODY()

    /** Unique event identifier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    FString EventID;

    /** Event type tag */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    FGameplayTag EventType;

    /** Event location */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    FVector EventLocation;

    /** Event radius/area of effect */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    float EventRadius;

    /** When the event started */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    float EventStartTime;

    /** How long the event lasts (-1 for indefinite) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    float EventDuration;

    /** Whether event is currently active */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    bool bIsActive;

    /** Event parameters */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    TMap<FString, FString> EventParameters;

    FWorldEventData()
    {
        EventID = FString();
        EventRadius = 1000.0f;
        EventStartTime = 0.0f;
        EventDuration = -1.0f;
        bIsActive = false;
    }
};

/** Game settings structure */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FGameSettings
{
    GENERATED_BODY()

    /** Current difficulty level */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    EDifficultyLevel DifficultyLevel;

    /** Master volume (0-1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float MasterVolume;

    /** Music volume (0-1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float MusicVolume;

    /** SFX volume (0-1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float SFXVolume;

    /** Auto-save interval in minutes */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float AutoSaveInterval;

    /** Whether to show damage numbers */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bShowDamageNumbers;

    /** Whether to enable tutorial hints */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bShowTutorialHints;

    FGameSettings()
    {
        DifficultyLevel = EDifficultyLevel::Normal;
        MasterVolume = 1.0f;
        MusicVolume = 0.8f;
        SFXVolume = 1.0f;
        AutoSaveInterval = 5.0f;
        bShowDamageNumbers = true;
        bShowTutorialHints = true;
    }
};

/** Player statistics for tracking gameplay metrics */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FPlayerStatistics
{
    GENERATED_BODY()

    /** Total playtime in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Statistics")
    float TotalPlayTime;

    /** Number of deaths */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Statistics")
    int32 DeathCount;

    /** Number of kills */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Statistics")
    int32 KillCount;

    /** Distance traveled */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Statistics")
    float DistanceTraveled;

    /** Items crafted */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Statistics")
    int32 ItemsCrafted;

    /** Quests completed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Statistics")
    int32 QuestsCompleted;

    /** Locations discovered */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Statistics")
    TArray<FGameplayTag> DiscoveredLocations;
    double CreationTime;

    FPlayerStatistics()
    {
        TotalPlayTime = 0.0f;
        DeathCount = 0;
        KillCount = 0;
        DistanceTraveled = 0.0f;
        ItemsCrafted = 0;
        QuestsCompleted = 0;
    }
};

// ================================================================================
// GLOBAL DELEGATES AND EVENTS
// ================================================================================

// Game state events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGameStateChanged, EGameState, OldState, EGameState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDifficultyChanged, EDifficultyLevel, NewDifficulty);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameSettingsChanged, const FGameSettings&, NewSettings);

// World events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldTimeChanged, const FWorldTimeData&, NewTimeData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldWeatherChanged, const FWorldWeatherData&, NewWeatherData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldEventStarted, const FWorldEventData&, EventData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldEventEnded, const FWorldEventData&, EventData);

// Faction events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFactionRelationshipChanged, FGameplayTag, FactionA, FGameplayTag, FactionB);

// Global state events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGlobalFlagChanged, FGameplayTag, Flag);

// Player progression events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSkillChanged, FGameplayTag, SkillTag, float, OldValue, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerLevelChanged, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatisticUpdated, const FString&, StatisticName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLocationDiscovered, FGameplayTag, LocationTag);

// ================================================================================
// UTILITY FUNCTIONS
// ================================================================================

/** Utility class for common type conversions and helpers */
UCLASS(BlueprintType)
class RADIANTRPG_API URadiantTypeUtils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Convert time of day enum to approximate hour */
    UFUNCTION(BlueprintPure, Category = "Time Utils")
    static float TimeOfDayToHour(ETimeOfDay TimeOfDay);

    /** Convert hour (0-24) to time of day enum */
    UFUNCTION(BlueprintPure, Category = "Time Utils")
    static ETimeOfDay HourToTimeOfDay(float Hour);

    /** Get weather type display name */
    UFUNCTION(BlueprintPure, Category = "Weather Utils")
    static FText GetWeatherDisplayName(EWeatherType WeatherType);

    /** Get faction relationship display name */
    UFUNCTION(BlueprintPure, Category = "Faction Utils")
    static FText GetRelationshipDisplayName(EFactionRelationship Relationship);

    /** Convert reputation value to relationship enum */
    UFUNCTION(BlueprintPure, Category = "Faction Utils")
    static EFactionRelationship ReputationToRelationship(float ReputationValue);

    /** Get skill category display name */
    UFUNCTION(BlueprintPure, Category = "Skill Utils")
    static FText GetSkillCategoryDisplayName(ESkillCategory Category);

    /** Check if skill is in specific category */
    UFUNCTION(BlueprintPure, Category = "Skill Utils")
    static bool IsSkillInCategory(FGameplayTag SkillTag, ESkillCategory Category);
};