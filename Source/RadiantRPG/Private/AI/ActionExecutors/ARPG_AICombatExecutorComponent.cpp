// Source/RadiantRPG/Private/AI/Core/ARPG_AICombatExecutorComponent.cpp

#include "AI/ActionExecutors/ARPG_AICombatExecutorComponent.h"
#include "AIController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "NavigationSystem.h"
#include "Core/RadiantGameplayTags.h"
#include "Navigation/PathFollowingComponent.h"

UARPG_AICombatExecutorComponent::UARPG_AICombatExecutorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f;

    // Initialize configuration
    CombatConfig.MaxCombatRange = 800.0f;
    CombatConfig.PreferredAttackRange = 150.0f;
    CombatConfig.MinCombatDistance = 75.0f;
    CombatConfig.AttackCooldown = 1.5f;
    CombatConfig.DefenseTime = 3.0f;
    CombatConfig.RetreatDistance = 600.0f;
    CombatConfig.CombatFOV = 180.0f;
    CombatConfig.bShowCombatDebug = false;
    CombatConfig.bEnableDebugLogging = false;

    // Initialize state
    CurrentCombatState = EARPG_CombatState::Idle;
    StateTime = 0.0f;
    TimeSinceLastAttack = 0.0f;
    bCombatSuccessful = false;
    CombatMoveRequestID = 0;
    LastAttackPosition = FVector::ZeroVector;
}

void UARPG_AICombatExecutorComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Cache AI controller and blackboard
    CachedAIController = GetAIController();
    if (CachedAIController)
    {
        CachedBlackboard = CachedAIController->GetBlackboardComponent();
    }

    LogCombatDebug(TEXT("Combat executor initialized"));
}

void UARPG_AICombatExecutorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    StateTime += DeltaTime;
    TimeSinceLastAttack += DeltaTime;
    
    // Update target information if we have one
    if (IsTargetValid())
    {
        UpdateTargetInfo();
    }
    
    UpdateCombatState(DeltaTime);
    
    if (CombatConfig.bShowCombatDebug)
    {
        DrawDebugVisualization();
    }
}

bool UARPG_AICombatExecutorComponent::CanExecuteIntent_Implementation(const FARPG_AIIntent& Intent)
{
    return Intent.IntentTag.MatchesTag(TAG_AI_Intent_Combat_Attack) ||
           Intent.IntentTag.MatchesTag(TAG_AI_Intent_Combat_Defend) ||
           Intent.IntentTag.MatchesTag(TAG_AI_Intent_Combat_Retreat) ||
           Intent.IntentTag.MatchesTag(TAG_AI_Intent_Combat);
}

bool UARPG_AICombatExecutorComponent::StartExecution_Implementation(const FARPG_AIIntent& Intent)
{
    if (!CanExecuteIntent_Implementation(Intent))
    {
        return false;
    }

    ExecutingIntent = Intent;
    SetCombatState(EARPG_CombatState::Idle);
    bCombatSuccessful = false;
    
    // Execute based on intent type
    bool bSuccess = false;
    
    if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Combat_Attack))
    {
        bSuccess = ExecuteAttackIntent(Intent);
    }
    else if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Combat_Defend))
    {
        bSuccess = ExecuteDefendIntent(Intent);
    }
    else if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Combat_Retreat))
    {
        bSuccess = ExecuteRetreatIntent(Intent);
    }
    else if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Combat))
    {
        // Generic combat - default to attack behavior
        bSuccess = ExecuteAttackIntent(Intent);
    }
    
    if (bSuccess)
    {
        LogCombatDebug(FString::Printf(TEXT("Started executing combat intent: %s"), *Intent.IntentTag.ToString()));
    }
    else
    {
        LogCombatDebug(FString::Printf(TEXT("Failed to start combat intent: %s"), *Intent.IntentTag.ToString()));
        SetCombatState(EARPG_CombatState::Failed);
    }
    
    return bSuccess;
}

EARPG_BehaviorExecutionStatus UARPG_AICombatExecutorComponent::UpdateExecution_Implementation(float DeltaTime)
{
    switch (CurrentCombatState)
    {
        case EARPG_CombatState::Idle:
            return EARPG_BehaviorExecutionStatus::NotStarted;
            
        case EARPG_CombatState::Engaging:
        case EARPG_CombatState::Attacking:
        case EARPG_CombatState::Circling:
            return EARPG_BehaviorExecutionStatus::InProgress;
            
        case EARPG_CombatState::Defending:
            // Check if defense time has elapsed
            if (StateTime >= CombatConfig.DefenseTime)
            {
                SetCombatState(EARPG_CombatState::Idle);
                return EARPG_BehaviorExecutionStatus::Succeeded;
            }
            return EARPG_BehaviorExecutionStatus::InProgress;
            
        case EARPG_CombatState::Retreating:
            // Check if we've moved far enough from threat
            if (GetDistanceToTarget() >= CombatConfig.RetreatDistance)
            {
                SetCombatState(EARPG_CombatState::Idle);
                return EARPG_BehaviorExecutionStatus::Succeeded;
            }
            return EARPG_BehaviorExecutionStatus::InProgress;
            
        case EARPG_CombatState::Failed:
            return EARPG_BehaviorExecutionStatus::Failed;
            
        default:
            return EARPG_BehaviorExecutionStatus::Failed;
    }
}

FARPG_BehaviorExecutionResult UARPG_AICombatExecutorComponent::StopExecution_Implementation(bool bForceStop)
{
    FARPG_BehaviorExecutionResult Result;
    Result.Status = UpdateExecution_Implementation(0.0f);
    Result.ExecutionTime = StateTime;
    Result.CompletionRatio = GetExecutionProgress_Implementation();
    
    // Stop any ongoing combat actions
    if (CachedAIController)
    {
        CachedAIController->StopMovement();
    }
    
    SetCombatState(EARPG_CombatState::Idle);
    ClearCombatTarget();
    
    if (bForceStop)
    {
        Result.Status = EARPG_BehaviorExecutionStatus::Interrupted;
        Result.ResultMessage = TEXT("Combat force stopped");
    }
    
    return Result;
}

float UARPG_AICombatExecutorComponent::GetExecutionProgress_Implementation() const
{
    switch (CurrentCombatState)
    {
        case EARPG_CombatState::Defending:
            return FMath::Clamp(StateTime / CombatConfig.DefenseTime, 0.0f, 1.0f);
            
        case EARPG_CombatState::Retreating:
            if (IsTargetValid())
            {
                float CurrentDistance = GetDistanceToTarget();
                return FMath::Clamp(CurrentDistance / CombatConfig.RetreatDistance, 0.0f, 1.0f);
            }
            return 1.0f;
            
        case EARPG_CombatState::Engaging:
        case EARPG_CombatState::Attacking:
        case EARPG_CombatState::Circling:
            return 0.5f; // Ongoing combat
            
        case EARPG_CombatState::Failed:
            return 0.0f;
            
        default:
            return 1.0f;
    }
}

bool UARPG_AICombatExecutorComponent::IsExecuting_Implementation() const
{
    return CurrentCombatState != EARPG_CombatState::Idle && CurrentCombatState != EARPG_CombatState::Failed;
}

FARPG_AIIntent UARPG_AICombatExecutorComponent::GetCurrentIntent_Implementation() const
{
    return ExecutingIntent;
}

TArray<FGameplayTag> UARPG_AICombatExecutorComponent::GetSupportedIntentTypes_Implementation() const
{
    TArray<FGameplayTag> SupportedTypes;
    SupportedTypes.Add(TAG_AI_Intent_Combat);
    SupportedTypes.Add(TAG_AI_Intent_Combat_Attack);
    SupportedTypes.Add(TAG_AI_Intent_Combat_Defend);
    SupportedTypes.Add(TAG_AI_Intent_Combat_Retreat);
    return SupportedTypes;
}

int32 UARPG_AICombatExecutorComponent::GetExecutionPriority_Implementation(const FARPG_AIIntent& Intent) const
{
    // Combat intents get high priority
    if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Combat_Retreat))
    {
        return 15; // Retreat has highest priority
    }
    
    if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Combat_Defend))
    {
        return 12; // Defense is high priority
    }
    
    if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Combat_Attack))
    {
        return 10; // Attack is high priority
    }
    
    return 8; // Generic combat
}

bool UARPG_AICombatExecutorComponent::ExecuteAttackIntent(const FARPG_AIIntent& Intent)
{
    // Try to get target from intent data
    FString TargetActorName;
    if (Intent.IntentParameters.Contains(TEXT("TargetActor")))
    {
        TargetActorName = Intent.IntentParameters[TEXT("TargetActor")];
        // In a full implementation, you'd resolve this to an actual actor reference
        // For now, we'll use a simple target acquisition system
    }
    
    // If no specific target, try to find nearby hostile
    if (!IsTargetValid())
    {
        // Simple target acquisition - find player character
        if (UWorld* World = GetWorld())
        {
            if (APawn* PlayerPawn = World->GetFirstPlayerController()->GetPawn())
            {
                float DistanceToPlayer = FVector::Dist(GetOwner()->GetActorLocation(), PlayerPawn->GetActorLocation());
                if (DistanceToPlayer <= CombatConfig.MaxCombatRange)
                {
                    SetCombatTarget(PlayerPawn);
                }
            }
        }
    }
    
    if (!IsTargetValid())
    {
        LogCombatDebug(TEXT("No valid target found for attack intent"));
        return false;
    }
    
    SetCombatState(EARPG_CombatState::Engaging);
    OnCombatStarted(CurrentTarget.TargetActor.Get());
    
    return true;
}

bool UARPG_AICombatExecutorComponent::ExecuteDefendIntent(const FARPG_AIIntent& Intent)
{
    SetCombatState(EARPG_CombatState::Defending);
    OnDefenseStarted();
    
    return EnterDefensiveStance();
}

bool UARPG_AICombatExecutorComponent::ExecuteRetreatIntent(const FARPG_AIIntent& Intent)
{
    // Try to get threat location from intent data
    FString ThreatLocationStr;
    if (Intent.IntentParameters.Contains(TEXT("ThreatLocation")))
    {
        ThreatLocationStr = Intent.IntentParameters[TEXT("ThreatLocation")];
        FVector ThreatLocation;
        if (ThreatLocation.InitFromString(ThreatLocationStr))
        {
            // Create a temporary target for retreat calculation
            CurrentTarget.LastKnownPosition = ThreatLocation;
            CurrentTarget.TargetActor = nullptr;
        }
    }
    
    SetCombatState(EARPG_CombatState::Retreating);
    OnRetreatStarted(CurrentTarget.TargetActor.Get());
    
    return RetreatFromCombat();
}

void UARPG_AICombatExecutorComponent::SetCombatTarget(AActor* TargetActor)
{
    if (TargetActor)
    {
        CurrentTarget.TargetActor = TargetActor;
        CurrentTarget.LastKnownPosition = TargetActor->GetActorLocation();
        CurrentTarget.LastSeenTime = GetWorld()->GetTimeSeconds();
        CurrentTarget.ThreatLevel = 0.7f; // Default threat level
        
        UpdateTargetInfo();
        
        LogCombatDebug(FString::Printf(TEXT("Set combat target: %s"), *TargetActor->GetName()));
    }
}

void UARPG_AICombatExecutorComponent::ClearCombatTarget()
{
    AActor* PreviousTarget = CurrentTarget.TargetActor.Get();
    CurrentTarget = FARPG_CombatTarget();
    
    if (PreviousTarget)
    {
        OnTargetLost(PreviousTarget);
        LogCombatDebug(TEXT("Cleared combat target"));
    }
}

void UARPG_AICombatExecutorComponent::UpdateTargetInfo()
{
    if (!IsTargetValid())
    {
        return;
    }
    
    AActor* Target = CurrentTarget.TargetActor.Get();
    FVector OwnerLocation = GetOwner()->GetActorLocation();
    FVector TargetLocation = Target->GetActorLocation();
    
    CurrentTarget.LastKnownPosition = TargetLocation;
    CurrentTarget.Distance = FVector::Dist(OwnerLocation, TargetLocation);
    CurrentTarget.bInLineOfSight = IsTargetInLineOfSight();
    
    if (CurrentTarget.bInLineOfSight)
    {
        CurrentTarget.LastSeenTime = GetWorld()->GetTimeSeconds();
    }
}

bool UARPG_AICombatExecutorComponent::IsTargetValid() const
{
    return CurrentTarget.TargetActor.IsValid() && !CurrentTarget.TargetActor->IsPendingKillPending();
}

bool UARPG_AICombatExecutorComponent::IsTargetInAttackRange() const
{
    if (!IsTargetValid())
    {
        return false;
    }
    
    return CurrentTarget.Distance <= CombatConfig.PreferredAttackRange;
}

bool UARPG_AICombatExecutorComponent::IsTargetInLineOfSight()
{
    if (!IsTargetValid())
    {
        return false;
    }
    
    return PerformLineOfSightCheck(CurrentTarget.LastKnownPosition);
}

float UARPG_AICombatExecutorComponent::GetDistanceToTarget() const
{
    if (!IsTargetValid())
    {
        return 0.0f;
    }
    
    return CurrentTarget.Distance;
}

bool UARPG_AICombatExecutorComponent::PerformAttack()
{
    if (!CanAttack())
    {
        return false;
    }
    
    TimeSinceLastAttack = 0.0f;
    LastAttackPosition = GetOwner()->GetActorLocation();
    
    OnAttackPerformed(CurrentTarget.TargetActor.Get());
    LogCombatDebug(TEXT("Performed attack"));
    
    return true;
}

bool UARPG_AICombatExecutorComponent::EnterDefensiveStance()
{
    // Stop movement and face target if available
    if (CachedAIController)
    {
        CachedAIController->StopMovement();
        
        if (IsTargetValid())
        {
            FVector LookDirection = (CurrentTarget.LastKnownPosition - GetOwner()->GetActorLocation()).GetSafeNormal();
            GetOwner()->SetActorRotation(LookDirection.Rotation());
        }
    }
    
    LogCombatDebug(TEXT("Entered defensive stance"));
    return true;
}

bool UARPG_AICombatExecutorComponent::MoveToAttackPosition()
{
    if (!IsTargetValid() || !CachedAIController)
    {
        return false;
    }
    
    FVector OptimalPosition = GetOptimalAttackPosition();
    
    FAIMoveRequest MoveRequest;
    MoveRequest.SetGoalLocation(OptimalPosition);
    MoveRequest.SetAcceptanceRadius(50.0f);
    
    FPathFollowingRequestResult Result = CachedAIController->MoveTo(MoveRequest);
    
    if (Result.Code == EPathFollowingRequestResult::RequestSuccessful)
    {
        LogCombatDebug(TEXT("Moving to attack position"));
        return true;
    }
    
    return false;
}

bool UARPG_AICombatExecutorComponent::CircleTarget()
{
    if (!IsTargetValid() || !CachedAIController)
    {
        return false;
    }
    
    // Calculate circling position
    FVector TargetLocation = CurrentTarget.LastKnownPosition;
    FVector OwnerLocation = GetOwner()->GetActorLocation();
    FVector DirectionToTarget = (TargetLocation - OwnerLocation).GetSafeNormal();
    
    // Get perpendicular vector for circling
    FVector CircleDirection = FVector::CrossProduct(DirectionToTarget, FVector::UpVector);
    FVector CirclePosition = TargetLocation + (CircleDirection * CombatConfig.PreferredAttackRange);
    
    FAIMoveRequest MoveRequest;
    MoveRequest.SetGoalLocation(CirclePosition);
    MoveRequest.SetAcceptanceRadius(75.0f);
    
    FPathFollowingRequestResult Result = CachedAIController->MoveTo(MoveRequest);
    
    return Result.Code == EPathFollowingRequestResult::RequestSuccessful;
}

bool UARPG_AICombatExecutorComponent::RetreatFromCombat()
{
    if (!CachedAIController)
    {
        return false;
    }
    
    FVector RetreatPosition = GetRetreatPosition();
    
    FAIMoveRequest MoveRequest;
    MoveRequest.SetGoalLocation(RetreatPosition);
    MoveRequest.SetAcceptanceRadius(100.0f);
    
    FPathFollowingRequestResult Result = CachedAIController->MoveTo(MoveRequest);
    
    if (Result.Code == EPathFollowingRequestResult::RequestSuccessful)
    {
        LogCombatDebug(TEXT("Retreating from combat"));
        return true;
    }
    
    return false;
}

AAIController* UARPG_AICombatExecutorComponent::GetAIController() const
{
    if (CachedAIController)
    {
        return CachedAIController;
    }
    
    if (APawn* Pawn = Cast<APawn>(GetOwner()))
    {
        return Cast<AAIController>(Pawn->GetController());
    }
    
    return nullptr;
}

UBlackboardComponent* UARPG_AICombatExecutorComponent::GetBlackboard() const
{
    return CachedBlackboard;
}

void UARPG_AICombatExecutorComponent::UpdateCombatState(float DeltaTime)
{
    switch (CurrentCombatState)
    {
        case EARPG_CombatState::Idle:
            HandleIdleState(DeltaTime);
            break;
            
        case EARPG_CombatState::Engaging:
            HandleEngagingState(DeltaTime);
            break;
            
        case EARPG_CombatState::Attacking:
            HandleAttackingState(DeltaTime);
            break;
            
        case EARPG_CombatState::Defending:
            HandleDefendingState(DeltaTime);
            break;
            
        case EARPG_CombatState::Retreating:
            HandleRetreatingState(DeltaTime);
            break;
            
        case EARPG_CombatState::Circling:
            HandleCirclingState(DeltaTime);
            break;
            
        case EARPG_CombatState::Failed:
            HandleFailedState(DeltaTime);
            break;
    }
}

void UARPG_AICombatExecutorComponent::HandleIdleState(float DeltaTime)
{
    // Nothing to do in idle state
}

void UARPG_AICombatExecutorComponent::HandleEngagingState(float DeltaTime)
{
    if (!IsTargetValid())
    {
        SetCombatState(EARPG_CombatState::Failed);
        return;
    }
    
    if (IsTargetInAttackRange())
    {
        SetCombatState(EARPG_CombatState::Attacking);
    }
    else
    {
        // Move closer to target
        MoveToAttackPosition();
    }
}

void UARPG_AICombatExecutorComponent::HandleAttackingState(float DeltaTime)
{
    if (!IsTargetValid())
    {
        SetCombatState(EARPG_CombatState::Failed);
        return;
    }
    
    if (CanAttack() && IsTargetInAttackRange())
    {
        PerformAttack();
        // After attacking, consider circling or repositioning
        if (FMath::RandBool())
        {
            SetCombatState(EARPG_CombatState::Circling);
        }
    }
    else if (!IsTargetInAttackRange())
    {
        SetCombatState(EARPG_CombatState::Engaging);
    }
}

void UARPG_AICombatExecutorComponent::HandleDefendingState(float DeltaTime)
{
    // Maintain defensive position and face threats
    EnterDefensiveStance();
}

void UARPG_AICombatExecutorComponent::HandleRetreatingState(float DeltaTime)
{
    // Continue retreating until safe distance is reached
    if (GetDistanceToTarget() < CombatConfig.RetreatDistance)
    {
        RetreatFromCombat();
    }
}

void UARPG_AICombatExecutorComponent::HandleCirclingState(float DeltaTime)
{
    if (!IsTargetValid())
    {
        SetCombatState(EARPG_CombatState::Failed);
        return;
    }
    
    // Circle for a bit, then return to attacking
    if (StateTime > 2.0f)
    {
        SetCombatState(EARPG_CombatState::Attacking);
    }
    else
    {
        CircleTarget();
    }
}

void UARPG_AICombatExecutorComponent::HandleFailedState(float DeltaTime)
{
    // Remain in failed state until new intent starts
}

void UARPG_AICombatExecutorComponent::SetCombatState(EARPG_CombatState NewState)
{
    if (CurrentCombatState != NewState)
    {
        EARPG_CombatState OldState = CurrentCombatState;
        CurrentCombatState = NewState;
        StateTime = 0.0f;
        
        LogCombatDebug(FString::Printf(TEXT("Combat state changed from %d to %d"), (int32)OldState, (int32)NewState));
    }
}

float UARPG_AICombatExecutorComponent::CalculateCombatProgress() const
{
    // This could be based on damage dealt, time in combat, etc.
    return FMath::Clamp(StateTime / 10.0f, 0.0f, 1.0f);
}

bool UARPG_AICombatExecutorComponent::CanAttack() const
{
    return TimeSinceLastAttack >= CombatConfig.AttackCooldown;
}

FVector UARPG_AICombatExecutorComponent::GetOptimalAttackPosition() const
{
    if (!IsTargetValid())
    {
        return GetOwner()->GetActorLocation();
    }
    
    FVector TargetLocation = CurrentTarget.LastKnownPosition;
    FVector OwnerLocation = GetOwner()->GetActorLocation();
    FVector DirectionToTarget = (TargetLocation - OwnerLocation).GetSafeNormal();
    
    // Position at preferred attack range
    return TargetLocation - (DirectionToTarget * CombatConfig.PreferredAttackRange);
}

FVector UARPG_AICombatExecutorComponent::GetRetreatPosition() const
{
    FVector OwnerLocation = GetOwner()->GetActorLocation();
    FVector ThreatLocation = IsTargetValid() ? CurrentTarget.LastKnownPosition : OwnerLocation;
    
    // Calculate retreat direction (away from threat)
    FVector RetreatDirection = (OwnerLocation - ThreatLocation).GetSafeNormal();
    FVector RetreatPosition = OwnerLocation + (RetreatDirection * CombatConfig.RetreatDistance);
    
    // Try to find a valid navigation point
    if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld()))
    {
        FNavLocation ProjectedPosition;
        if (NavSys->ProjectPointToNavigation(RetreatPosition, ProjectedPosition))
        {
            return ProjectedPosition;
        }
    }
    
    return RetreatPosition;
}

bool UARPG_AICombatExecutorComponent::PerformLineOfSightCheck(const FVector& TargetLocation) const
{
    if (!GetWorld())
    {
        return false;
    }
    
    FVector StartLocation = GetOwner()->GetActorLocation();
    FVector EndLocation = TargetLocation;
    
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetOwner());
    QueryParams.bTraceComplex = false;
    
    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        StartLocation,
        EndLocation,
        ECC_Visibility,
        QueryParams
    );
    
    // If we hit something, check if it's the target
    if (bHit && IsTargetValid())
    {
        return HitResult.GetActor() == CurrentTarget.TargetActor.Get();
    }
    
    return !bHit; // Clear line of sight if nothing was hit
}

void UARPG_AICombatExecutorComponent::LogCombatDebug(const FString& Message) const
{
    if (CombatConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("[CombatExecutor] %s"), *Message);
    }
}

void UARPG_AICombatExecutorComponent::DrawDebugVisualization() const
{
    if (!GetWorld())
    {
        return;
    }
    
    FVector OwnerLocation = GetOwner()->GetActorLocation();
    
    // Draw combat range
    DrawDebugCircle(GetWorld(), OwnerLocation, CombatConfig.MaxCombatRange, 32, FColor::Red, false, -1.0f, 0, 3.0f, FVector(1,0,0), FVector(0,1,0));
    
    // Draw attack range
    DrawDebugCircle(GetWorld(), OwnerLocation, CombatConfig.PreferredAttackRange, 24, FColor::Orange, false, -1.0f, 0, 2.0f, FVector(1,0,0), FVector(0,1,0));
    
    // Draw target information
    if (IsTargetValid())
    {
        FVector TargetLocation = CurrentTarget.LastKnownPosition;
        
        // Line to target
        FColor LineColor = CurrentTarget.bInLineOfSight ? FColor::Green : FColor::Yellow;
        DrawDebugLine(GetWorld(), OwnerLocation, TargetLocation, LineColor, false, -1.0f, 0, 3.0f);
        
        // Target marker
        DrawDebugSphere(GetWorld(), TargetLocation, 50.0f, 12, FColor::Red, false, -1.0f);
        
        // Display target info
        FString TargetInfo = FString::Printf(TEXT("Target: %.0fm\nLOS: %s"), 
            CurrentTarget.Distance, 
            CurrentTarget.bInLineOfSight ? TEXT("Yes") : TEXT("No"));
        DrawDebugString(GetWorld(), TargetLocation + FVector(0,0,100), TargetInfo, nullptr, FColor::White, -1.0f);
    }
    
    // Draw state info
    FString StateInfo = FString::Printf(TEXT("Combat State: %d\nTime: %.1f"), 
        (int32)CurrentCombatState, StateTime);
    DrawDebugString(GetWorld(), OwnerLocation + FVector(0,0,200), StateInfo, nullptr, FColor::Cyan, -1.0f);
}