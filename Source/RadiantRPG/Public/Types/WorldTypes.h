// Source/RadiantRPG/Public/Types/WorldTypes.h

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CoreTypes.h"
#include "FactionTypes.h"
#include "TimeTypes.h"
#include "WorldTypes.generated.h"


/**
 * Spawn rule types
 */
UENUM(BlueprintType)
enum class ESpawnRuleType : uint8
{
    Ambient         UMETA(DisplayName = "Ambient"),
    Patrol          UMETA(DisplayName = "Patrol"),
    Guard           UMETA(DisplayName = "Guard"),
    Wildlife        UMETA(DisplayName = "Wildlife"),
    Hostile         UMETA(DisplayName = "Hostile"),
    Boss            UMETA(DisplayName = "Boss"),
    Event           UMETA(DisplayName = "Event-Based"),
    Timed           UMETA(DisplayName = "Time-Based"),
    
    MAX             UMETA(Hidden)
};

/**
 * Environmental hazard types
 */
UENUM(BlueprintType)
enum class EEnvironmentalHazard : uint8
{
    None            UMETA(DisplayName = "None"),
    Radiation       UMETA(DisplayName = "Radiation"),
    Poison          UMETA(DisplayName = "Poison Gas"),
    Extreme_Heat    UMETA(DisplayName = "Extreme Heat"),
    Extreme_Cold    UMETA(DisplayName = "Extreme Cold"),
    Lightning       UMETA(DisplayName = "Lightning"),
    Earthquake      UMETA(DisplayName = "Earthquake"),
    Flood           UMETA(DisplayName = "Flood"),
    Sandstorm       UMETA(DisplayName = "Sandstorm"),
    MagicDrain      UMETA(DisplayName = "Magic Drain"),
    
    MAX             UMETA(Hidden)
};

/**
 * Point of interest types
 */
UENUM(BlueprintType)
enum class EPointOfInterestType : uint8
{
    None            UMETA(DisplayName = "None"),
    Landmark        UMETA(DisplayName = "Landmark"),
    Resource        UMETA(DisplayName = "Resource Node"),
    Merchant        UMETA(DisplayName = "Merchant"),
    QuestGiver      UMETA(DisplayName = "Quest Giver"),
    Dungeon         UMETA(DisplayName = "Dungeon Entrance"),
    FastTravel      UMETA(DisplayName = "Fast Travel"),
    CraftingStation UMETA(DisplayName = "Crafting Station"),
    Storage         UMETA(DisplayName = "Storage"),
    RestArea        UMETA(DisplayName = "Rest Area"),
    Arena           UMETA(DisplayName = "Arena"),
    Hidden          UMETA(DisplayName = "Hidden Secret"),
    
    MAX             UMETA(Hidden)
};

/**
 * Zone danger levels
 */
UENUM(BlueprintType)
enum class EZoneDanger : uint8
{
    Safe            UMETA(DisplayName = "Safe"),
    Low             UMETA(DisplayName = "Low Danger"),
    Moderate        UMETA(DisplayName = "Moderate Danger"),
    High            UMETA(DisplayName = "High Danger"),
    Extreme         UMETA(DisplayName = "Extreme Danger"),
    Deadly          UMETA(DisplayName = "Deadly"),
    
    MAX             UMETA(Hidden)
};

/**
 * Zone type for different gameplay rules
 */
UENUM(BlueprintType)
enum class EZoneType : uint8
{
    Wilderness      UMETA(DisplayName = "Wilderness"),
    Settlement      UMETA(DisplayName = "Settlement"),
    Dungeon         UMETA(DisplayName = "Dungeon"),
    Cave            UMETA(DisplayName = "Cave"),
    Forest          UMETA(DisplayName = "Forest"),
    Desert          UMETA(DisplayName = "Desert"),
    Mountain        UMETA(DisplayName = "Mountain"),
    Swamp           UMETA(DisplayName = "Swamp"),
    Ruins           UMETA(DisplayName = "Ruins"),
    Battlefield     UMETA(DisplayName = "Battlefield"),
    Sacred          UMETA(DisplayName = "Sacred Ground"),
    Corrupted       UMETA(DisplayName = "Corrupted"),
    
    MAX             UMETA(Hidden)
};

/**
 * Zone weather state
 */
UENUM(BlueprintType)
enum class EZoneWeather : uint8
{
    Clear           UMETA(DisplayName = "Clear"),
    Cloudy          UMETA(DisplayName = "Cloudy"),
    Rain            UMETA(DisplayName = "Rain"),
    Storm           UMETA(DisplayName = "Storm"),
    Snow            UMETA(DisplayName = "Snow"),
    Fog             UMETA(DisplayName = "Fog")
};

/**
 * Weather conditions affecting gameplay
 */
UENUM(BlueprintType)
enum class EWeatherType : uint8
{
    Clear           UMETA(DisplayName = "Clear"),
    Cloudy          UMETA(DisplayName = "Cloudy"),
    Fog             UMETA(DisplayName = "Fog"),
    Rain            UMETA(DisplayName = "Rain"),
    LightRain       UMETA(DisplayName = "LightRain"),
    HeavyRain       UMETA(DisplayName = "HeavyRain"),
    Thunderstorm    UMETA(DisplayName = "Thunderstorm"),
    Storm           UMETA(DisplayName = "Storm"),
    Snow            UMETA(DisplayName = "Snow"),
    Blizzard        UMETA(DisplayName = "Blizzard"),
    Sandstorm       UMETA(DisplayName = "Sandstorm"),
    AcidRain        UMETA(DisplayName = "Acid Rain"),
    MagicalStorm    UMETA(DisplayName = "Magical Storm"),
    
    MAX             UMETA(Hidden)
};

/**
 * Environmental effect configuration
 */
USTRUCT(BlueprintType)
struct FEnvironmentalEffect
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EEnvironmentalHazard HazardType = EEnvironmentalHazard::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Intensity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DamagePerSecond = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FStatModifier StatEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRequiresProtection = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag ProtectionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector EffectCenter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float EffectRadius = 1000.0f;

    FEnvironmentalEffect()
    {
    }
};

/**
 * Point of interest data
 */
USTRUCT(BlueprintType)
struct FPointOfInterest
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName POIID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EPointOfInterestType Type = EPointOfInterestType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Location;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsDiscovered = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsActive = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UTexture2D> MapIcon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer POITags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TWeakObjectPtr<AActor> AssociatedActor;

    FPointOfInterest()
    {
    }
};




/**
 * Zone transition event
 */
USTRUCT(BlueprintType)
struct FZoneTransition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName FromZone;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ToZone;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector TransitionLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TransitionTime = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TWeakObjectPtr<AActor> TravelingActor;

    FZoneTransition()
    {
    }
};

/**
 * Zone boundary configuration
 */
USTRUCT(BlueprintType)
struct FZoneBoundary
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Center;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Extents;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FVector> BoundaryPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bUseComplexBoundary = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TransitionDistance = 100.0f;

    FZoneBoundary()
    {
        Extents = FVector(1000.0f, 1000.0f, 500.0f);
    }

    bool IsLocationInZone(const FVector& Location) const;
    float GetDistanceToBoundary(const FVector& Location) const;
};

/**
 * Level range for items and encounters
 */
USTRUCT(BlueprintType)
struct FLevelRange
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "100"))
    int32 MinLevel = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "100"))
    int32 MaxLevel = 100;

    FLevelRange()
    {
    }

    FLevelRange(int32 InMin, int32 InMax)
        : MinLevel(InMin), MaxLevel(InMax)
    {
    }

    bool IsInRange(int32 Level) const
    {
        return Level >= MinLevel && Level <= MaxLevel;
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
	float TransitionDuration;

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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldWeatherChanged, const FWorldWeatherData&, NewWeatherData);

/**
 * Single spawn rule configuration
 */
USTRUCT(BlueprintType)
struct FSpawnRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SpawnID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESpawnRuleType RuleType = ESpawnRuleType::Ambient;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> ActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MinCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpawnChance = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RespawnTime = 300.0f; // In seconds

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLevelRange LevelRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FValueRange SpawnRadius = FValueRange(100.0f, 500.0f);

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ETimeOfDay ActiveTime = ETimeOfDay::Morning;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer RequiredTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer SpawnTags;

	FSpawnRule()
	{
		LevelRange = FLevelRange(1, 10);
	}
};

/**
 * Complete zone data
 */
USTRUCT(BlueprintType)
struct FZoneData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ZoneID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EZoneType ZoneType = EZoneType::Wilderness;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EZoneDanger DangerLevel = EZoneDanger::Low;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLevelRange RecommendedLevel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FZoneBoundary Boundary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FSpawnRule> SpawnRules;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FEnvironmentalEffect> EnvironmentalEffects;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FPointOfInterest> PointsOfInterest;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EWeatherType DefaultWeather = EWeatherType::Clear;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<EWeatherType> PossibleWeatherTypes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<USoundBase> AmbientSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<USoundBase> DiscoverySound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor ZoneColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer ZoneTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FName> ConnectedZones;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ControllingFaction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAllowFastTravel = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAllowCombat = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsSafeZone = false;

    FZoneData()
    {
        RecommendedLevel = FLevelRange(1, 10);
    }
};

/**
 * World state snapshot
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FWorldState
{
    GENERATED_BODY()

    // === SIMPLIFIED TIME DATA ===
    
    /** Current world time data */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    FSimpleWorldTime CurrentTime;

    /** Whether time progression is enabled */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    bool bTimeProgressionEnabled = true;

    // === WEATHER DATA ===
    
    /** Current global weather state */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
    FWorldWeatherData GlobalWeather;

    // === ZONE DATA ===
    
    /** All zone data indexed by zone tag */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zones")
    TMap<FString, FZoneData> Zones;

    /** Zone population counts */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zones")
    TMap<FString, int32> ZonePopulations;

    /** Zone weather overrides (zones with custom weather) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zones")
    TMap<FString, FWorldWeatherData> ZoneWeatherOverrides;

    // === WORLD EVENTS ===
    
    /** Currently active world events */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Events")
    FGameplayTagContainer ActiveWorldEvents;

    /** Events that have been completed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Events")
    TArray<FString> CompletedEvents;

    /** Event metadata for active events */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Events")
    TMap<FString, FString> EventMetadata;

    // === FACTION DATA ===
    
    /** Faction relationships and standings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Factions")
    TMap<FString, FFactionRelationship> FactionRelationships;

    /** Current wars between factions */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Factions")
    TArray<FString> ActiveWars;

    /** Territory control by faction */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Factions")
    TMap<FString, FString> TerritoryControl; // ZoneID -> FactionID

    // === SIMULATION STATE ===
    
    /** Simulation version for compatibility checking */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
    int32 SimulationVersion = 1;

    /** Timestamp when this state was created */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
    FDateTime StateTimestamp;

    /** Total simulation time elapsed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
    float TotalSimulationTime = 0.0f;

    /** Simulation flags for various systems */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
    FGameplayTagContainer SimulationFlags;

    FWorldState()
    {
        CurrentTime = FSimpleWorldTime();
        GlobalWeather = FWorldWeatherData();
        SimulationVersion = 1;
        StateTimestamp = FDateTime::Now();
        TotalSimulationTime = 0.0f;
        bTimeProgressionEnabled = true;
    }

    /** Get formatted timestamp string */
    FString GetTimestampString() const
    {
        return StateTimestamp.ToString(TEXT("%Y-%m-%d %H:%M:%S"));
    }

    /** Check if this state is compatible with current simulation version */
    bool IsCompatibleVersion(int32 CurrentVersion) const
    {
        return SimulationVersion == CurrentVersion;
    }

    /** Get total number of active events */
    int32 GetActiveEventCount() const
    {
        return ActiveWorldEvents.Num();
    }

    /** Check if a specific event is active */
    bool IsEventActive(const FString& EventName) const
    {
        FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(*EventName);
        return ActiveWorldEvents.HasTag(EventTag);
    }

    /** Check if an event has been completed */
    bool IsEventCompleted(const FString& EventName) const
    {
        return CompletedEvents.Contains(EventName);
    }

    /** Get zone count */
    int32 GetZoneCount() const
    {
        return Zones.Num();
    }

    /** Get total population across all zones */
    int32 GetTotalPopulation() const
    {
        int32 Total = 0;
        for (const auto& PopPair : ZonePopulations)
        {
            Total += PopPair.Value;
        }
        return Total;
    }
};