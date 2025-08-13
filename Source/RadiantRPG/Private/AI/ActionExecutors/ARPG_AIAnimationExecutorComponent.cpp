// Source/RadiantRPG/Private/AI/Core/ARPG_AIAnimationExecutorComponent.cpp

#include "AI/ActionExecutors/ARPG_AIAnimationExecutorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Core/RadiantGameplayTags.h"

UARPG_AIAnimationExecutorComponent::UARPG_AIAnimationExecutorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f;

    // Initialize configuration
    AnimationConfig.DefaultPlayRate = 1.0f;
    AnimationConfig.IdleAnimationInterval = 8.0f;
    AnimationConfig.IdleTimingVariance = 3.0f;
    AnimationConfig.bAllowInterruption = true;
    AnimationConfig.bEnableDebugLogging = false;
    AnimationConfig.bShowAnimationDebug = false;

    // Initialize state
    CurrentAnimationState = EARPG_AnimationState::Idle;
    CurrentMontage = nullptr;
    StateTime = 0.0f;
    TimeToNextIdle = 0.0f;
    CurrentAnimationPriority = 0;
    bCurrentAnimationInterrupted = false;
    MontageInstanceID = INDEX_NONE;
}

void UARPG_AIAnimationExecutorComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Cache skeletal mesh component
    if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
        CachedSkeletalMesh = Character->GetMesh();
    }
    else
    {
        CachedSkeletalMesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
    }

    // Calculate initial idle time
    CalculateNextIdleTime();

    LogAnimationDebug(TEXT("Animation executor initialized"));
}

void UARPG_AIAnimationExecutorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    StateTime += DeltaTime;
    
    // Update idle timer
    if (CurrentAnimationState == EARPG_AnimationState::Idle)
    {
        TimeToNextIdle -= DeltaTime;
        if (TimeToNextIdle <= 0.0f && IdleAnimations.Num() > 0)
        {
            PlayRandomIdleAnimation();
        }
    }
    
    UpdateAnimationState(DeltaTime);
    
    if (AnimationConfig.bShowAnimationDebug)
    {
        DrawDebugInfo();
    }
}

bool UARPG_AIAnimationExecutorComponent::CanExecuteIntent_Implementation(const FARPG_AIIntent& Intent)
{
    // Check if we have an animation mapping for this intent
    FARPG_AnimationMapping Mapping = FindAnimationMapping(Intent.IntentTag);
    if (Mapping.AnimationMontage || Mapping.AnimationSequence)
    {
        return true;
    }
    
    // Check for supported intent types that don't require specific animations
    return Intent.IntentTag.MatchesTag(TAG_AI_Intent_Idle) ||
           Intent.IntentTag.MatchesTag(TAG_AI_Intent_Social) ||
           Intent.IntentTag.MatchesTag(TAG_AI_Intent_Curiosity_Watch);
}

bool UARPG_AIAnimationExecutorComponent::StartExecution_Implementation(const FARPG_AIIntent& Intent)
{
    if (!CanExecuteIntent_Implementation(Intent))
    {
        return false;
    }

    ExecutingIntent = Intent;
    
    // Try to play animation for this intent
    bool bSuccess = PlayAnimationForIntent(Intent);
    
    if (bSuccess)
    {
        LogAnimationDebug(FString::Printf(TEXT("Started executing animation intent: %s"), *Intent.IntentTag.ToString()));
    }
    else
    {
        // Even if no specific animation, we can handle certain intents
        if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Idle))
        {
            SetAnimationState(EARPG_AnimationState::Waiting);
            bSuccess = true;
        }
        else if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Social))
        {
            bSuccess = PlayRandomGesture();
        }
        else if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Curiosity_Watch))
        {
            bSuccess = PlayRandomEmote();
        }
        
        if (!bSuccess)
        {
            LogAnimationDebug(FString::Printf(TEXT("Failed to start animation intent: %s"), *Intent.IntentTag.ToString()));
            SetAnimationState(EARPG_AnimationState::Failed);
        }
    }
    
    return bSuccess;
}

EARPG_BehaviorExecutionStatus UARPG_AIAnimationExecutorComponent::UpdateExecution_Implementation(float DeltaTime)
{
    switch (CurrentAnimationState)
    {
        case EARPG_AnimationState::Idle:
            return EARPG_BehaviorExecutionStatus::NotStarted;
            
        case EARPG_AnimationState::Playing:
        case EARPG_AnimationState::Blending:
            return EARPG_BehaviorExecutionStatus::InProgress;
            
        case EARPG_AnimationState::Waiting:
            // Check if wait time has elapsed (for intents that don't have specific animations)
            if (StateTime >= 2.0f)
            {
                SetAnimationState(EARPG_AnimationState::Idle);
                return EARPG_BehaviorExecutionStatus::Succeeded;
            }
            return EARPG_BehaviorExecutionStatus::InProgress;
            
        case EARPG_AnimationState::Failed:
            return EARPG_BehaviorExecutionStatus::Failed;
            
        default:
            return EARPG_BehaviorExecutionStatus::Failed;
    }
}

FARPG_BehaviorExecutionResult UARPG_AIAnimationExecutorComponent::StopExecution_Implementation(bool bForceStop)
{
    FARPG_BehaviorExecutionResult Result;
    Result.Status = UpdateExecution_Implementation(0.0f);
    Result.ExecutionTime = StateTime;
    Result.CompletionRatio = GetExecutionProgress_Implementation();
    
    if (bForceStop || CurrentAnimationState == EARPG_AnimationState::Playing)
    {
        StopCurrentAnimation();
        Result.Status = EARPG_BehaviorExecutionStatus::Interrupted;
        Result.ResultMessage = TEXT("Animation interrupted");
    }
    
    SetAnimationState(EARPG_AnimationState::Idle);
    
    return Result;
}

float UARPG_AIAnimationExecutorComponent::GetExecutionProgress_Implementation() const
{
    if (CurrentAnimationState == EARPG_AnimationState::Playing && CurrentMontage)
    {
        if (USkeletalMeshComponent* SkeletalMesh = GetSkeletalMeshComponent())
        {
            if (UAnimInstance* AnimInstance = SkeletalMesh->GetAnimInstance())
            {
                float MontageLength = CurrentMontage->GetPlayLength();
                float CurrentTime = AnimInstance->Montage_GetPosition(CurrentMontage);
                if (MontageLength > 0.0f)
                {
                    return FMath::Clamp(CurrentTime / MontageLength, 0.0f, 1.0f);
                }
            }
        }
    }
    else if (CurrentAnimationState == EARPG_AnimationState::Waiting)
    {
        return FMath::Clamp(StateTime / 2.0f, 0.0f, 1.0f);
    }
    
    return CurrentAnimationState == EARPG_AnimationState::Failed ? 0.0f : 1.0f;
}

bool UARPG_AIAnimationExecutorComponent::IsExecuting_Implementation() const
{
    return CurrentAnimationState == EARPG_AnimationState::Playing || 
           CurrentAnimationState == EARPG_AnimationState::Blending ||
           CurrentAnimationState == EARPG_AnimationState::Waiting;
}

FARPG_AIIntent UARPG_AIAnimationExecutorComponent::GetCurrentIntent_Implementation() const
{
    return ExecutingIntent;
}

TArray<FGameplayTag> UARPG_AIAnimationExecutorComponent::GetSupportedIntentTypes_Implementation() const
{
    TArray<FGameplayTag> SupportedTypes;
    
    // Add intent tags from animation mappings
    for (const FARPG_AnimationMapping& Mapping : AnimationMappings)
    {
        if (Mapping.IntentTag.IsValid())
        {
            SupportedTypes.AddUnique(Mapping.IntentTag);
        }
    }
    
    // Add default supported types
    SupportedTypes.AddUnique(TAG_AI_Intent_Idle);
    SupportedTypes.AddUnique(TAG_AI_Intent_Social);
    SupportedTypes.AddUnique(TAG_AI_Intent_Curiosity_Watch);
    
    return SupportedTypes;
}

int32 UARPG_AIAnimationExecutorComponent::GetExecutionPriority_Implementation(const FARPG_AIIntent& Intent) const
{
    // Animation executor has low priority compared to movement/combat
    FARPG_AnimationMapping Mapping = FindAnimationMapping(Intent.IntentTag);
    return Mapping.Priority > 0 ? Mapping.Priority : 2;
}

bool UARPG_AIAnimationExecutorComponent::PlayAnimationForIntent(const FARPG_AIIntent& Intent)
{
    FARPG_AnimationMapping Mapping = FindAnimationMapping(Intent.IntentTag);
    
    if (Mapping.AnimationMontage)
    {
        CurrentMapping = Mapping;
        return PlayMontage(Mapping.AnimationMontage, Mapping.PlayRate, Mapping.bCanBeInterrupted);
    }
    
    // No specific animation found
    return false;
}

bool UARPG_AIAnimationExecutorComponent::PlayMontage(UAnimMontage* Montage, float PlayRate, bool bCanInterrupt)
{
    if (!Montage || !CachedSkeletalMesh)
    {
        return false;
    }
    
    UAnimInstance* AnimInstance = CachedSkeletalMesh->GetAnimInstance();
    if (!AnimInstance)
    {
        return false;
    }
    
    // Check if we can interrupt current animation
    if (IsAnimationPlaying() && !CanInterruptCurrentAnimation())
    {
        LogAnimationDebug(TEXT("Cannot interrupt current animation"));
        return false;
    }
    
    // Stop current animation if playing
    if (IsAnimationPlaying())
    {
        StopCurrentAnimation(0.25f);
    }
    
    // Play the new montage
    float MontageLength = AnimInstance->Montage_Play(Montage, PlayRate);
    
    if (MontageLength > 0.0f)
    {
        CurrentMontage = Montage;
        CurrentAnimationPriority = CurrentMapping.Priority;
        SetAnimationState(EARPG_AnimationState::Playing);
        
        // Bind montage events
        AnimInstance->OnMontageEnded.AddDynamic(this, &UARPG_AIAnimationExecutorComponent::OnMontageEnded);
        AnimInstance->OnMontageBlendingOut.AddDynamic(this, &UARPG_AIAnimationExecutorComponent::OnMontageBlendingOut);
        
        OnAnimationStarted(Montage, ExecutingIntent.IntentTag);
        LogAnimationDebug(FString::Printf(TEXT("Started playing montage: %s"), *Montage->GetName()));
        
        return true;
    }
    
    return false;
}

void UARPG_AIAnimationExecutorComponent::StopCurrentAnimation(float BlendOutTime)
{
    if (!IsAnimationPlaying() || !CachedSkeletalMesh)
    {
        return;
    }
    
    UAnimInstance* AnimInstance = CachedSkeletalMesh->GetAnimInstance();
    if (AnimInstance && CurrentMontage)
    {
        AnimInstance->Montage_Stop(BlendOutTime, CurrentMontage);
        bCurrentAnimationInterrupted = true;
        LogAnimationDebug(TEXT("Stopped current animation"));
    }
}

bool UARPG_AIAnimationExecutorComponent::PlayRandomIdleAnimation()
{
    if (IdleAnimations.IsEmpty())
    {
        return false;
    }
    
    UAnimMontage* RandomIdle = IdleAnimations[FMath::RandRange(0, IdleAnimations.Num() - 1)];
    if (PlayMontage(RandomIdle))
    {
        OnIdleAnimationStarted(RandomIdle);
        CalculateNextIdleTime();
        LogAnimationDebug(TEXT("Playing random idle animation"));
        return true;
    }
    
    return false;
}

bool UARPG_AIAnimationExecutorComponent::PlayRandomGesture()
{
    if (GestureAnimations.IsEmpty())
    {
        return false;
    }
    
    UAnimMontage* RandomGesture = GestureAnimations[FMath::RandRange(0, GestureAnimations.Num() - 1)];
    if (PlayMontage(RandomGesture))
    {
        OnGesturePerformed(RandomGesture);
        LogAnimationDebug(TEXT("Playing random gesture"));
        return true;
    }
    
    return false;
}

bool UARPG_AIAnimationExecutorComponent::PlayRandomEmote()
{
    if (EmoteAnimations.IsEmpty())
    {
        return false;
    }
    
    UAnimMontage* RandomEmote = EmoteAnimations[FMath::RandRange(0, EmoteAnimations.Num() - 1)];
    if (PlayMontage(RandomEmote))
    {
        OnEmotePerformed(RandomEmote);
        LogAnimationDebug(TEXT("Playing random emote"));
        return true;
    }
    
    return false;
}

FARPG_AnimationMapping UARPG_AIAnimationExecutorComponent::FindAnimationMapping(const FGameplayTag& IntentTag) const
{
    for (const FARPG_AnimationMapping& Mapping : AnimationMappings)
    {
        if (Mapping.IntentTag.MatchesTagExact(IntentTag) || IntentTag.MatchesTag(Mapping.IntentTag))
        {
            return Mapping;
        }
    }
    
    return FARPG_AnimationMapping();
}

bool UARPG_AIAnimationExecutorComponent::IsAnimationPlaying() const
{
    if (!CachedSkeletalMesh || !CurrentMontage)
    {
        return false;
    }
    
    UAnimInstance* AnimInstance = CachedSkeletalMesh->GetAnimInstance();
    return AnimInstance && AnimInstance->Montage_IsPlaying(CurrentMontage);
}

float UARPG_AIAnimationExecutorComponent::GetRemainingAnimationTime() const
{
    if (!IsAnimationPlaying())
    {
        return 0.0f;
    }
    
    UAnimInstance* AnimInstance = CachedSkeletalMesh->GetAnimInstance();
    if (AnimInstance && CurrentMontage)
    {
        float CurrentTime = AnimInstance->Montage_GetPosition(CurrentMontage);
        float TotalLength = CurrentMontage->GetPlayLength();
        return FMath::Max(0.0f, TotalLength - CurrentTime);
    }
    
    return 0.0f;
}

bool UARPG_AIAnimationExecutorComponent::CanInterruptCurrentAnimation() const
{
    if (!IsAnimationPlaying())
    {
        return true;
    }
    
    return AnimationConfig.bAllowInterruption && CurrentMapping.bCanBeInterrupted;
}

USkeletalMeshComponent* UARPG_AIAnimationExecutorComponent::GetSkeletalMeshComponent() const
{
    return CachedSkeletalMesh;
}

void UARPG_AIAnimationExecutorComponent::UpdateAnimationState(float DeltaTime)
{
    switch (CurrentAnimationState)
    {
        case EARPG_AnimationState::Idle:
            HandleIdleState(DeltaTime);
            break;
            
        case EARPG_AnimationState::Playing:
            HandlePlayingState(DeltaTime);
            break;
            
        case EARPG_AnimationState::Blending:
            HandleBlendingState(DeltaTime);
            break;
            
        case EARPG_AnimationState::Waiting:
            HandleWaitingState(DeltaTime);
            break;
            
        case EARPG_AnimationState::Failed:
            HandleFailedState(DeltaTime);
            break;
    }
}

void UARPG_AIAnimationExecutorComponent::HandleIdleState(float DeltaTime)
{
    // Idle state handled in TickComponent for idle animations
}

void UARPG_AIAnimationExecutorComponent::HandlePlayingState(float DeltaTime)
{
    // Check if animation is still playing
    if (!IsAnimationPlaying())
    {
        // Animation finished naturally
        SetAnimationState(EARPG_AnimationState::Idle);
        OnAnimationCompleted(CurrentMontage, bCurrentAnimationInterrupted);
        CurrentMontage = nullptr;
        bCurrentAnimationInterrupted = false;
    }
}

void UARPG_AIAnimationExecutorComponent::HandleBlendingState(float DeltaTime)
{
    // Check if blend is complete
    if (!IsAnimationPlaying())
    {
        SetAnimationState(EARPG_AnimationState::Idle);
    }
}

void UARPG_AIAnimationExecutorComponent::HandleWaitingState(float DeltaTime)
{
    // Waiting state duration handled in UpdateExecution_Implementation
}

void UARPG_AIAnimationExecutorComponent::HandleFailedState(float DeltaTime)
{
    // Remain in failed state until new intent starts
}

void UARPG_AIAnimationExecutorComponent::SetAnimationState(EARPG_AnimationState NewState)
{
    if (CurrentAnimationState != NewState)
    {
        EARPG_AnimationState OldState = CurrentAnimationState;
        CurrentAnimationState = NewState;
        StateTime = 0.0f;
        
        LogAnimationDebug(FString::Printf(TEXT("Animation state changed from %d to %d"), (int32)OldState, (int32)NewState));
    }
}

void UARPG_AIAnimationExecutorComponent::CalculateNextIdleTime()
{
    float BaseTime = AnimationConfig.IdleAnimationInterval;
    float Variance = FMath::RandRange(-AnimationConfig.IdleTimingVariance, AnimationConfig.IdleTimingVariance);
    TimeToNextIdle = FMath::Max(1.0f, BaseTime + Variance);
    
    LogAnimationDebug(FString::Printf(TEXT("Next idle animation in %.1f seconds"), TimeToNextIdle));
}

void UARPG_AIAnimationExecutorComponent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage == CurrentMontage)
    {
        bCurrentAnimationInterrupted = bInterrupted;
        LogAnimationDebug(FString::Printf(TEXT("Montage ended: %s (Interrupted: %s)"), 
            *Montage->GetName(), bInterrupted ? TEXT("Yes") : TEXT("No")));
    }
}

void UARPG_AIAnimationExecutorComponent::OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage == CurrentMontage)
    {
        SetAnimationState(EARPG_AnimationState::Blending);
        LogAnimationDebug(FString::Printf(TEXT("Montage blending out: %s"), *Montage->GetName()));
    }
}

void UARPG_AIAnimationExecutorComponent::LogAnimationDebug(const FString& Message) const
{
    if (AnimationConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("[AnimationExecutor] %s"), *Message);
    }
}

void UARPG_AIAnimationExecutorComponent::DrawDebugInfo() const
{
    if (!GetWorld())
    {
        return;
    }
    
    FVector OwnerLocation = GetOwner()->GetActorLocation();
    
    // Draw animation state info
    FString StateInfo = FString::Printf(TEXT("Animation State: %d\nMontage: %s\nTime: %.1f"), 
        (int32)CurrentAnimationState,
        CurrentMontage ? *CurrentMontage->GetName() : TEXT("None"),
        StateTime);
    
    DrawDebugString(GetWorld(), OwnerLocation + FVector(0,0,250), StateInfo, nullptr, FColor::Magenta, -1.0f);
    
    // Draw idle timer
    if (CurrentAnimationState == EARPG_AnimationState::Idle)
    {
        FString IdleInfo = FString::Printf(TEXT("Next Idle: %.1fs"), TimeToNextIdle);
        DrawDebugString(GetWorld(), OwnerLocation + FVector(0,0,220), IdleInfo, nullptr, FColor::Green, -1.0f);
    }
    
    // Draw animation progress
    if (IsAnimationPlaying())
    {
        float Progress = GetExecutionProgress_Implementation();
        FString ProgressInfo = FString::Printf(TEXT("Progress: %.1f%%"), Progress * 100.0f);
        DrawDebugString(GetWorld(), OwnerLocation + FVector(0,0,190), ProgressInfo, nullptr, FColor::Yellow, -1.0f);
    }
}