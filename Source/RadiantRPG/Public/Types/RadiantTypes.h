// Public/Types/RadiantTypes.h
// Core type definitions and delegates for RadiantRPG to prevent circular dependencies

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
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