// Source/RadiantRPG/Public/Types/CoreTypes.h

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CoreTypes.generated.h"

/**
 * Core game statistics used across multiple systems
 */
UENUM(BlueprintType)
enum class ERadiantStatType : uint8
{
    Health          UMETA(DisplayName = "Health"),
    Stamina         UMETA(DisplayName = "Stamina"),
    Mana            UMETA(DisplayName = "Mana"),
    Strength        UMETA(DisplayName = "Strength"),
    Dexterity       UMETA(DisplayName = "Dexterity"),
    Intelligence    UMETA(DisplayName = "Intelligence"),
    Wisdom          UMETA(DisplayName = "Wisdom"),
    Constitution    UMETA(DisplayName = "Constitution"),
    Charisma        UMETA(DisplayName = "Charisma"),
    Luck            UMETA(DisplayName = "Luck"),
    
    MAX             UMETA(Hidden)
};

/**
 * Damage types for combat calculations
 */
UENUM(BlueprintType)
enum class EDamageType : uint8
{
    Physical        UMETA(DisplayName = "Physical"),
    Fire            UMETA(DisplayName = "Fire"),
    Cold            UMETA(DisplayName = "Cold"),
    Lightning       UMETA(DisplayName = "Lightning"),
    Poison          UMETA(DisplayName = "Poison"),
    Arcane          UMETA(DisplayName = "Arcane"),
    Divine          UMETA(DisplayName = "Divine"),
    Necrotic        UMETA(DisplayName = "Necrotic"),
    Psychic         UMETA(DisplayName = "Psychic"),
    
    MAX             UMETA(Hidden)
};

/**
 * Generic rarity tiers used for items, encounters, etc.
 */
UENUM(BlueprintType)
enum class ERarityTier : uint8
{
    Common          UMETA(DisplayName = "Common"),
    Uncommon        UMETA(DisplayName = "Uncommon"),
    Rare            UMETA(DisplayName = "Rare"),
    Epic            UMETA(DisplayName = "Epic"),
    Legendary       UMETA(DisplayName = "Legendary"),
    Mythic          UMETA(DisplayName = "Mythic"),
    Unique          UMETA(DisplayName = "Unique"),
    
    MAX             UMETA(Hidden)
};

/**
 * Basic stat modifier structure
 */
USTRUCT(BlueprintType)
struct FStatModifier
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERadiantStatType StatType = ERadiantStatType::Health;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FlatBonus = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PercentBonus = 0.0f;

    // Duration in seconds, 0 means permanent
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Duration = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag SourceTag;

    FStatModifier()
    {
    }
};

/**
 * Damage information structure
 */
USTRUCT(BlueprintType)
struct FDamageInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BaseDamage = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EDamageType DamageType = EDamageType::Physical;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCanCrit = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CritChance = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CritMultiplier = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIgnoreArmor = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TWeakObjectPtr<AActor> Source;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TWeakObjectPtr<AActor> Causer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer DamageTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Amount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    AActor* DamageSource;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector HitLocation;

    FDamageInfo(): Amount(0), DamageSource(nullptr), HitLocation()
    {
    }
};

USTRUCT(BlueprintType)
struct FDamageResistance
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resistance")
    EDamageType DamageType;

    // Percentage resistance (0.0 = no resistance, 1.0 = full immunity)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resistance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ResistancePercent;

    FDamageResistance()
    {
        DamageType = EDamageType::Physical;
        ResistancePercent = 0.0f;
    }
};

/**
 * Range structure for min/max values
 */
USTRUCT(BlueprintType)
struct FValueRange
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
    float Min = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
    float Max = 100.0f;

    FValueRange()
    {
    }

    FValueRange(float InMin, float InMax)
        : Min(InMin), Max(InMax)
    {
    }

    float GetRandomValue() const
    {
        return FMath::FRandRange(Min, Max);
    }

    float Clamp(float Value) const
    {
        return FMath::Clamp(Value, Min, Max);
    }
};


/**
 * Weighted selection entry for randomized choices
 */
USTRUCT(BlueprintType)
struct FWeightedChoice
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ChoiceName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
    float Weight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag RequiredTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer BlockingTags;

    FWeightedChoice()
    {
    }
};

