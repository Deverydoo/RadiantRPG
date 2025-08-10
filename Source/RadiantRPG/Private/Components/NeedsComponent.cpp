// Private/Components/NeedsComponent.cpp
// Basic needs system implementation for RadiantRPG

#include "Components/NeedsComponent.h"
#include "Characters/BaseCharacter.h"
#include "Engine/World.h"
#include "Types/ARPG_AITypes.h"
#include "RadiantRPG.h"

UNeedsComponent::UNeedsComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.TickInterval = 1.0f; // Update every second

    bNeedsActive = true;
    GlobalDecayMultiplier = 1.0f;
    OwnerCharacter = nullptr;
    LastLogTime = 0.0f;
}

void UNeedsComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCharacter = Cast<ABaseCharacter>(GetOwner());
    
    if (Needs.Num() == 0)
    {
        InitializeDefaultNeeds();
    }

    for (auto& NeedPair : Needs)
    {
        UpdateNeedCriticalStatus(NeedPair.Key);
    }

    UE_LOG(LogRadiantRPG, Log, TEXT("NeedsComponent initialized for %s with %d needs"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"), 
           Needs.Num());
}

void UNeedsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bNeedsActive)
    {
        UpdateNeeds(DeltaTime);
    }
}

void UNeedsComponent::InitializeDefaultNeeds()
{
    AddNeed(EARPG_NeedType::Hunger, 0.8f, 0.0008f);    // ~21 minutes to starve
    AddNeed(EARPG_NeedType::Fatigue, 1.0f, 0.0003f);   // ~55 minutes to exhaustion
    AddNeed(EARPG_NeedType::Safety, 0.7f, 0.0001f);    // ~2.8 hours to fear
    AddNeed(EARPG_NeedType::Social, 0.6f, 0.0002f);    // ~83 minutes to loneliness
    AddNeed(EARPG_NeedType::Comfort, 0.5f, 0.00005f);  // ~5.5 hours to discomfort
    AddNeed(EARPG_NeedType::Curiosity, 0.4f, 0.0001f); // ~2.8 hours to boredom
    
    UE_LOG(LogRadiantRPG, Log, TEXT("NeedsComponent: Initialized default needs"));
}

void UNeedsComponent::UpdateNeeds(float DeltaTime)
{
    bool bAnyNeedChanged = false;
    int32 CriticalCount = 0;

    for (auto& NeedPair : Needs)
    {
        FARPG_AINeed& Need = NeedPair.Value;
        
        if (!Need.bIsActive)
            continue;

        float DecayAmount = Need.DecayRate * GlobalDecayMultiplier * DeltaTime;
        float PreviousLevel = Need.CurrentLevel;
        
        Need.CurrentLevel = FMath::Clamp(Need.CurrentLevel - DecayAmount, 0.0f, 1.0f);
        
        if (FMath::Abs(Need.CurrentLevel - PreviousLevel) > 0.001f)
        {
            UpdateNeedCriticalStatus(NeedPair.Key);
            BroadcastNeedChanged(NeedPair.Key, Need.CurrentLevel);
            bAnyNeedChanged = true;
        }

        if (Need.bIsCritical)
        {
            CriticalCount++;
        }
    }

    LogNeedsUpdate(bAnyNeedChanged, CriticalCount);
}

void UNeedsComponent::LogNeedsUpdate(bool bAnyNeedChanged, int32 CriticalCount)
{
    if (!bAnyNeedChanged)
        return;

    float CurrentTime = GetWorld()->GetTimeSeconds();
    float LogInterval = CriticalCount > 0 ? 10.0f : 60.0f;
    
    if (CurrentTime - LastLogTime > LogInterval)
    {
        LastLogTime = CurrentTime;
        
        if (CriticalCount > 0)
        {
            UE_LOG(LogRadiantRPG, Warning, TEXT("%s has %d critical needs"), 
                   OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
                   CriticalCount);
        }
        else
        {
            UE_LOG(LogRadiantRPG, Verbose, TEXT("%s needs updated - all stable"), 
                   OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"));
        }
    }
}

void UNeedsComponent::UpdateNeedCriticalStatus(EARPG_NeedType NeedType)
{
    FARPG_AINeed* Need = Needs.Find(NeedType);
    if (!Need)
        return;

    bool bWasCritical = Need->bIsCritical;
    Need->bIsCritical = Need->CurrentLevel <= Need->CriticalThreshold;

    if (!bWasCritical && Need->bIsCritical)
    {
        OnNeedCritical.Broadcast(NeedType);
        OnNeedCriticalBP(NeedType);
        
        UE_LOG(LogRadiantRPG, Warning, TEXT("%s need '%s' became CRITICAL (%.1f%%)"), 
               OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
               *UEnum::GetValueAsString(NeedType), 
               Need->CurrentLevel * 100.0f);
    }
    else if (bWasCritical && !Need->bIsCritical)
    {
        OnNeedSatisfied.Broadcast(NeedType);
        OnNeedSatisfiedBP(NeedType);
        
        UE_LOG(LogRadiantRPG, Log, TEXT("%s need '%s' no longer critical (%.1f%%)"), 
               OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
               *UEnum::GetValueAsString(NeedType), 
               Need->CurrentLevel * 100.0f);
    }
}

void UNeedsComponent::BroadcastNeedChanged(EARPG_NeedType NeedType, float NewLevel)
{
    OnNeedChanged.Broadcast(NeedType, NewLevel);
    OnNeedChangedBP(NeedType, NewLevel);
}

float UNeedsComponent::GetNeedLevel(EARPG_NeedType NeedType) const
{
    const FARPG_AINeed* Need = Needs.Find(NeedType);
    return Need ? Need->CurrentLevel : 0.0f;
}

void UNeedsComponent::SetNeedLevel(EARPG_NeedType NeedType, float NewLevel)
{
    FARPG_AINeed* Need = Needs.Find(NeedType);
    if (!Need)
    {
        UE_LOG(LogRadiantRPG, Warning, TEXT("SetNeedLevel: Need type %s not found"), 
               *UEnum::GetValueAsString(NeedType));
        return;
    }

    float PreviousLevel = Need->CurrentLevel;
    Need->CurrentLevel = FMath::Clamp(NewLevel, 0.0f, 1.0f);
    
    if (FMath::Abs(Need->CurrentLevel - PreviousLevel) > 0.001f)
    {
        UpdateNeedCriticalStatus(NeedType);
        BroadcastNeedChanged(NeedType, Need->CurrentLevel);
        
        UE_LOG(LogRadiantRPG, Verbose, TEXT("%s need '%s' set to %.1f%% (was %.1f%%)"), 
               OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
               *UEnum::GetValueAsString(NeedType),
               Need->CurrentLevel * 100.0f, PreviousLevel * 100.0f);
    }
}

void UNeedsComponent::ModifyNeedLevel(EARPG_NeedType NeedType, float Amount)
{
    FARPG_AINeed* Need = Needs.Find(NeedType);
    if (!Need)
    {
        UE_LOG(LogRadiantRPG, Warning, TEXT("ModifyNeedLevel: Need type %s not found"), 
               *UEnum::GetValueAsString(NeedType));
        return;
    }

    float NewLevel = Need->CurrentLevel + Amount;
    SetNeedLevel(NeedType, NewLevel);
}

bool UNeedsComponent::IsNeedCritical(EARPG_NeedType NeedType) const
{
    const FARPG_AINeed* Need = Needs.Find(NeedType);
    return Need ? Need->bIsCritical : false;
}

TArray<EARPG_NeedType> UNeedsComponent::GetCriticalNeeds() const
{
    TArray<EARPG_NeedType> CriticalNeeds;
    
    for (const auto& NeedPair : Needs)
    {
        if (NeedPair.Value.bIsCritical && NeedPair.Value.bIsActive)
        {
            CriticalNeeds.Add(NeedPair.Key);
        }
    }
    
    CriticalNeeds.Sort([this](const EARPG_NeedType& A, const EARPG_NeedType& B) {
        float LevelA = GetNeedLevel(A);
        float LevelB = GetNeedLevel(B);
        return LevelA < LevelB;
    });
    
    return CriticalNeeds;
}

EARPG_NeedType UNeedsComponent::GetMostCriticalNeed() const
{
    EARPG_NeedType MostCritical = EARPG_NeedType::Hunger;
    float LowestLevel = 1.0f;
    bool bFoundAnyActiveNeed = false;
    
    for (const auto& NeedPair : Needs)
    {
        if (NeedPair.Value.bIsActive && NeedPair.Value.CurrentLevel < LowestLevel)
        {
            LowestLevel = NeedPair.Value.CurrentLevel;
            MostCritical = NeedPair.Key;
            bFoundAnyActiveNeed = true;
        }
    }
    
    if (!bFoundAnyActiveNeed)
    {
        UE_LOG(LogRadiantRPG, Warning, TEXT("GetMostCriticalNeed: No active needs found"));
    }
    
    return MostCritical;
}

bool UNeedsComponent::HasCriticalNeeds() const
{
    for (const auto& NeedPair : Needs)
    {
        if (NeedPair.Value.bIsCritical && NeedPair.Value.bIsActive)
        {
            return true;
        }
    }
    
    return false;
}

void UNeedsComponent::SatisfyNeed(EARPG_NeedType NeedType)
{
    float PreviousLevel = GetNeedLevel(NeedType);
    SetNeedLevel(NeedType, 1.0f);
    
    UE_LOG(LogRadiantRPG, Log, TEXT("%s need '%s' satisfied (%.1f%% → 100%%)"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
           *UEnum::GetValueAsString(NeedType),
           PreviousLevel * 100.0f);
}

void UNeedsComponent::SatisfyAllNeeds()
{
    int32 SatisfiedCount = 0;
    
    for (auto& NeedPair : Needs)
    {
        if (NeedPair.Value.bIsActive && NeedPair.Value.CurrentLevel < 1.0f)
        {
            NeedPair.Value.CurrentLevel = 1.0f;
            UpdateNeedCriticalStatus(NeedPair.Key);
            BroadcastNeedChanged(NeedPair.Key, 1.0f);
            SatisfiedCount++;
        }
    }
    
    UE_LOG(LogRadiantRPG, Log, TEXT("%s satisfied %d needs"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
           SatisfiedCount);
}

void UNeedsComponent::SetNeedsActive(bool bActive)
{
    if (bNeedsActive == bActive)
        return;

    bNeedsActive = bActive;
    
    UE_LOG(LogRadiantRPG, Log, TEXT("%s needs system %s"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
           bNeedsActive ? TEXT("ACTIVATED") : TEXT("DEACTIVATED"));

    if (bNeedsActive)
    {
        for (auto& NeedPair : Needs)
        {
            UpdateNeedCriticalStatus(NeedPair.Key);
        }
    }
}

void UNeedsComponent::AddNeed(EARPG_NeedType NeedType, float StartingLevel, float DecayRate)
{
    if (Needs.Contains(NeedType))
    {
        UE_LOG(LogRadiantRPG, Warning, TEXT("AddNeed: Need type %s already exists, updating instead"), 
               *UEnum::GetValueAsString(NeedType));
        
        FARPG_AINeed* ExistingNeed = Needs.Find(NeedType);
        ExistingNeed->CurrentLevel = FMath::Clamp(StartingLevel, 0.0f, 1.0f);
        ExistingNeed->DecayRate = FMath::Max(0.0f, DecayRate);
        UpdateNeedCriticalStatus(NeedType);
        return;
    }

    FARPG_AINeed NewNeed;
    NewNeed.NeedType = NeedType;
    NewNeed.CurrentLevel = FMath::Clamp(StartingLevel, 0.0f, 1.0f);
    NewNeed.DecayRate = FMath::Max(0.0f, DecayRate);
    NewNeed.bIsActive = true;
    
    // Set critical thresholds based on need type
    SetCriticalThresholdForNeedType(NewNeed);
    
    NewNeed.bIsCritical = NewNeed.CurrentLevel <= NewNeed.CriticalThreshold;
    Needs.Add(NeedType, NewNeed);
    
    LogNeedAddition(NeedType, NewNeed);
}

void UNeedsComponent::SetCriticalThresholdForNeedType(FARPG_AINeed& Need)
{
    switch (Need.NeedType)
    {
        case EARPG_NeedType::Hunger:
            Need.CriticalThreshold = 0.15f; // More urgent
            break;
        case EARPG_NeedType::Fatigue:
            Need.CriticalThreshold = 0.10f; // Very urgent when tired
            break;
        case EARPG_NeedType::Social:
        case EARPG_NeedType::Comfort:
            Need.CriticalThreshold = 0.30f; // Less urgent
            break;
        case EARPG_NeedType::Safety:
            Need.CriticalThreshold = 0.20f; // Moderately urgent
            break;
        case EARPG_NeedType::Curiosity:
            Need.CriticalThreshold = 0.25f; // Default
            break;
        default:
            Need.CriticalThreshold = 0.25f; // Default
            break;
    }
}

void UNeedsComponent::LogNeedAddition(EARPG_NeedType NeedType, const FARPG_AINeed& Need)
{
    float TimeToCritical = (Need.CurrentLevel - Need.CriticalThreshold) / Need.DecayRate;
    float TimeToEmpty = Need.CurrentLevel / Need.DecayRate;
    
    UE_LOG(LogRadiantRPG, Log, TEXT("Added need '%s' to %s (Level: %.1f%%, Decay: %.5f/s, Critical in: %.1fm, Empty in: %.1fm)"), 
           *UEnum::GetValueAsString(NeedType),
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
           Need.CurrentLevel * 100.0f, 
           Need.DecayRate,
           TimeToCritical / 60.0f,
           TimeToEmpty / 60.0f);
}

void UNeedsComponent::RemoveNeed(EARPG_NeedType NeedType)
{
    if (Needs.Remove(NeedType) > 0)
    {
        UE_LOG(LogRadiantRPG, Log, TEXT("Removed need '%s' from %s"), 
               *UEnum::GetValueAsString(NeedType),
               OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"));
    }
    else
    {
        UE_LOG(LogRadiantRPG, Warning, TEXT("RemoveNeed: Need type %s not found"), 
               *UEnum::GetValueAsString(NeedType));
    }
}