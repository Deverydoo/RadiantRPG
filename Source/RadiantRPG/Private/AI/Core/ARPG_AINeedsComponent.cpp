// Source/RadiantRPG/Private/AI/Core/ARPG_AINeedsComponent.cpp

#include "AI/Core/ARPG_AINeedsComponent.h"
#include "AI/Core/ARPG_AIBrainComponent.h"
#include "Engine/World.h"

UARPG_AINeedsComponent::UARPG_AINeedsComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 1.0f; // Default to 1 second updates
    
    // Initialize default configuration
    NeedsConfig = FARPG_NeedsConfiguration();
    bNeedsUpdatePaused = false;
    TimeSinceLastUpdate = 0.0f;
    
    UE_LOG(LogTemp, Log, TEXT("AINeedsComponent: Component created"));
}

void UARPG_AINeedsComponent::BeginPlay()
{
    Super::BeginPlay();
    
    InitializeComponentReferences();
    InitializeDefaultNeeds();
    
    UE_LOG(LogTemp, Log, TEXT("AINeedsComponent: Initialized for %s with %d needs"), 
           GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"), CurrentNeeds.Num());
}

void UARPG_AINeedsComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CurrentNeeds.Empty();
    Super::EndPlay(EndPlayReason);
}

void UARPG_AINeedsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    if (!bNeedsUpdatePaused && NeedsConfig.bEnableNeedsInfluence)
    {
        UpdateNeedsOverTime(DeltaTime);
    }
    
    TimeSinceLastUpdate += DeltaTime;
}

void UARPG_AINeedsComponent::InitializeNeeds(const FARPG_NeedsConfiguration& Config, const TArray<FARPG_AINeed>& InitialNeeds)
{
    NeedsConfig = Config;
    PrimaryComponentTick.TickInterval = NeedsConfig.UpdateFrequency;
    
    // Clear current needs and set new ones
    CurrentNeeds.Empty();
    for (const FARPG_AINeed& Need : InitialNeeds)
    {
        CurrentNeeds.Add(Need.NeedType, Need);
    }
    
    if (NeedsConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("Needs initialized with %d custom needs"), InitialNeeds.Num());
    }
}

void UARPG_AINeedsComponent::UpdateNeedsConfiguration(const FARPG_NeedsConfiguration& NewConfig)
{
    NeedsConfig = NewConfig;
    PrimaryComponentTick.TickInterval = NeedsConfig.UpdateFrequency;
    
    if (NeedsConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("Needs configuration updated"));
    }
}

float UARPG_AINeedsComponent::GetNeedLevel(EARPG_NeedType NeedType) const
{
    if (const FARPG_AINeed* Need = CurrentNeeds.Find(NeedType))
    {
        return Need->CurrentLevel;
    }
    return 0.5f; // Default neutral level
}

void UARPG_AINeedsComponent::SetNeedLevel(EARPG_NeedType NeedType, float NewLevel)
{
    if (FARPG_AINeed* Need = CurrentNeeds.Find(NeedType))
    {
        float OldLevel = Need->CurrentLevel;
        Need->CurrentLevel = FMath::Clamp(NewLevel, 0.0f, 1.0f);
        
        // Check for state changes
        FARPG_AINeed OldNeed = *Need;
        OldNeed.CurrentLevel = OldLevel;
        CheckUrgentStateChanges(NeedType, OldNeed, *Need);
        CheckSatisfiedStateChanges(NeedType, OldNeed, *Need);
        
        // Broadcast change
        OnNeedChanged.Broadcast(NeedType, Need->CurrentLevel);
        BP_OnNeedUpdated(NeedType, OldLevel, Need->CurrentLevel);
        
        if (NeedsConfig.bEnableDebugLogging)
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("Need %d level set from %.2f to %.2f"), 
                   (int32)NeedType, OldLevel, Need->CurrentLevel);
        }
    }
}

void UARPG_AINeedsComponent::ModifyNeedLevel(EARPG_NeedType NeedType, float DeltaAmount)
{
    float CurrentLevel = GetNeedLevel(NeedType);
    SetNeedLevel(NeedType, CurrentLevel + DeltaAmount);
}

TArray<FARPG_AINeed> UARPG_AINeedsComponent::GetAllNeeds() const
{
    TArray<FARPG_AINeed> AllNeeds;
    CurrentNeeds.GenerateValueArray(AllNeeds);
    return AllNeeds;
}

bool UARPG_AINeedsComponent::IsNeedUrgent(EARPG_NeedType NeedType) const
{
    if (const FARPG_AINeed* Need = CurrentNeeds.Find(NeedType))
    {
        return Need->bIsUrgent;
    }
    return false;
}

bool UARPG_AINeedsComponent::HasUrgentNeeds() const
{
    for (const auto& NeedPair : CurrentNeeds)
    {
        if (NeedPair.Value.bIsUrgent)
        {
            return true;
        }
    }
    return false;
}

TArray<EARPG_NeedType> UARPG_AINeedsComponent::GetUrgentNeeds() const
{
    TArray<EARPG_NeedType> UrgentNeeds;
    for (const auto& NeedPair : CurrentNeeds)
    {
        if (NeedPair.Value.bIsUrgent)
        {
            UrgentNeeds.Add(NeedPair.Key);
        }
    }
    return UrgentNeeds;
}

EARPG_NeedType UARPG_AINeedsComponent::GetMostUrgentNeed() const
{
    EARPG_NeedType MostUrgent = EARPG_NeedType::MAX;
    float HighestUrgency = 0.0f;
    
    for (const auto& NeedPair : CurrentNeeds)
    {
        if (NeedPair.Value.bIsUrgent && NeedPair.Value.CurrentLevel > HighestUrgency)
        {
            HighestUrgency = NeedPair.Value.CurrentLevel;
            MostUrgent = NeedPair.Key;
        }
    }
    
    return MostUrgent;
}

bool UARPG_AINeedsComponent::IsNeedSatisfied(EARPG_NeedType NeedType) const
{
    if (const FARPG_AINeed* Need = CurrentNeeds.Find(NeedType))
    {
        return Need->CurrentLevel <= Need->SatisfiedThreshold;
    }
    return true; // If need doesn't exist, consider it satisfied
}

void UARPG_AINeedsComponent::SatisfyNeed(EARPG_NeedType NeedType)
{
    if (FARPG_AINeed* Need = CurrentNeeds.Find(NeedType))
    {
        float OldLevel = Need->CurrentLevel;
        Need->CurrentLevel = Need->SatisfiedThreshold;
        
        OnNeedChanged.Broadcast(NeedType, Need->CurrentLevel);
        OnNeedSatisfied.Broadcast(NeedType, Need->CurrentLevel);
        BP_OnNeedUpdated(NeedType, OldLevel, Need->CurrentLevel);
        BP_OnNeedSatisfied(NeedType, Need->CurrentLevel);
        
        if (NeedsConfig.bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Log, TEXT("Need %d satisfied (level: %.2f)"), (int32)NeedType, Need->CurrentLevel);
        }
    }
}

void UARPG_AINeedsComponent::SetNeedsUpdatePaused(bool bPaused)
{
    bNeedsUpdatePaused = bPaused;
    
    if (NeedsConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("Needs updates %s"), 
               bPaused ? TEXT("paused") : TEXT("resumed"));
    }
}

void UARPG_AINeedsComponent::ResetAllNeeds()
{
    CurrentNeeds.Empty();
    InitializeDefaultNeeds();
    
    if (NeedsConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("All needs reset to defaults"));
    }
}

void UARPG_AINeedsComponent::ContributeToInputs(FARPG_AIInputVector& InputVector) const
{
    // Add current need levels to the input vector
    for (const auto& NeedPair : CurrentNeeds)
    {
        InputVector.NeedLevels.Add(NeedPair.Key, NeedPair.Value.CurrentLevel);
    }
    
    // Add need-based factors
    if (HasUrgentNeeds())
    {
        InputVector.StatusFactors.Add(TEXT("HasUrgentNeeds"), 1.0f);
        
        EARPG_NeedType MostUrgent = GetMostUrgentNeed();
        if (MostUrgent != EARPG_NeedType::MAX)
        {
            float UrgencyLevel = GetNeedLevel(MostUrgent);
            InputVector.StatusFactors.Add(TEXT("MostUrgentNeedLevel"), UrgencyLevel);
            InputVector.StatusFactors.Add(TEXT("MostUrgentNeedType"), static_cast<float>((int32)MostUrgent));
        }
    }
    else
    {
        InputVector.StatusFactors.Add(TEXT("HasUrgentNeeds"), 0.0f);
    }
    
    // Calculate overall well-being
    float TotalWellBeing = 0.0f;
    int32 NeedCount = 0;
    for (const auto& NeedPair : CurrentNeeds)
    {
        // Invert the need level for well-being calculation
        // (lower needs = higher well-being)
        TotalWellBeing += (1.0f - NeedPair.Value.CurrentLevel);
        NeedCount++;
    }
    
    if (NeedCount > 0)
    {
        float AverageWellBeing = TotalWellBeing / NeedCount;
        InputVector.StatusFactors.Add(TEXT("OverallWellBeing"), AverageWellBeing);
    }
}

TArray<FGameplayTag> UARPG_AINeedsComponent::GetNeedBasedIntentSuggestions() const
{
    TArray<FGameplayTag> IntentSuggestions;
    
    // Get suggestions for urgent needs first
    TArray<EARPG_NeedType> UrgentNeeds = GetUrgentNeeds();
    for (EARPG_NeedType NeedType : UrgentNeeds)
    {
        FGameplayTag IntentTag = GetIntentTagForNeed(NeedType);
        if (IntentTag.IsValid())
        {
            IntentSuggestions.Add(IntentTag);
        }
        
        // Try Blueprint implementation
        float UrgencyLevel = GetNeedLevel(NeedType);
        FGameplayTag BlueprintIntent = BP_GetIntentForNeed(NeedType, UrgencyLevel);
        if (BlueprintIntent.IsValid() && !IntentSuggestions.Contains(BlueprintIntent))
        {
            IntentSuggestions.Add(BlueprintIntent);
        }
    }
    
    return IntentSuggestions;
}

void UARPG_AINeedsComponent::InitializeComponentReferences()
{
    if (AActor* Owner = GetOwner())
    {
        BrainComponent = Owner->FindComponentByClass<UARPG_AIBrainComponent>();
        
        if (!BrainComponent.IsValid() && NeedsConfig.bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Warning, TEXT("AINeedsComponent: No brain component found on %s"), *Owner->GetName());
        }
    }
}

void UARPG_AINeedsComponent::InitializeDefaultNeeds()
{
    CurrentNeeds.Empty();
    
    for (const FARPG_AINeed& DefaultNeed : DefaultNeeds)
    {
        CurrentNeeds.Add(DefaultNeed.NeedType, DefaultNeed);
    }
    
    // If no default needs configured, create basic ones
    if (CurrentNeeds.Num() == 0)
    {
        TArray<EARPG_NeedType> BasicNeeds = {
            EARPG_NeedType::Hunger,
            EARPG_NeedType::Fatigue,
            EARPG_NeedType::Safety,
            EARPG_NeedType::Social
        };
        
        for (EARPG_NeedType NeedType : BasicNeeds)
        {
            FARPG_AINeed DefaultNeed = GetDefaultNeedForType(NeedType);
            CurrentNeeds.Add(NeedType, DefaultNeed);
        }
    }
    
    if (NeedsConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("Initialized %d default needs"), CurrentNeeds.Num());
    }
}

void UARPG_AINeedsComponent::UpdateNeedsOverTime(float DeltaTime)
{
    for (auto& NeedPair : CurrentNeeds)
    {
        FARPG_AINeed OldNeed = NeedPair.Value;
        UpdateIndividualNeed(NeedPair.Value, DeltaTime);
        
        // Check for state changes
        CheckUrgentStateChanges(NeedPair.Key, OldNeed, NeedPair.Value);
        CheckSatisfiedStateChanges(NeedPair.Key, OldNeed, NeedPair.Value);
        
        // Broadcast if significant change occurred
        if (!FMath::IsNearlyEqual(OldNeed.CurrentLevel, NeedPair.Value.CurrentLevel, 0.01f))
        {
            OnNeedChanged.Broadcast(NeedPair.Key, NeedPair.Value.CurrentLevel);
            BP_OnNeedUpdated(NeedPair.Key, OldNeed.CurrentLevel, NeedPair.Value.CurrentLevel);
        }
    }
}

void UARPG_AINeedsComponent::UpdateIndividualNeed(FARPG_AINeed& Need, float DeltaTime)
{
    float EffectiveChangeRate = Need.ChangeRate * NeedsConfig.GlobalChangeRateMultiplier;
    float Change = EffectiveChangeRate * DeltaTime;
    
    Need.CurrentLevel = FMath::Clamp(Need.CurrentLevel + Change, 0.0f, 1.0f);
    
    // Update urgent state
    bool WasUrgent = Need.bIsUrgent;
    Need.bIsUrgent = Need.CurrentLevel >= Need.UrgentThreshold;
    
    if (NeedsConfig.bEnableDebugLogging && WasUrgent != Need.bIsUrgent)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("Need %d urgent state changed: %s"), 
               (int32)Need.NeedType, Need.bIsUrgent ? TEXT("urgent") : TEXT("not urgent"));
    }
}

void UARPG_AINeedsComponent::CheckUrgentStateChanges(EARPG_NeedType NeedType, const FARPG_AINeed& OldNeed, const FARPG_AINeed& NewNeed)
{
    if (!OldNeed.bIsUrgent && NewNeed.bIsUrgent)
    {
        OnNeedBecameUrgent.Broadcast(NeedType, NewNeed.CurrentLevel);
        BP_OnNeedBecameUrgent(NeedType, NewNeed.CurrentLevel);
        
        if (NeedsConfig.bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Log, TEXT("Need %d became urgent (level: %.2f)"), (int32)NeedType, NewNeed.CurrentLevel);
        }
    }
}

void UARPG_AINeedsComponent::CheckSatisfiedStateChanges(EARPG_NeedType NeedType, const FARPG_AINeed& OldNeed, const FARPG_AINeed& NewNeed)
{
    bool WasSatisfied = OldNeed.CurrentLevel <= OldNeed.SatisfiedThreshold;
    bool IsSatisfied = NewNeed.CurrentLevel <= NewNeed.SatisfiedThreshold;
    
    if (!WasSatisfied && IsSatisfied)
    {
        OnNeedSatisfied.Broadcast(NeedType, NewNeed.CurrentLevel);
        
        if (NeedsConfig.bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Log, TEXT("Need %d became satisfied (level: %.2f)"), (int32)NeedType, NewNeed.CurrentLevel);
        }
    }
}

FARPG_AINeed UARPG_AINeedsComponent::GetDefaultNeedForType(EARPG_NeedType NeedType) const
{
    FARPG_AINeed DefaultNeed;
    DefaultNeed.NeedType = NeedType;
    DefaultNeed.CurrentLevel = 0.3f; // Start slightly needy
    DefaultNeed.UrgentThreshold = 0.8f;
    DefaultNeed.SatisfiedThreshold = 0.2f;
    DefaultNeed.bIsUrgent = false;
    
    // Set different change rates for different needs
    switch (NeedType)
    {
        case EARPG_NeedType::Hunger:
            DefaultNeed.ChangeRate = 0.02f; // Increases over time
            break;
        case EARPG_NeedType::Fatigue:
            DefaultNeed.ChangeRate = 0.01f; // Slower than hunger
            break;
        case EARPG_NeedType::Safety:
            DefaultNeed.ChangeRate = 0.0f; // Context-dependent
            break;
        case EARPG_NeedType::Social:
            DefaultNeed.ChangeRate = 0.005f; // Very slow
            break;
        default:
            DefaultNeed.ChangeRate = 0.01f;
            break;
    }
    
    return DefaultNeed;
}

FGameplayTag UARPG_AINeedsComponent::GetIntentTagForNeed(EARPG_NeedType NeedType) const
{
    switch (NeedType)
    {
        case EARPG_NeedType::Hunger:
            return FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.FindFood"));
        case EARPG_NeedType::Fatigue:
            return FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Rest"));
        case EARPG_NeedType::Safety:
            return FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.FindSafety"));
        case EARPG_NeedType::Social:
            return FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Socialize"));
        default:
            return FGameplayTag();
    }
}

TMap<EARPG_NeedType, float> UARPG_AINeedsComponent::GetAllNeedsAsMap() const
{
    TMap<EARPG_NeedType, float> NeedLevels;
    for (const auto& NeedPair : CurrentNeeds)
    {
        NeedLevels.Add(NeedPair.Key, NeedPair.Value.CurrentLevel);
    }
    return NeedLevels;
}