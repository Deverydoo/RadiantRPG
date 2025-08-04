// Public/Components/SkillsComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "SkillsComponent.generated.h"

class ABaseCharacter;

UENUM(BlueprintType)
enum class ESkillType : uint8
{
    // Combat Skills
    OneHanded UMETA(DisplayName = "One-Handed"),
    TwoHanded UMETA(DisplayName = "Two-Handed"),
    Archery UMETA(DisplayName = "Archery"),
    Block UMETA(DisplayName = "Block"),
    HeavyArmor UMETA(DisplayName = "Heavy Armor"),
    LightArmor UMETA(DisplayName = "Light Armor"),
    
    // Magic Skills
    Destruction UMETA(DisplayName = "Destruction"),
    Restoration UMETA(DisplayName = "Restoration"),
    Illusion UMETA(DisplayName = "Illusion"),
    Conjuration UMETA(DisplayName = "Conjuration"),
    Enchanting UMETA(DisplayName = "Enchanting"),
    Alchemy UMETA(DisplayName = "Alchemy"),
    
    // Stealth Skills
    Sneak UMETA(DisplayName = "Sneak"),
    Lockpicking UMETA(DisplayName = "Lockpicking"),
    Pickpocket UMETA(DisplayName = "Pickpocket"),
    
    // Crafting Skills
    Smithing UMETA(DisplayName = "Smithing"),
    Tailoring UMETA(DisplayName = "Tailoring"),
    Cooking UMETA(DisplayName = "Cooking"),
    
    // Utility Skills
    Athletics UMETA(DisplayName = "Athletics"),
    Acrobatics UMETA(DisplayName = "Acrobatics"),
    Speechcraft UMETA(DisplayName = "Speechcraft"),
    Mercantile UMETA(DisplayName = "Mercantile"),
    
    // Knowledge Skills
    Lore UMETA(DisplayName = "Lore"),
    Survival UMETA(DisplayName = "Survival")
};

USTRUCT(BlueprintType)
struct FSkillData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    ESkillType SkillType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill", meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float CurrentValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill", meta = (ClampMin = "0.0"))
    float Experience;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    bool bIsLocked;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    float Modifier; // Temporary bonuses/penalties

    FSkillData()
    {
        SkillType = ESkillType::OneHanded;
        CurrentValue = 5.0f; // Starting skill level
        Experience = 0.0f;
        bIsLocked = false;
        Modifier = 0.0f;
    }
};

USTRUCT(BlueprintType, meta = (BlueprintType = true))
struct FSkillTableRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FText SkillName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    float ExperienceMultiplier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    float StartingValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    float MaxValue;

    FSkillTableRow()
    {
        SkillName = FText::FromString("Unknown Skill");
        Description = FText::GetEmpty();
        ExperienceMultiplier = 1.0f;
        StartingValue = 5.0f;
        MaxValue = 100.0f;
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSkillChanged, ESkillType, SkillType, float, NewValue, float, Experience);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillLevelUp, ESkillType, SkillType, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillCapReached, float, TotalSkillPoints);

/**
 * Component that manages character skills in an Ultima Online inspired system
 * Skills improve through use and have a total skill point cap to encourage specialization
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RADIANTRPG_API USkillsComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    USkillsComponent();

protected:
    virtual void BeginPlay() override;

    void InitializeDefaultSkills();

    // Skill System Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
    float TotalSkillCap; // Total skill points allowed (like UO's 700 points)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
    float IndividualSkillCap; // Max for any single skill (usually 100)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
    float ExperienceGainMultiplier; // Global experience multiplier

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
    bool bAllowSkillLoss; // Whether skills can decrease when capped

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
    float SkillLossRate; // How much skills decay when capped

    // Current Skills
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
    TMap<ESkillType, FSkillData> Skills;

    // Data Table for skill configurations
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
    UDataTable* SkillDataTable;

    // Cached owner reference
    UPROPERTY()
    ABaseCharacter* OwnerCharacter;

    // Current total skill points
    UPROPERTY(BlueprintReadOnly, Category = "Skills")
    float CurrentTotalSkillPoints;

public:
    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnSkillChanged OnSkillChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnSkillLevelUp OnSkillLevelUp;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnSkillCapReached OnSkillCapReached;

    // Public functions
    UFUNCTION(BlueprintCallable, Category = "Skills")
    void GainSkillExperience(ESkillType SkillType, float ExperienceAmount);

    UFUNCTION(BlueprintCallable, Category = "Skills")
    void SetSkillValue(ESkillType SkillType, float NewValue);

    UFUNCTION(BlueprintCallable, Category = "Skills")
    void AddSkillModifier(ESkillType SkillType, float ModifierAmount);

    UFUNCTION(BlueprintCallable, Category = "Skills")
    void RemoveSkillModifier(ESkillType SkillType, float ModifierAmount);

    UFUNCTION(BlueprintCallable, Category = "Skills")
    void LockSkill(ESkillType SkillType, bool bLocked);

    // Getters
    UFUNCTION(BlueprintPure, Category = "Skills")
    float GetSkillValue(ESkillType SkillType) const;

    UFUNCTION(BlueprintPure, Category = "Skills")
    float GetEffectiveSkillValue(ESkillType SkillType) const; // Includes modifiers

    UFUNCTION(BlueprintPure, Category = "Skills")
    float GetSkillExperience(ESkillType SkillType) const;

    UFUNCTION(BlueprintPure, Category = "Skills")
    bool IsSkillLocked(ESkillType SkillType) const;

    UFUNCTION(BlueprintPure, Category = "Skills")
    float GetTotalSkillPoints() const { return CurrentTotalSkillPoints; }

    UFUNCTION(BlueprintPure, Category = "Skills")
    float GetRemainingSkillPoints() const;

    UFUNCTION(BlueprintPure, Category = "Skills")
    bool IsAtSkillCap() const;

    UFUNCTION(BlueprintPure, Category = "Skills")
    TArray<ESkillType> GetHighestSkills(int32 Count = 5) const;

    // Skill management
    UFUNCTION(BlueprintCallable, Category = "Skills")
    void InitializeSkillsFromDataTable();

    UFUNCTION(BlueprintCallable, Category = "Skills")
    void ResetAllSkills();

    UFUNCTION(BlueprintCallable, Category = "Skills")
    void ResetSkill(ESkillType SkillType);

protected:
    // Internal functions
    void UpdateTotalSkillPoints();
    void HandleSkillCapExceeded();
    void TryLevelUpSkill(ESkillType SkillType);
    float CalculateExperienceForLevel(float Level) const;
    float CalculateLevelFromExperience(float Experience) const;
    void BroadcastSkillChanged(ESkillType SkillType);

    // Blueprint events
    UFUNCTION(BlueprintImplementableEvent, Category = "Skills")
    void OnSkillChangedBP(ESkillType SkillType, float NewValue, float Experience);

    UFUNCTION(BlueprintImplementableEvent, Category = "Skills")
    void OnSkillLevelUpBP(ESkillType SkillType, float NewValue);

    UFUNCTION(BlueprintImplementableEvent, Category = "Skills")
    void OnSkillCapReachedBP(float TotalSkillPoints);
};