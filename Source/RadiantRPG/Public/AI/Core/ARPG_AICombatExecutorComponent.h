// Source/RadiantRPG/Public/AI/Core/ARPG_AICombatExecutorComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTags.h"
#include "Types/ARPG_AITypes.h"
#include "AI/Interfaces/IARPG_AIBehaviorExecutorInterface.h"
#include "Engine/EngineTypes.h"
#include "ARPG_AICombatExecutorComponent.generated.h"

class AAIController;
class UBlackboardComponent;

/**
 * Combat execution configuration
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_CombatExecutorConfig
{
    GENERATED_BODY()

    /** Maximum combat engagement range */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "100.0", ClampMax = "2000.0"))
    float MaxCombatRange = 800.0f;

    /** Preferred attack range */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "50.0", ClampMax = "500.0"))
    float PreferredAttackRange = 150.0f;

    /** Minimum distance to maintain from target */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "25.0", ClampMax = "200.0"))
    float MinCombatDistance = 75.0f;

    /** Time between attack attempts */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.5", ClampMax = "5.0"))
    float AttackCooldown = 1.5f;

    /** Time to wait in defensive stance */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "1.0", ClampMax = "10.0"))
    float DefenseTime = 3.0f;

    /** Distance to retreat when fleeing */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "300.0", ClampMax = "1500.0"))
    float RetreatDistance = 600.0f;

    /** Field of view for target detection (degrees) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "60.0", ClampMax = "360.0"))
    float CombatFOV = 180.0f;

    /** Whether to enable debug visualization */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowCombatDebug = false;

    /** Whether to enable debug logging */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bEnableDebugLogging = false;
};

/**
 * Combat execution states
 */
UENUM(BlueprintType)
enum class EARPG_CombatState : uint8
{
    Idle            UMETA(DisplayName = "Idle"),
    Engaging        UMETA(DisplayName = "Engaging"),
    Attacking       UMETA(DisplayName = "Attacking"),
    Defending       UMETA(DisplayName = "Defending"),
    Retreating      UMETA(DisplayName = "Retreating"),
    Circling        UMETA(DisplayName = "Circling"),
    Failed          UMETA(DisplayName = "Failed"),
    MAX             UMETA(Hidden)
};

/**
 * Combat target information
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_CombatTarget
{
    GENERATED_BODY()

    /** Target actor */
    UPROPERTY(BlueprintReadWrite, Category = "Target")
    TWeakObjectPtr<AActor> TargetActor;

    /** Last known position */
    UPROPERTY(BlueprintReadWrite, Category = "Target")
    FVector LastKnownPosition = FVector::ZeroVector;

    /** Time when target was last seen */
    UPROPERTY(BlueprintReadWrite, Category = "Target")
    float LastSeenTime = 0.0f;

    /** Threat level (0.0 to 1.0) */
    UPROPERTY(BlueprintReadWrite, Category = "Target")
    float ThreatLevel = 0.5f;

    /** Distance to target */
    UPROPERTY(BlueprintReadWrite, Category = "Target")
    float Distance = 0.0f;

    /** Whether target is in line of sight */
    UPROPERTY(BlueprintReadWrite, Category = "Target")
    bool bInLineOfSight = false;

    FARPG_CombatTarget()
    {
        TargetActor = nullptr;
        LastKnownPosition = FVector::ZeroVector;
        LastSeenTime = 0.0f;
        ThreatLevel = 0.5f;
        Distance = 0.0f;
        bInLineOfSight = false;
    }
};

/**
 * AI Combat Executor Component
 * Handles all combat-related intent execution including attacking, defending, and retreating
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class RADIANTRPG_API UARPG_AICombatExecutorComponent : public UActorComponent, public IARPG_AIBehaviorExecutorInterface
{
    GENERATED_BODY()

public:
    UARPG_AICombatExecutorComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // === CONFIGURATION ===
    
    /** Combat execution configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FARPG_CombatExecutorConfig CombatConfig;

    // === CURRENT STATE ===
    
    /** Current combat state */
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    EARPG_CombatState CurrentCombatState;

    /** Current combat target */
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    FARPG_CombatTarget CurrentTarget;

    /** Time spent in current state */
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    float StateTime;

    /** Time since last attack */
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    float TimeSinceLastAttack;

    /** Whether combat was successful */
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    bool bCombatSuccessful;

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

    // === COMBAT EXECUTION ===
    
    /** Execute an attack intent */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool ExecuteAttackIntent(const FARPG_AIIntent& Intent);

    /** Execute a defend intent */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool ExecuteDefendIntent(const FARPG_AIIntent& Intent);

    /** Execute a retreat intent */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool ExecuteRetreatIntent(const FARPG_AIIntent& Intent);

    // === TARGET MANAGEMENT ===
    
    /** Set combat target */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void SetCombatTarget(AActor* TargetActor);

    /** Clear current combat target */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ClearCombatTarget();

    /** Update target information */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void UpdateTargetInfo();

    /** Check if target is valid and available */
    UFUNCTION(BlueprintPure, Category = "Combat")
    bool IsTargetValid() const;

    /** Check if target is in attack range */
    UFUNCTION(BlueprintPure, Category = "Combat")
    bool IsTargetInAttackRange() const;

    /** Check if target is in line of sight */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool IsTargetInLineOfSight();

    /** Get distance to current target */
    UFUNCTION(BlueprintPure, Category = "Combat")
    float GetDistanceToTarget() const;

    // === COMBAT ACTIONS ===
    
    /** Perform an attack */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool PerformAttack();

    /** Enter defensive stance */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool EnterDefensiveStance();

    /** Move to optimal attack position */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool MoveToAttackPosition();

    /** Circle around target */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool CircleTarget();

    /** Retreat from combat */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool RetreatFromCombat();

    // === AI CONTROLLER INTEGRATION ===
    
    /** Get the AI controller */
    UFUNCTION(BlueprintPure, Category = "Combat")
    AAIController* GetAIController() const;

    /** Get blackboard component */
    UFUNCTION(BlueprintPure, Category = "Combat")
    UBlackboardComponent* GetBlackboard() const;

    // === EVENTS ===
    
    /** Called when combat starts */
    UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
    void OnCombatStarted(AActor* Target);

    /** Called when attack is performed */
    UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
    void OnAttackPerformed(AActor* Target);

    /** Called when entering defensive stance */
    UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
    void OnDefenseStarted();

    /** Called when retreating */
    UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
    void OnRetreatStarted(AActor* ThreatActor);

    /** Called when combat ends */
    UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
    void OnCombatEnded(bool bVictorious);

    /** Called when target is lost */
    UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
    void OnTargetLost(AActor* LostTarget);

protected:
    // === INTERNAL LOGIC ===
    
    /** Update combat state machine */
    void UpdateCombatState(float DeltaTime);

    /** Handle idle state */
    void HandleIdleState(float DeltaTime);

    /** Handle engaging state */
    void HandleEngagingState(float DeltaTime);

    /** Handle attacking state */
    void HandleAttackingState(float DeltaTime);

    /** Handle defending state */
    void HandleDefendingState(float DeltaTime);

    /** Handle retreating state */
    void HandleRetreatingState(float DeltaTime);

    /** Handle circling state */
    void HandleCirclingState(float DeltaTime);

    /** Handle failed state */
    void HandleFailedState(float DeltaTime);

    /** Set new combat state */
    void SetCombatState(EARPG_CombatState NewState);

    /** Calculate combat progress (0.0 to 1.0) */
    float CalculateCombatProgress() const;

    /** Check if can attack */
    bool CanAttack() const;

    /** Get optimal attack position */
    FVector GetOptimalAttackPosition() const;

    /** Get retreat position */
    FVector GetRetreatPosition() const;

    /** Perform line of sight check */
    bool PerformLineOfSightCheck(const FVector& TargetLocation) const;

    /** Log debug information */
    void LogCombatDebug(const FString& Message) const;

    /** Draw debug visualization */
    void DrawDebugVisualization() const;

private:
    /** Currently executing intent */
    FARPG_AIIntent ExecutingIntent;

    /** Cached AI controller reference */
    UPROPERTY()
    TObjectPtr<AAIController> CachedAIController;

    /** Cached blackboard reference */
    UPROPERTY()
    TObjectPtr<UBlackboardComponent> CachedBlackboard;

    /** Last attack position */
    FVector LastAttackPosition;

    /** Movement request ID for combat positioning */
    uint32 CombatMoveRequestID;
};