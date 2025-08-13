// Source/RadiantRPG/Private/AI/Core/ARPG_AIMovementExecutorComponent.cpp

#include "AI/Core/ARPG_AIMovementExecutorComponent.h"

#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Core/RadiantGameplayTags.h"

UARPG_AIMovementExecutorComponent::UARPG_AIMovementExecutorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f;

    // Initialize configuration
    MovementConfig.DefaultSpeed = 300.0f;
    MovementConfig.WanderRadius = 1000.0f;
    MovementConfig.MaxRandomDistance = 800.0f;
    MovementConfig.MinRandomDistance = 200.0f;
    MovementConfig.AcceptanceRadius = 100.0f;
    MovementConfig.WaitTimeAtDestination = 3.0f;
    MovementConfig.bShowDebugPaths = false;
    MovementConfig.bEnableDebugLogging = false;

    // Initialize state
    CurrentMovementState = EARPG_MovementState::Idle;
    CurrentDestination = FVector::ZeroVector;
    MovementStartLocation = FVector::ZeroVector;
    StateTime = 0.0f;
    bMovementSuccessful = false;
    bBoundToPathFollowing = false;
    CurrentMoveRequestID = FAIRequestID::InvalidRequest;
}

void UARPG_AIMovementExecutorComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Cache home location
    HomeLocation = GetOwner()->GetActorLocation();
    
    // Get AI controller
    CachedAIController = GetAIController();
    
    // Bind to path following events
    if (CachedAIController && CachedAIController->GetPathFollowingComponent())
    {
        CachedAIController->GetPathFollowingComponent()->OnRequestFinished.AddUObject(this, &ThisClass::OnMoveCompleted);
        bBoundToPathFollowing = true;
    }

    LogMovementDebug(TEXT("Movement executor initialized"));
}

void UARPG_AIMovementExecutorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    StateTime += DeltaTime;
    UpdateMovementState(DeltaTime);
    
    if (MovementConfig.bShowDebugPaths)
    {
        DrawDebugVisualization();
    }
}

bool UARPG_AIMovementExecutorComponent::CanExecuteIntent_Implementation(const FARPG_AIIntent& Intent)
{
    // Check if this is a movement-related intent
    return Intent.IntentTag.MatchesTag(TAG_AI_Intent_Wander) ||
           Intent.IntentTag.MatchesTag(TAG_AI_Intent_Patrol) ||
           Intent.IntentTag.MatchesTag(TAG_AI_Intent_Curiosity_Explore) ||
           Intent.IntentTag.MatchesTag(TAG_AI_Intent_Social_Follow) ||
           Intent.IntentTag.MatchesTag(TAG_AI_Intent_Survival_Flee) ||
           Intent.IntentTag.MatchesTag(TAG_AI_Intent_Idle) ||
           Intent.IntentTag.MatchesTag(TAG_AI_Intent_Curiosity_Watch);
}

bool UARPG_AIMovementExecutorComponent::StartExecution_Implementation(const FARPG_AIIntent& Intent)
{
    if (!CanExecuteIntent_Implementation(Intent))
    {
        return false;
    }

    // Stop any current movement
    StopMovement();
    
    ExecutingIntent = Intent;
    SetMovementState(EARPG_MovementState::Idle);
    
    // Execute based on intent type
    bool bSuccess = false;
    
    if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Wander) || Intent.IntentTag.MatchesTag(TAG_AI_Intent_Curiosity_Explore))
    {
        bSuccess = ExecuteWanderIntent(Intent);
    }
    else if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Patrol))
    {
        bSuccess = ExecutePatrolIntent(Intent);
    }
    else if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Social_Follow))
    {
        bSuccess = ExecuteFollowIntent(Intent);
    }
    else if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Survival_Flee))
    {
        bSuccess = ExecuteFleeIntent(Intent);
    }
    else if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Idle) || Intent.IntentTag.MatchesTag(TAG_AI_Intent_Curiosity_Watch))
    {
        // For idle/watch intents, just wait
        SetMovementState(EARPG_MovementState::Waiting);
        bSuccess = true;
    }
    else
    {
        // Try to extract destination from intent data
        FString DestinationStr;
        if (Intent.IntentParameters.Contains(TEXT("Destination")))
        {
            DestinationStr = Intent.IntentParameters[TEXT("Destination")];
            FVector Destination;
            if (Destination.InitFromString(DestinationStr))
            {
                bSuccess = ExecuteMoveToIntent(Intent);
            }
        }
        else
        {
            // Default to wandering
            bSuccess = ExecuteWanderIntent(Intent);
        }
    }
    
    if (bSuccess)
    {
        LogMovementDebug(FString::Printf(TEXT("Started executing movement intent: %s"), *Intent.IntentTag.ToString()));
    }
    else
    {
        LogMovementDebug(FString::Printf(TEXT("Failed to start movement intent: %s"), *Intent.IntentTag.ToString()));
        SetMovementState(EARPG_MovementState::Failed);
    }
    
    return bSuccess;
}

EARPG_BehaviorExecutionStatus UARPG_AIMovementExecutorComponent::UpdateExecution_Implementation(float DeltaTime)
{
    switch (CurrentMovementState)
    {
        case EARPG_MovementState::Idle:
            return EARPG_BehaviorExecutionStatus::NotStarted;
            
        case EARPG_MovementState::Moving:
            return EARPG_BehaviorExecutionStatus::InProgress;
            
        case EARPG_MovementState::Waiting:
            // Check if wait time has elapsed
            if (StateTime >= MovementConfig.WaitTimeAtDestination)
            {
                SetMovementState(EARPG_MovementState::Idle);
                return EARPG_BehaviorExecutionStatus::Succeeded;
            }
            return EARPG_BehaviorExecutionStatus::InProgress;
            
        case EARPG_MovementState::Failed:
            return EARPG_BehaviorExecutionStatus::Failed;
            
        default:
            return EARPG_BehaviorExecutionStatus::Failed;
    }
}

FARPG_BehaviorExecutionResult UARPG_AIMovementExecutorComponent::StopExecution_Implementation(bool bForceStop)
{
    FARPG_BehaviorExecutionResult Result;
    Result.Status = UpdateExecution_Implementation(0.0f);
    Result.ExecutionTime = StateTime;
    Result.CompletionRatio = GetExecutionProgress_Implementation();
    
    StopMovement();
    SetMovementState(EARPG_MovementState::Idle);
    
    if (bForceStop)
    {
        Result.Status = EARPG_BehaviorExecutionStatus::Interrupted;
        Result.ResultMessage = TEXT("Movement force stopped");
    }
    
    return Result;
}

float UARPG_AIMovementExecutorComponent::GetExecutionProgress_Implementation() const
{
    if (CurrentMovementState == EARPG_MovementState::Moving)
    {
        return CalculateMovementProgress();
    }
    else if (CurrentMovementState == EARPG_MovementState::Waiting)
    {
        return FMath::Clamp(StateTime / MovementConfig.WaitTimeAtDestination, 0.0f, 1.0f);
    }
    else if (CurrentMovementState == EARPG_MovementState::Failed)
    {
        return 0.0f;
    }
    
    return 1.0f; // Idle state is considered complete
}

bool UARPG_AIMovementExecutorComponent::IsExecuting_Implementation() const
{
    return CurrentMovementState == EARPG_MovementState::Moving || CurrentMovementState == EARPG_MovementState::Waiting;
}

FARPG_AIIntent UARPG_AIMovementExecutorComponent::GetCurrentIntent_Implementation() const
{
    return ExecutingIntent;
}

TArray<FGameplayTag> UARPG_AIMovementExecutorComponent::GetSupportedIntentTypes_Implementation() const
{
    TArray<FGameplayTag> SupportedTypes;
    SupportedTypes.Add(TAG_AI_Intent_Wander);
    SupportedTypes.Add(TAG_AI_Intent_Patrol);
    SupportedTypes.Add(TAG_AI_Intent_Curiosity_Explore);
    SupportedTypes.Add(TAG_AI_Intent_Social_Follow);
    SupportedTypes.Add(TAG_AI_Intent_Survival_Flee);
    SupportedTypes.Add(TAG_AI_Intent_Idle);
    SupportedTypes.Add(TAG_AI_Intent_Curiosity_Watch);
    return SupportedTypes;
}

int32 UARPG_AIMovementExecutorComponent::GetExecutionPriority_Implementation(const FARPG_AIIntent& Intent) const
{
    // Survival movement gets highest priority
    if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Survival_Flee))
    {
        return 10;
    }
    
    // Combat-related movement gets high priority
    if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Combat))
    {
        return 8;
    }
    
    // Social following gets medium priority
    if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Social_Follow))
    {
        return 5;
    }
    
    // Exploration and wandering get low priority
    return 3;
}

bool UARPG_AIMovementExecutorComponent::ExecuteWanderIntent(const FARPG_AIIntent& Intent)
{
    FVector WanderLocation = GetRandomWanderLocation();
    
    if (WanderLocation == FVector::ZeroVector)
    {
        LogMovementDebug(TEXT("Failed to find valid wander location"));
        return false;
    }
    
    return MoveToLocation(WanderLocation);
}

bool UARPG_AIMovementExecutorComponent::ExecuteMoveToIntent(const FARPG_AIIntent& Intent)
{
    FString DestinationStr;
    if (!Intent.IntentParameters.Contains(TEXT("Destination")))
    {
        LogMovementDebug(TEXT("MoveToIntent missing destination data"));
        return false;
    }
    
    DestinationStr = Intent.IntentParameters[TEXT("Destination")];
    FVector Destination;
    if (!Destination.InitFromString(DestinationStr))
    {
        LogMovementDebug(TEXT("Failed to parse destination from intent data"));
        return false;
    }
    
    return MoveToLocation(Destination);
}

bool UARPG_AIMovementExecutorComponent::ExecuteFollowIntent(const FARPG_AIIntent& Intent)
{
    FString TargetActorName;
    if (!Intent.IntentParameters.Contains(TEXT("TargetActor")))
    {
        LogMovementDebug(TEXT("FollowIntent missing target actor data"));
        return false;
    }
    
    // In a full implementation, you'd resolve the actor reference
    // For now, just move to a random location near the home position
    FVector FollowLocation = HomeLocation + FVector(FMath::RandRange(-200.0f, 200.0f), FMath::RandRange(-200.0f, 200.0f), 0.0f);
    return MoveToLocation(FollowLocation);
}

bool UARPG_AIMovementExecutorComponent::ExecutePatrolIntent(const FARPG_AIIntent& Intent)
{
    // Simple patrol implementation - move to a random location
    return ExecuteWanderIntent(Intent);
}

bool UARPG_AIMovementExecutorComponent::ExecuteFleeIntent(const FARPG_AIIntent& Intent)
{
    FString ThreatLocationStr;
    FVector ThreatLocation = GetOwner()->GetActorLocation(); // Default to current location
    
    if (Intent.IntentParameters.Contains(TEXT("ThreatLocation")))
    {
        ThreatLocationStr = Intent.IntentParameters[TEXT("ThreatLocation")];
        ThreatLocation.InitFromString(ThreatLocationStr);
    }
    
    // Find a location away from the threat
    FVector CurrentLocation = GetOwner()->GetActorLocation();
    FVector FleeDirection = (CurrentLocation - ThreatLocation).GetSafeNormal();
    FVector FleeLocation = CurrentLocation + (FleeDirection * MovementConfig.MaxRandomDistance);
    
    return MoveToLocation(FleeLocation);
}

FVector UARPG_AIMovementExecutorComponent::GetRandomWanderLocation()
{
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    if (!NavSys)
    {
        return FVector::ZeroVector;
    }
    
    FNavLocation  RandomLocation;
    float RandomDistance = FMath::RandRange(MovementConfig.MinRandomDistance, MovementConfig.MaxRandomDistance);
    
    if (NavSys->GetRandomReachablePointInRadius(HomeLocation, RandomDistance, RandomLocation))
    {
        return RandomLocation;
    }
    
    return FVector::ZeroVector;
}

bool UARPG_AIMovementExecutorComponent::IsLocationReachable(const FVector& Location)
{
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    if (!NavSys)
    {
        return false;
    }
    
    FNavLocation ProjectedLocation;
    return NavSys->ProjectPointToNavigation(Location, ProjectedLocation);
}

float UARPG_AIMovementExecutorComponent::GetDistanceToDestination() const
{
    if (CurrentDestination == FVector::ZeroVector)
    {
        return 0.0f;
    }
    
    return FVector::Dist(GetOwner()->GetActorLocation(), CurrentDestination);
}

bool UARPG_AIMovementExecutorComponent::HasReachedDestination() const
{
    return GetDistanceToDestination() <= MovementConfig.AcceptanceRadius;
}

void UARPG_AIMovementExecutorComponent::StopMovement()
{
    if (CachedAIController)
    {
        CachedAIController->StopMovement();
    }
    
    CurrentMoveRequestID = FAIRequestID::InvalidRequest;
}

AAIController* UARPG_AIMovementExecutorComponent::GetAIController() const
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

bool UARPG_AIMovementExecutorComponent::MoveToLocation(const FVector& Location, float AcceptanceRadius)
{
    AAIController* AIController = GetAIController();
    if (!AIController)
    {
        LogMovementDebug(TEXT("No AI Controller found for movement"));
        return false;
    }
    
    float UseAcceptanceRadius = AcceptanceRadius > 0.0f ? AcceptanceRadius : MovementConfig.AcceptanceRadius;
    
    CurrentDestination = Location;
    MovementStartLocation = GetOwner()->GetActorLocation();
    
    FAIMoveRequest MoveRequest;
    MoveRequest.SetGoalLocation(Location);
    MoveRequest.SetAcceptanceRadius(UseAcceptanceRadius);
    
    FPathFollowingRequestResult Result = AIController->MoveTo(MoveRequest);
    CurrentMoveRequestID = Result.MoveId;
    
    if (Result.Code == EPathFollowingRequestResult::RequestSuccessful)
    {
        SetMovementState(EARPG_MovementState::Moving);
        OnMovementStarted(Location);
        LogMovementDebug(FString::Printf(TEXT("Started movement to location: %s"), *Location.ToString()));
        return true;
    }
    else
    {
        LogMovementDebug(FString::Printf(TEXT("Failed to start movement to location: %s"), *Location.ToString()));
        SetMovementState(EARPG_MovementState::Failed);
        return false;
    }
}

bool UARPG_AIMovementExecutorComponent::MoveToActor(AActor* TargetActor, float AcceptanceRadius)
{
    if (!TargetActor)
    {
        return false;
    }
    
    AAIController* AIController = GetAIController();
    if (!AIController)
    {
        return false;
    }
    
    float UseAcceptanceRadius = AcceptanceRadius > 0.0f ? AcceptanceRadius : MovementConfig.AcceptanceRadius;
    
    CurrentDestination = TargetActor->GetActorLocation();
    MovementStartLocation = GetOwner()->GetActorLocation();
    FollowTarget = TargetActor;
    
    FAIMoveRequest MoveRequest;
    MoveRequest.SetGoalActor(TargetActor);
    MoveRequest.SetAcceptanceRadius(UseAcceptanceRadius);
    
    FPathFollowingRequestResult Result = AIController->MoveTo(MoveRequest);
    CurrentMoveRequestID = Result.MoveId;
    
    if (Result.Code == EPathFollowingRequestResult::RequestSuccessful)
    {
        SetMovementState(EARPG_MovementState::Moving);
        OnMovementStarted(CurrentDestination);
        return true;
    }
    else
    {
        SetMovementState(EARPG_MovementState::Failed);
        return false;
    }
}

void UARPG_AIMovementExecutorComponent::UpdateMovementState(float DeltaTime)
{
    switch (CurrentMovementState)
    {
        case EARPG_MovementState::Idle:
            HandleIdleState(DeltaTime);
            break;
            
        case EARPG_MovementState::Moving:
            HandleMovingState(DeltaTime);
            break;
            
        case EARPG_MovementState::Waiting:
            HandleWaitingState(DeltaTime);
            break;
            
        case EARPG_MovementState::Failed:
            HandleFailedState(DeltaTime);
            break;
    }
}

void UARPG_AIMovementExecutorComponent::HandleIdleState(float DeltaTime)
{
    // Nothing to do in idle state
}

void UARPG_AIMovementExecutorComponent::HandleMovingState(float DeltaTime)
{
    // Check if we've reached the destination manually (backup to path following events)
    if (HasReachedDestination())
    {
        SetMovementState(EARPG_MovementState::Waiting);
        OnDestinationReached(CurrentDestination);
        bMovementSuccessful = true;
    }
}

void UARPG_AIMovementExecutorComponent::HandleWaitingState(float DeltaTime)
{
    // State time is tracked in TickComponent
    // Transition handled in UpdateExecution_Implementation
}

void UARPG_AIMovementExecutorComponent::HandleFailedState(float DeltaTime)
{
    // Remain in failed state until new intent starts
}

void UARPG_AIMovementExecutorComponent::SetMovementState(EARPG_MovementState NewState)
{
    if (CurrentMovementState != NewState)
    {
        EARPG_MovementState OldState = CurrentMovementState;
        CurrentMovementState = NewState;
        StateTime = 0.0f;
        
        LogMovementDebug(FString::Printf(TEXT("Movement state changed from %d to %d"), (int32)OldState, (int32)NewState));
    }
}

float UARPG_AIMovementExecutorComponent::CalculateMovementProgress() const
{
    if (MovementStartLocation == FVector::ZeroVector || CurrentDestination == FVector::ZeroVector)
    {
        return 0.0f;
    }
    
    float TotalDistance = FVector::Dist(MovementStartLocation, CurrentDestination);
    if (TotalDistance <= 0.0f)
    {
        return 1.0f;
    }
    
    float RemainingDistance = GetDistanceToDestination();
    float TraveledDistance = TotalDistance - RemainingDistance;
    
    return FMath::Clamp(TraveledDistance / TotalDistance, 0.0f, 1.0f);
}

void UARPG_AIMovementExecutorComponent::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    if (RequestID != CurrentMoveRequestID)
    {
        return; // Not our request
    }
    
    CurrentMoveRequestID = FAIRequestID::InvalidRequest;
    
    if (Result.Code == EPathFollowingResult::Success)
    {
        SetMovementState(EARPG_MovementState::Waiting);
        OnMovementCompleted(CurrentDestination);
        bMovementSuccessful = true;
        LogMovementDebug(TEXT("Movement completed successfully"));
    }
    else
    {
        SetMovementState(EARPG_MovementState::Failed);
        FString ResultString = FString::Printf(TEXT("PathFollowingResult(Code: %s, Flags: %d)"), *UEnum::GetValueAsString(Result.Code), (int32)Result.Flags);
        OnMovementFailed(CurrentDestination, ResultString);
        bMovementSuccessful = false;
        LogMovementDebug(FString::Printf(TEXT("Movement failed: %s"), *ResultString));
    }
}

void UARPG_AIMovementExecutorComponent::LogMovementDebug(const FString& Message) const
{
    if (MovementConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("[MovementExecutor] %s"), *Message);
    }
}

void UARPG_AIMovementExecutorComponent::DrawDebugVisualization() const
{
    if (!GetWorld())
    {
        return;
    }
    
    FVector CurrentLocation = GetOwner()->GetActorLocation();
    
    // Draw home location
    DrawDebugSphere(GetWorld(), HomeLocation, 50.0f, 12, FColor::Blue, false, -1.0f);
    
    // Draw current destination
    if (CurrentDestination != FVector::ZeroVector)
    {
        DrawDebugSphere(GetWorld(), CurrentDestination, MovementConfig.AcceptanceRadius, 12, FColor::Green, false, -1.0f);
        DrawDebugLine(GetWorld(), CurrentLocation, CurrentDestination, FColor::Yellow, false, -1.0f, 0, 2.0f);
    }
    
    // Draw wander radius
    DrawDebugCircle(GetWorld(), HomeLocation, MovementConfig.WanderRadius, 32, FColor::Cyan, false, -1.0f, 0, 5.0f, FVector(1,0,0), FVector(0,1,0));
}