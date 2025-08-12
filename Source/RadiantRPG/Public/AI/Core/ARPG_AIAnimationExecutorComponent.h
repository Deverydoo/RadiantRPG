// Source/RadiantRPG/Public/AI/Core/ARPG_AIAnimationExecutorComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTags.h"
#include "Types/ARPG_AITypes.h"
#include "AI/Interfaces/IARPG_AIBehaviorExecutorInterface.h"
#include "Animation/AnimMontage.h"
#include "ARPG_AIAnimationExecutorComponent.generated.h"

class UAnimationAsset;
class USkeletalMeshComponent;

/**
 * Animation execution configuration
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_AnimationExecutorConfig
{
    GENERATED_BODY()

    /** Default animation playback rate */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (ClampMin = "0.1", ClampMax = "3.0"))
    float DefaultPlayRate = 1.0f;

    /** Time between idle animations */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (ClampMin = "3.0", ClampMax = "30.0"))
    float IdleAnimationInterval = 8.0f;

    /** Random variance for idle timing */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float IdleTimingVariance = 3.0f;

    /** Whether to interrupt animations for higher priority intents */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    bool bAllowInterruption = true;

    /** Whether to enable debug logging */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bEnableDebugLogging = false;

    /** Whether to show animation debug info */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowAnimationDebug = false;
};

/**
 * Animation execution states
 */
UENUM(BlueprintType)
enum class EARPG_AnimationState : uint8
{
    Idle            UMETA(DisplayName = "Idle"),
    Playing         UMETA(DisplayName = "Playing"),
    Blending        UMETA(DisplayName = "Blending"),
    Waiting         UMETA(DisplayName = "Waiting"),
    Failed          UMETA(DisplayName = "Failed"),
    MAX             UMETA(Hidden)
};

/**
 * Animation data for different intents
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_AnimationMapping
{
    GENERATED_BODY()

    /** Intent tag this animation maps to */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    FGameplayTag IntentTag;

    /** Animation montage to play */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    TObjectPtr<UAnimMontage> AnimationMontage;

    /** Animation sequence (alternative to montage) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    TObjectPtr<UAnimationAsset> AnimationSequence;

    /** Playback rate override */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (ClampMin = "0.1", ClampMax = "3.0"))
    float PlayRate = 1.0f;

    /** Whether this animation loops */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    bool bLooping = false;

    /** Priority level (higher = more important) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (ClampMin = "0", ClampMax = "10"))
    int32 Priority = 1;

    /** Whether this animation can be interrupted */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    bool bCanBeInterrupted = true;

    FARPG_AnimationMapping()
    {
        IntentTag = FGameplayTag::EmptyTag;
        AnimationMontage = nullptr;
        AnimationSequence = nullptr;
        PlayRate = 1.0f;
        bLooping = false;
        Priority = 1;
        bCanBeInterrupted = true;
    }
};

/**
 * AI Animation Executor Component
 * Handles animation playback for AI intents including gestures, emotes, and behavioral animations
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class RADIANTRPG_API UARPG_AIAnimationExecutorComponent : public UActorComponent, public IARPG_AIBehaviorExecutorInterface
{
    GENERATED_BODY()

public:
    UARPG_AIAnimationExecutorComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // === CONFIGURATION ===
    
    /** Animation execution configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FARPG_AnimationExecutorConfig AnimationConfig;

    /** Animation mappings for different intents */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    TArray<FARPG_AnimationMapping> AnimationMappings;

    /** Idle animations to play randomly */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    TArray<UAnimMontage*> IdleAnimations;

    /** Gesture animations for social interactions */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    TArray<UAnimMontage*> GestureAnimations;

    /** Emote animations for personality expression */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    TArray<UAnimMontage*> EmoteAnimations;

    // === CURRENT STATE ===
    
    /** Current animation state */
    UPROPERTY(BlueprintReadOnly, Category = "Animation")
    EARPG_AnimationState CurrentAnimationState;

    /** Currently playing animation */
    UPROPERTY(BlueprintReadOnly, Category = "Animation")
    TObjectPtr<UAnimMontage> CurrentMontage;

    /** Time spent in current state */
    UPROPERTY(BlueprintReadOnly, Category = "Animation")
    float StateTime;

    /** Time until next idle animation */
    UPROPERTY(BlueprintReadOnly, Category = "Animation")
    float TimeToNextIdle;

    /** Current animation priority */
    UPROPERTY(BlueprintReadOnly, Category = "Animation")
    int32 CurrentAnimationPriority;

public:
    // === IARPG_AIBehaviorExecutorInterface IMPLEMENTATION ===
    
    virtual bool CanExecuteIntent_Implementation(const FARPG_AIIntent& Intent);
    virtual bool StartExecution_Implementation(const FARPG_AIIntent& Intent);
    virtual EARPG_BehaviorExecutionStatus UpdateExecution_Implementation(float DeltaTime);
    virtual FARPG_BehaviorExecutionResult StopExecution_Implementation(bool bForceStop) ;
    virtual float GetExecutionProgress_Implementation() const ;
    virtual bool IsExecuting_Implementation() const ;
    virtual FARPG_AIIntent GetCurrentIntent_Implementation() const ;
    virtual TArray<FGameplayTag> GetSupportedIntentTypes_Implementation() const;
    virtual int32 GetExecutionPriority_Implementation(const FARPG_AIIntent& Intent) const;

    // === ANIMATION EXECUTION ===
    
    /** Play animation for specific intent */
    UFUNCTION(BlueprintCallable, Category = "Animation")
    bool PlayAnimationForIntent(const FARPG_AIIntent& Intent);

    /** Play a specific montage */
    UFUNCTION(BlueprintCallable, Category = "Animation")
    bool PlayMontage(UAnimMontage* Montage, float PlayRate = 1.0f, bool bCanInterrupt = true);

    /** Stop current animation */
    UFUNCTION(BlueprintCallable, Category = "Animation")
    void StopCurrentAnimation(float BlendOutTime = 0.25f);

    /** Play random idle animation */
    UFUNCTION(BlueprintCallable, Category = "Animation")
    bool PlayRandomIdleAnimation();

    /** Play random gesture animation */
    UFUNCTION(BlueprintCallable, Category = "Animation")
    bool PlayRandomGesture();

    /** Play random emote animation */
    UFUNCTION(BlueprintCallable, Category = "Animation")
    bool PlayRandomEmote();

    // === ANIMATION MANAGEMENT ===
    
    /** Get animation mapping for intent */
    UFUNCTION(BlueprintCallable, Category = "Animation")
    FARPG_AnimationMapping FindAnimationMapping(const FGameplayTag& IntentTag) const;

    /** Check if animation is currently playing */
    UFUNCTION(BlueprintPure, Category = "Animation")
    bool IsAnimationPlaying() const;

    /** Get remaining animation time */
    UFUNCTION(BlueprintPure, Category = "Animation")
    float GetRemainingAnimationTime() const;

    /** Check if current animation can be interrupted */
    UFUNCTION(BlueprintPure, Category = "Animation")
    bool CanInterruptCurrentAnimation() const;

    /** Get skeletal mesh component */
    UFUNCTION(BlueprintPure, Category = "Animation")
    USkeletalMeshComponent* GetSkeletalMeshComponent() const;

    // === EVENTS ===
    
    /** Called when animation starts */
    UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
    void OnAnimationStarted(UAnimMontage* Montage, const FGameplayTag& IntentTag);

    /** Called when animation completes */
    UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
    void OnAnimationCompleted(UAnimMontage* Montage, bool bInterrupted);

    /** Called when idle animation plays */
    UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
    void OnIdleAnimationStarted(UAnimMontage* IdleMontage);

    /** Called when gesture is performed */
    UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
    void OnGesturePerformed(UAnimMontage* GestureMontage);

    /** Called when emote is performed */
    UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
    void OnEmotePerformed(UAnimMontage* EmoteMontage);

protected:
    // === INTERNAL LOGIC ===
    
    /** Update animation state machine */
    void UpdateAnimationState(float DeltaTime);

    /** Handle idle state */
    void HandleIdleState(float DeltaTime);

    /** Handle playing state */
    void HandlePlayingState(float DeltaTime);

    /** Handle blending state */
    void HandleBlendingState(float DeltaTime);

    /** Handle waiting state */
    void HandleWaitingState(float DeltaTime);

    /** Handle failed state */
    void HandleFailedState(float DeltaTime);

    /** Set new animation state */
    void SetAnimationState(EARPG_AnimationState NewState);

    /** Calculate next idle time */
    void CalculateNextIdleTime();

    /** Handle montage completion */
    UFUNCTION()
    void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    /** Handle montage blend out */
    UFUNCTION()
    void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

    /** Log debug information */
    void LogAnimationDebug(const FString& Message) const;

    /** Draw debug information */
    void DrawDebugInfo() const;

private:
    /** Currently executing intent */
    FARPG_AIIntent ExecutingIntent;

    /** Cached skeletal mesh component */
    UPROPERTY()
    TObjectPtr<USkeletalMeshComponent> CachedSkeletalMesh;

    /** Current animation mapping being executed */
    FARPG_AnimationMapping CurrentMapping;

    /** Whether the current animation was interrupted */
    bool bCurrentAnimationInterrupted;

    /** Montage instance handle */
    int32 MontageInstanceID;
};