// Public/AI/Core/ARPG_AIPersonalityComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTags.h"
#include "Engine/DataTable.h"
#include "Types/ARPG_AIEventTypes.h"
#include "Types/ARPG_AITypes.h"
#include "Types/ARPG_AIDataTableTypes.h"
#include "ARPG_AIPersonalityComponent.generated.h"

class UARPG_AIBrainComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPersonalityTraitChanged, EARPG_PersonalityTrait, TraitType, float, NewStrength);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPersonalityChanged, const FString&, PersonalityDescription);

/**
 * Configuration for the personality system
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_PersonalityConfiguration
{
    GENERATED_BODY()

    /** Whether personality traits can change over time */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    bool bAllowPersonalityChange = true;

    /** Whether personality affects AI behavior */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    bool bEnablePersonalityInfluence = true;

    /** Rate at which personality can change */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float PersonalityChangeRate = 1.0f;

    /** How much experiences affect personality change */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float ExperienceImpactMultiplier = 1.0f;

    /** Whether to enable debug logging */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    bool bEnableDebugLogging = false;

    FARPG_PersonalityConfiguration()
    {
        bAllowPersonalityChange = true;
        bEnablePersonalityInfluence = true;
        PersonalityChangeRate = 1.0f;
        ExperienceImpactMultiplier = 1.0f;
        bEnableDebugLogging = false;
    }
};

/**
 * AI Personality Component
 * Manages personality traits that influence AI behavior
 * Traits can evolve based on experiences
 * Supports data table overrides for configuration
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class RADIANTRPG_API UARPG_AIPersonalityComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UARPG_AIPersonalityComponent();

    UFUNCTION(BlueprintCallable, Category = "AI Personality")
    void SetPersonalityTrait(EARPG_PersonalityTrait TraitType, float Strength, float Flexibility = 0.2f);

    // === COMPONENT OVERRIDES ===
    
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // === CONFIGURATION METHODS ===
    
    /** Load configuration from data table if set, otherwise use blueprint defaults */
    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void LoadPersonalityConfiguration();
    
    /** Set data table config (call this before BeginPlay) */
    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void SetDataTableConfig(UDataTable* DataTable, FName RowName);
    
    /** Get current effective configuration */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Configuration")
    FARPG_PersonalityConfiguration GetEffectiveConfiguration() const;

    // === TRAIT MANAGEMENT ===

    /** Get trait strength */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Personality")
    float GetTraitStrength(EARPG_PersonalityTrait TraitType) const;

    /** Set trait strength */
    UFUNCTION(BlueprintCallable, Category = "AI Personality")
    void SetTraitStrength(EARPG_PersonalityTrait TraitType, float Strength, float Flexibility = 0.2f);

    /** Modify trait strength by amount */
    UFUNCTION(BlueprintCallable, Category = "AI Personality")
    bool ModifyTrait(EARPG_PersonalityTrait TraitType, float Amount);

    /** Check if trait exists */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Personality")
    bool HasTrait(EARPG_PersonalityTrait TraitType) const;

    /** Get all current traits */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Personality")
    TMap<EARPG_PersonalityTrait, FARPG_AIPersonalityTrait> GetAllTraits() const;

    // === BEHAVIOR INFLUENCE ===

    /** Modify intent based on personality */
    void ModifyIntent(FARPG_AIIntent& Intent) const;
    
    /** Get weight for action based on personality */
    float GetActionWeight(FGameplayTag ActionTag) const;
    
    /** Check if personality supports specific action */
    bool SupportsAction(FGameplayTag ActionTag) const;
    
    /** Get relationship modifier between personalities */
    float GetRelationshipModifier(const UARPG_AIPersonalityComponent* OtherPersonality) const;
    
    /** Calculate compatibility between personalities */
    float GetPersonalityCompatibility(const UARPG_AIPersonalityComponent* OtherPersonality) const;

    // === BLUEPRINT ACCESSORS ===

    /** Get personality-based intents */
    UFUNCTION(BlueprintCallable, Category = "AI Personality")
    TArray<FGameplayTag> GetPersonalityBasedIntents() const;

    /** Get personality traits as gameplay tag map */
    UFUNCTION(BlueprintCallable, Category = "AI Personality")
    TMap<FGameplayTag, float> GetPersonalityTraits() const;

    /** Get personality traits as enum map */
    UFUNCTION(BlueprintCallable, Category = "AI Personality")
    TMap<EARPG_PersonalityTrait, float> GetPersonalityTraitsAsEnumMap() const;

    /** Check if trait is strong */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Personality")
    bool IsTraitStrong(EARPG_PersonalityTrait TraitType, float Threshold = 0.7f) const;

    /** Check if trait is weak */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Personality")
    bool IsTraitWeak(EARPG_PersonalityTrait TraitType, float Threshold = 0.3f) const;

    /** Get the strongest personality trait */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Personality")
    EARPG_PersonalityTrait GetDominantTrait() const;

    // === EXPERIENCE AND LEARNING ===

    /** Add experience that affects personality development */
    UFUNCTION(BlueprintCallable, Category = "AI Personality")
    void AddExperience(FGameplayTag ExperienceTag, float Intensity);

    /** Get experience history */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Personality")
    TMap<FGameplayTag, float> GetExperienceHistory() const;

    // === UTILITY ===

    /** Get readable personality description */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Personality")
    FString GetPersonalityDescription() const;

    /** Reset personality to defaults */
    UFUNCTION(BlueprintCallable, Category = "AI Personality")
    void ResetPersonality();

    /** Get personality summary */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Personality")
    FString GetPersonalitySummary() const;

    /** Check if personality favors specific action type */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Personality")
    float BP_GetCustomActionWeight(FGameplayTag ActionTag) const;

    // === EVENTS ===

    /** Personality trait changed event */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPersonalityTraitChanged OnPersonalityTraitChanged;

    /** Personality description changed event */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPersonalityChanged OnPersonalityChanged;

protected:
    // === CONFIGURATION ===

    /** Data table containing personality configurations */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    TObjectPtr<UDataTable> PersonalityDataTable;

    /** Row name in data table to use for configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FName DataTableRowName;

    /** Personality system configuration (used if no data table is set) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FARPG_PersonalityConfiguration PersonalityConfig;

    /** Default personality traits (used if no data table is set) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    TArray<FARPG_AIPersonalityTrait> DefaultTraits;

    // === STATE ===

    /** Current personality traits */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    TMap<EARPG_PersonalityTrait, FARPG_AIPersonalityTrait> CurrentTraits;

    /** Experience history for personality development */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    TMap<FGameplayTag, float> ExperienceHistory;

    /** Reference to brain component */
    UPROPERTY(BlueprintReadOnly, Category = "Components")
    TWeakObjectPtr<UARPG_AIBrainComponent> BrainComponent;

    /** Current effective configuration (either from data table or blueprint) */
    FARPG_PersonalityConfiguration EffectiveConfig;

private:
    // === INTERNAL PROCESSING ===

    /** Load configuration from data table */
    bool LoadConfigurationFromDataTable(FARPG_PersonalityConfiguration& OutConfig, TArray<FARPG_AIPersonalityTrait>& OutTraits) const;
    FARPG_AIPersonalityTrait CreateTraitFromDataTable(EARPG_PersonalityTrait TraitType, float Strength) const;

    /** Get trait display name */
    FString GetTraitDisplayName(EARPG_PersonalityTrait TraitType) const;
    
    /** Initialize component references */
    void InitializeComponentReferences();

    /** Initialize default personality traits */
    void InitializeDefaultTraits();

    /** Get default trait configuration for a specific type */
    FARPG_AIPersonalityTrait GetDefaultTraitForType(EARPG_PersonalityTrait TraitType) const;

    /** Calculate personality change based on experience */
    void ApplyExperienceToTrait(EARPG_PersonalityTrait TraitType, FGameplayTag ExperienceTag, float Intensity);

    /** Get trait influence on specific action types */
    float GetTraitInfluenceOnAction(EARPG_PersonalityTrait TraitType, FGameplayTag ActionTag) const;

    /** Validate trait strength (clamp to 0-1 range) */
    float ValidateTraitStrength(float Strength) const;

    /** Convert personality trait enum to gameplay tag */
    FGameplayTag GetGameplayTagForTrait(EARPG_PersonalityTrait TraitType) const;
};