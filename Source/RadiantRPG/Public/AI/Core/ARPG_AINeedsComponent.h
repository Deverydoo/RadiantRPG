// Source/RadiantRPG/Public/AI/Core/ARPG_AINeedsComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTags.h"
#include "Types/ARPG_AITypes.h"
#include "Types/ARPG_NPCTypes.h"
#include "ARPG_AINeedsComponent.generated.h"

class UARPG_AIBrainComponent;

// Use unique delegate names to avoid conflicts with other components
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAINeedChanged, EARPG_NeedType, NeedType, float, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAINeedBecameUrgent, EARPG_NeedType, NeedType, float, Level);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAINeedSatisfied, EARPG_NeedType, NeedType, float, Level);



/**
 * AI Needs Component
 * Manages basic needs like hunger, fatigue, safety, etc.
 * Contributes to AI decision making through the brain component
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class RADIANTRPG_API UARPG_AINeedsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UARPG_AINeedsComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // === Primary Interface ===

    /** Initialize needs with configuration and custom needs list */
    UFUNCTION(BlueprintCallable, Category = "AI Needs")
    void InitializeNeeds(const FARPG_NeedsConfiguration& Config, const TArray<FARPG_AINeed>& InitialNeeds);

    /** Update needs configuration */
    UFUNCTION(BlueprintCallable, Category = "AI Needs")
    void UpdateNeedsConfiguration(const FARPG_NeedsConfiguration& NewConfig);

    /** Get current level of a specific need */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Needs")
    float GetNeedLevel(EARPG_NeedType NeedType) const;

    /** Set level of a specific need */
    UFUNCTION(BlueprintCallable, Category = "AI Needs")
    void SetNeedLevel(EARPG_NeedType NeedType, float NewLevel);

    /** Modify need level by delta amount */
    UFUNCTION(BlueprintCallable, Category = "AI Needs")
    void ModifyNeedLevel(EARPG_NeedType NeedType, float DeltaAmount);

    /** Get all current needs */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Needs")
    TArray<FARPG_AINeed> GetAllNeeds() const;

    /** Check if a specific need is urgent */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Needs")
    bool IsNeedUrgent(EARPG_NeedType NeedType) const;

    /** Check if any needs are urgent */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Needs")
    bool HasUrgentNeeds() const;

    /** Get list of all urgent needs */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Needs")
    TArray<EARPG_NeedType> GetUrgentNeeds() const;

    /** Get the most urgent need */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Needs")
    EARPG_NeedType GetMostUrgentNeed() const;

    /** Check if a specific need is satisfied */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Needs")
    bool IsNeedSatisfied(EARPG_NeedType NeedType) const;

    /** Satisfy a specific need (reduce to satisfied threshold) */
    UFUNCTION(BlueprintCallable, Category = "AI Needs")
    void SatisfyNeed(EARPG_NeedType NeedType);

    /** Contribute to AI input vector (called by brain) */
    UFUNCTION(BlueprintCallable, Category = "AI Needs")
    void ContributeToInputs(FARPG_AIInputVector& InputVector) const;

    /** Get intent suggestions based on current needs */
    UFUNCTION(BlueprintCallable, Category = "AI Needs")
    TArray<FGameplayTag> GetNeedBasedIntentSuggestions() const;

    // === Utility Functions ===

    /** Pause or resume needs updates */
    UFUNCTION(BlueprintCallable, Category = "AI Needs")
    void SetNeedsUpdatePaused(bool bPaused);

    /** Reset all needs to default values */
    UFUNCTION(BlueprintCallable, Category = "AI Needs")
    void ResetAllNeeds();

    // === Events ===

    /** Triggered when any need level changes */
    UPROPERTY(BlueprintAssignable, Category = "AI Needs")
    FOnAINeedChanged OnNeedChanged;

    /** Triggered when a need becomes urgent */
    UPROPERTY(BlueprintAssignable, Category = "AI Needs")
    FOnAINeedBecameUrgent OnNeedBecameUrgent;

    /** Triggered when a need becomes satisfied */
    UPROPERTY(BlueprintAssignable, Category = "AI Needs")
    FOnAINeedSatisfied OnNeedSatisfied;

    // === Blueprint Events ===

    /** Blueprint event for need updates */
    UFUNCTION(BlueprintImplementableEvent, Category = "AI Needs")
    void BP_OnNeedUpdated(EARPG_NeedType NeedType, float OldLevel, float NewLevel);

    /** Blueprint event for urgent need state changes */
    UFUNCTION(BlueprintImplementableEvent, Category = "AI Needs")
    void BP_OnNeedBecameUrgent(EARPG_NeedType NeedType, float Level);

    /** Blueprint event for satisfied need state changes */
    UFUNCTION(BlueprintImplementableEvent, Category = "AI Needs")
    void BP_OnNeedSatisfied(EARPG_NeedType NeedType, float Level);

    /** Blueprint function to get custom intent for need */
    UFUNCTION(BlueprintImplementableEvent, Category = "AI Needs")
    FGameplayTag BP_GetIntentForNeed(EARPG_NeedType NeedType, float UrgencyLevel) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Needs")
    TMap<EARPG_NeedType, float> GetAllNeedsAsMap() const;

protected:
    // === Configuration ===

    /** Needs system configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FARPG_NeedsConfiguration NeedsConfig;

    /** Default needs setup */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    TArray<FARPG_AINeed> DefaultNeeds;

    // === State ===

    /** Map of current need levels */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    TMap<EARPG_NeedType, FARPG_AINeed> CurrentNeeds;

    /** Whether needs updates are paused */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bNeedsUpdatePaused = false;

    /** Time since last update */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    float TimeSinceLastUpdate = 0.0f;

    /** Reference to brain component */
    UPROPERTY(BlueprintReadOnly, Category = "Components")
    TWeakObjectPtr<UARPG_AIBrainComponent> BrainComponent;

private:
    // === Internal Processing ===

    /** Initialize component references */
    void InitializeComponentReferences();

    /** Initialize default needs */
    void InitializeDefaultNeeds();

    /** Update all needs over time */
    void UpdateNeedsOverTime(float DeltaTime);

    /** Update individual need */
    void UpdateIndividualNeed(FARPG_AINeed& Need, float DeltaTime);

    /** Check for urgent state changes */
    void CheckUrgentStateChanges(EARPG_NeedType NeedType, const FARPG_AINeed& OldNeed, const FARPG_AINeed& NewNeed);

    /** Check for satisfied state changes */
    void CheckSatisfiedStateChanges(EARPG_NeedType NeedType, const FARPG_AINeed& OldNeed, const FARPG_AINeed& NewNeed);

    /** Get default need configuration for a specific type */
    FARPG_AINeed GetDefaultNeedForType(EARPG_NeedType NeedType) const;

    /** Get intent tag for specific need type */
    FGameplayTag GetIntentTagForNeed(EARPG_NeedType NeedType) const;
};