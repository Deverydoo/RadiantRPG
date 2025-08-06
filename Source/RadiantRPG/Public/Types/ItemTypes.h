// Source/RadiantRPG/Public/Types/ItemTypes.h

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "CoreTypes.h"
#include "WorldTypes.h"
#include "ItemTypes.generated.h"

/**
 * Item categories for inventory management
 */
UENUM(BlueprintType)
enum class EItemCategory : uint8
{
    None            UMETA(DisplayName = "None"),
    Weapon          UMETA(DisplayName = "Weapon"),
    Armor           UMETA(DisplayName = "Armor"),
    Consumable      UMETA(DisplayName = "Consumable"),
    CraftingMaterial UMETA(DisplayName = "Crafting Material"),
    Tool            UMETA(DisplayName = "Tool"),
    Quest           UMETA(DisplayName = "Quest Item"),
    Junk            UMETA(DisplayName = "Junk"),
    Currency        UMETA(DisplayName = "Currency"),
    Ammo            UMETA(DisplayName = "Ammunition"),
    Book            UMETA(DisplayName = "Book/Scroll"),
    Key             UMETA(DisplayName = "Key"),
    Misc            UMETA(DisplayName = "Miscellaneous"),
    
    MAX             UMETA(Hidden)
};

/**
 * Weapon types for combat
 */
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
    None            UMETA(DisplayName = "None"),
    Sword           UMETA(DisplayName = "Sword"),
    Axe             UMETA(DisplayName = "Axe"),
    Mace            UMETA(DisplayName = "Mace"),
    Dagger          UMETA(DisplayName = "Dagger"),
    Spear           UMETA(DisplayName = "Spear"),
    Bow             UMETA(DisplayName = "Bow"),
    Crossbow        UMETA(DisplayName = "Crossbow"),
    Staff           UMETA(DisplayName = "Staff"),
    Wand            UMETA(DisplayName = "Wand"),
    Shield          UMETA(DisplayName = "Shield"),
    Fist            UMETA(DisplayName = "Fist Weapon"),
    Thrown          UMETA(DisplayName = "Thrown"),
    
    MAX             UMETA(Hidden)
};

/**
 * Armor slots for equipment
 */
UENUM(BlueprintType)
enum class EArmorSlot : uint8
{
    None            UMETA(DisplayName = "None"),
    Head            UMETA(DisplayName = "Head"),
    Chest           UMETA(DisplayName = "Chest"),
    Legs            UMETA(DisplayName = "Legs"),
    Feet            UMETA(DisplayName = "Feet"),
    Hands           UMETA(DisplayName = "Hands"),
    Shoulders       UMETA(DisplayName = "Shoulders"),
    Back            UMETA(DisplayName = "Back"),
    Waist           UMETA(DisplayName = "Waist"),
    
    MAX             UMETA(Hidden)
};

/**
 * Equipment slots including weapons and accessories
 */
UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
    None            UMETA(DisplayName = "None"),
    MainHand        UMETA(DisplayName = "Main Hand"),
    OffHand         UMETA(DisplayName = "Off Hand"),
    TwoHand         UMETA(DisplayName = "Two Hand"),
    Head            UMETA(DisplayName = "Head"),
    Chest           UMETA(DisplayName = "Chest"),
    Legs            UMETA(DisplayName = "Legs"),
    Feet            UMETA(DisplayName = "Feet"),
    Hands           UMETA(DisplayName = "Hands"),
    Shoulders       UMETA(DisplayName = "Shoulders"),
    Back            UMETA(DisplayName = "Back"),
    Waist           UMETA(DisplayName = "Waist"),
    Neck            UMETA(DisplayName = "Neck"),
    Ring1           UMETA(DisplayName = "Ring 1"),
    Ring2           UMETA(DisplayName = "Ring 2"),
    Trinket1        UMETA(DisplayName = "Trinket 1"),
    Trinket2        UMETA(DisplayName = "Trinket 2"),
    
    MAX             UMETA(Hidden)
};

/**
 * Crafting quality tiers
 */
UENUM(BlueprintType)
enum class ECraftingQuality : uint8
{
    Poor            UMETA(DisplayName = "Poor"),
    Standard        UMETA(DisplayName = "Standard"),
    Fine            UMETA(DisplayName = "Fine"),
    Superior        UMETA(DisplayName = "Superior"),
    Exceptional     UMETA(DisplayName = "Exceptional"),
    Masterwork      UMETA(DisplayName = "Masterwork"),
    
    MAX             UMETA(Hidden)
};

/**
 * Resource types for harvesting
 */
UENUM(BlueprintType)
enum class EResourceType : uint8
{
    None            UMETA(DisplayName = "None"),
    Wood            UMETA(DisplayName = "Wood"),
    Stone           UMETA(DisplayName = "Stone"),
    Metal           UMETA(DisplayName = "Metal Ore"),
    Herb            UMETA(DisplayName = "Herb"),
    Fiber           UMETA(DisplayName = "Fiber"),
    Hide            UMETA(DisplayName = "Hide"),
    Meat            UMETA(DisplayName = "Meat"),
    Gem             UMETA(DisplayName = "Gem"),
    Crystal         UMETA(DisplayName = "Crystal"),
    Essence         UMETA(DisplayName = "Essence"),
    
    MAX             UMETA(Hidden)
};

/**
 * Single item stat bonus
 */
USTRUCT(BlueprintType)
struct FItemStatBonus
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERadiantStatType StatType = ERadiantStatType::Health;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Value = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsPercentage = false;

    FItemStatBonus()
    {
    }
};

/**
 * Randomly rolled item affix
 */
USTRUCT(BlueprintType)
struct FItemAffix
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName AffixName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FItemStatBonus> StatBonuses;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer GrantedTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERarityTier MinRarity = ERarityTier::Uncommon;

    FItemAffix()
    {
    }
};

/**
 * Core item data structure
 */
USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ItemID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UStaticMesh> WorldMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EItemCategory Category = EItemCategory::Misc;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERarityTier Rarity = ERarityTier::Common;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxStackSize = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Weight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 BaseValue = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLevelRange RequiredLevel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FItemStatBonus> BaseStats;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer ItemTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer RequiredTags;

    // Weapon specific
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "Category == EItemCategory::Weapon"))
    EWeaponType WeaponType = EWeaponType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "Category == EItemCategory::Weapon"))
    FValueRange DamageRange;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "Category == EItemCategory::Weapon"))
    float AttackSpeed = 1.0f;

    // Armor specific
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "Category == EItemCategory::Armor"))
    EArmorSlot ArmorSlot = EArmorSlot::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "Category == EItemCategory::Armor"))
    float ArmorValue = 0.0f;

    // Consumable specific
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "Category == EItemCategory::Consumable"))
    float UseTime = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "Category == EItemCategory::Consumable"))
    int32 UsesPerItem = 1;

    FItemData()
    {
        RequiredLevel = FLevelRange(1, 100);
    }
};

/**
 * Runtime item instance with unique properties
 */
USTRUCT(BlueprintType)
struct FItemInstance
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ItemID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid UniqueID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Quantity = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Durability = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxDurability = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECraftingQuality Quality = ECraftingQuality::Standard;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FItemAffix> Affixes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FItemStatBonus> BonusStats;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer CustomTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FName, float> CustomData;

    FItemInstance()
    {
        UniqueID = FGuid::NewGuid();
    }

    bool IsStackable() const;
    bool CanStackWith(const FItemInstance& Other) const;
};

/**
 * Crafting recipe structure
 */
USTRUCT(BlueprintType)
struct FCraftingRecipe : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName RecipeID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FItemInstance> RequiredItems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FItemInstance> OutputItems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CraftingTime = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag RequiredSkill;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RequiredSkillLevel = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer RequiredTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag CraftingStation;

    FCraftingRecipe()
    {
    }
};

/**
 * Loot table entry
 */
USTRUCT(BlueprintType)
struct FLootEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ItemID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DropChance = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FValueRange QuantityRange = FValueRange(1.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERarityTier MinRarity = ERarityTier::Common;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERarityTier MaxRarity = ERarityTier::Legendary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer RequiredTags;

    FLootEntry()
    {
    }
};