// Public/Types/ARPG_NeedsDataTypes.h
// Data table structures for creature needs configurations

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Types/ARPG_AITypes.h"
#include "ARPG_NeedsDataTypes.generated.h"

/**
 * Creature archetype for needs configuration
 */
UENUM(BlueprintType)
enum class EARPG_CreatureArchetype : uint8
{
    // === HUMANOIDS ===
    Human           UMETA(DisplayName = "Human"),
    Villager        UMETA(DisplayName = "Villager"),
    Guard           UMETA(DisplayName = "Guard"),
    Merchant        UMETA(DisplayName = "Merchant"),
    Bandit          UMETA(DisplayName = "Bandit"),
    Soldier         UMETA(DisplayName = "Soldier"),
    
    // === FANTASY RACES ===
    Dwarf           UMETA(DisplayName = "Dwarf"),
    Elf             UMETA(DisplayName = "Elf"),
    HalfOrc         UMETA(DisplayName = "Half-Orc"),
    Goblin          UMETA(DisplayName = "Goblin"),
    Orc             UMETA(DisplayName = "Orc"),
    Troll           UMETA(DisplayName = "Troll"),
    
    // === ANIMALS ===
    Wolf            UMETA(DisplayName = "Wolf"),
    Bear            UMETA(DisplayName = "Bear"),
    Deer            UMETA(DisplayName = "Deer"),
    CobraSnake      UMETA(DisplayName = "Cobra Snake"),
    
    // === INSECTS & ARACHNIDS ===
    Spider          UMETA(DisplayName = "Spider"),
    BombardierBeetle UMETA(DisplayName = "Bombardier Beetle"),
    BomberBug       UMETA(DisplayName = "Bomber Bug"),
    
    // === UNDEAD ===
    Undead          UMETA(DisplayName = "Undead"),
    Skeleton        UMETA(DisplayName = "Skeleton"),
    Zombie          UMETA(DisplayName = "Zombie"),
    Vampire         UMETA(DisplayName = "Vampire"),
    Lich            UMETA(DisplayName = "Lich"),
    Spirit          UMETA(DisplayName = "Spirit"),
    BoneDragon      UMETA(DisplayName = "Bone Dragon"),
    
    // === CONSTRUCTS & GOLEMS ===
    Construct       UMETA(DisplayName = "Construct"),
    Golem           UMETA(DisplayName = "Golem"),
    Gargoyle        UMETA(DisplayName = "Gargoyle"),
    RockMonster     UMETA(DisplayName = "Rock Monster"),
    
    // === ELEMENTALS ===
    Elemental       UMETA(DisplayName = "Elemental"),
    
    // === DRAGONS ===
    Dragon          UMETA(DisplayName = "Dragon"),
    
    // === MYTHOLOGICAL ===
    Minotaur        UMETA(DisplayName = "Minotaur"),
    Medusa          UMETA(DisplayName = "Medusa"),
    Phoenix         UMETA(DisplayName = "Phoenix"),
    
    // === DEMONS & FIENDS ===
    Demon           UMETA(DisplayName = "Demon"),
    Succubus        UMETA(DisplayName = "Succubus"),
    Imp             UMETA(DisplayName = "Imp"),
    
    // === CELESTIALS ===
    Angel           UMETA(DisplayName = "Angel"),
    
    // === FEY ===
    Fairy           UMETA(DisplayName = "Fairy"),
    Harlequin       UMETA(DisplayName = "Harlequin"),
    
    // === ABERRATIONS ===
    Gazer           UMETA(DisplayName = "Gazer"),
    
    // === SLIMES & OOZES ===
    Slime           UMETA(DisplayName = "Slime"),
    
    // === MIMICS ===
    Mimic           UMETA(DisplayName = "Mimic"),
    
    // === AQUATIC ===
    FishMan         UMETA(DisplayName = "Fish Man"),
    
    // === PLANT CREATURES ===
    PlantMonster    UMETA(DisplayName = "Plant Monster"),
    MushroomMonster UMETA(DisplayName = "Mushroom Monster"),
    
    // === WORMS ===
    GiantWorm       UMETA(DisplayName = "Giant Worm"),

    Unknown       UMETA(DisplayName = "Unknown"),
    
    MAX             UMETA(Hidden)
};

/**
 * Individual need configuration for a specific creature type
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_CreatureNeedConfig
{
    GENERATED_BODY()

    /** Type of need */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need Config")
    EARPG_NeedType NeedType = EARPG_NeedType::Hunger;

    /** Starting level for this need (0.0 - 1.0) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need Config", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float StartingLevel = 0.3f;

    /** Rate at which this need decays/increases over time per second */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need Config")
    float DecayRate = 0.001f;

    /** Level at which this need becomes urgent (0.0 - 1.0) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need Config", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float UrgencyThreshold = 0.7f;

    /** Level at which this need becomes critical (0.0 - 1.0) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need Config", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CriticalThreshold = 0.9f;

    /** Maximum level this need can reach */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need Config", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float MaxLevel = 1.0f;

    /** Whether this need is active for this creature type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need Config")
    bool bIsActive = true;

    /** Priority of this need for AI decision making (higher = more important) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need Config", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Priority = 0.5f;

    FARPG_CreatureNeedConfig()
    {
        NeedType = EARPG_NeedType::Hunger;
        StartingLevel = 0.3f;
        DecayRate = 0.001f;
        UrgencyThreshold = 0.7f;
        CriticalThreshold = 0.9f;
        MaxLevel = 1.0f;
        bIsActive = true;
        Priority = 0.5f;
    }
};

/**
 * Complete needs configuration for a creature archetype
 * Used as data table row for configuring different creature types
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_CreatureNeedsProfile : public FTableRowBase
{
    GENERATED_BODY()

    /** Display name for this creature profile */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profile")
    FString DisplayName;

    /** Description of this creature type's needs */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profile")
    FString Description;

    /** Creature archetype this profile applies to */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profile")
    EARPG_CreatureArchetype ArchetypeType = EARPG_CreatureArchetype::Human;

    /** Individual need configurations for this creature */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Needs")
    TArray<FARPG_CreatureNeedConfig> NeedConfigs;

    /** Global decay multiplier for all needs of this creature type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float GlobalDecayMultiplier = 1.0f;

    /** How frequently to update needs (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float UpdateFrequency = 1.0f;

    /** Whether to enable debug logging for this creature type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    bool bEnableDebugLogging = false;

    FARPG_CreatureNeedsProfile()
    {
        DisplayName = TEXT("Default");
        Description = TEXT("Default creature needs configuration");
        ArchetypeType = EARPG_CreatureArchetype::Human;
        GlobalDecayMultiplier = 1.0f;
        UpdateFrequency = 1.0f;
        bEnableDebugLogging = false;
    }
};