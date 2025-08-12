// Public/Types/ARPG_AIDataTableTypes.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Types/ARPG_AITypes.h"
#include "ARPG_AIDataTableTypes.generated.h"

enum class EARPG_MemoryType : uint8;
/**
 * Data table row for Species Memory Configurations
 * Allows customization of memory settings per creature type/species
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_MemoryConfigRow : public FTableRowBase
{
    GENERATED_BODY()

    /** Display name for this memory configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText DisplayName;

    /** Description of this memory configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText Description;

    /** Species/creature type this config applies to */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FGameplayTag SpeciesType;

    // === CAPACITY SETTINGS ===

    /** Maximum number of short-term memories per type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capacity", meta = (ClampMin = "1", ClampMax = "1000"))
    int32 MaxShortTermMemories = 50;

    /** Maximum number of long-term memories per type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capacity", meta = (ClampMin = "1", ClampMax = "10000"))
    int32 MaxLongTermMemories = 200;

    // === TIMING SETTINGS ===

    /** How long memories stay in short-term storage (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transfer", meta = (ClampMin = "30.0", ClampMax = "3600.0"))
    float ShortTermDuration = 300.0f;

    /** Minimum strength required for long-term transfer */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transfer", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float LongTermThreshold = 0.6f;

    /** Strength threshold below which memories are forgotten */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forgetting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ForgetThreshold = 0.1f;

    /** How often to update memory decay (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance", meta = (ClampMin = "0.1", ClampMax = "60.0"))
    float DecayUpdateFrequency = 5.0f;

    // === MEMORY CHARACTERISTICS ===

    /** Base memory decay rate multiplier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Characteristics", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float MemoryDecayRate = 1.0f;

    /** How much emotional events boost memory formation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Characteristics", meta = (ClampMin = "0.0", ClampMax = "3.0"))
    float EmotionalMemoryBoost = 1.5f;

    /** Whether this species forms vivid memories easily */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Characteristics")
    bool bFormsVividMemories = true;

    /** Whether this species can form long-term memories */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory Characteristics")
    bool bCanFormLongTermMemories = true;

    // === INTELLIGENCE FACTORS ===

    /** General intelligence modifier affecting memory quality */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intelligence", meta = (ClampMin = "0.1", ClampMax = "3.0"))
    float IntelligenceModifier = 1.0f;

    /** Whether this species learns from experience */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intelligence")
    bool bCanLearnFromMemories = true;

    /** Whether memories affect future decision making */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intelligence")
    bool bMemoriesInfluenceBehavior = true;

    // === SPECIAL TRAITS ===

    /** Whether to enable debug logging for this species */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bEnableDebugLogging = false;

    /** Special memory tags this species prioritizes */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special Traits")
    TArray<FGameplayTag> PriorityMemoryTags;

    /** Memory types this species tends to ignore */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special Traits")
    TArray<EARPG_MemoryType> IgnoredMemoryTypes;

    FARPG_MemoryConfigRow()
    {
        DisplayName = FText::FromString(TEXT("Default Memory Config"));
        Description = FText::FromString(TEXT("Default memory configuration for standard NPCs"));
        SpeciesType = FGameplayTag::RequestGameplayTag(TEXT("Species.Human"));
        
        MaxShortTermMemories = 50;
        MaxLongTermMemories = 200;
        ShortTermDuration = 300.0f;
        LongTermThreshold = 0.6f;
        ForgetThreshold = 0.1f;
        DecayUpdateFrequency = 5.0f;
        MemoryDecayRate = 1.0f;
        EmotionalMemoryBoost = 1.5f;
        IntelligenceModifier = 1.0f;
        
        bFormsVividMemories = true;
        bCanFormLongTermMemories = true;
        bCanLearnFromMemories = true;
        bMemoriesInfluenceBehavior = true;
        bEnableDebugLogging = false;
    }
};

/**
 * Data table row for AI Brain Configurations
 * Used to configure different NPC types with varying AI behaviors
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_AIBrainConfigRow : public FTableRowBase
{
    GENERATED_BODY()

    /** Display name for this brain configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText DisplayName;

    /** Description of this brain configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText Description;

    /** NPC type this config applies to */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FGameplayTag NPCType;

    /** How often the brain updates (in seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float BrainUpdateFrequency = 1.0f;

    /** How long stimuli are remembered (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "1.0", ClampMax = "60.0"))
    float StimuliMemoryDuration = 10.0f;

    /** Time without stimuli before curiosity activates (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curiosity", meta = (ClampMin = "5.0", ClampMax = "300.0"))
    float CuriosityThreshold = 30.0f;

    /** How strong curiosity is when activated */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curiosity", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float CuriosityStrength = 1.0f;

    /** Whether this brain can form memories */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Features")
    bool bCanFormMemories = true;

    /** Whether this brain can learn and adapt */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Features")
    bool bCanLearn = false;

    /** Whether to log brain decisions for debugging */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bEnableDebugLogging = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    int32 MaxTrackedStimuli;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    FGameplayTag DefaultIdleIntent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    TArray<FGameplayTag> CuriosityIntents;

    FARPG_AIBrainConfigRow()
    {
        DisplayName = FText::FromString(TEXT("Default Brain Config"));
        Description = FText::FromString(TEXT("Default brain configuration for standard NPCs"));
        NPCType = FGameplayTag::RequestGameplayTag(TEXT("NPC.Type.Human"));
        
        BrainUpdateFrequency = 1.0f;
        StimuliMemoryDuration = 10.0f;
        CuriosityThreshold = 30.0f;
        CuriosityStrength = 1.0f;
        
        bCanFormMemories = true;
        bCanLearn = false;
        bEnableDebugLogging = false;
    }
};
 
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_AINeedsConfigRow : public FTableRowBase
{
    GENERATED_BODY()

    /** Display name for this needs configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText ConfigName;

    /** Description of this needs setup */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText Description;

    /** NPC type this config applies to */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FGameplayTag NPCType;

    /** How quickly hunger decreases (per minute) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decay Rates", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float HungerDecayRate = 1.0f;

    /** How quickly fatigue increases (per minute) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decay Rates", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float FatigueDecayRate = 0.5f;

    /** How quickly safety need changes */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decay Rates", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float SafetyDecayRate = 0.2f;

    /** How quickly social need changes */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decay Rates", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float SocialDecayRate = 0.3f;

    /** How quickly entertainment need changes */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decay Rates", meta = (ClampMin = "0.0", ClampMax = "3.0"))
    float EntertainmentDecayRate = 0.1f;

    /** Hunger threshold for critical state */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds", meta = (ClampMin = "0.0", ClampMax = "50.0"))
    float HungerCriticalThreshold = 20.0f;

    /** Fatigue threshold for critical state */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds", meta = (ClampMin = "0.0", ClampMax = "50.0"))
    float FatigueCriticalThreshold = 25.0f;

    /** Safety threshold for critical state */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds", meta = (ClampMin = "0.0", ClampMax = "50.0"))
    float SafetyCriticalThreshold = 30.0f;

    /** Social threshold for critical state */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds", meta = (ClampMin = "0.0", ClampMax = "50.0"))
    float SocialCriticalThreshold = 15.0f;

    /** Starting hunger level */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Starting Values", meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float StartingHunger = 70.0f;

    /** Starting fatigue level */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Starting Values", meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float StartingFatigue = 80.0f;

    /** Starting safety level */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Starting Values", meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float StartingSafety = 90.0f;

    /** Starting social level */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Starting Values", meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float StartingSocial = 60.0f;

    /** Whether this NPC type needs food */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Features")
    bool bNeedsFood = true;

    /** Whether this NPC type needs sleep */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Features")
    bool bNeedsSleep = true;

    /** Whether this NPC type needs social interaction */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Features")
    bool bNeedsSocial = true;

    /** How often to update needs (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance", meta = (ClampMin = "1.0", ClampMax = "30.0"))
    float UpdateFrequency = 5.0f;

    FARPG_AINeedsConfigRow()
    {
        ConfigName = FText::FromString(TEXT("Standard Human"));
        Description = FText::FromString(TEXT("Standard human needs configuration"));
    }
};

// Addition to Public/Types/ARPG_AIDataTableTypes.h

/**
 * Data table row for Faction Definitions
 * Defines all factions in the game world
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_FactionDefinitionRow : public FTableRowBase
{
    GENERATED_BODY()

    /** Faction name displayed to players */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText FactionName;

    /** Detailed description of this faction */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText Description;

    /** Gameplay tag for this faction */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FGameplayTag FactionTag;

    /** Parent faction (if any) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hierarchy")
    FGameplayTag ParentFaction;

    /** Child/subfactions */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hierarchy")
    TArray<FGameplayTag> ChildFactions;

    /** Whether this faction is hostile to all others by default */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default Behavior")
    bool bDefaultHostile = false;

    /** Whether this faction is friendly to all others by default */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default Behavior")
    bool bDefaultFriendly = false;

    /** Default reputation with this faction */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default Behavior", meta = (ClampMin = "-100.0", ClampMax = "100.0"))
    float DefaultPlayerReputation = 0.0f;

    /** Whether player can join this faction */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Interaction")
    bool bPlayerCanJoin = true;

    /** Whether this faction shows reputation to player */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Interaction")
    bool bShowReputationToPlayer = true;

    /** Color used to represent this faction in UI */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    FLinearColor FactionColor = FLinearColor::White;

    /** Icon used to represent this faction */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    TSoftObjectPtr<UTexture2D> FactionIcon;

    /** Goals this faction is actively pursuing */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Goals")
    TArray<FGameplayTag> ActiveGoals;

    /** Territories this faction controls */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory")
    TArray<FGameplayTag> ControlledZones;

    /** Resource types this faction values */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
    TArray<FGameplayTag> ValuedResources;

    /** Whether this faction can declare war */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warfare")
    bool bCanDeclareWar = true;

    /** Whether this faction can form alliances */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Diplomacy")
    bool bCanFormAlliances = true;

    FARPG_FactionDefinitionRow()
    {
        FactionName = FText::FromString(TEXT("Unknown Faction"));
        Description = FText::FromString(TEXT("A mysterious faction"));
        FactionColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
    }
};

/**
 * Data table row for AI Personality Trait Templates
 * Used to define personality combinations for different NPC types
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_AIPersonalityTraitRow : public FTableRowBase
{
    GENERATED_BODY()

    /** Display name for this personality */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText PersonalityName;

    /** Description of this personality type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText Description;

    /** How aggressive this personality is */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traits", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Aggression = 0.5f;

    /** How curious/exploratory this personality is */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traits", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Curiosity = 0.5f;

    /** How social this personality is */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traits", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Sociability = 0.5f;

    /** How brave this personality is */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traits", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Bravery = 0.5f;

    /** How greedy/materialistic this personality is */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traits", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Greed = 0.5f;

    /** How loyal this personality is */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traits", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Loyalty = 0.5f;

    /** How intelligent this personality is */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traits", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Intelligence = 0.5f;

    /** How quickly this personality gets bored (minutes) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "1.0", ClampMax = "30.0"))
    float BoredomThreshold = 5.0f;

    /** How cautious vs reckless this personality is */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traits", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Caution = 0.5f;

    /** How impulsive vs methodical this personality is */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traits", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Impulsiveness = 0.5f;

    /** Personality-specific intent preferences */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
    TMap<FGameplayTag, float> IntentBiases;

    FARPG_AIPersonalityTraitRow()
    {
        PersonalityName = FText::FromString(TEXT("Balanced"));
        Description = FText::FromString(TEXT("A well-balanced personality"));
    }
};

