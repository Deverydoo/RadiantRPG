// Public/Components/ManaComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ManaComponent.generated.h"

class ABaseCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnManaChanged, float, NewMana, float, NewMaxMana);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnManaSpent, float, ManaCost, AActor*, SpellCaster);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnManaEmpty);

UENUM(BlueprintType)
enum class EManaSchool : uint8
{
    Arcane UMETA(DisplayName = "Arcane"),
    Fire UMETA(DisplayName = "Fire"),
    Ice UMETA(DisplayName = "Ice"),
    Lightning UMETA(DisplayName = "Lightning"),
    Nature UMETA(DisplayName = "Nature"),
    Holy UMETA(DisplayName = "Holy"),
    Dark UMETA(DisplayName = "Dark"),
    Psychic UMETA(DisplayName = "Psychic")
};

USTRUCT(BlueprintType)
struct FManaAffinityModifier
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affinity")
    EManaSchool School;

    // Cost multiplier for this school (0.5 = half cost, 2.0 = double cost)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affinity", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float CostMultiplier;

    FManaAffinityModifier()
    {
        School = EManaSchool::Arcane;
        CostMultiplier = 1.0f;
    }
};

USTRUCT(BlueprintType)
struct FSpellCostInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell")
    float BaseCost;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell")
    EManaSchool School;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell")
    AActor* SpellCaster;

    FSpellCostInfo()
    {
        BaseCost = 0.0f;
        School = EManaSchool::Arcane;
        SpellCaster = nullptr;
    }
};

/**
 * Component that manages character mana/magic points for spellcasting
 * Handles mana consumption, regeneration, and school-based cost modifiers
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RADIANTRPG_API UManaComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UManaComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Base Mana Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mana", meta = (ClampMin = "0.0"))
    float BaseMaxMana;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mana")
    float CurrentMana;

    UPROPERTY(BlueprintReadOnly, Category = "Mana")
    float CurrentMaxMana;

    // Regeneration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regeneration")
    bool bCanRegenerate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regeneration", meta = (ClampMin = "0.0"))
    float ManaRegenRate; // Mana per second

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regeneration", meta = (ClampMin = "0.0"))
    float RegenDelay; // Delay after spending mana before regen starts

    UPROPERTY(BlueprintReadOnly, Category = "Regeneration")
    float TimeSinceLastSpell;

    // School Affinities
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affinities")
    TArray<FManaAffinityModifier> SchoolAffinities;

    // State
    UPROPERTY(BlueprintReadOnly, Category = "Mana")
    bool bHasMana;

    // Cached owner reference
    UPROPERTY()
    ABaseCharacter* OwnerCharacter;

public:
    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnManaChanged OnManaChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnManaSpent OnManaSpent;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnManaEmpty OnManaEmpty;

    // Public functions
    UFUNCTION(BlueprintCallable, Category = "Mana")
    bool TrySpendMana(const FSpellCostInfo& SpellInfo);

    UFUNCTION(BlueprintCallable, Category = "Mana")
    bool TrySpendManaSimple(float ManaCost, AActor* SpellCaster = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Mana")
    void RestoreMana(float ManaAmount);

    UFUNCTION(BlueprintCallable, Category = "Mana")
    void SetMana(float NewMana);

    UFUNCTION(BlueprintCallable, Category = "Mana")
    void SetMaxMana(float NewMaxMana, bool bRestoreToFull = false);

    UFUNCTION(BlueprintCallable, Category = "Mana")
    void AddMaxMana(float ManaToAdd, bool bRestoreToFull = false);

    UFUNCTION(BlueprintCallable, Category = "Mana")
    void DrainAllMana();

    // Getters
    UFUNCTION(BlueprintPure, Category = "Mana")
    float GetMana() const { return CurrentMana; }

    UFUNCTION(BlueprintPure, Category = "Mana")
    float GetMaxMana() const { return CurrentMaxMana; }

    UFUNCTION(BlueprintPure, Category = "Mana")
    float GetManaPercent() const;

    UFUNCTION(BlueprintPure, Category = "Mana")
    bool HasMana() const { return bHasMana && CurrentMana > 0.0f; }

    UFUNCTION(BlueprintPure, Category = "Mana")
    bool IsFullMana() const;

    UFUNCTION(BlueprintPure, Category = "Mana")
    bool CanCastSpell(const FSpellCostInfo& SpellInfo) const;

    UFUNCTION(BlueprintPure, Category = "Mana")
    bool CanCastSpellSimple(float ManaCost) const;

    UFUNCTION(BlueprintPure, Category = "Mana")
    float CalculateSpellCost(const FSpellCostInfo& SpellInfo) const;

    // Affinity functions
    UFUNCTION(BlueprintCallable, Category = "Affinities")
    void SetSchoolAffinity(EManaSchool School, float CostMultiplier);

    UFUNCTION(BlueprintCallable, Category = "Affinities")
    void RemoveSchoolAffinity(EManaSchool School);

    UFUNCTION(BlueprintPure, Category = "Affinities")
    float GetSchoolCostMultiplier(EManaSchool School) const;

protected:
    // Internal functions
    void UpdateRegeneration(float DeltaTime);
    void BroadcastManaChanged();
    void CheckManaState();

    // Blueprint events
    UFUNCTION(BlueprintImplementableEvent, Category = "Mana")
    void OnManaChangedBP(float NewMana, float NewMaxMana);

    UFUNCTION(BlueprintImplementableEvent, Category = "Mana")
    void OnManaSpentBP(float ManaCost, AActor* SpellCaster);

    UFUNCTION(BlueprintImplementableEvent, Category = "Mana")
    void OnManaEmptyBP();

    UFUNCTION(BlueprintImplementableEvent, Category = "Mana")
    void OnManaRestoredBP(float ManaAmount);
};