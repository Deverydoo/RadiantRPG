// Source/RadiantRPG/Public/Types/ARPG_NPCTypes.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTags.h"
#include "Types/ARPG_AITypes.h"
#include "ARPG_NPCTypes.generated.h"

/**
 * NPC archetype types
 */
UENUM(BlueprintType)
enum class EARPG_NPCArchetype : uint8
{
    Generic         UMETA(DisplayName = "Generic"),
    Merchant        UMETA(DisplayName = "Merchant"),
    Guard           UMETA(DisplayName = "Guard"),
    Bandit          UMETA(DisplayName = "Bandit"),
    Villager        UMETA(DisplayName = "Villager"),
    Soldier         UMETA(DisplayName = "Soldier"),
    Noble           UMETA(DisplayName = "Noble"),
    Mage            UMETA(DisplayName = "Mage"),
    Hunter          UMETA(DisplayName = "Hunter"),
    Crafter         UMETA(DisplayName = "Crafter"),
    MAX             UMETA(Hidden)
};

/**
 * NPC behavior categories
 */
UENUM(BlueprintType)
enum class EARPG_NPCBehaviorCategory : uint8
{
    Passive         UMETA(DisplayName = "Passive"),
    Defensive       UMETA(DisplayName = "Defensive"),
    Aggressive      UMETA(DisplayName = "Aggressive"),
    Neutral         UMETA(DisplayName = "Neutral"),
    Friendly        UMETA(DisplayName = "Friendly"),
    Hostile         UMETA(DisplayName = "Hostile"),
    MAX             UMETA(Hidden)
};

/**
 * Configuration for the needs system
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_NeedsConfiguration
{
    GENERATED_BODY()

    /** How frequently to update needs (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "0.1"))
    float UpdateFrequency = 1.0f;

    /** Global multiplier for all need change rates */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "0.1"))
    float GlobalChangeRateMultiplier = 1.0f;

    /** Whether needs can affect AI behavior */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    bool bEnableNeedsInfluence = true;

    /** Whether to enable debug logging */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    bool bEnableDebugLogging = false;

    /** Whether to enable debug visualization */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    bool bEnableDebugVisualization = false;

    FARPG_NeedsConfiguration()
    {
        UpdateFrequency = 1.0f;
        GlobalChangeRateMultiplier = 1.0f;
        bEnableNeedsInfluence = true;
        bEnableDebugLogging = false;
        bEnableDebugVisualization = false;
    }
};

/**
 * NPC configuration data
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_NPCConfiguration
{
    GENERATED_BODY()

    /** NPC type identifier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FGameplayTag NPCType;

    /** NPC archetype */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    EARPG_NPCArchetype Archetype = EARPG_NPCArchetype::Generic;

    /** Faction this NPC belongs to */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FGameplayTag Faction;

    /** Display name for this NPC */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText DisplayName;

    /** Behavior category */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
    EARPG_NPCBehaviorCategory BehaviorCategory = EARPG_NPCBehaviorCategory::Neutral;

    /** Brain configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    FARPG_AIBrainConfiguration BrainConfig;

    /** Needs configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    FARPG_NeedsConfiguration NeedsConfig;

    /** Whether to use custom needs setup */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    bool bUseCustomNeeds = false;

    /** Initial needs if using custom setup */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (EditCondition = "bUseCustomNeeds"))
    TArray<FARPG_AINeed> InitialNeeds;

    /** Personality traits */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    TArray<FARPG_AIPersonalityTrait> PersonalityTraits;

    /** Tags that define this NPC's characteristics */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
    FGameplayTagContainer NPCTags;

    /** Default dialogue tree asset */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social")
    TSoftObjectPtr<class UObject> DialogueAsset;

    /** Trade/merchant configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trading")
    TSoftObjectPtr<class UObject> MerchantData;

    /** Spawn probability modifier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SpawnProbability = 1.0f;

    /** Level range for this NPC type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
    FIntPoint LevelRange = FIntPoint(1, 10);

    /** Equipment sets this NPC can spawn with */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    TArray<FGameplayTag> EquipmentSets;

    FARPG_NPCConfiguration()
    {
        NPCType = FGameplayTag::RequestGameplayTag(TEXT("NPC.Type.Generic"));
        Faction = FGameplayTag::RequestGameplayTag(TEXT("Faction.Neutral"));
        DisplayName = FText::FromString(TEXT("NPC"));
        Archetype = EARPG_NPCArchetype::Generic;
        BehaviorCategory = EARPG_NPCBehaviorCategory::Neutral;
        SpawnProbability = 1.0f;
        LevelRange = FIntPoint(1, 10);
        bUseCustomNeeds = false;
    }
};

/**
 * NPC spawn configuration
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_NPCSpawnData
{
    GENERATED_BODY()

    /** Class to spawn */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
    TSoftClassPtr<class AARPG_BaseNPCCharacter> NPCClass;

    /** Configuration to apply */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
    FARPG_NPCConfiguration Configuration;

    /** Spawn location offset */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
    FVector LocationOffset = FVector::ZeroVector;

    /** Spawn rotation offset */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
    FRotator RotationOffset = FRotator::ZeroRotator;

    /** Zone restrictions */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
    TArray<FGameplayTag> AllowedZones;

    /** Time of day restrictions */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
    TArray<FGameplayTag> AllowedTimeOfDay;

    /** Weather restrictions */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
    TArray<FGameplayTag> AllowedWeather;

    FARPG_NPCSpawnData()
    {
        LocationOffset = FVector::ZeroVector;
        RotationOffset = FRotator::ZeroRotator;
    }
};