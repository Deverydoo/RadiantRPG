// Public/AI/Core/ARPG_AINeedsComponent.h
// AI-specific needs system that extends base NeedsComponent

#pragma once

#include "CoreMinimal.h"
#include "Components/NeedsComponent.h"
#include "Types/ARPG_AITypes.h"
#include "GameplayTagContainer.h"
#include "ARPG_AINeedsComponent.generated.h"

class UARPG_AIBrainComponent;

/**
 * AI-specific needs component that extends the base needs system
 * 
 * Adds AI-specific functionality like:
 * - Intent suggestions based on needs
 * - Brain communication
 * - AI-driven need fulfillment behaviors
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RADIANTRPG_API UARPG_AINeedsComponent : public UNeedsComponent
{
    GENERATED_BODY()

public:
    UARPG_AINeedsComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** Reference to brain component for AI communication */
    UPROPERTY(BlueprintReadOnly, Category = "AI Components")
    TWeakObjectPtr<UARPG_AIBrainComponent> BrainComponent;

    /** Default needs configuration for AI characters */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Configuration")
    TArray<FARPG_AINeed> DefaultNeeds;

public:
    // === AI-Specific Interface ===

    /** Get intent suggestions based on current needs */
    UFUNCTION(BlueprintCallable, Category = "AI Needs")
    TArray<FGameplayTag> GetNeedBasedIntentSuggestions() const;

    /** Get the most urgent need that requires attention */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Needs")
    EARPG_NeedType GetMostUrgentNeed() const;

    /** Check if any needs are urgent enough to interrupt current behavior */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Needs")
    bool HasInterruptingNeeds() const;

    /** Get intent tag for a specific need type */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Needs")
    FGameplayTag GetIntentTagForNeed(EARPG_NeedType NeedType) const;

    /** Notify brain of need state changes */
    UFUNCTION(BlueprintCallable, Category = "AI Needs")
    void NotifyBrainOfNeedChange(EARPG_NeedType NeedType, float NewLevel);

    /** Evaluate if need can be satisfied with available actions */
    UFUNCTION(BlueprintCallable, Category = "AI Needs")
    bool CanSatisfyNeed(EARPG_NeedType NeedType) const;

    /** Get all current needs as a map for brain input processing */
    UFUNCTION(BlueprintCallable, Category = "Needs")
    TMap<EARPG_NeedType, float> GetAllNeedsAsMap() const;

    /** Get all current needs as normalized values (0.0 to 1.0) */
    UFUNCTION(BlueprintCallable, Category = "Needs")
    TMap<EARPG_NeedType, float> GetAllNeedsNormalized() const;

protected:
    // === Overridden Base Functions ===

    virtual void OnNeedValueChanged(EARPG_NeedType NeedType, float OldValue, float NewValue) override;
    virtual void OnNeedBecomeCritical(EARPG_NeedType NeedType) override;
    virtual void OnNeedNoLongerCritical(EARPG_NeedType NeedType) override;

    // === AI-Specific Internal Functions ===

    /** Initialize references to other AI components */
    void InitializeAIComponentReferences();

    /** Send need state update to brain component */
    void SendNeedUpdateToBrain(EARPG_NeedType NeedType, float Level, bool bIsCritical);

    /** Calculate urgency score for a need (0.0 to 1.0) */
    float CalculateNeedUrgency(EARPG_NeedType NeedType) const;

    /** Get default intent tag for need satisfaction */
    FGameplayTag GetDefaultIntentForNeed(EARPG_NeedType NeedType) const;

    // === Blueprint Implementable Events ===

    /** Blueprint event for AI-specific need changes */
    UFUNCTION(BlueprintImplementableEvent, Category = "AI Events")
    void BP_OnAINeedChanged(EARPG_NeedType NeedType, float UrgencyLevel, bool bIsInterrupting);

    /** Blueprint function to get custom intent for need */
    UFUNCTION(BlueprintImplementableEvent, Category = "AI Events")
    FGameplayTag BP_GetIntentForNeed(EARPG_NeedType NeedType, float UrgencyLevel) const;

    /** Blueprint function to check if need can be satisfied */
    UFUNCTION(BlueprintImplementableEvent, Category = "AI Events")
    bool BP_CanSatisfyNeed(EARPG_NeedType NeedType) const;
};