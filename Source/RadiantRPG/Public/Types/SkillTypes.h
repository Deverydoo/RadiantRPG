// Source/RadiantRPG/Public/Types/SkillTypes.h

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CoreTypes.h"
#include "SkillTypes.generated.h"

/**
 * Skill categories for organization
 */
UENUM(BlueprintType)
enum class ESkillCategory : uint8
{
    Combat          UMETA(DisplayName = "Combat"),
    Magic           UMETA(DisplayName = "Magic"),
    Crafting        UMETA(DisplayName = "Crafting"),
    Foraging        UMETA(DisplayName = "Foraging"),
    Social          UMETA(DisplayName = "Social"),
    Movement        UMETA(DisplayName = "Movement"),
    Survival        UMETA(DisplayName = "Survival"),
    Support         UMETA(DisplayName = "Support"),
    Knowledge       UMETA(DisplayName = "Knowledge"),
    
    MAX             UMETA(Hidden)
};

/**
 * Combat skill types
 */
UENUM(BlueprintType)
enum class ECombatSkill : uint8
{
    Swordsmanship   UMETA(DisplayName = "Swordsmanship"),
    AxeFighting     UMETA(DisplayName = "Axe Fighting"),
    MaceFighting    UMETA(DisplayName = "Mace Fighting"),
    Archery         UMETA(DisplayName = "Archery"),
    Marksmanship    UMETA(DisplayName = "Marksmanship"),
    UnarmedCombat   UMETA(DisplayName = "Unarmed Combat"),
    Blocking        UMETA(DisplayName = "Blocking"),
    Parrying        UMETA(DisplayName = "Parrying"),
    DualWielding    UMETA(DisplayName = "Dual Wielding"),
    CriticalStrikes UMETA(DisplayName = "Critical Strikes"),
    
    MAX             UMETA(Hidden)
};

/**
 * Magic skill types
 */
UENUM(BlueprintType)
enum class EMagicSkill : uint8
{
    ElementalMagic  UMETA(DisplayName = "Elemental Magic"),
    DivineMagic     UMETA(DisplayName = "Divine Magic"),
    Necromancy      UMETA(DisplayName = "Necromancy"),
    Illusion        UMETA(DisplayName = "Illusion"),
    Enchanting      UMETA(DisplayName = "Enchanting"),
    Summoning       UMETA(DisplayName = "Summoning"),
    Telekinesis     UMETA(DisplayName = "Telekinesis"),
    ManaControl     UMETA(DisplayName = "Mana Control"),
    SpellPower      UMETA(DisplayName = "Spell Power"),
    
    MAX             UMETA(Hidden)
};

/**
 * Crafting skill types
 */
UENUM(BlueprintType)
enum class ECraftingSkill : uint8
{
    Blacksmithing   UMETA(DisplayName = "Blacksmithing"),
    Tailoring       UMETA(DisplayName = "Tailoring"),
    Leatherworking  UMETA(DisplayName = "Leatherworking"),
    Alchemy         UMETA(DisplayName = "Alchemy"),
    Cooking         UMETA(DisplayName = "Cooking"),
    Jewelcrafting   UMETA(DisplayName = "Jewelcrafting"),
    Engineering     UMETA(DisplayName = "Engineering"),
    Woodworking     UMETA(DisplayName = "Woodworking"),
    
    MAX             UMETA(Hidden)
};

/**
 * Foraging skill types
 */
UENUM(BlueprintType)
enum class EForagingSkill : uint8
{
    Mining          UMETA(DisplayName = "Mining"),
    Herbalism       UMETA(DisplayName = "Herbalism"),
    Lumberjacking   UMETA(DisplayName = "Lumberjacking"),
    Scavenging      UMETA(DisplayName = "Scavenging"),
    Fishing         UMETA(DisplayName = "Fishing"),
    Hunting         UMETA(DisplayName = "Hunting"),
    Skinning        UMETA(DisplayName = "Skinning"),
    
    MAX             UMETA(Hidden)
};

/**
 * Support skill types
 */
UENUM(BlueprintType)
enum class ESupportSkill : uint8
{
    StaminaControl  UMETA(DisplayName = "Stamina Control"),
    Focus           UMETA(DisplayName = "Focus"),
    Meditation      UMETA(DisplayName = "Meditation"),
    Learning        UMETA(DisplayName = "Learning"),
    Teaching        UMETA(DisplayName = "Teaching"),
    Leadership      UMETA(DisplayName = "Leadership"),
    Tactics         UMETA(DisplayName = "Tactics"),
    
    MAX             UMETA(Hidden)
};

/**
 * Skill advancement methods
 */
UENUM(BlueprintType)
enum class ESkillAdvancementType : uint8
{
    UseBase         UMETA(DisplayName = "Use-Based"),
    Experience      UMETA(DisplayName = "Experience Points"),
    Training        UMETA(DisplayName = "Training"),
    Study           UMETA(DisplayName = "Study"),
    Practice        UMETA(DisplayName = "Practice"),
    
    MAX             UMETA(Hidden)
};

/**
 * Skill tier/mastery levels
 */
UENUM(BlueprintType)
enum class ESkillTier : uint8
{
    Novice          UMETA(DisplayName = "Novice"),         // 0-20
    Apprentice      UMETA(DisplayName = "Apprentice"),     // 20-40
    Journeyman      UMETA(DisplayName = "Journeyman"),     // 40-60
    Expert          UMETA(DisplayName = "Expert"),         // 60-80
    Master          UMETA(DisplayName = "Master"),         // 80-95
    Grandmaster     UMETA(DisplayName = "Grandmaster"),    // 95-100
    
    MAX             UMETA(Hidden)
};

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

/**
 * Single skill data
 */
USTRUCT(BlueprintType)
struct FSkillData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName SkillID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ESkillCategory Category = ESkillCategory::Combat;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag SkillTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CurrentLevel = 0.0f; // 0-100

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxLevel = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Experience = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ExperienceToNext = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ESkillTier CurrentTier = ESkillTier::Novice;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsLocked = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCanDecay = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DecayRate = 0.01f; // Per day of no use

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float LastUsedTime = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TimesUsed = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer RequiredTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FStatModifier> SkillBonuses;
    float CurrentExperience;
    float CurrentValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	ESkillType SkillType;

	/** Legacy modifier field */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float Modifier;
	float MaxValue;
	float TotalExperience;
	float TemporaryModifier;

    FSkillData()
    {
    }

    ESkillTier CalculateTier() const
    {
        if (CurrentLevel < 20.0f) return ESkillTier::Novice;
        if (CurrentLevel < 40.0f) return ESkillTier::Apprentice;
        if (CurrentLevel < 60.0f) return ESkillTier::Journeyman;
        if (CurrentLevel < 80.0f) return ESkillTier::Expert;
        if (CurrentLevel < 95.0f) return ESkillTier::Master;
        return ESkillTier::Grandmaster;
    }
};

/**
 * Skill progression configuration
 */
USTRUCT(BlueprintType)
struct FSkillProgression
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName SkillID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ESkillAdvancementType AdvancementType = ESkillAdvancementType::UseBase;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BaseGainRate = 0.1f; // Per use

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DifficultyMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UCurveFloat* ProgressionCurve = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MinGainDelay = 1.0f; // Minimum seconds between gains

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRequiresSuccess = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FName> PrerequisiteSkills;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PrerequisiteLevel = 0.0f;

    FSkillProgression()
    {
    }
};

/**
 * Skill cap configuration for specialization
 */
USTRUCT(BlueprintType)
struct FSkillCapConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TotalSkillCap = 700.0f; // Total points across all skills

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float IndividualSkillCap = 100.0f; // Max for any single skill

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxGrandmasterSkills = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxMasterSkills = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAllowRespecialization = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RespecializationCost = 1000.0f;

    FSkillCapConfig()
    {
    }
};

/**
 * Skill use context for gaining experience
 */
USTRUCT(BlueprintType)
struct FSkillUseContext
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName SkillID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bWasSuccessful = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DifficultyRating = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TWeakObjectPtr<AActor> Target;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TWeakObjectPtr<AActor> Tool;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer ContextTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Timestamp = 0.0f;

    FSkillUseContext()
    {
    }
};

/**
 * Skill check parameters
 */
USTRUCT(BlueprintType)
struct FSkillCheck
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName SkillID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DifficultyClass = 50.0f; // 0-100

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BonusModifier = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCanCriticalSuccess = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCanCriticalFail = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer RequiredTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer BonusTags;

    FSkillCheck()
    {
    }
};

/**
 * Result of a skill check
 */
USTRUCT(BlueprintType)
struct FSkillCheckResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    bool bSuccess = false;

    UPROPERTY(BlueprintReadWrite)
    bool bCriticalSuccess = false;

    UPROPERTY(BlueprintReadWrite)
    bool bCriticalFailure = false;

    UPROPERTY(BlueprintReadWrite)
    float RollValue = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    float SkillValue = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    float TotalValue = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    float Margin = 0.0f; // How much passed/failed by

    FSkillCheckResult()
    {
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

/**
 * Skill synergy bonus
 */
USTRUCT(BlueprintType)
struct FSkillSynergy
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName PrimarySkill;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName SecondarySkill;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SynergyBonus = 0.1f; // 10% bonus

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RequiredSecondaryLevel = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Description;

    FSkillSynergy()
    {
    }
};

/**
 * Character's complete skill set
 */
USTRUCT(BlueprintType)
struct FCharacterSkillSet
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FName, FSkillData> Skills;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TotalSkillPoints = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FSkillCapConfig CapConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FSkillSynergy> ActiveSynergies;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float LearningRate = 1.0f; // Global modifier

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer SkillTags;

    FCharacterSkillSet()
    {
    }

    bool CanImproveSkill(const FName& SkillID) const
    {
        if (!Skills.Contains(SkillID)) return false;
        
        const FSkillData& Skill = Skills[SkillID];
        if (Skill.bIsLocked) return false;
        if (Skill.CurrentLevel >= Skill.MaxLevel) return false;
        if (TotalSkillPoints >= CapConfig.TotalSkillCap) return false;
        
        return true;
    }
};