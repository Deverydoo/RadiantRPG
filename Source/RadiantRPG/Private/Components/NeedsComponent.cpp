// Private/Components/NeedsComponent.cpp
// Basic needs system implementation for RadiantRPG

#include "Components/NeedsComponent.h"
#include "Characters/BaseCharacter.h"
#include "Engine/World.h"

UNeedsComponent::UNeedsComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    // Slow tick rate for needs - they change gradually
    PrimaryComponentTick.TickInterval = 1.0f; // Update every second

    // Initialize settings
    bNeedsActive = true;
    GlobalDecayMultiplier = 1.0f;
    OwnerCharacter = nullptr;
}

void UNeedsComponent::BeginPlay()
{
    Super::BeginPlay();

    // Cache owner reference
    OwnerCharacter = Cast<ABaseCharacter>(GetOwner());
    
    // Initialize default needs if none are set
    if (Needs.Num() == 0)
    {
        InitializeDefaultNeeds();
    }

    // Initialize critical status for all needs
    for (auto& NeedPair : Needs)
    {
        UpdateNeedCriticalStatus(NeedPair.Key);
    }

    UE_LOG(LogTemp, Log, TEXT("NeedsComponent initialized for %s with %d needs"), 
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
    // Add basic needs that most characters should have
    // Timing calculations: 1.0 / DecayRate = seconds to go from 1.0 to 0.0
    
    AddNeed(ENeedType::Hunger, 0.8f, 0.0008f);    // ~21 minutes to starve
    AddNeed(ENeedType::Thirst, 0.9f, 0.001f);     // ~17 minutes to dehydrate
    AddNeed(ENeedType::Sleep, 1.0f, 0.0003f);     // ~55 minutes to exhaustion
    AddNeed(ENeedType::Safety, 0.7f, 0.0001f);    // ~2.8 hours to fear
    AddNeed(ENeedType::Social, 0.6f, 0.0002f);    // ~83 minutes to loneliness
    AddNeed(ENeedType::Comfort, 0.5f, 0.00005f);  // ~5.5 hours to discomfort
    
    UE_LOG(LogTemp, Log, TEXT("NeedsComponent: Initialized default needs"));
}

void UNeedsComponent::UpdateNeeds(float DeltaTime)
{
    bool bAnyNeedChanged = false;
    int32 CriticalCount = 0;

    for (auto& NeedPair : Needs)
    {
        FNeedData& Need = NeedPair.Value;
        
        if (!Need.bIsActive)
            continue;

        // Calculate decay for this tick
        float DecayAmount = Need.DecayRate * GlobalDecayMultiplier * DeltaTime;
        float PreviousLevel = Need.CurrentLevel;
        
        // Apply decay
        Need.CurrentLevel = FMath::Clamp(Need.CurrentLevel - DecayAmount, 0.0f, 1.0f);
        
        // Check if need level changed significantly
        if (FMath::Abs(Need.CurrentLevel - PreviousLevel) > 0.001f)
        {
            UpdateNeedCriticalStatus(NeedPair.Key);
            BroadcastNeedChanged(NeedPair.Key, Need.CurrentLevel);
            bAnyNeedChanged = true;
        }

        // Count critical needs
        if (Need.bIsCritical)
        {
            CriticalCount++;
        }
    }

    // Log periodically for debugging (only when needs change)
    if (bAnyNeedChanged)
    {
        static float LastLogTime = 0.0f;
        float CurrentTime = GetWorld()->GetTimeSeconds();
        
        // More frequent logging if there are critical needs
        float LogInterval = CriticalCount > 0 ? 10.0f : 60.0f;
        
        if (CurrentTime - LastLogTime > LogInterval)
        {
            LastLogTime = CurrentTime;
            
            if (CriticalCount > 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("%s has %d critical needs"), 
                       OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
                       CriticalCount);
            }
            else
            {
                UE_LOG(LogTemp, Verbose, TEXT("%s needs updated - all stable"), 
                       OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"));
            }
        }
    }
}

void UNeedsComponent::UpdateNeedCriticalStatus(ENeedType NeedType)
{
    FNeedData* Need = Needs.Find(NeedType);
    if (!Need)
        return;

    bool bWasCritical = Need->bIsCritical;
    Need->bIsCritical = Need->CurrentLevel <= Need->CriticalThreshold;

    // Broadcast events on critical status change
    if (!bWasCritical && Need->bIsCritical)
    {
        OnNeedCritical.Broadcast(NeedType);
        OnNeedCriticalBP(NeedType);
        
        UE_LOG(LogTemp, Warning, TEXT("%s need '%s' became CRITICAL (%.1f%%)"), 
               OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
               *UEnum::GetValueAsString(NeedType), 
               Need->CurrentLevel * 100.0f);
    }
    else if (bWasCritical && !Need->bIsCritical)
    {
        OnNeedSatisfied.Broadcast(NeedType);
        OnNeedSatisfiedBP(NeedType);
        
        UE_LOG(LogTemp, Log, TEXT("%s need '%s' no longer critical (%.1f%%)"), 
               OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
               *UEnum::GetValueAsString(NeedType), 
               Need->CurrentLevel * 100.0f);
    }
}

void UNeedsComponent::BroadcastNeedChanged(ENeedType NeedType, float NewLevel)
{
    OnNeedChanged.Broadcast(NeedType, NewLevel);
    OnNeedChangedBP(NeedType, NewLevel);
}

float UNeedsComponent::GetNeedLevel(ENeedType NeedType) const
{
    const FNeedData* Need = Needs.Find(NeedType);
    return Need ? Need->CurrentLevel : 0.0f;
}

void UNeedsComponent::SetNeedLevel(ENeedType NeedType, float NewLevel)
{
    FNeedData* Need = Needs.Find(NeedType);
    if (!Need)
    {
        UE_LOG(LogTemp, Warning, TEXT("SetNeedLevel: Need type %s not found"), 
               *UEnum::GetValueAsString(NeedType));
        return;
    }

    float PreviousLevel = Need->CurrentLevel;
    Need->CurrentLevel = FMath::Clamp(NewLevel, 0.0f, 1.0f);
    
    if (FMath::Abs(Need->CurrentLevel - PreviousLevel) > 0.001f)
    {
        UpdateNeedCriticalStatus(NeedType);
        BroadcastNeedChanged(NeedType, Need->CurrentLevel);
        
        UE_LOG(LogTemp, Verbose, TEXT("%s need '%s' set to %.1f%% (was %.1f%%)"), 
               OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
               *UEnum::GetValueAsString(NeedType),
               Need->CurrentLevel * 100.0f, PreviousLevel * 100.0f);
    }
}

void UNeedsComponent::ModifyNeedLevel(ENeedType NeedType, float Amount)
{
    FNeedData* Need = Needs.Find(NeedType);
    if (!Need)
    {
        UE_LOG(LogTemp, Warning, TEXT("ModifyNeedLevel: Need type %s not found"), 
               *UEnum::GetValueAsString(NeedType));
        return;
    }

    float NewLevel = Need->CurrentLevel + Amount;
    SetNeedLevel(NeedType, NewLevel);
}

bool UNeedsComponent::IsNeedCritical(ENeedType NeedType) const
{
    const FNeedData* Need = Needs.Find(NeedType);
    return Need ? Need->bIsCritical : false;
}

TArray<ENeedType> UNeedsComponent::GetCriticalNeeds() const
{
    TArray<ENeedType> CriticalNeeds;
    
    for (const auto& NeedPair : Needs)
    {
        if (NeedPair.Value.bIsCritical && NeedPair.Value.bIsActive)
        {
            CriticalNeeds.Add(NeedPair.Key);
        }
    }
    
    // Sort by urgency (lowest level first)
    CriticalNeeds.Sort([this](const ENeedType& A, const ENeedType& B) {
        float LevelA = GetNeedLevel(A);
        float LevelB = GetNeedLevel(B);
        return LevelA < LevelB;
    });
    
    return CriticalNeeds;
}

ENeedType UNeedsComponent::GetMostCriticalNeed() const
{
    ENeedType MostCritical = ENeedType::Hunger; // Default fallback
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
        UE_LOG(LogTemp, Warning, TEXT("GetMostCriticalNeed: No active needs found"));
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

void UNeedsComponent::SatisfyNeed(ENeedType NeedType)
{
    float PreviousLevel = GetNeedLevel(NeedType);
    SetNeedLevel(NeedType, 1.0f);
    
    UE_LOG(LogTemp, Log, TEXT("%s need '%s' satisfied (%.1f%% → 100%%)"), 
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
    
    UE_LOG(LogTemp, Log, TEXT("%s satisfied %d needs"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
           SatisfiedCount);
}

void UNeedsComponent::SetNeedsActive(bool bActive)
{
    if (bNeedsActive == bActive)
        return;

    bNeedsActive = bActive;
    
    UE_LOG(LogTemp, Log, TEXT("%s needs system %s"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
           bNeedsActive ? TEXT("ACTIVATED") : TEXT("DEACTIVATED"));

    // If reactivating, update all need statuses
    if (bNeedsActive)
    {
        for (auto& NeedPair : Needs)
        {
            UpdateNeedCriticalStatus(NeedPair.Key);
        }
    }
}

void UNeedsComponent::AddNeed(ENeedType NeedType, float StartingLevel, float DecayRate)
{
    // Check if need already exists
    if (Needs.Contains(NeedType))
    {
        UE_LOG(LogTemp, Warning, TEXT("AddNeed: Need type %s already exists, updating instead"), 
               *UEnum::GetValueAsString(NeedType));
        
        FNeedData* ExistingNeed = Needs.Find(NeedType);
        ExistingNeed->CurrentLevel = FMath::Clamp(StartingLevel, 0.0f, 1.0f);
        ExistingNeed->DecayRate = FMath::Max(0.0f, DecayRate);
        UpdateNeedCriticalStatus(NeedType);
        return;
    }

    // Create new need
    FNeedData NewNeed;
    NewNeed.NeedType = NeedType;
    NewNeed.CurrentLevel = FMath::Clamp(StartingLevel, 0.0f, 1.0f);
    NewNeed.DecayRate = FMath::Max(0.0f, DecayRate);
    NewNeed.CriticalThreshold = 0.25f; // 25% threshold for most needs
    NewNeed.bIsCritical = NewNeed.CurrentLevel <= NewNeed.CriticalThreshold;
    NewNeed.bIsActive = true;
    
    // Special thresholds for specific needs
    switch (NeedType)
    {
        case ENeedType::Hunger:
        case ENeedType::Thirst:
            NewNeed.CriticalThreshold = 0.15f; // More urgent
            break;
        case ENeedType::Sleep:
            NewNeed.CriticalThreshold = 0.10f; // Very urgent when tired
            break;
        case ENeedType::Social:
        case ENeedType::Comfort:
            NewNeed.CriticalThreshold = 0.30f; // Less urgent
            break;
        default:
            // Use default 0.25f
            break;
    }
    
    NewNeed.bIsCritical = NewNeed.CurrentLevel <= NewNeed.CriticalThreshold;
    Needs.Add(NeedType, NewNeed);
    
    // Calculate time to critical/empty for logging
    float TimeToCritical = (NewNeed.CurrentLevel - NewNeed.CriticalThreshold) / NewNeed.DecayRate;
    float TimeToEmpty = NewNeed.CurrentLevel / NewNeed.DecayRate;
    
    UE_LOG(LogTemp, Log, TEXT("Added need '%s' to %s (Level: %.1f%%, Decay: %.5f/s, Critical in: %.1fm, Empty in: %.1fm)"), 
           *UEnum::GetValueAsString(NeedType),
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
           StartingLevel * 100.0f, 
           DecayRate,
           TimeToCritical / 60.0f,
           TimeToEmpty / 60.0f);
}

void UNeedsComponent::RemoveNeed(ENeedType NeedType)
{
    if (Needs.Remove(NeedType) > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Removed need '%s' from %s"), 
               *UEnum::GetValueAsString(NeedType),
               OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("RemoveNeed: Need type %s not found"), 
               *UEnum::GetValueAsString(NeedType));
    }
}