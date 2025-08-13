 // Source/RadiantRPG/Public/AI/Core/ARPG_AIMovementExecutorComponent.h

#pragma once

#include "CoreMinimal.h"
#include "AITypes.h"
#include "Components/ActorComponent.h"
#include "GameplayTags.h"
#include "Types/ARPG_AITypes.h"
#include "AI/Interfaces/IARPG_AIBehaviorExecutorInterface.h"
#include "AIController.h"

#include "ARPG_AIMovementExecutorComponent.generated.h"

class AAIController;
/**
 * Movement execution configuration
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_MovementExecutorConfig
{
    GENERATED_BODY()

    /** Default movement speed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "50.0", ClampMax = "1000.0"))
    float DefaultSpeed = 300.0f;

    /** Wander radius for exploration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "100.0", ClampMax = "5000.0"))
    float WanderRadius = 1000.0f;

    /** Maximum distance for random movement */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "200.0", ClampMax = "2000.0"))
    float MaxRandomDistance = 800.0f;

    /** Minimum distance for random movement */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "100.0", ClampMax = "1000.0"))
    float MinRandomDistance = 200.0f;

    /** How close to destination before considering arrival */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "50.0", ClampMax = "300.0"))
    float AcceptanceRadius = 100.0f;

    /** Time to wait at destination before next movement */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "1.0", ClampMax = "30.0"))
    float WaitTimeAtDestination = 3.0f;

    /** Whether to enable debug visualization */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowDebugPaths = false;

    /** Whether to enable debug logging */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bEnableDebugLogging = false;
};

/**
 * Movement execution states
 */
UENUM(BlueprintType)
enum class EARPG_MovementState : uint8
{
    Idle            UMETA(DisplayName = "Idle"),
    Moving          UMETA(DisplayName = "Moving"),
    Waiting         UMETA(DisplayName = "Waiting"),
    Failed          UMETA(DisplayName = "Failed"),
    MAX             UMETA(Hidden)
};

/**
 * AI Movement Executor Component
 * Handles all movement-related intent execution including wandering, pathfinding, and following
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class RADIANTRPG_API UARPG_AIMovementExecutorComponent : public UActorComponent, public IARPG_AIBehaviorExecutorInterface
{
    GENERATED_BODY()

public:
    UARPG_AIMovementExecutorComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // === CONFIGURATION ===
    
    /** Movement execution configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FARPG_MovementExecutorConfig MovementConfig;

    // === CURRENT STATE ===
    
    /** Current movement state */
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    EARPG_MovementState CurrentMovementState;

    /** Current destination */
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    FVector CurrentDestination;

    /** Start location of current movement */
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    FVector MovementStartLocation;

    /** Time spent in current state */
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    float StateTime;

    /** Whether movement was successful */
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bMovementSuccessful;

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

    // === MOVEMENT EXECUTION ===
    
    /** Execute a wander intent */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    bool ExecuteWanderIntent(const FARPG_AIIntent& Intent);

    /** Execute a move to location intent */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    bool ExecuteMoveToIntent(const FARPG_AIIntent& Intent);

    /** Execute a follow target intent */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    bool ExecuteFollowIntent(const FARPG_AIIntent& Intent);

    /** Execute a patrol intent */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    bool ExecutePatrolIntent(const FARPG_AIIntent& Intent);

    /** Execute a flee intent */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    bool ExecuteFleeIntent(const FARPG_AIIntent& Intent);

    // === UTILITY FUNCTIONS ===
    
    /** Get a random location within wander radius */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    FVector GetRandomWanderLocation();

    /** Check if location is reachable */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    bool IsLocationReachable(const FVector& Location);

    /** Get distance to current destination */
    UFUNCTION(BlueprintPure, Category = "Movement")
    float GetDistanceToDestination() const;

    /** Check if character has reached destination */
    UFUNCTION(BlueprintPure, Category = "Movement")
    bool HasReachedDestination() const;

    /** Stop current movement */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void StopMovement();

    // === AI CONTROLLER INTEGRATION ===
    
    /** Get the AI controller */
    UFUNCTION(BlueprintPure, Category = "Movement")
    AAIController* GetAIController() const;

    /** Move to location using AI controller */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    bool MoveToLocation(const FVector& Location, float AcceptanceRadius = -1.0f);

    /** Move to actor using AI controller */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    bool MoveToActor(AActor* TargetActor, float AcceptanceRadius = -1.0f);

    // === EVENTS ===
    
    /** Called when movement starts */
    UFUNCTION(BlueprintImplementableEvent, Category = "Movement")
    void OnMovementStarted(const FVector& Destination);

    /** Called when movement completes successfully */
    UFUNCTION(BlueprintImplementableEvent, Category = "Movement")
    void OnMovementCompleted(const FVector& Destination);

    /** Called when movement fails */
    UFUNCTION(BlueprintImplementableEvent, Category = "Movement")
    void OnMovementFailed(const FVector& Destination, const FString& Reason);

    /** Called when destination is reached */
    UFUNCTION(BlueprintImplementableEvent, Category = "Movement")
    void OnDestinationReached(const FVector& Destination);

protected:
    // === INTERNAL LOGIC ===
    
    /** Update movement state machine */
    void UpdateMovementState(float DeltaTime);

    /** Handle idle state */
    void HandleIdleState(float DeltaTime);

    /** Handle moving state */
    void HandleMovingState(float DeltaTime);

    /** Handle waiting state */
    void HandleWaitingState(float DeltaTime);

    /** Handle failed state */
    void HandleFailedState(float DeltaTime);

    /** Set new movement state */
    void SetMovementState(EARPG_MovementState NewState);

    /** Calculate movement progress (0.0 to 1.0) */
    float CalculateMovementProgress() const;

    /** Handle path following completion */
    void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);

    /** Log debug information */
    void LogMovementDebug(const FString& Message) const;

    /** Draw debug visualization */
    void DrawDebugVisualization() const;

private:
    /** Currently executing intent */
    FARPG_AIIntent ExecutingIntent;

    /** Current AI move request ID */
    FAIRequestID CurrentMoveRequestID;

    /** Cached AI controller reference */
    UPROPERTY()
    TObjectPtr<AAIController> CachedAIController;

    /** Whether we're bound to path following events */
    bool bBoundToPathFollowing;

    /** Home location for wandering */
    FVector HomeLocation;

    /** Target actor for following */
    UPROPERTY()
    TWeakObjectPtr<AActor> FollowTarget;
};