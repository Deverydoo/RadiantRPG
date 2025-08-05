// Public/Components/SkillsComponent.h
// Skills component for character skill management in RadiantRPG

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Types/RadiantTypes.h"
#include "SkillsComponent.generated.h"

class ABaseCharacter;

/** Legacy skill type enum for backward compatibility */
UENUM(BlueprintType)
enum class ESkillType : uint8
{
    // Combat Skills
    OneHanded UMETA(DisplayName = "One-Handed"),
    TwoHanded UMETA(DisplayName = "Two-Handed"),
    Archery UMETA(DisplayName = "Archery"),
    Marksman UMETA(DisplayName = "Marksman"),
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
    Speech UMETA(DisplayName = "Speech"),
    Alteration UMETA(DisplayName = "Alteration"),
    
    // Knowledge Skills
    Lore UMETA(DisplayName = "Lore"),
    Survival UMETA(DisplayName = "Survival")
};

/** Legacy skill data structure extending the base FSkillData */
USTRUCT(BlueprintType)
struct FLegacySkillData : public FSkillData
{
    GENERATED_BODY()

    /** Legacy skill type for backward compatibility */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    ESkillType SkillType;

    /** Legacy experience field */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill", meta = (ClampMin = "0.0"))
    float Experience;

    /** Legacy modifier field */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    float Modifier;

    FLegacySkillData() : FSkillData()
    {
        SkillType = ESkillType::OneHanded;
        Experience = 0.0f;
        Modifier = 0.0f;
    }
};

/** Skill configuration data table row */
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    ESkillCategory Category;
    float MaxSkillCap;
    bool bStartsLocked;
    ESkillCategory SkillCategory;
    ESkillType SkillType;

    FSkillTableRow()
    {
        SkillName = FText::FromString("Unknown Skill");
        Description = FText::GetEmpty();
        ExperienceMultiplier = 1.0f;
        StartingValue = 5.0f;
        MaxValue = 100.0f;
        Category = ESkillCategory::Combat;
    }
};

// Legacy skill events for backward compatibility
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLegacySkillChanged, ESkillType, SkillType, float, NewValue, float, Experience);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLegacySkillLevelUp, ESkillType, SkillType, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillCapReached, float, TotalSkillPoints);

/**
 * Component that manages character skills in an Ultima Online inspired system
 * Skills improve through use and have a total skill point cap to encourage specialization
 * 
 * NOTE: This component provides legacy support for the old ESkillType system.
 * New implementations should use the FGameplayTag-based skill system in RadiantPlayerState.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RADIANTRPG_API USkillsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USkillsComponent();

    // === COMPONENT OVERRIDES ===
    
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    void InitializeDefaultSkills();

protected:
    // === SKILL DATA ===
    
    /** Legacy skill data map */
    UPROPERTY(BlueprintReadOnly, Category = "Skills")
    TMap<ESkillType, FLegacySkillData> Skills;

    // === CONFIGURATION ===
    
    /** Skill configuration data table */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    TObjectPtr<UDataTable> SkillDataTable;

    /** Total skill cap (sum of all skills) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    float TotalSkillCap;

    /** Experience gain multiplier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    float ExperienceGainMultiplier;

    /** Whether to enforce skill caps */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    bool bEnforceSkillCaps;

    // === CACHED VALUES ===
    
    /** Current total skill points used */
    UPROPERTY(BlueprintReadOnly, Category = "Skills")
    float CurrentTotalSkillPoints;

    /** Cached reference to owner character */
    UPROPERTY()
    TObjectPtr<ABaseCharacter> OwnerCharacter;

public:
    // === EVENTS ===
    
    UPROPERTY(BlueprintAssignable, Category = "Skill Events")
    FOnLegacySkillChanged OnSkillChanged;

    UPROPERTY(BlueprintAssignable, Category = "Skill Events")
    FOnLegacySkillLevelUp OnSkillLevelUp;

    UPROPERTY(BlueprintAssignable, Category = "Skill Events")
    FOnSkillCapReached OnSkillCapReached;

    // === SKILL INTERFACE ===
    
    /** Gain experience in a skill */
    UFUNCTION(BlueprintCallable, Category = "Skills")
    void GainSkillExperience(ESkillType SkillType, float ExperienceAmount);
    void CalculateLevelFromExperience(FLegacySkillData& SkillData);

    /** Set skill value directly */
    UFUNCTION(BlueprintCallable, Category = "Skills")
    void SetSkillValue(ESkillType SkillType, float NewValue);

    /** Add temporary modifier to skill */
    UFUNCTION(BlueprintCallable, Category = "Skills")
    void AddSkillModifier(ESkillType SkillType, float ModifierAmount);

    /** Remove temporary modifier from skill */
    UFUNCTION(BlueprintCallable, Category = "Skills")
    void RemoveSkillModifier(ESkillType SkillType, float ModifierAmount);

    /** Lock or unlock a skill */
    UFUNCTION(BlueprintCallable, Category = "Skills")
    void LockSkill(ESkillType SkillType, bool bLocked);

    // === GETTERS ===
    
    /** Get base skill value */
    UFUNCTION(BlueprintPure, Category = "Skills")
    float GetSkillValue(ESkillType SkillType) const;

    /** Get effective skill value including modifiers */
    UFUNCTION(BlueprintPure, Category = "Skills")
    float GetEffectiveSkillValue(ESkillType SkillType) const;

    /** Get skill experience */
    UFUNCTION(BlueprintPure, Category = "Skills")
    float GetSkillExperience(ESkillType SkillType) const;
    float GetSkillProgress(ESkillType SkillType) const;

    /** Check if skill is locked */
    UFUNCTION(BlueprintPure, Category = "Skills")
    bool IsSkillLocked(ESkillType SkillType) const;

    /** Get total skill points used */
    UFUNCTION(BlueprintPure, Category = "Skills")
    float GetTotalSkillPoints() const { return CurrentTotalSkillPoints; }

    /** Get remaining skill points */
    UFUNCTION(BlueprintPure, Category = "Skills")
    float GetRemainingSkillPoints() const;

    /** Check if at skill cap */
    UFUNCTION(BlueprintPure, Category = "Skills")
    bool IsAtSkillCap() const;

    /** Get highest skills */
    UFUNCTION(BlueprintPure, Category = "Skills")
    TArray<ESkillType> GetHighestSkills(int32 Count = 5) const;

    /** Get skill data structure */
    UFUNCTION(BlueprintPure, Category = "Skills")
    FLegacySkillData GetSkillData(ESkillType SkillType) const;

    // === SKILL MANAGEMENT ===
    
    /** Initialize skills from data table */
    UFUNCTION(BlueprintCallable, Category = "Skills")
    void InitializeSkillsFromDataTable();
    ESkillCategory GetSkillCategory(ESkillType SkillType) const;

    /** Reset all skills to starting values */
    UFUNCTION(BlueprintCallable, Category = "Skills")
    void ResetAllSkills();

    /** Reset specific skill */
    UFUNCTION(BlueprintCallable, Category = "Skills")
    void ResetSkill(ESkillType SkillType);

    // === CONVERSION UTILITIES ===
    
    /** Convert legacy skill type to gameplay tag */
    UFUNCTION(BlueprintPure, Category = "Skills")
    static FGameplayTag SkillTypeToGameplayTag(ESkillType SkillType);

    /** Convert gameplay tag to legacy skill type */
    UFUNCTION(BlueprintPure, Category = "Skills")
    static ESkillType GameplayTagToSkillType(FGameplayTag SkillTag);

protected:
    // === INTERNAL FUNCTIONS ===
    
    /** Update total skill points */
    void UpdateTotalSkillPoints();

    /** Handle skill cap exceeded */
    void HandleSkillCapExceeded();

    /** Try to level up skill */
    void TryLevelUpSkill(ESkillType SkillType);

    /** Calculate experience needed for level */
    float CalculateExperienceForLevel(float Level) const;
    void HandleSkillCap();

    /** Calculate level from experience */
    float CalculateLevelFromExperience(float Experience) const;

    /** Broadcast skill changed event */
    void BroadcastSkillChanged(ESkillType SkillType);

    // === REPLICATION ===
    
    UFUNCTION()
    void OnRep_Skills();

    // === BLUEPRINT EVENTS ===
    
    UFUNCTION(BlueprintImplementableEvent, Category = "Skills")
    void OnSkillChangedBP(ESkillType SkillType, float NewValue, float Experience);

    UFUNCTION(BlueprintImplementableEvent, Category = "Skills")
    void OnSkillLevelUpBP(ESkillType SkillType, float NewValue);

    UFUNCTION(BlueprintImplementableEvent, Category = "Skills")
    void OnSkillCapReachedBP(float TotalSkillPoints);
};