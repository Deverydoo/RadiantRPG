// Source/RadiantRPG/Public/AI/Core/ARPG_AIBrainComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTags.h"
#include "Types/ARPG_AITypes.h"
#include "AI/Interfaces/IARPG_AIBrainInterface.h"
#include "Types/EventTypes.h"
#include "ARPG_AIBrainComponent.generated.h"

class UARPG_AIMemoryComponent;
class UARPG_AIPerceptionComponent;
class UARPG_AINeedsComponent;
class UARPG_AIPersonalityComponent;
class UARPG_AIEventManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIntentChanged, const FARPG_AIIntent&, NewIntent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBrainStateChanged, EARPG_BrainState, NewState);
// Note: FOnStimulusReceived is defined in EventTypes.h - using that definition

/**
 * Central AI Brain Component
 * Coordinates between perception, memory, needs, and personality to generate intents
 * Implements event-driven, curiosity-based AI behavior
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class RADIANTRPG_API UARPG_AIBrainComponent : public UActorComponent, public IARPG_AIBrainInterface
{
    GENERATED_BODY()

public:
    UARPG_AIBrainComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // === IARPG_AIBrainInterface Implementation ===
    
    UFUNCTION(BlueprintCallable, Category = "AI Brain")
    virtual void InitializeBrain(const FARPG_AIBrainConfiguration& Config) override;

    UFUNCTION(BlueprintCallable, Category = "AI Brain")
    virtual void ProcessStimulus(const FARPG_AIStimulus& Stimulus) override;

    UFUNCTION(BlueprintCallable, Category = "AI Brain")
    virtual FARPG_AIIntent GenerateIntent() override;

    UFUNCTION(BlueprintCallable, Category = "AI Brain")
    virtual void UpdateBrain(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "AI Brain")
    virtual bool ShouldActivateCuriosity() const override;

    UFUNCTION(BlueprintCallable, Category = "AI Brain")
    virtual FARPG_AIBrainState GetBrainState() const override;

    UFUNCTION(BlueprintCallable, Category = "AI Brain")
    virtual void SetBrainEnabled(bool bEnabled) override;

    UFUNCTION(BlueprintCallable, Category = "AI Brain")
    virtual float GetIntentConfidence() const override;

    // === Additional Brain Functions ===

    /** Force immediate intent generation */
    UFUNCTION(BlueprintCallable, Category = "AI Brain")
    void ForceIntentGeneration();

    /** Clear current brain state */
    UFUNCTION(BlueprintCallable, Category = "AI Brain")
    void ClearBrainState();

    /** Get recent stimuli */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Brain")
    TArray<FARPG_AIStimulus> GetRecentStimuli() const;

    /** Get current input vector for debugging */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Brain")
    FARPG_AIInputVector GetCurrentInputVector() const;

    // === Events ===

    /** Triggered when intent changes */
    UPROPERTY(BlueprintAssignable, Category = "AI Brain")
    FOnIntentChanged OnIntentChanged;

    /** Triggered when brain state changes */
    UPROPERTY(BlueprintAssignable, Category = "AI Brain")
    FOnBrainStateChanged OnBrainStateChanged;

    /** Triggered when stimulus is received - using EventTypes.h definition */
    UPROPERTY(BlueprintAssignable, Category = "AI Brain")
    FOnStimulusReceivedBP OnStimulusReceived;

    UFUNCTION(BlueprintCallable, Category = "AI Brain")
    virtual FARPG_AIIntent GetCurrentIntent() const ;

    UFUNCTION(BlueprintCallable, Category = "AI Brain")
    virtual void ExecuteIntent(const FARPG_AIIntent& Intent);

    // Add this to the Blueprint Implementable Functions section in the protected area:

    /** Blueprint implementation for intent execution */
    UFUNCTION(BlueprintImplementableEvent, Category = "AI Brain")
    bool BP_ExecuteIntent(const FARPG_AIIntent& Intent);


protected:
    // === Configuration ===

    /** Brain configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FARPG_AIBrainConfiguration BrainConfig;

    /** Current brain state */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    FARPG_AIBrainState CurrentBrainState;

    // === Component References ===

    /** Reference to memory component */
    UPROPERTY(BlueprintReadOnly, Category = "Components")
    TWeakObjectPtr<UARPG_AIMemoryComponent> MemoryComponent;

    /** Reference to perception component */
    UPROPERTY(BlueprintReadOnly, Category = "Components")
    TWeakObjectPtr<UARPG_AIPerceptionComponent> PerceptionComponent;

    /** Reference to needs component */
    UPROPERTY(BlueprintReadOnly, Category = "Components")
    TWeakObjectPtr<UARPG_AINeedsComponent> NeedsComponent;

    /** Reference to personality component */
    UPROPERTY(BlueprintReadOnly, Category = "Components")
    TWeakObjectPtr<UARPG_AIPersonalityComponent> PersonalityComponent;

    // === Intent Generation ===

    /** Default intent when no stimuli or needs are active */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intent")
    FGameplayTag DefaultIdleIntent;

    /** Possible curiosity intents */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intent")
    TArray<FGameplayTag> CuriosityIntentTags;

    // === Blueprint Implementable Functions ===

    /** Blueprint implementation for stimulus processing */
    UFUNCTION(BlueprintImplementableEvent, Category = "AI Brain")
    void BP_OnStimulusProcessed(const FARPG_AIStimulus& Stimulus);

    /** Blueprint implementation for custom intent generation */
    UFUNCTION(BlueprintImplementableEvent, Category = "AI Brain")
    FARPG_AIIntent BP_GenerateCustomIntent(const FARPG_AIInputVector& InputVector);

    /** Blueprint implementation for curiosity intent generation */
    UFUNCTION(BlueprintImplementableEvent, Category = "AI Brain")
    FARPG_AIIntent BP_GenerateCuriosityIntent();

private:
    // === Internal State ===

    /** Recent stimuli buffer */
    TArray<FARPG_AIStimulus> RecentStimuli;

    /** Time tracking */
    float TimeSinceLastUpdate;
    float LastStimulusTime;

    // === Internal Methods ===

    /** Initialize component references */
    void InitializeComponentReferences();

    /** Register with event manager */
    void RegisterWithEventManager();

    /** Unregister from event manager */
    void UnregisterFromEventManager();

    /** Build input vector from current state */
    FARPG_AIInputVector BuildInputVector() const;

    /** Process input vector to generate intent */
    FARPG_AIIntent ProcessInputVector(const FARPG_AIInputVector& InputVector);

    /** Apply personality modifications to intent */
    void ApplyPersonalityToIntent(FARPG_AIIntent& Intent);

    /** Validate generated intent */
    bool ValidateIntent(const FARPG_AIIntent& Intent) const;

    /** Update stimuli memory */
    void UpdateStimuliMemory(float DeltaTime);

    /** Generate curiosity intent */
    FARPG_AIIntent GenerateCuriosityIntent();

    /** Set new brain state */
    void SetBrainState(EARPG_BrainState NewState);

    /** Add stimulus to recent buffer */
    void AddStimulus(const FARPG_AIStimulus& Stimulus);

    /** Clean up old stimuli */
    void CleanupOldStimuli();

    /** Handle AI events */
    UFUNCTION()
    void OnAIEventReceived(const FARPG_AIEvent& Event);
};