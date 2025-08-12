// Source/RadiantRPG/Public/AI/Core/ARPG_AIPerceptionComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Perception/AIPerceptionComponent.h"
#include "GameplayTags.h"
#include "Types/ARPG_AITypes.h"
#include "ARPG_AIPerceptionComponent.generated.h"

class UARPG_AIBrainComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UAISenseConfig_Touch;
class UAISense;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPerceptionStimulus, const FARPG_AIStimulus&, Stimulus, AActor*, PerceivedActor);

/**
 * Perception configuration structure
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_PerceptionConfiguration
{
    GENERATED_BODY()

    /** Sight radius in cm */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sight")
    float SightRadius = 1500.0f;

    /** Sight angle in degrees */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sight")
    float SightAngle = 90.0f;

    /** How long sight memories last */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sight")
    float SightMaxAge = 5.0f;

    /** Hearing radius in cm */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hearing")
    float HearingRadius = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hearing")
    float HearingMaxAge = 1000.0f;

    /** Touch radius in cm */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch")
    float TouchRadius = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch")
    float TouchMaxAge = 100.0f;

    /** Whether to enable sight */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    bool bEnableSight = true;

    /** Whether to enable hearing */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    bool bEnableHearing = true;

    /** Whether to enable touch */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    bool bEnableTouch = true;

    /** Whether to enable debug visualization */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    bool bEnableDebugVisualization = false;

    /** Whether to enable debug logging */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    bool bEnableDebugLogging = false;

    FARPG_PerceptionConfiguration()
    {
        SightRadius = 1500.0f;
        SightAngle = 90.0f;
        SightMaxAge = 5.0f;
        HearingRadius = 1000.0f;
        TouchRadius = 100.0f;
        bEnableSight = true;
        bEnableHearing = true;
        bEnableTouch = true;
        bEnableDebugVisualization = false;
        bEnableDebugLogging = false;
    }
};

/**
 * Enhanced AI Perception Component
 * Wraps UE's AIPerceptionComponent and converts perceptions to AI stimuli
 * Integrates with the brain-based AI system
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class RADIANTRPG_API UARPG_AIPerceptionComponent : public UAIPerceptionComponent
{
    GENERATED_BODY()

public:
    UARPG_AIPerceptionComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // === Configuration ===

    /** Initialize perception with custom configuration */
    UFUNCTION(BlueprintCallable, Category = "AI Perception")
    void InitializePerception(const FARPG_PerceptionConfiguration& Config);

    /** Update perception configuration at runtime */
    UFUNCTION(BlueprintCallable, Category = "AI Perception")
    void UpdatePerceptionConfiguration(const FARPG_PerceptionConfiguration& NewConfig);

    /** Get current perception configuration */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Perception")
    FARPG_PerceptionConfiguration GetPerceptionConfiguration() const { return PerceptionConfig; }

    // === Perception Queries ===

    /** Get all currently perceived actors - No UFUNCTION to avoid override conflict */
    TArray<AActor*> GetCurrentlyPerceivedActors() const;

    /** Get actors perceived by specific sense */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Perception")
    TArray<AActor*> GetActorsPerceivedBySense(TSubclassOf<UAISense> SenseClass) const;

    /** Check if specific actor is currently perceived */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Perception")
    bool IsActorPerceived(AActor* Actor);

    /** Get last known location of actor */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Perception")
    FVector GetLastKnownLocation(AActor* Actor);

    /** Check if we can see a specific location */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Perception")
    bool CanSeeLocation(FVector Location) const;

    // === Manual Stimulus Generation ===

    /** Add a manual stimulus to the brain */
    UFUNCTION(BlueprintCallable, Category = "AI Perception")
    void AddManualStimulus(const FARPG_AIStimulus& Stimulus);

    /** Generate a sound stimulus */
    UFUNCTION(BlueprintCallable, Category = "AI Perception")
    void GenerateSoundStimulus(FVector SoundLocation, float Loudness, FGameplayTag SoundTag);

    /** Generate a visual stimulus */
    UFUNCTION(BlueprintCallable, Category = "AI Perception")
    void GenerateVisualStimulus(AActor* VisualActor, float Intensity, FGameplayTag VisualTag);

    // === Events ===

    /** Event triggered when a stimulus is generated from perception */
    UPROPERTY(BlueprintAssignable, Category = "AI Perception")
    FOnPerceptionStimulus OnPerceptionStimulus;

    // === Blueprint Overrides ===

    /** Blueprint hook to filter which stimuli should be generated */
    UFUNCTION(BlueprintImplementableEvent, Category = "AI Perception")
    bool BP_ShouldGenerateStimulus(AActor* PerceivedActor, TSubclassOf<UAISense> SenseClass) const;

    /** Blueprint hook to modify generated stimulus */
    UFUNCTION(BlueprintImplementableEvent, Category = "AI Perception")
    FARPG_AIStimulus BP_ModifyGeneratedStimulus(const FARPG_AIStimulus& OriginalStimulus, AActor* PerceivedActor);

protected:
    // === Configuration ===

    /** Perception configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FARPG_PerceptionConfiguration PerceptionConfig;

    // === Sense Configurations ===

    /** Sight sense configuration */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Senses")
    TObjectPtr<UAISenseConfig_Sight> SightConfig;

    /** Hearing sense configuration */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Senses")
    TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

    /** Touch sense configuration */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Senses")
    TObjectPtr<UAISenseConfig_Touch> TouchConfig;

    // === Components ===

    /** Reference to the brain component */
    UPROPERTY()
    TWeakObjectPtr<UARPG_AIBrainComponent> BrainComponent;

    // === Internal Methods ===

    /** Initialize reference to brain component */
    void InitializeBrainReference();

    /** Initialize sense configurations based on config */
    void InitializeSenseConfigurations();

    /** Setup perception event bindings */
    void SetupPerceptionEvents();

    /** Convert UE perception to ARPG stimulus */
    FARPG_AIStimulus ConvertPerceptionToStimulus(const FActorPerceptionUpdateInfo& UpdateInfo);

    /** Get stimulus type from sense ID */
    EARPG_StimulusType GetStimulusTypeFromSenseID(const FAISenseID& SenseID) const;

    /** Get sense class from sense ID */
    TSubclassOf<UAISense> GetSenseClassFromSenseID(const FAISenseID& SenseID) const;

    /** Calculate stimulus intensity based on perception data */
    float CalculateStimulusIntensity(const FActorPerceptionUpdateInfo& UpdateInfo) const;

    /** Generate appropriate gameplay tag for perception */
    FGameplayTag GeneratePerceptionTag(AActor* PerceivedActor, const FAISenseID& SenseID) const;

    // === Event Handlers ===

    /** Handle perception updates */
    UFUNCTION()
    void HandlePerceptionUpdated(const TArray<AActor*>& UpdatedActors);

    /** Handle target-specific perception updates */
    UFUNCTION()
    void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
};