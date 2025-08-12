// Source/RadiantRPG/Public/AI/Core/ARPG_AIInteractionExecutorComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTags.h"
#include "Types/ARPG_AITypes.h"
#include "AI/Interfaces/IARPG_AIBehaviorExecutorInterface.h"
#include "Interaction/InteractableInterface.h"
#include "ARPG_AIInteractionExecutorComponent.generated.h"

class AAIController;

/**
 * Interaction execution configuration
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_InteractionExecutorConfig
{
    GENERATED_BODY()

    /** Maximum interaction range */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "50.0", ClampMax = "500.0"))
    float MaxInteractionRange = 200.0f;

    /** Time to spend in conversation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "2.0", ClampMax = "30.0"))
    float ConversationDuration = 5.0f;

    /** Time to spend trading */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "3.0", ClampMax = "15.0"))
    float TradeDuration = 8.0f;

    /** Time for object interaction */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "1.0", ClampMax = "10.0"))
    float ObjectInteractionTime = 3.0f;

    /** Whether to enable debug logging */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bEnableDebugLogging = false;

    /** Whether to show interaction debug info */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowInteractionDebug = false;
};

/**
 * Interaction execution states
 */
UENUM(BlueprintType)
enum class EARPG_InteractionState : uint8
{
    Idle            UMETA(DisplayName = "Idle"),
    Approaching     UMETA(DisplayName = "Approaching"),
    Interacting     UMETA(DisplayName = "Interacting"),
    Talking         UMETA(DisplayName = "Talking"),
    Trading         UMETA(DisplayName = "Trading"),
    Failed          UMETA(DisplayName = "Failed"),
    MAX             UMETA(Hidden)
};

/**
 * AI Interaction Executor Component
 * Handles social interactions, trading, and object manipulation for AI characters
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class RADIANTRPG_API UARPG_AIInteractionExecutorComponent : public UActorComponent, public IARPG_AIBehaviorExecutorInterface
{
    GENERATED_BODY()

public:
    UARPG_AIInteractionExecutorComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // === CONFIGURATION ===
    
    /** Interaction execution configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FARPG_InteractionExecutorConfig InteractionConfig;

    // === CURRENT STATE ===
    
    /** Current interaction state */
    UPROPERTY(BlueprintReadOnly, Category = "Interaction")
    EARPG_InteractionState CurrentInteractionState;

    /** Current interaction target */
    UPROPERTY(BlueprintReadOnly, Category = "Interaction")
    TWeakObjectPtr<AActor> InteractionTarget;

    /** Time spent in current state */
    UPROPERTY(BlueprintReadOnly, Category = "Interaction")
    float StateTime;

    /** Whether interaction was successful */
    UPROPERTY(BlueprintReadOnly, Category = "Interaction")
    bool bInteractionSuccessful;

public:
    // === IARPG_AIBehaviorExecutorInterface IMPLEMENTATION ===
    
    virtual bool CanExecuteIntent_Implementation(const FARPG_AIIntent& Intent);
    virtual bool StartExecution_Implementation(const FARPG_AIIntent& Intent);
    virtual EARPG_BehaviorExecutionStatus UpdateExecution_Implementation(float DeltaTime);
    virtual FARPG_BehaviorExecutionResult StopExecution_Implementation(bool bForceStop);
    virtual float GetExecutionProgress_Implementation() const;
    virtual bool IsExecuting_Implementation() const;
    virtual FARPG_AIIntent GetCurrentIntent_Implementation() const;
    virtual TArray<FGameplayTag> GetSupportedIntentTypes_Implementation() const;
    virtual int32 GetExecutionPriority_Implementation(const FARPG_AIIntent& Intent) const;

    // === INTERACTION EXECUTION ===
    
    /** Execute a talk intent */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    bool ExecuteTalkIntent(const FARPG_AIIntent& Intent);

    /** Execute a trade intent */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    bool ExecuteTradeIntent(const FARPG_AIIntent& Intent);

    /** Execute object interaction intent */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    bool ExecuteObjectInteractionIntent(const FARPG_AIIntent& Intent);

    // === TARGET MANAGEMENT ===
    
    /** Set interaction target */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void SetInteractionTarget(AActor* Target);

    /** Clear interaction target */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void ClearInteractionTarget();

    /** Check if target is valid for interaction */
    UFUNCTION(BlueprintPure, Category = "Interaction")
    bool IsTargetValid() const;

    /** Check if target is in interaction range */
    UFUNCTION(BlueprintPure, Category = "Interaction")
    bool IsTargetInRange() const;

    /** Get distance to interaction target */
    UFUNCTION(BlueprintPure, Category = "Interaction")
    float GetDistanceToTarget() const;

    // === INTERACTION ACTIONS ===
    
    /** Approach interaction target */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    bool ApproachTarget();

    /** Start conversation with target */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    bool StartConversation();

    /** Start trading with target */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    bool StartTrading();

    /** Interact with object */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    bool InteractWithObject();

    /** Face interaction target */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void FaceTarget();

    // === AI CONTROLLER INTEGRATION ===
    
    /** Get the AI controller */
    UFUNCTION(BlueprintPure, Category = "Interaction")
    AAIController* GetAIController() const;

    // === EVENTS ===
    
    /** Called when interaction starts */
    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
    void OnInteractionStarted(AActor* Target, const FGameplayTag& InteractionType);

    /** Called when conversation begins */
    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
    void OnConversationStarted(AActor* ConversationPartner);

    /** Called when trading begins */
    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
    void OnTradingStarted(AActor* TradePartner);

    /** Called when object interaction occurs */
    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
    void OnObjectInteraction(AActor* InteractedObject);

    /** Called when interaction ends */
    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
    void OnInteractionEnded(AActor* Target, bool bSuccessful);

protected:
    // === INTERNAL LOGIC ===
    
    /** Update interaction state machine */
    void UpdateInteractionState(float DeltaTime);

    /** Handle idle state */
    void HandleIdleState(float DeltaTime);

    /** Handle approaching state */
    void HandleApproachingState(float DeltaTime);

    /** Handle interacting state */
    void HandleInteractingState(float DeltaTime);

    /** Handle talking state */
    void HandleTalkingState(float DeltaTime);

    /** Handle trading state */
    void HandleTradingState(float DeltaTime);

    /** Handle failed state */
    void HandleFailedState(float DeltaTime);

    /** Set new interaction state */
    void SetInteractionState(EARPG_InteractionState NewState);

    /** Calculate interaction progress (0.0 to 1.0) */
    float CalculateInteractionProgress() const;

    /** Get interaction duration for current state */
    float GetCurrentStateDuration() const;

    /** Log debug information */
    void LogInteractionDebug(const FString& Message) const;

    /** Draw debug visualization */
    void DrawDebugVisualization() const;

private:
    /** Currently executing intent */
    FARPG_AIIntent ExecutingIntent;

    /** Cached AI controller reference */
    UPROPERTY()
    TObjectPtr<AAIController> CachedAIController;

    /** Movement request ID for approaching */
    uint32 ApproachMoveRequestID;
};