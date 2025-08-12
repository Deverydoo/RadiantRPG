// Source/RadiantRPG/Private/AI/Core/ARPG_AIInteractionExecutorComponent.cpp

#include "CoreMinimal.h"
#include "AI/Core/ARPG_AIInteractionExecutorComponent.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Characters/BaseCharacter.h"
#include "Core/RadiantGameplayTags.h"
#include "Navigation/PathFollowingComponent.h"

UARPG_AIInteractionExecutorComponent::UARPG_AIInteractionExecutorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f;

    // Initialize configuration
    InteractionConfig.MaxInteractionRange = 200.0f;
    InteractionConfig.ConversationDuration = 5.0f;
    InteractionConfig.TradeDuration = 8.0f;
    InteractionConfig.ObjectInteractionTime = 3.0f;
    InteractionConfig.bEnableDebugLogging = false;
    InteractionConfig.bShowInteractionDebug = false;

    // Initialize state
    CurrentInteractionState = EARPG_InteractionState::Idle;
    StateTime = 0.0f;
    bInteractionSuccessful = false;
    ApproachMoveRequestID = 0;
}

void UARPG_AIInteractionExecutorComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Cache AI controller
    CachedAIController = GetAIController();

    LogInteractionDebug(TEXT("Interaction executor initialized"));
}

void UARPG_AIInteractionExecutorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    StateTime += DeltaTime;
    UpdateInteractionState(DeltaTime);
    
    if (InteractionConfig.bShowInteractionDebug)
    {
        DrawDebugVisualization();
    }
}

bool UARPG_AIInteractionExecutorComponent::CanExecuteIntent_Implementation(const FARPG_AIIntent& Intent)
{
    return Intent.IntentTag.MatchesTag(TAG_AI_Intent_Social_Talk) ||
           Intent.IntentTag.MatchesTag(TAG_AI_Intent_Social_Trade) ||
           Intent.IntentTag.MatchesTag(TAG_AI_Intent_Social);
}

bool UARPG_AIInteractionExecutorComponent::StartExecution_Implementation(const FARPG_AIIntent& Intent)
{
    if (!CanExecuteIntent_Implementation(Intent))
    {
        return false;
    }

    ExecutingIntent = Intent;
    SetInteractionState(EARPG_InteractionState::Idle);
    bInteractionSuccessful = false;
    
    // Execute based on intent type
    bool bSuccess = false;
    
    if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Social_Talk))
    {
        bSuccess = ExecuteTalkIntent(Intent);
    }
    else if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Social_Trade))
    {
        bSuccess = ExecuteTradeIntent(Intent);
    }
    else if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Social))
    {
        // Generic social - try to determine type from intent data
        if (Intent.IntentParameters.Contains(TEXT("InteractionType")))
        {
            FString InteractionType = Intent.IntentParameters[TEXT("InteractionType")];
            if (InteractionType == TEXT("Trade"))
            {
                bSuccess = ExecuteTradeIntent(Intent);
            }
            else if (InteractionType == TEXT("Object"))
            {
                bSuccess = ExecuteObjectInteractionIntent(Intent);
            }
            else
            {
                bSuccess = ExecuteTalkIntent(Intent); // Default to talk
            }
        }
        else
        {
            bSuccess = ExecuteTalkIntent(Intent); // Default to talk
        }
    }
    
    if (bSuccess)
    {
        LogInteractionDebug(FString::Printf(TEXT("Started executing interaction intent: %s"), *Intent.IntentTag.ToString()));
    }
    else
    {
        LogInteractionDebug(FString::Printf(TEXT("Failed to start interaction intent: %s"), *Intent.IntentTag.ToString()));
        SetInteractionState(EARPG_InteractionState::Failed);
    }
    
    return bSuccess;
}

EARPG_BehaviorExecutionStatus UARPG_AIInteractionExecutorComponent::UpdateExecution_Implementation(float DeltaTime)
{
    switch (CurrentInteractionState)
    {
        case EARPG_InteractionState::Idle:
            return EARPG_BehaviorExecutionStatus::NotStarted;
            
        case EARPG_InteractionState::Approaching:
            return EARPG_BehaviorExecutionStatus::InProgress;
            
        case EARPG_InteractionState::Interacting:
        case EARPG_InteractionState::Talking:
        case EARPG_InteractionState::Trading:
            // Check if interaction duration has elapsed
            if (StateTime >= GetCurrentStateDuration())
            {
                SetInteractionState(EARPG_InteractionState::Idle);
                bInteractionSuccessful = true;
                return EARPG_BehaviorExecutionStatus::Succeeded;
            }
            return EARPG_BehaviorExecutionStatus::InProgress;
            
        case EARPG_InteractionState::Failed:
            return EARPG_BehaviorExecutionStatus::Failed;
            
        default:
            return EARPG_BehaviorExecutionStatus::Failed;
    }
}

FARPG_BehaviorExecutionResult UARPG_AIInteractionExecutorComponent::StopExecution_Implementation(bool bForceStop)
{
    FARPG_BehaviorExecutionResult Result;
    Result.Status = UpdateExecution_Implementation(0.0f);
    Result.ExecutionTime = StateTime;
    Result.CompletionRatio = GetExecutionProgress_Implementation();
    
    // Stop any ongoing movement
    if (CachedAIController)
    {
        CachedAIController->StopMovement();
    }
    
    // End interaction
    if (IsTargetValid())
    {
        OnInteractionEnded(InteractionTarget.Get(), bInteractionSuccessful);
    }
    
    SetInteractionState(EARPG_InteractionState::Idle);
    ClearInteractionTarget();
    
    if (bForceStop)
    {
        Result.Status = EARPG_BehaviorExecutionStatus::Interrupted;
        Result.ResultMessage = TEXT("Interaction force stopped");
    }
    
    return Result;
}

float UARPG_AIInteractionExecutorComponent::GetExecutionProgress_Implementation() const
{
    float TotalDuration = GetCurrentStateDuration();
    if (TotalDuration > 0.0f)
    {
        return FMath::Clamp(StateTime / TotalDuration, 0.0f, 1.0f);
    }
    
    switch (CurrentInteractionState)
    {
        case EARPG_InteractionState::Approaching:
            return 0.2f; // Approaching progress
        case EARPG_InteractionState::Failed:
            return 0.0f;
        default:
            return 1.0f;
    }
}

bool UARPG_AIInteractionExecutorComponent::IsExecuting_Implementation() const
{
    return CurrentInteractionState != EARPG_InteractionState::Idle && 
           CurrentInteractionState != EARPG_InteractionState::Failed;
}

FARPG_AIIntent UARPG_AIInteractionExecutorComponent::GetCurrentIntent_Implementation() const
{
    return ExecutingIntent;
}

TArray<FGameplayTag> UARPG_AIInteractionExecutorComponent::GetSupportedIntentTypes_Implementation() const
{
    TArray<FGameplayTag> SupportedTypes;
    SupportedTypes.Add(TAG_AI_Intent_Social);
    SupportedTypes.Add(TAG_AI_Intent_Social_Talk);
    SupportedTypes.Add(TAG_AI_Intent_Social_Trade);
    return SupportedTypes;
}

int32 UARPG_AIInteractionExecutorComponent::GetExecutionPriority_Implementation(const FARPG_AIIntent& Intent) const
{
    // Social interactions have medium priority
    if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Social_Trade))
    {
        return 6; // Trade gets slightly higher priority
    }
    
    return 4; // Default social priority
}

bool UARPG_AIInteractionExecutorComponent::ExecuteTalkIntent(const FARPG_AIIntent& Intent)
{
    // Try to get target from intent data
    FString TargetActorName;
    if (Intent.IntentParameters.Contains(TEXT("TargetActor")))
    {
        TargetActorName = Intent.IntentParameters[TEXT("TargetActor")];
        // In a full implementation, resolve actor reference
    }
    
    // For demo purposes, find player as conversation target
    if (!IsTargetValid())
    {
        if (UWorld* World = GetWorld())
        {
            if (APawn* PlayerPawn = World->GetFirstPlayerController()->GetPawn())
            {
                float DistanceToPlayer = GetDistanceToTarget();
                if (DistanceToPlayer <= InteractionConfig.MaxInteractionRange)
                {
                    SetInteractionTarget(PlayerPawn);
                }
            }
        }
    }
    
    if (!IsTargetValid())
    {
        LogInteractionDebug(TEXT("No valid target found for talk intent"));
        return false;
    }
    
    SetInteractionState(EARPG_InteractionState::Approaching);
    OnInteractionStarted(InteractionTarget.Get(), TAG_AI_Intent_Social_Talk);
    
    return ApproachTarget();
}

bool UARPG_AIInteractionExecutorComponent::ExecuteTradeIntent(const FARPG_AIIntent& Intent)
{
    // Similar to talk intent but for trading
    if (!IsTargetValid())
    {
        if (UWorld* World = GetWorld())
        {
            if (APawn* PlayerPawn = World->GetFirstPlayerController()->GetPawn())
            {
                SetInteractionTarget(PlayerPawn);
            }
        }
    }
    
    if (!IsTargetValid())
    {
        LogInteractionDebug(TEXT("No valid target found for trade intent"));
        return false;
    }
    
    SetInteractionState(EARPG_InteractionState::Approaching);
    OnInteractionStarted(InteractionTarget.Get(), TAG_AI_Intent_Social_Trade);
    
    return ApproachTarget();
}

bool UARPG_AIInteractionExecutorComponent::ExecuteObjectInteractionIntent(const FARPG_AIIntent& Intent)
{
    FString ObjectLocationStr;
    if (Intent.IntentParameters.Contains(TEXT("ObjectLocation")))
    {
        ObjectLocationStr = Intent.IntentParameters[TEXT("ObjectLocation")];
        FVector ObjectLocation;
        if (ObjectLocation.InitFromString(ObjectLocationStr))
        {
            // Find nearest object at that location
            // For now, just set a dummy target location
            SetInteractionState(EARPG_InteractionState::Approaching);
            return true;
        }
    }
    
    LogInteractionDebug(TEXT("No valid object found for interaction intent"));
    return false;
}

void UARPG_AIInteractionExecutorComponent::SetInteractionTarget(AActor* Target)
{
    if (Target)
    {
        InteractionTarget = Target;
        LogInteractionDebug(FString::Printf(TEXT("Set interaction target: %s"), *Target->GetName()));
    }
}

void UARPG_AIInteractionExecutorComponent::ClearInteractionTarget()
{
    AActor* PreviousTarget = InteractionTarget.Get();
    InteractionTarget = nullptr;
    
    if (PreviousTarget)
    {
        LogInteractionDebug(TEXT("Cleared interaction target"));
    }
}

bool UARPG_AIInteractionExecutorComponent::IsTargetValid() const
{
    return InteractionTarget.IsValid() && !InteractionTarget->IsPendingKillPending();
}

bool UARPG_AIInteractionExecutorComponent::IsTargetInRange() const
{
    if (!IsTargetValid())
    {
        return false;
    }
    
    return GetDistanceToTarget() <= InteractionConfig.MaxInteractionRange;
}

float UARPG_AIInteractionExecutorComponent::GetDistanceToTarget() const
{
    if (!IsTargetValid())
    {
        return 0.0f;
    }
    
    return FVector::Dist(GetOwner()->GetActorLocation(), InteractionTarget->GetActorLocation());
}

bool UARPG_AIInteractionExecutorComponent::ApproachTarget()
{
    if (!IsTargetValid() || !CachedAIController)
    {
        return false;
    }
    
    FVector TargetLocation = InteractionTarget->GetActorLocation();
    
    FAIMoveRequest MoveRequest;
    MoveRequest.SetGoalLocation(TargetLocation);
    MoveRequest.SetAcceptanceRadius(InteractionConfig.MaxInteractionRange * 0.8f);
    
    FPathFollowingRequestResult Result = CachedAIController->MoveTo(MoveRequest);
    
    if (Result.Code == EPathFollowingRequestResult::RequestSuccessful)
    {
        LogInteractionDebug(TEXT("Approaching interaction target"));
        return true;
    }
    
    return false;
}

bool UARPG_AIInteractionExecutorComponent::StartConversation()
{
    if (!IsTargetValid())
    {
        return false;
    }
    
    FaceTarget();
    SetInteractionState(EARPG_InteractionState::Talking);
    OnConversationStarted(InteractionTarget.Get());
    
    LogInteractionDebug(TEXT("Started conversation"));
    return true;
}

bool UARPG_AIInteractionExecutorComponent::StartTrading()
{
    if (!IsTargetValid())
    {
        return false;
    }
    
    FaceTarget();
    SetInteractionState(EARPG_InteractionState::Trading);
    OnTradingStarted(InteractionTarget.Get());
    
    LogInteractionDebug(TEXT("Started trading"));
    return true;
}

bool UARPG_AIInteractionExecutorComponent::InteractWithObject()
{
    if (!IsTargetValid())
    {
        return false;
    }
    
    SetInteractionState(EARPG_InteractionState::Interacting);
    OnObjectInteraction(InteractionTarget.Get());
    
    // Try to interact using the interface
    if (IInteractableInterface* InteractableInterface = Cast<IInteractableInterface>(InteractionTarget.Get()))
    {
        FVector InteractionPoint = InteractionTarget->GetActorLocation();
        FVector InteractionNormal = FVector::UpVector;
        
        if (ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner()))
        {
            InteractableInterface->OnInteract(Character, InteractionPoint, InteractionNormal);
        }
    }
    
    LogInteractionDebug(TEXT("Interacted with object"));
    return true;
}

void UARPG_AIInteractionExecutorComponent::FaceTarget()
{
    if (!IsTargetValid())
    {
        return;
    }
    
    FVector OwnerLocation = GetOwner()->GetActorLocation();
    FVector TargetLocation = InteractionTarget->GetActorLocation();
    FVector Direction = (TargetLocation - OwnerLocation).GetSafeNormal();
    
    if (!Direction.IsZero())
    {
        FRotator LookRotation = Direction.Rotation();
        GetOwner()->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f));
    }
}

AAIController* UARPG_AIInteractionExecutorComponent::GetAIController() const
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

void UARPG_AIInteractionExecutorComponent::UpdateInteractionState(float DeltaTime)
{
    switch (CurrentInteractionState)
    {
        case EARPG_InteractionState::Idle:
            HandleIdleState(DeltaTime);
            break;
            
        case EARPG_InteractionState::Approaching:
            HandleApproachingState(DeltaTime);
            break;
            
        case EARPG_InteractionState::Interacting:
            HandleInteractingState(DeltaTime);
            break;
            
        case EARPG_InteractionState::Talking:
            HandleTalkingState(DeltaTime);
            break;
            
        case EARPG_InteractionState::Trading:
            HandleTradingState(DeltaTime);
            break;
            
        case EARPG_InteractionState::Failed:
            HandleFailedState(DeltaTime);
            break;
    }
}

void UARPG_AIInteractionExecutorComponent::HandleIdleState(float DeltaTime)
{
    // Nothing to do in idle state
}

void UARPG_AIInteractionExecutorComponent::HandleApproachingState(float DeltaTime)
{
    if (!IsTargetValid())
    {
        SetInteractionState(EARPG_InteractionState::Failed);
        return;
    }
    
    if (IsTargetInRange())
    {
        // Reached target, start appropriate interaction
        if (ExecutingIntent.IntentTag.MatchesTag(TAG_AI_Intent_Social_Trade))
        {
            StartTrading();
        }
        else if (ExecutingIntent.IntentTag.MatchesTag(TAG_AI_Intent_Social_Talk))
        {
            StartConversation();
        }
        else
        {
            InteractWithObject();
        }
    }
}

void UARPG_AIInteractionExecutorComponent::HandleInteractingState(float DeltaTime)
{
    // Object interaction duration handled in UpdateExecution_Implementation
}

void UARPG_AIInteractionExecutorComponent::HandleTalkingState(float DeltaTime)
{
    // Conversation duration handled in UpdateExecution_Implementation
    FaceTarget(); // Keep facing target during conversation
}

void UARPG_AIInteractionExecutorComponent::HandleTradingState(float DeltaTime)
{
    // Trading duration handled in UpdateExecution_Implementation
    FaceTarget(); // Keep facing target during trade
}

void UARPG_AIInteractionExecutorComponent::HandleFailedState(float DeltaTime)
{
    // Remain in failed state until new intent starts
}

void UARPG_AIInteractionExecutorComponent::SetInteractionState(EARPG_InteractionState NewState)
{
    if (CurrentInteractionState != NewState)
    {
        EARPG_InteractionState OldState = CurrentInteractionState;
        CurrentInteractionState = NewState;
        StateTime = 0.0f;
        
        LogInteractionDebug(FString::Printf(TEXT("Interaction state changed from %d to %d"), (int32)OldState, (int32)NewState));
    }
}

float UARPG_AIInteractionExecutorComponent::CalculateInteractionProgress() const
{
    float TotalDuration = GetCurrentStateDuration();
    if (TotalDuration > 0.0f)
    {
        return FMath::Clamp(StateTime / TotalDuration, 0.0f, 1.0f);
    }
    return 0.0f;
}

float UARPG_AIInteractionExecutorComponent::GetCurrentStateDuration() const
{
    switch (CurrentInteractionState)
    {
        case EARPG_InteractionState::Talking:
            return InteractionConfig.ConversationDuration;
        case EARPG_InteractionState::Trading:
            return InteractionConfig.TradeDuration;
        case EARPG_InteractionState::Interacting:
            return InteractionConfig.ObjectInteractionTime;
        default:
            return 0.0f;
    }
}

void UARPG_AIInteractionExecutorComponent::LogInteractionDebug(const FString& Message) const
{
    if (InteractionConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("[InteractionExecutor] %s"), *Message);
    }
}

void UARPG_AIInteractionExecutorComponent::DrawDebugVisualization() const
{
    if (!GetWorld())
    {
        return;
    }
    
    FVector OwnerLocation = GetOwner()->GetActorLocation();
    
    // Draw interaction range
    DrawDebugCircle(GetWorld(), OwnerLocation, InteractionConfig.MaxInteractionRange, 32, FColor::Blue, false, -1.0f, 0, 2.0f, FVector(1,0,0), FVector(0,1,0));
    
    // Draw target information
    if (IsTargetValid())
    {
        FVector TargetLocation = InteractionTarget->GetActorLocation();
        
        // Line to target
        DrawDebugLine(GetWorld(), OwnerLocation, TargetLocation, FColor::Green, false, -1.0f, 0, 3.0f);
        
        // Target marker
        DrawDebugSphere(GetWorld(), TargetLocation, 25.0f, 12, FColor::Blue, false, -1.0f);
        
        // Display target info
        FString TargetInfo = FString::Printf(TEXT("Target: %s\nDistance: %.0fm"), 
            *InteractionTarget->GetName(), GetDistanceToTarget());
        DrawDebugString(GetWorld(), TargetLocation + FVector(0,0,100), TargetInfo, nullptr, FColor::White, -1.0f);
    }
    
    // Draw state info
    FString StateInfo = FString::Printf(TEXT("Interaction State: %d\nTime: %.1f"), 
        (int32)CurrentInteractionState, StateTime);
    DrawDebugString(GetWorld(), OwnerLocation + FVector(0,0,180), StateInfo, nullptr, FColor::Blue, -1.0f);
}