// Source/RadiantRPG/Public/AI/Core/ARPG_AIPersonalityComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTags.h"
#include "Types/ARPG_AIEventTypes.h"
#include "Types/ARPG_AITypes.h"
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    float PersonalityChangeRate = true;

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
        ExperienceImpactMultiplier = 1.0f;
        bEnableDebugLogging = false;
    }
};

/**
 * AI Personality Component
 * Manages personality traits that influence AI behavior
 * Traits can evolve based on experiences
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class RADIANTRPG_API UARPG_AIPersonalityComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UARPG_AIPersonalityComponent();

    /** Set personality trait (alias for SetTraitStrength for compatibility) */
    UFUNCTION(BlueprintCallable, Category = "AI Personality")
    void SetPersonalityTrait(EARPG_PersonalityTrait TraitType, float NewStrength);

    // === Primary Interface ===
    /** Initialize personality with configuration and traits */
    UFUNCTION(BlueprintCallable, Category = "AI Personality")
    void InitializePersonality(const FARPG_PersonalityConfiguration& Config, const TArray<FARPG_AIPersonalityTrait>& InitialTraits);

    /** Update personality configuration */
    UFUNCTION(BlueprintCallable, Category = "AI Personality")
    void UpdatePersonalityConfiguration(const FARPG_PersonalityConfiguration& NewConfig);

    /** Get strength of a specific personality trait */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Personality")
    float GetTraitStrength(EARPG_PersonalityTrait TraitType) const;
    
    /** Set strength of a specific personality trait */
    UFUNCTION(BlueprintCallable, Category = "AI Personality")
    void SetTraitStrength(EARPG_PersonalityTrait TraitType, float NewStrength);

    /** Modify trait strength by delta amount */
    UFUNCTION(BlueprintCallable, Category = "AI Personality")
    void ModifyTraitStrength(EARPG_PersonalityTrait TraitType, float DeltaAmount);

    /** Get all personality traits */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Personality")
    TArray<FARPG_AIPersonalityTrait> GetAllTraits() const;

    /** Get traits that are above threshold strength */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Personality")
    TArray<EARPG_PersonalityTrait> GetStrongTraits(float Threshold = 0.7f) const;
    void AddOrUpdateTrait(const FARPG_AIPersonalityTrait& Trait);
    void RemoveTrait(EARPG_PersonalityTrait TraitType);
    void SetTraitFlexibility(EARPG_PersonalityTrait TraitType, float NewFlexibility);
    void ProcessExperience(FGameplayTag ExperienceTag, float Intensity, AActor* RelatedActor);
    void ReactToEvent(const FARPG_AIEvent& AIEvent);
    void ApplyPersonalityDevelopment();
    void ModifyIntent(FARPG_AIIntent& Intent) const;
    float GetActionWeight(FGameplayTag ActionTag) const;
    bool SupportsAction(FGameplayTag ActionTag) const;
    float GetRelationshipModifier(const UARPG_AIPersonalityComponent* OtherPersonality) const;
    float GetPersonalityCompatibility(const UARPG_AIPersonalityComponent* OtherPersonality) const;

    UFUNCTION(BlueprintCallable, Category = "AI Personality")
    TArray<FGameplayTag> GetPersonalityBasedIntents() const;

    UFUNCTION(BlueprintCallable, Category = "AI Personality")
    TMap<FGameplayTag, float> GetPersonalityTraits() const;

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

    /** Add experience that affects personality development */
    UFUNCTION(BlueprintCallable, Category = "AI Personality")
    void AddExperience(FGameplayTag ExperienceTag, float Intensity);

    /** Get experience history */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Personality")
    TMap<FGameplayTag, float> GetExperienceHistory() const;

    /** Get readable personality description */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Personality")
    FString GetPersonalityDescription() const;
    void ResetPersonality();

    /** Get personality summary */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Personality")
    FString GetPersonalitySummary() const;

    /** Check if personality favors specific action type */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Personality")
    bool FavorsActionType(FGameplayTag ActionTag, float Threshold = 0.6f) const;

    /** Check if personality opposes specific action type */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Personality")
    bool OpposesActionType(FGameplayTag ActionTag, float Threshold = 0.6f) const;

    /** Modify intent based on personality */
    UFUNCTION(BlueprintCallable, Category = "AI Personality")
    FARPG_AIIntent ModifyIntentByPersonality(const FARPG_AIIntent& BaseIntent) const;

    /** Get custom action weight for personality */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Personality")
    float GetCustomActionWeight(FGameplayTag ActionTag) const;

    /** Generate completely random personality */
    UFUNCTION(BlueprintCallable, Category = "AI Personality")
    void GenerateRandomPersonality();

    // === Events ===

    /** Blueprint event for experience processing */
    UFUNCTION(BlueprintImplementableEvent, Category = "AI Personality")
    void BP_OnExperienceProcessed(FGameplayTag ExperienceTag, float Intensity);

    /** Blueprint function for custom intent modification */
    UFUNCTION(BlueprintImplementableEvent, Category = "AI Personality")
    FARPG_AIIntent BP_ModifyIntentByPersonality(const FARPG_AIIntent& BaseIntent) const;
    
    /** Triggered when a personality trait changes */
    UPROPERTY(BlueprintAssignable, Category = "AI Personality")
    FOnPersonalityTraitChanged OnPersonalityTraitChanged;

    /** Triggered when overall personality changes significantly */
    UPROPERTY(BlueprintAssignable, Category = "AI Personality")
    FOnPersonalityChanged OnPersonalityChanged;

    // === Blueprint Events ===

    /** Blueprint event for trait changes */
    UFUNCTION(BlueprintImplementableEvent, Category = "AI Personality")
    void BP_OnPersonalityTraitChanged(EARPG_PersonalityTrait TraitType, float OldStrength, float NewStrength);

    /** Blueprint function for custom action weight calculation */
    UFUNCTION(BlueprintImplementableEvent, BlueprintPure, Category = "AI Personality")
    float BP_GetCustomActionWeight(FGameplayTag ActionTag) const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // === Configuration ===

    /** Personality system configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FARPG_PersonalityConfiguration PersonalityConfig;

    /** Default personality traits */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    TArray<FARPG_AIPersonalityTrait> DefaultTraits;

    // === State ===

    /** Current personality traits */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    TMap<EARPG_PersonalityTrait, FARPG_AIPersonalityTrait> CurrentTraits;

    /** Experience history for personality development */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    TMap<FGameplayTag, float> ExperienceHistory;

    /** Reference to brain component */
    UPROPERTY(BlueprintReadOnly, Category = "Components")
    TWeakObjectPtr<UARPG_AIBrainComponent> BrainComponent;

private:
    // === Internal Processing ===

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