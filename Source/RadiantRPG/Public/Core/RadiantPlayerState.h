// Public/Core/RadiantPlayerState.h
// Player state implementation for RadiantRPG - manages skill-based character progression and player data

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "Types/RadiantTypes.h"
#include "RadiantPlayerState.generated.h"

// Forward declarations
class UAbilitySystemComponent;
class UAttributeSet;
class UGameplayAbility;
class ARadiantGameState;

// Replication-safe skill entry
USTRUCT(BlueprintType)
struct RADIANTRPG_API FSkillEntry
{
    GENERATED_BODY()

    /** The skill tag identifier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FGameplayTag SkillTag;

    /** The skill data */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FSkillData SkillData;

    FSkillEntry()
    {
        SkillTag = FGameplayTag();
        SkillData = FSkillData();
    }

    FSkillEntry(const FGameplayTag& InSkillTag, const FSkillData& InSkillData)
        : SkillTag(InSkillTag), SkillData(InSkillData)
    {
    }
};

// Replication-safe faction reputation entry
USTRUCT(BlueprintType)
struct RADIANTRPG_API FFactionReputationEntry
{
    GENERATED_BODY()

    /** The faction tag identifier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
    FGameplayTag FactionTag;

    /** The reputation value */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
    float ReputationValue;

    FFactionReputationEntry()
    {
        FactionTag = FGameplayTag();
        ReputationValue = 0.0f;
    }

    FFactionReputationEntry(const FGameplayTag& InFactionTag, float InReputationValue)
        : FactionTag(InFactionTag), ReputationValue(InReputationValue)
    {
    }
};

/**
 * RadiantPlayerState - Manages skill-based character progression and player data
 * 
 * Core responsibilities:
 * - Skill-based character progression (Ultima Online inspired)
 * - Faction reputation tracking
 * - Player statistics and achievements
 * - Ability system integration
 * - Persistence data management
 * 
 * Uses replication-safe TArray structures instead of TMaps for UE 5.5 compatibility.
 */
UCLASS(BlueprintType, Blueprintable)
class RADIANTRPG_API ARadiantPlayerState : public APlayerState, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    ARadiantPlayerState();

    // === PLAYERSTATE BASE OVERRIDES ===
    
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PostInitializeComponents() override;

    // === ABILITY SYSTEM INTERFACE ===
    
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    virtual UAttributeSet* GetAttributeSet() const { return AttributeSet; }

protected:
    // === CORE COMPONENTS ===
    
    /** Ability system component for gameplay abilities */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    /** Attribute set for character stats */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System")
    TObjectPtr<UAttributeSet> AttributeSet;

    // === REPLICATED DATA (Using TArrays for UE 5.5 compatibility) ===
    
    /** Current skill values and data - replicated as array */
    UPROPERTY(ReplicatedUsing = OnRep_SkillEntries, BlueprintReadOnly, Category = "Skills")
    TArray<FSkillEntry> SkillEntries;

    /** Faction reputation values - replicated as array */
    UPROPERTY(ReplicatedUsing = OnRep_FactionReputationEntries, BlueprintReadOnly, Category = "Factions")
    TArray<FFactionReputationEntry> FactionReputationEntries;

    /** Player gameplay statistics */
    UPROPERTY(ReplicatedUsing = OnRep_Statistics, BlueprintReadOnly, Category = "Statistics")
    FPlayerStatistics Statistics;

    /** Player's unique character ID for save/load */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Identity")
    FString CharacterID;

    /** Player's character name */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Identity")
    FString CharacterName;

    // === LOCAL CACHES (Non-replicated for fast access) ===
    
    /** Local cache of skills as TMap for fast lookup */
    UPROPERTY(BlueprintReadOnly, Category = "Skills")
    TMap<FGameplayTag, FSkillData> SkillsCache;

    /** Local cache of faction reputations as TMap for fast lookup */
    UPROPERTY(BlueprintReadOnly, Category = "Factions")
    TMap<FGameplayTag, float> FactionReputationsCache;

    // === CONFIGURATION ===
    
    /** Total skill cap (sum of all skills cannot exceed this) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    float TotalSkillCap;

    /** Individual skill cap (max value for any single skill) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    float IndividualSkillCap;

    /** Experience multiplier for skill gains */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    float ExperienceMultiplier;

    /** Primary skills that contribute more to character level */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    TArray<FGameplayTag> PrimarySkills;

    /** Default abilities granted to all players */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    TArray<TSubclassOf<class UGameplayAbility>> DefaultAbilities;

    // === CACHED REFERENCES ===
    
    /** Cached reference to game state */
    UPROPERTY(BlueprintReadOnly, Category = "References")
    TObjectPtr<ARadiantGameState> RadiantGameState;

    // === INTERNAL STATE ===
    
    /** Last calculated effective level */
    int32 CachedEffectiveLevel;

    /** Whether player data has been initialized */
    bool bPlayerDataInitialized;

    /** Session start time for playtime tracking */
    double SessionStartTime;

    /** Last position for distance tracking */
    FVector LastPosition;

public:
    // === EVENTS ===
    
    UPROPERTY(BlueprintAssignable, Category = "Player Events")
    FOnSkillChanged OnSkillChanged;

    UPROPERTY(BlueprintAssignable, Category = "Player Events")
    FOnPlayerLevelChanged OnPlayerLevelChanged;

    UPROPERTY(BlueprintAssignable, Category = "Player Events")
    FOnStatisticUpdated OnStatisticUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Player Events")
    FOnLocationDiscovered OnLocationDiscovered;

    // === SKILL SYSTEM ===
    
    /** Get current value of a skill */
    UFUNCTION(BlueprintPure, Category = "Skills")
    float GetSkillValue(FGameplayTag SkillTag) const;

    /** Get skill data structure */
    UFUNCTION(BlueprintPure, Category = "Skills")
    FSkillData GetSkillData(FGameplayTag SkillTag) const;

    /** Add experience to a skill */
    UFUNCTION(BlueprintCallable, Category = "Skills")
    void AddSkillExperience(FGameplayTag SkillTag, float ExperienceAmount);

    /** Set skill value directly (admin/cheat function) */
    UFUNCTION(BlueprintCallable, Category = "Skills")
    void SetSkillValue(FGameplayTag SkillTag, float NewValue);

    /** Get total skill points used */
    UFUNCTION(BlueprintPure, Category = "Skills")
    float GetTotalSkillPoints() const;

    /** Get effective character level based on skills */
    UFUNCTION(BlueprintPure, Category = "Skills")
    int32 GetEffectiveLevel() const;

    /** Get all skills */
    UFUNCTION(BlueprintPure, Category = "Skills")
    TMap<FGameplayTag, FSkillData> GetAllSkills() const { return SkillsCache; }

    /** Get skill progress as percentage */
    UFUNCTION(BlueprintPure, Category = "Skills")
    float GetSkillProgress(FGameplayTag SkillTag) const;
    
    bool CanLearnSkill(FGameplayTag SkillTag) const;

    // === FACTION SYSTEM ===
    
    /** Get faction reputation */
    UFUNCTION(BlueprintPure, Category = "Factions")
    float GetFactionReputation(FGameplayTag FactionTag) const;

    /** Set faction reputation */
    UFUNCTION(BlueprintCallable, Category = "Factions")
    void SetFactionReputation(FGameplayTag FactionTag, float NewReputation);

    /** Add to faction reputation */
    UFUNCTION(BlueprintCallable, Category = "Factions")
    void AddFactionReputation(FGameplayTag FactionTag, float ReputationChange);

    /** Get all faction reputations */
    UFUNCTION(BlueprintPure, Category = "Factions")
    TMap<FGameplayTag, float> GetAllFactionReputations() const { return FactionReputationsCache; }

protected:
    // === INTERNAL FUNCTIONS ===
    
    /** Initialize player data and components */
    void InitializePlayerData();

    /** Calculate effective character level from skills */
    int32 CalculateEffectiveLevel() const;

    /** Update cached values when skills change */
    void UpdateCachedValues();

    /** Apply skill experience with proper calculations */
    void ApplySkillExperience(FGameplayTag SkillTag, float ExperienceAmount);

    /** Grant default abilities to player */
    void GrantDefaultAbilities();

    /** Update playtime statistics */
    void UpdatePlaytimeStatistics();

    /** Calculate experience needed for next skill level */
    float CalculateExperienceForNextLevel(FGameplayTag SkillTag) const;

    /** Calculate skill level from experience */
    float CalculateSkillLevelFromExperience(float Experience) const;

    /** Rebuild cache from replicated arrays */
    void RebuildSkillsCache();
    void RebuildFactionReputationsCache();

    /** Update replicated arrays from cache */
    void UpdateReplicatedSkillEntries();
    void UpdateReplicatedFactionEntries();

    // === REPLICATION CALLBACKS ===
    
    UFUNCTION()
    void OnRep_SkillEntries();

    UFUNCTION()
    void OnRep_FactionReputationEntries();

    UFUNCTION()
    void OnRep_Statistics(const FPlayerStatistics& OldStatistics);

    UFUNCTION()
    bool MeetsSkillRequirements(const TMap<FGameplayTag, float>& Requirements) const;

    // === BLUEPRINT EVENTS ===
    
    UFUNCTION(BlueprintImplementableEvent, Category = "Player Events")
    void OnSkillChangedBP(FGameplayTag SkillTag, float OldValue, float NewValue);

    UFUNCTION(BlueprintImplementableEvent, Category = "Player Events")
    void OnPlayerLevelChangedBP(int32 NewLevel);

    UFUNCTION(BlueprintImplementableEvent, Category = "Player Events")
    void OnStatisticUpdatedBP(const FString& StatisticName, float NewValue);

    UFUNCTION(BlueprintImplementableEvent, Category = "Player Events")
    void OnLocationDiscoveredBP(FGameplayTag LocationTag);

    UFUNCTION(BlueprintImplementableEvent, Category = "Player Events")
    void OnFactionReputationChangedBP(FGameplayTag FactionTag, float OldReputation, float NewReputation);
};