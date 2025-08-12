// Source/RadiantRPG/Private/AI/Core/ARPG_AIBehaviorExecutorComponent.cpp

#include "AI/Core/ARPG_AIBehaviorExecutorComponent.h"
#include "AI/Core/ARPG_AIMovementExecutorComponent.h"
#include "AI/Core/ARPG_AICombatExecutorComponent.h"
#include "AI/Core/ARPG_AIInteractionExecutorComponent.h"
#include "AI/Core/ARPG_AIAnimationExecutorComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Core/RadiantGameplayTags.h"

UARPG_AIBehaviorExecutorComponent::UARPG_AIBehaviorExecutorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f; // 10 FPS for behavior execution

    // Initialize default configuration
    ExecutorConfig.bEnableDebugLogging = false;
    ExecutorConfig.bEnableDebugVisualization = false;
    ExecutorConfig.MaxExecutionTime = 10.0f;
    ExecutorConfig.UrgentIntentPriorityBoost = 2.0f;
    ExecutorConfig.ExecutionUpdateFrequency = 0.2f;

    // Initialize state
    ExecutionStatus = EARPG_BehaviorExecutionStatus::NotStarted;
    CurrentExecutionTime = 0.0f;
    LastExecutionUpdate = 0.0f;
    CurrentExecutionProgress = 0.0f;

    // Setup supported intent types
    SupportedIntentTypes.Add(TAG_AI_Intent_Idle);
    SupportedIntentTypes.Add(TAG_AI_Intent_Wander);
    SupportedIntentTypes.Add(TAG_AI_Intent_Patrol);
    SupportedIntentTypes.Add(TAG_AI_Intent_Guard);
    SupportedIntentTypes.Add(TAG_AI_Intent_Curiosity_Explore);
    SupportedIntentTypes.Add(TAG_AI_Intent_Curiosity_Watch);
    SupportedIntentTypes.Add(TAG_AI_Intent_Combat_Attack);
    SupportedIntentTypes.Add(TAG_AI_Intent_Combat_Defend);
    SupportedIntentTypes.Add(TAG_AI_Intent_Combat_Retreat);
    SupportedIntentTypes.Add(TAG_AI_Intent_Social_Talk);
    SupportedIntentTypes.Add(TAG_AI_Intent_Social_Follow);
    SupportedIntentTypes.Add(TAG_AI_Intent_Survival_Flee);
}

void UARPG_AIBehaviorExecutorComponent::BeginPlay()
{
    Super::BeginPlay();
    InitializeSubExecutors();
}

void UARPG_AIBehaviorExecutorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Only tick if we're executing something
    if (ExecutionStatus == EARPG_BehaviorExecutionStatus::InProgress)
    {
        // Check for timeout
        CurrentExecutionTime += DeltaTime;
        if (HasExecutionTimedOut())
        {
            HandleExecutionTimeout();
            return;
        }

        // Update execution progress periodically
        LastExecutionUpdate += DeltaTime;
        if (LastExecutionUpdate >= ExecutorConfig.ExecutionUpdateFrequency)
        {
            UpdateExecutionProgress();
            LastExecutionUpdate = 0.0f;
        }
    }

    // Process next intent if we're idle and have queued intents
    if (ExecutionStatus == EARPG_BehaviorExecutionStatus::NotStarted || 
        ExecutionStatus == EARPG_BehaviorExecutionStatus::Succeeded ||
        ExecutionStatus == EARPG_BehaviorExecutionStatus::Failed)
    {
        ProcessNextIntent();
    }
}

bool UARPG_AIBehaviorExecutorComponent::CanExecuteIntent_Implementation(const FARPG_AIIntent& Intent)
{
    // Check if intent type is supported
    for (const FGameplayTag& SupportedType : SupportedIntentTypes)
    {
        if (Intent.IntentTag.MatchesTag(SupportedType))
        {
            return true;
        }
    }
    return false;
}

bool UARPG_AIBehaviorExecutorComponent::StartExecution_Implementation(const FARPG_AIIntent& Intent)
{
    if (!CanExecuteIntent_Implementation(Intent))
    {
        LogExecutionDebug(FString::Printf(TEXT("Cannot execute unsupported intent: %s"), *Intent.IntentTag.ToString()));
        return false;
    }

    // Stop current execution if running
    if (ExecutionStatus == EARPG_BehaviorExecutionStatus::InProgress)
    {
        StopExecution_Implementation(true);
    }

    // Set new intent as current
    CurrentIntent = Intent;
    ExecutionStatus = EARPG_BehaviorExecutionStatus::InProgress;
    CurrentExecutionTime = 0.0f;
    CurrentExecutionProgress = 0.0f;
    ActiveSubExecutor = nullptr;

    // Determine which sub-executor should handle this intent
    UActorComponent* SubExecutor = DetermineSubExecutor(Intent);
    if (!SubExecutor)
    {
        LogExecutionDebug(FString::Printf(TEXT("No sub-executor found for intent: %s"), *Intent.IntentTag.ToString()));
        ExecutionStatus = EARPG_BehaviorExecutionStatus::Failed;
        return false;
    }

    ActiveSubExecutor = SubExecutor;

    // Try to start execution on sub-executor
    if (IARPG_AIBehaviorExecutorInterface* ExecutorInterface = Cast<IARPG_AIBehaviorExecutorInterface>(SubExecutor))
    {
        if (ExecutorInterface->StartExecution(Intent))
        {
            OnIntentExecutionStarted(Intent);
            LogExecutionDebug(FString::Printf(TEXT("Started executing intent: %s"), *Intent.IntentTag.ToString()));
            return true;
        }
    }

    // Failed to start execution
    ExecutionStatus = EARPG_BehaviorExecutionStatus::Failed;
    ActiveSubExecutor = nullptr;
    LogExecutionDebug(FString::Printf(TEXT("Failed to start execution for intent: %s"), *Intent.IntentTag.ToString()));
    return false;
}

EARPG_BehaviorExecutionStatus UARPG_AIBehaviorExecutorComponent::UpdateExecution_Implementation(float DeltaTime)
{
    if (ExecutionStatus != EARPG_BehaviorExecutionStatus::InProgress)
    {
        return ExecutionStatus;
    }

    if (!ActiveSubExecutor)
    {
        ExecutionStatus = EARPG_BehaviorExecutionStatus::Failed;
        return ExecutionStatus;
    }

    // Update the active sub-executor
    if (IARPG_AIBehaviorExecutorInterface* ExecutorInterface = Cast<IARPG_AIBehaviorExecutorInterface>(ActiveSubExecutor))
    {
        EARPG_BehaviorExecutionStatus SubStatus = ExecutorInterface->UpdateExecution(DeltaTime);
        
        if (SubStatus != EARPG_BehaviorExecutionStatus::InProgress)
        {
            // Sub-executor has finished
            ExecutionStatus = SubStatus;
            OnIntentExecutionCompleted(CurrentIntent, SubStatus);
            
            if (SubStatus == EARPG_BehaviorExecutionStatus::Succeeded)
            {
                LogExecutionDebug(FString::Printf(TEXT("Successfully completed intent: %s"), *CurrentIntent.IntentTag.ToString()));
            }
            else
            {
                LogExecutionDebug(FString::Printf(TEXT("Failed to complete intent: %s"), *CurrentIntent.IntentTag.ToString()));
            }
        }
    }

    return ExecutionStatus;
}

FARPG_BehaviorExecutionResult UARPG_AIBehaviorExecutorComponent::StopExecution_Implementation(bool bForceStop)
{
    FARPG_BehaviorExecutionResult Result;
    Result.Status = ExecutionStatus;
    Result.ExecutionTime = CurrentExecutionTime;
    Result.CompletionRatio = CurrentExecutionProgress;

    if (ActiveSubExecutor)
    {
        if (IARPG_AIBehaviorExecutorInterface* ExecutorInterface = Cast<IARPG_AIBehaviorExecutorInterface>(ActiveSubExecutor))
        {
            ExecutorInterface->StopExecution(bForceStop);
        }
        ActiveSubExecutor = nullptr;
    }

    if (ExecutionStatus == EARPG_BehaviorExecutionStatus::InProgress)
    {
        ExecutionStatus = EARPG_BehaviorExecutionStatus::Interrupted;
        Result.Status = EARPG_BehaviorExecutionStatus::Interrupted;
        Result.ResultMessage = bForceStop ? TEXT("Force stopped") : TEXT("Gracefully stopped");
        
        OnIntentExecutionInterrupted(CurrentIntent, Result.ResultMessage);
        LogExecutionDebug(FString::Printf(TEXT("Stopped execution of intent: %s"), *CurrentIntent.IntentTag.ToString()));
    }

    return Result;
}

float UARPG_AIBehaviorExecutorComponent::GetExecutionProgress_Implementation() const
{
    return CurrentExecutionProgress;
}

bool UARPG_AIBehaviorExecutorComponent::IsExecuting_Implementation() const
{
    return ExecutionStatus == EARPG_BehaviorExecutionStatus::InProgress;
}

FARPG_AIIntent UARPG_AIBehaviorExecutorComponent::GetCurrentIntent_Implementation() const
{
    return CurrentIntent;
}

TArray<FGameplayTag> UARPG_AIBehaviorExecutorComponent::GetSupportedIntentTypes_Implementation() const
{
    return SupportedIntentTypes;
}

int32 UARPG_AIBehaviorExecutorComponent::GetExecutionPriority_Implementation(const FARPG_AIIntent& Intent) const
{
    int32 BasePriority = 1;

    // Higher priority for urgent intents
    if (static_cast<int32>(Intent.Priority) > static_cast<int32>(EARPG_AIIntentPriority::Idle))
    {
        BasePriority += static_cast<int32>(Intent.Priority);
    }

    // Combat intents get priority boost
    if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Combat))
    {
        BasePriority += 2;
    }

    // Survival intents get highest priority
    if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Survival))
    {
        BasePriority += 3;
    }

    return BasePriority;
}

void UARPG_AIBehaviorExecutorComponent::QueueIntent(const FARPG_AIIntent& Intent)
{
    if (IntentQueue.Num() >= MaxQueueSize)
    {
        LogExecutionDebug(TEXT("Intent queue is full, cannot queue new intent"));
        return;
    }

    IntentQueue.Add(Intent);
    LogExecutionDebug(FString::Printf(TEXT("Queued intent: %s"), *Intent.IntentTag.ToString()));
}

void UARPG_AIBehaviorExecutorComponent::ClearIntentQueue()
{
    IntentQueue.Empty();
    LogExecutionDebug(TEXT("Cleared intent queue"));
}

bool UARPG_AIBehaviorExecutorComponent::CanQueueIntent() const
{
    return IntentQueue.Num() < MaxQueueSize;
}

int32 UARPG_AIBehaviorExecutorComponent::GetQueuedIntentCount() const
{
    return IntentQueue.Num();
}

void UARPG_AIBehaviorExecutorComponent::InitializeSubExecutors()
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        LogExecutionDebug(TEXT("No owner found, cannot initialize sub-executors"));
        return;
    }

    // Find or create Movement Executor
    MovementExecutor = Owner->FindComponentByClass<UARPG_AIMovementExecutorComponent>();
    if (!MovementExecutor)
    {
        MovementExecutor = NewObject<UARPG_AIMovementExecutorComponent>(Owner);
        Owner->AddInstanceComponent(MovementExecutor);
        MovementExecutor->RegisterComponent();
    }

    // Find or create Combat Executor
    CombatExecutor = Owner->FindComponentByClass<UARPG_AICombatExecutorComponent>();
    if (!CombatExecutor)
    {
        CombatExecutor = NewObject<UARPG_AICombatExecutorComponent>(Owner);
        Owner->AddInstanceComponent(CombatExecutor);
        CombatExecutor->RegisterComponent();
    }

    // Find or create Interaction Executor
    InteractionExecutor = Owner->FindComponentByClass<UARPG_AIInteractionExecutorComponent>();
    if (!InteractionExecutor)
    {
        InteractionExecutor = NewObject<UARPG_AIInteractionExecutorComponent>(Owner);
        Owner->AddInstanceComponent(InteractionExecutor);
        InteractionExecutor->RegisterComponent();
    }

    // Find or create Animation Executor
    AnimationExecutor = Owner->FindComponentByClass<UARPG_AIAnimationExecutorComponent>();
    if (!AnimationExecutor)
    {
        AnimationExecutor = NewObject<UARPG_AIAnimationExecutorComponent>(Owner);
        Owner->AddInstanceComponent(AnimationExecutor);
        AnimationExecutor->RegisterComponent();
    }

    LogExecutionDebug(TEXT("All sub-executors initialized"));
}

UActorComponent* UARPG_AIBehaviorExecutorComponent::GetExecutorForIntent(const FARPG_AIIntent& Intent)
{
    return DetermineSubExecutor(Intent);
}

bool UARPG_AIBehaviorExecutorComponent::AreSubExecutorsReady() const
{
    return MovementExecutor != nullptr && 
           CombatExecutor != nullptr && 
           InteractionExecutor != nullptr && 
           AnimationExecutor != nullptr;
}

void UARPG_AIBehaviorExecutorComponent::ProcessNextIntent()
{
    if (IntentQueue.IsEmpty())
    {
        return;
    }

    FARPG_AIIntent NextIntent = IntentQueue[0];
    IntentQueue.RemoveAt(0);

    StartExecution_Implementation(NextIntent);
}

UActorComponent* UARPG_AIBehaviorExecutorComponent::DetermineSubExecutor(const FARPG_AIIntent& Intent)
{
    // Combat-related intents - highest priority
    if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Combat))
    {
        return CombatExecutor;
    }

    // Movement-related intents
    if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Wander) ||
        Intent.IntentTag.MatchesTag(TAG_AI_Intent_Patrol) ||
        Intent.IntentTag.MatchesTag(TAG_AI_Intent_Curiosity_Explore) ||
        Intent.IntentTag.MatchesTag(TAG_AI_Intent_Social_Follow) ||
        Intent.IntentTag.MatchesTag(TAG_AI_Intent_Survival_Flee))
    {
        return MovementExecutor;
    }

    // Social/interaction intents with targets
    if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Social_Talk) ||
        Intent.IntentTag.MatchesTag(TAG_AI_Intent_Social_Trade) ||
        (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Social) && Intent.IntentParameters.Contains(TEXT("TargetActor"))))
    {
        return InteractionExecutor;
    }

    // Animation/emote intents (social without movement)
    if ((Intent.IntentTag.MatchesTag(TAG_AI_Intent_Social) && !Intent.IntentParameters.Contains(TEXT("TargetActor"))) ||
        Intent.IntentTag.MatchesTag(TAG_AI_Intent_Curiosity_Watch))
    {
        return AnimationExecutor;
    }

    // Default fallback intents - route to movement executor for basic behavior
    if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Idle) ||
        Intent.IntentTag.MatchesTag(TAG_AI_Intent_Guard))
    {
        return MovementExecutor;
    }

    // No suitable executor found
    return nullptr;
}

bool UARPG_AIBehaviorExecutorComponent::HasExecutionTimedOut() const
{
    return CurrentExecutionTime >= ExecutorConfig.MaxExecutionTime;
}

void UARPG_AIBehaviorExecutorComponent::HandleExecutionTimeout()
{
    LogExecutionDebug(FString::Printf(TEXT("Execution timeout for intent: %s"), *CurrentIntent.IntentTag.ToString()));
    
    FARPG_BehaviorExecutionResult Result = StopExecution_Implementation(true);
    Result.ResultMessage = TEXT("Execution timeout");
    
    ExecutionStatus = EARPG_BehaviorExecutionStatus::Failed;
    OnIntentExecutionInterrupted(CurrentIntent, TEXT("Execution timeout"));
}

void UARPG_AIBehaviorExecutorComponent::UpdateExecutionProgress()
{
    if (ActiveSubExecutor)
    {
        if (IARPG_AIBehaviorExecutorInterface* ExecutorInterface = Cast<IARPG_AIBehaviorExecutorInterface>(ActiveSubExecutor))
        {
            CurrentExecutionProgress = ExecutorInterface->GetExecutionProgress();
        }
    }
}

void UARPG_AIBehaviorExecutorComponent::LogExecutionDebug(const FString& Message) const
{
    if (ExecutorConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("[BehaviorExecutor] %s"), *Message);
    }
}