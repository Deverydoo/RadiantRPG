// Private/AI/Core/ARPG_AINeedsComponent.cpp
// AI-specific needs system implementation

#include "AI/Core/ARPG_AINeedsComponent.h"
#include "AI/Core/ARPG_AIBrainComponent.h"
#include "Engine/World.h"
#include "RadiantRPG.h"

UARPG_AINeedsComponent::UARPG_AINeedsComponent()
{
    // AI components typically update less frequently than player components
    PrimaryComponentTick.TickInterval = 1.0f;
    
    // Initialize default AI needs
    FARPG_AINeed HungerNeed;
    HungerNeed.NeedType = EARPG_NeedType::Hunger;
    HungerNeed.CurrentLevel = 0.3f;
    HungerNeed.ChangeRate = 0.002f;
    HungerNeed.CriticalThreshold = 0.7f;
    HungerNeed.DecayRate = 0.9f;
    HungerNeed.bIsActive = true;
    HungerNeed.bIsUrgent = false;
    DefaultNeeds.Add(HungerNeed);

    FARPG_AINeed FatigueNeed;
    FatigueNeed.NeedType = EARPG_NeedType::Fatigue;
    FatigueNeed.CurrentLevel = 0.2f;
    FatigueNeed.ChangeRate = 0.001f;
    FatigueNeed.CriticalThreshold = 0.7f;
    FatigueNeed.DecayRate = 0.9f;
    FatigueNeed.bIsActive = true;
    FatigueNeed.bIsUrgent = false;
    DefaultNeeds.Add(FatigueNeed);

    FARPG_AINeed SafetyNeed;
    SafetyNeed.NeedType = EARPG_NeedType::Safety;
    SafetyNeed.CurrentLevel = 0.1f;
    SafetyNeed.ChangeRate = 0.0f;
    SafetyNeed.CriticalThreshold = 0.6f;
    SafetyNeed.DecayRate = 0.8f;
    SafetyNeed.bIsActive = true;
    SafetyNeed.bIsUrgent = false;
    DefaultNeeds.Add(SafetyNeed);

    FARPG_AINeed SocialNeed;
    SocialNeed.NeedType = EARPG_NeedType::Social;
    SocialNeed.CurrentLevel = 0.4f;
    SocialNeed.ChangeRate = 0.0005f;
    SocialNeed.CriticalThreshold = 0.6f;
    SocialNeed.DecayRate = 0.8f;
    SocialNeed.bIsActive = true;
    SocialNeed.bIsUrgent = false;
    DefaultNeeds.Add(SocialNeed);
}

void UARPG_AINeedsComponent::BeginPlay()
{
    Super::BeginPlay();
    
    InitializeAIComponentReferences();
    
    // Initialize with default needs if none were set
    if (CurrentNeeds.Num() == 0)
    {
        for (const FARPG_AINeed& DefaultNeed : DefaultNeeds)
        {
            CurrentNeeds.Add(DefaultNeed.NeedType, DefaultNeed);
        }
    }
    
    UE_LOG(LogRadiantRPG, Log, TEXT("AINeedsComponent initialized for %s with %d needs"), 
           GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"), CurrentNeeds.Num());
}

void UARPG_AINeedsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    // AI-specific tick logic can go here if needed
    // Base class handles the main needs updates
}

TArray<FGameplayTag> UARPG_AINeedsComponent::GetNeedBasedIntentSuggestions() const
{
    TArray<FGameplayTag> IntentSuggestions;
    
    // Get urgent needs first
    TArray<EARPG_NeedType> UrgentNeeds = GetUrgentNeeds();
    for (EARPG_NeedType NeedType : UrgentNeeds)
    {
        FGameplayTag IntentTag = GetIntentTagForNeed(NeedType);
        if (IntentTag.IsValid())
        {
            IntentSuggestions.Add(IntentTag);
        }
        
        // Try Blueprint implementation for custom intents
        float UrgencyLevel = GetNeedLevel(NeedType);
        FGameplayTag BlueprintIntent = BP_GetIntentForNeed(NeedType, UrgencyLevel);
        if (BlueprintIntent.IsValid() && !IntentSuggestions.Contains(BlueprintIntent))
        {
            IntentSuggestions.Add(BlueprintIntent);
        }
    }
    
    return IntentSuggestions;
}

EARPG_NeedType UARPG_AINeedsComponent::GetMostUrgentNeed() const
{
    EARPG_NeedType MostUrgentNeed = EARPG_NeedType::Hunger; // Default
    float HighestUrgency = 0.0f;
    
    for (const auto& NeedPair : CurrentNeeds)
    {
        const FARPG_AINeed& Need = NeedPair.Value;
        if (!Need.bIsActive)
        {
            continue;
        }
        
        float Urgency = CalculateNeedUrgency(NeedPair.Key);
        if (Urgency > HighestUrgency)
        {
            HighestUrgency = Urgency;
            MostUrgentNeed = NeedPair.Key;
        }
    }
    
    return MostUrgentNeed;
}

bool UARPG_AINeedsComponent::HasInterruptingNeeds() const
{
    for (const auto& NeedPair : CurrentNeeds)
    {
        const FARPG_AINeed& Need = NeedPair.Value;
        if (Need.bIsActive && Need.bIsCritical)
        {
            return true;
        }
    }
    return false;
}

FGameplayTag UARPG_AINeedsComponent::GetIntentTagForNeed(EARPG_NeedType NeedType) const
{
    return GetDefaultIntentForNeed(NeedType);
}

void UARPG_AINeedsComponent::NotifyBrainOfNeedChange(EARPG_NeedType NeedType, float NewLevel)
{
    if (BrainComponent.IsValid())
    {
        bool bIsCritical = IsNeedCritical(NeedType);
        SendNeedUpdateToBrain(NeedType, NewLevel, bIsCritical);
    }
}

bool UARPG_AINeedsComponent::CanSatisfyNeed(EARPG_NeedType NeedType) const
{
    // Try Blueprint implementation first
    if (BP_CanSatisfyNeed(NeedType))
    {
        return true;
    }
    
    // Default logic - most needs can potentially be satisfied
    switch (NeedType)
    {
        case EARPG_NeedType::Hunger:
        case EARPG_NeedType::Fatigue:
        case EARPG_NeedType::Social:
            return true;
        case EARPG_NeedType::Safety:
            // Safety depends on environment, harder to satisfy
            return false;
        default:
            return true;
    }
}

void UARPG_AINeedsComponent::OnNeedValueChanged(EARPG_NeedType NeedType, float OldValue, float NewValue)
{
    Super::OnNeedValueChanged(NeedType, OldValue, NewValue);
    
    // AI-specific handling
    float UrgencyLevel = CalculateNeedUrgency(NeedType);
    bool bIsInterrupting = IsNeedCritical(NeedType);
    
    // Notify brain of the change
    NotifyBrainOfNeedChange(NeedType, NewValue);
    
    // Trigger AI-specific event
    BP_OnAINeedChanged(NeedType, UrgencyLevel, bIsInterrupting);
}

void UARPG_AINeedsComponent::OnNeedBecomeCritical(EARPG_NeedType NeedType)
{
    Super::OnNeedBecomeCritical(NeedType);
    
    // AI-specific critical need handling
    if (BrainComponent.IsValid())
    {
        // Send high priority stimulus to brain about critical need
        SendNeedUpdateToBrain(NeedType, GetNeedLevel(NeedType), true);
    }
    
    if (NeedsConfig.bEnableDebugLogging)
    {
        UE_LOG(LogRadiantRPG, Warning, TEXT("AI %s: Critical need %s requires immediate attention!"), 
               GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"),
               *UEnum::GetValueAsString(NeedType));
    }
}

void UARPG_AINeedsComponent::OnNeedNoLongerCritical(EARPG_NeedType NeedType)
{
    Super::OnNeedNoLongerCritical(NeedType);
    
    // Notify brain that need is no longer critical
    if (BrainComponent.IsValid())
    {
        SendNeedUpdateToBrain(NeedType, GetNeedLevel(NeedType), false);
    }
}

void UARPG_AINeedsComponent::InitializeAIComponentReferences()
{
    if (AActor* Owner = GetOwner())
    {
        BrainComponent = Owner->FindComponentByClass<UARPG_AIBrainComponent>();
        
        if (!BrainComponent.IsValid() && NeedsConfig.bEnableDebugLogging)
        {
            UE_LOG(LogRadiantRPG, Warning, TEXT("AINeedsComponent: No brain component found on %s"), *Owner->GetName());
        }
    }
}

void UARPG_AINeedsComponent::SendNeedUpdateToBrain(EARPG_NeedType NeedType, float Level, bool bIsCritical)
{
    if (!BrainComponent.IsValid())
    {
        return;
    }
    
    // Create a stimulus representing the need state
    // This would typically involve creating a stimulus struct and sending it to the brain
    // Implementation depends on your stimulus system structure
    
    if (NeedsConfig.bEnableDebugLogging)
    {
        UE_LOG(LogRadiantRPG, VeryVerbose, TEXT("Sent need update to brain: %s = %.2f (%s)"), 
               *UEnum::GetValueAsString(NeedType), Level, 
               bIsCritical ? TEXT("CRITICAL") : TEXT("Normal"));
    }
}

float UARPG_AINeedsComponent::CalculateNeedUrgency(EARPG_NeedType NeedType) const
{
    const FARPG_AINeed* Need = CurrentNeeds.Find(NeedType);
    if (!Need || !Need->bIsActive)
    {
        return 0.0f;
    }
    
    // Calculate urgency as a normalized value from urgency threshold to critical threshold
    if (Need->CurrentLevel < Need->UrgencyThreshold)
    {
        return 0.0f;
    }
    
    float UrgencyRange = Need->CriticalThreshold - Need->UrgencyThreshold;
    if (UrgencyRange <= 0.0f)
    {
        return Need->CurrentLevel >= Need->CriticalThreshold ? 1.0f : 0.0f;
    }
    
    float UrgencyProgress = (Need->CurrentLevel - Need->UrgencyThreshold) / UrgencyRange;
    return FMath::Clamp(UrgencyProgress, 0.0f, 1.0f);
}

FGameplayTag UARPG_AINeedsComponent::GetDefaultIntentForNeed(EARPG_NeedType NeedType) const
{
    // Map needs to default intent tags
    switch (NeedType)
    {
        case EARPG_NeedType::Hunger:
            return FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.FindFood"));
        case EARPG_NeedType::Fatigue:
            return FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Rest"));
        case EARPG_NeedType::Safety:
            return FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Flee"));
        case EARPG_NeedType::Social:
            return FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Socialize"));
        default:
            return FGameplayTag::EmptyTag;
    }
}

TMap<EARPG_NeedType, float> UARPG_AINeedsComponent::GetAllNeedsAsMap() const
{
    TMap<EARPG_NeedType, float> NeedsMap;
    
    for (const auto& NeedPair : CurrentNeeds)
    {
        NeedsMap.Add(NeedPair.Key, NeedPair.Value.CurrentLevel);
    }
    
    return NeedsMap;
}

TMap<EARPG_NeedType, float> UARPG_AINeedsComponent::GetAllNeedsNormalized() const
{
    TMap<EARPG_NeedType, float> NormalizedMap;
    
    for (const auto& NeedPair : CurrentNeeds)
    {
        const FARPG_AINeed& Need = NeedPair.Value;
        float NormalizedValue = Need.MaxLevel > 0.0f ? (Need.CurrentLevel / Need.MaxLevel) : 0.0f;
        NormalizedMap.Add(NeedPair.Key, FMath::Clamp(NormalizedValue, 0.0f, 1.0f));
    }
    
    return NormalizedMap;
}