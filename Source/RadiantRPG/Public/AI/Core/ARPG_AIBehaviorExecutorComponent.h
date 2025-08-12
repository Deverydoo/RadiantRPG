#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTags.h"
#include "Types/ARPG_AITypes.h"
#include "AI/Interfaces/IARPG_AIBehaviorExecutorInterface.h"
#include "ARPG_AIBehaviorExecutorComponent.generated.h"

class UARPG_AIMovementExecutorComponent;
class UARPG_AICombatExecutorComponent;
class UARPG_AIInteractionExecutorComponent;
class UARPG_AIAnimationExecutorComponent;

/**
 * Configuration for behavior execution
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_BehaviorExecutorConfig
{
    GENERATED_BODY()

    /** Whether to enable debug logging */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bEnableDebugLogging = false;

    /** Whether to enable debug visualization */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug") 
    bool bEnableDebugVisualization = false;

    /** Maximum time to execute a single intent before timeout */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Execution", meta = (ClampMin = "1.0", ClampMax = "30.0"))
    float MaxExecutionTime = 10.0f;

    /** Priority boost for urgent intents */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Execution", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float UrgentIntentPriorityBoost = 2.0f;

    /** How often to update execution progress (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance", meta = (ClampMin = "0.1", ClampMax = "1.0"))
    float ExecutionUpdateFrequency = 0.2f;
};

/**
 * Main AI Behavior Executor Component
 * Coordinates sub-executors to translate AI intents into actual character actions
 * Handles movement, combat, interaction, and animation execution
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class RADIANTRPG_API UARPG_AIBehaviorExecutorComponent : public UActorComponent, public IARPG_AIBehaviorExecutorInterface
{
    GENERATED_BODY()

public:
    UARPG_AIBehaviorExecutorComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // === CONFIGURATION ===
    
    /** Executor configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FARPG_BehaviorExecutorConfig ExecutorConfig;

    /** Intent types this executor can handle */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    TArray<FGameplayTag> SupportedIntentTypes;

    // === CURRENT STATE ===
    
    /** Currently executing intent */
    UPROPERTY(BlueprintReadOnly, Category = "Execution")
    FARPG_AIIntent CurrentIntent;

    /** Current execution status */
    UPROPERTY(BlueprintReadOnly, Category = "Execution")
    EARPG_BehaviorExecutionStatus ExecutionStatus;

    /** Time spent executing current intent */
    UPROPERTY(BlueprintReadOnly, Category = "Execution")
    float CurrentExecutionTime;

    /** Intent execution queue for chaining behaviors */
    UPROPERTY(BlueprintReadOnly, Category = "Execution")
    TArray<FARPG_AIIntent> IntentQueue;

    // === SUB-EXECUTORS ===
    
    /** Movement executor reference */
    UPROPERTY(BlueprintReadOnly, Category = "Sub-Executors")
    TObjectPtr<UARPG_AIMovementExecutorComponent> MovementExecutor;

    /** Combat executor reference */
    UPROPERTY(BlueprintReadOnly, Category = "Sub-Executors")
    TObjectPtr<UARPG_AICombatExecutorComponent> CombatExecutor;

    /** Interaction executor reference */
    UPROPERTY(BlueprintReadOnly, Category = "Sub-Executors")
    TObjectPtr<UARPG_AIInteractionExecutorComponent> InteractionExecutor;

    /** Animation executor reference */
    UPROPERTY(BlueprintReadOnly, Category = "Sub-Executors")
    TObjectPtr<UARPG_AIAnimationExecutorComponent> AnimationExecutor;

public:
    // === IARPG_AIBehaviorExecutorInterface IMPLEMENTATION ===
    
    virtual bool CanExecuteIntent_Implementation(const FARPG_AIIntent& Intent) ;
    virtual bool StartExecution_Implementation(const FARPG_AIIntent& Intent) ;
    virtual EARPG_BehaviorExecutionStatus UpdateExecution_Implementation(float DeltaTime) ;
    virtual FARPG_BehaviorExecutionResult StopExecution_Implementation(bool bForceStop) ;
    virtual float GetExecutionProgress_Implementation() const ;
    virtual bool IsExecuting_Implementation() const ;
    virtual FARPG_AIIntent GetCurrentIntent_Implementation() const ;
    virtual TArray<FGameplayTag> GetSupportedIntentTypes_Implementation() const ;
    virtual int32 GetExecutionPriority_Implementation(const FARPG_AIIntent& Intent) const ;

    // === INTENT MANAGEMENT ===
    
    /** Queue an intent for execution after current one completes */
    UFUNCTION(BlueprintCallable, Category = "Behavior Execution")
    void QueueIntent(const FARPG_AIIntent& Intent);

    /** Clear the intent queue */
    UFUNCTION(BlueprintCallable, Category = "Behavior Execution") 
    void ClearIntentQueue();

    /** Check if intent queue has space */
    UFUNCTION(BlueprintPure, Category = "Behavior Execution")
    bool CanQueueIntent() const;

    /** Get number of queued intents */
    UFUNCTION(BlueprintPure, Category = "Behavior Execution")
    int32 GetQueuedIntentCount() const;

    // === SUB-EXECUTOR MANAGEMENT ===
    
    /** Initialize all sub-executors */
    UFUNCTION(BlueprintCallable, Category = "Sub-Executors")
    void InitializeSubExecutors();

    /** Get sub-executor that can handle the intent */
    UFUNCTION(BlueprintCallable, Category = "Sub-Executors")
    UActorComponent* GetExecutorForIntent(const FARPG_AIIntent& Intent);

    /** Check if all required sub-executors are available */
    UFUNCTION(BlueprintPure, Category = "Sub-Executors")
    bool AreSubExecutorsReady() const;

    // === BLUEPRINT EVENTS ===
    
    /** Called when intent execution starts */
    UFUNCTION(BlueprintImplementableEvent, Category = "Behavior Execution")
    void OnIntentExecutionStarted(const FARPG_AIIntent& Intent);

    /** Called when intent execution completes */
    UFUNCTION(BlueprintImplementableEvent, Category = "Behavior Execution")
    void OnIntentExecutionCompleted(const FARPG_AIIntent& Intent, EARPG_BehaviorExecutionStatus Result);

    /** Called when execution is interrupted */
    UFUNCTION(BlueprintImplementableEvent, Category = "Behavior Execution") 
    void OnIntentExecutionInterrupted(const FARPG_AIIntent& Intent, const FString& Reason);

protected:
    // === INTERNAL LOGIC ===
    
    /** Process the next intent in the queue */
    void ProcessNextIntent();

    /** Determine which sub-executor should handle the intent */
    UActorComponent* DetermineSubExecutor(const FARPG_AIIntent& Intent);

    /** Check if current execution has timed out */
    bool HasExecutionTimedOut() const;

    /** Handle execution timeout */
    void HandleExecutionTimeout();

    /** Update execution progress from active sub-executor */
    void UpdateExecutionProgress();

    /** Log debug information about execution */
    void LogExecutionDebug(const FString& Message) const;

private:
    /** Last time execution was updated */
    float LastExecutionUpdate;

    /** Current execution progress (0.0 to 1.0) */
    float CurrentExecutionProgress;

    /** Currently active sub-executor */
    UPROPERTY()
    TObjectPtr<UActorComponent> ActiveSubExecutor;

    /** Maximum queue size to prevent memory issues */
    static constexpr int32 MaxQueueSize = 10;
};