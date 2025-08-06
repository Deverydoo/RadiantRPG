// Private/Components/StaminaComponent.cpp

#include "Components/StaminaComponent.h"
#include "Characters/BaseCharacter.h"
#include "Engine/World.h"

UStaminaComponent::UStaminaComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;

    // Default settings
    BaseMaxStamina = 100.0f;
    CurrentStamina = 100.0f; // Start with full stamina
    CurrentMaxStamina = 100.0f;
    
    // Regeneration settings
    bCanRegenerate = true;
    StaminaRegenRate = 10.0f; // 10 stamina per second
    RegenDelay = 2.0f; // 2 seconds after activity
    TimeSinceLastActivity = 0.0f;

    // Exhaustion settings
    ExhaustionThreshold = 0.2f; // Exhausted when below 20%
    ExhaustedRegenMultiplier = 0.5f; // Half regen when exhausted
    bIsExhausted = false;

    // State
    bHasStamina = true;
    OwnerCharacter = nullptr;
}

void UStaminaComponent::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("StaminaComponent BeginPlay called!"));

    // Cache owner reference
    OwnerCharacter = Cast<ABaseCharacter>(GetOwner());
    
    // Initialize stamina to max
    CurrentMaxStamina = BaseMaxStamina;
    CurrentStamina = CurrentMaxStamina;
    
    UpdateExhaustionState();
    CheckStaminaState();
    BroadcastStaminaChanged();
    
    UE_LOG(LogTemp, Log, TEXT("StaminaComponent initialized for %s with %f/%f stamina"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"), 
           CurrentStamina, CurrentMaxStamina);
}

void UStaminaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Update time since last activity
    TimeSinceLastActivity += DeltaTime;
    
    // Update continuous activities
    UpdateContinuousActivities(DeltaTime);
    
    // Update regeneration
    UpdateRegeneration(DeltaTime);
    
    // Update exhaustion state
    UpdateExhaustionState();
}

void UStaminaComponent::UpdateRegeneration(float DeltaTime)
{
    if (!bCanRegenerate || IsFullStamina())
        return;

    // Only regen if enough time has passed since last activity
    if (TimeSinceLastActivity >= RegenDelay)
    {
        float RegenAmount = StaminaRegenRate * DeltaTime;
        
        // Apply exhaustion multiplier
        if (bIsExhausted)
        {
            RegenAmount *= ExhaustedRegenMultiplier;
        }
        
        RestoreStamina(RegenAmount);
    }
}

void UStaminaComponent::UpdateContinuousActivities(float DeltaTime)
{
    if (ActiveContinuousActivities.Num() == 0)
        return;

    for (int32 i = ActiveContinuousActivities.Num() - 1; i >= 0; i--)
    {
        const FStaminaCostInfo& ActivityInfo = ActiveContinuousActivities[i];
        float CostPerSecond = CalculateActivityCost(ActivityInfo);
        float CostThisFrame = CostPerSecond * DeltaTime;
        
        if (CurrentStamina >= CostThisFrame)
        {
            // Spend stamina for continuous activity
            CurrentStamina = FMath::Max(0.0f, CurrentStamina - CostThisFrame);
            TimeSinceLastActivity = 0.0f; // Reset regen timer
            
            // Broadcast events
            OnStaminaSpent.Broadcast(CostThisFrame, ActivityInfo.Actor);
            OnStaminaSpentBP(CostThisFrame, ActivityInfo.Actor);
        }
        else
        {
            // Not enough stamina, stop this activity
            UE_LOG(LogTemp, Warning, TEXT("%s ran out of stamina for %s activity"), 
                   OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
                   *UEnum::GetValueAsString(ActivityInfo.Activity));
            
            ActiveContinuousActivities.RemoveAt(i);
        }
    }
    
    CheckStaminaState();
    BroadcastStaminaChanged();
}

void UStaminaComponent::UpdateExhaustionState()
{
    bool bWasExhausted = bIsExhausted;
    float StaminaPercent = GetStaminaPercent();
    
    if (!bIsExhausted && StaminaPercent <= ExhaustionThreshold)
    {
        bIsExhausted = true;
    }
    else if (bIsExhausted && StaminaPercent > (ExhaustionThreshold + 0.1f)) // Add hysteresis
    {
        bIsExhausted = false;
    }
    
    if (bWasExhausted != bIsExhausted)
    {
        OnExhaustionStateChanged.Broadcast(bIsExhausted);
        OnExhaustionStateChangedBP(bIsExhausted);
        
        UE_LOG(LogTemp, Log, TEXT("%s exhaustion state changed to: %s"), 
               OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
               bIsExhausted ? TEXT("Exhausted") : TEXT("Recovered"));
    }
}

bool UStaminaComponent::TrySpendStamina(const FStaminaCostInfo& ActivityInfo)
{
    float TotalCost = CalculateActivityCost(ActivityInfo);
    
    if (CurrentStamina < TotalCost)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s insufficient stamina: %f needed, %f available"), 
               OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"), 
               TotalCost, CurrentStamina);
        return false;
    }

    // Handle continuous activities differently
    if (ActivityInfo.bIsContinuous)
    {
        StartContinuousActivity(ActivityInfo);
        return true;
    }

    // Spend the stamina for one-time activities
    CurrentStamina = FMath::Max(0.0f, CurrentStamina - TotalCost);
    
    // Reset activity timer for regeneration
    TimeSinceLastActivity = 0.0f;
    
    // Broadcast events
    OnStaminaSpent.Broadcast(TotalCost, ActivityInfo.Actor);
    OnStaminaSpentBP(TotalCost, ActivityInfo.Actor);
    
    CheckStaminaState();
    BroadcastStaminaChanged();
    
    UE_LOG(LogTemp, Log, TEXT("%s spent %f stamina for %s, %f remaining"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"), 
           TotalCost, *UEnum::GetValueAsString(ActivityInfo.Activity), CurrentStamina);
    
    return true;
}

bool UStaminaComponent::TrySpendStaminaSimple(float StaminaCost, AActor* Actor)
{
    FStaminaCostInfo ActivityInfo;
    ActivityInfo.BaseCost = StaminaCost;
    ActivityInfo.Activity = EStaminaActivity::Walking;
    ActivityInfo.Actor = Actor;
    ActivityInfo.bIsContinuous = false;
    
    return TrySpendStamina(ActivityInfo);
}

void UStaminaComponent::RestoreStamina(float StaminaAmount)
{
    if (StaminaAmount <= 0.0f)
        return;

    float PreviousStamina = CurrentStamina;
    CurrentStamina = FMath::Clamp(CurrentStamina + StaminaAmount, 0.0f, CurrentMaxStamina);
    
    if (CurrentStamina != PreviousStamina)
    {
        CheckStaminaState();
        BroadcastStaminaChanged();
        OnStaminaRestoredBP(StaminaAmount);
        
        UE_LOG(LogTemp, Verbose, TEXT("%s restored %f stamina, now %f/%f"), 
               OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"), 
               StaminaAmount, CurrentStamina, CurrentMaxStamina);
    }
}

void UStaminaComponent::SetStamina(float NewStamina)
{
    CurrentStamina = FMath::Clamp(NewStamina, 0.0f, CurrentMaxStamina);
    CheckStaminaState();
    BroadcastStaminaChanged();
}

void UStaminaComponent::SetMaxStamina(float NewMaxStamina, bool bRestoreToFull)
{
    float OldMaxStamina = CurrentMaxStamina;
    CurrentMaxStamina = FMath::Max(1.0f, NewMaxStamina);
    
    if (bRestoreToFull)
    {
        CurrentStamina = CurrentMaxStamina;
    }
    else
    {
        // Adjust current stamina proportionally
        if (OldMaxStamina > 0.0f)
        {
            float StaminaRatio = CurrentStamina / OldMaxStamina;
            CurrentStamina = CurrentMaxStamina * StaminaRatio;
        }
        else
        {
            CurrentStamina = CurrentMaxStamina;
        }
    }
    
    CheckStaminaState();
    BroadcastStaminaChanged();
}

void UStaminaComponent::AddMaxStamina(float StaminaToAdd, bool bRestoreToFull)
{
    SetMaxStamina(CurrentMaxStamina + StaminaToAdd, bRestoreToFull);
}

void UStaminaComponent::DrainAllStamina()
{
    SetStamina(0.0f);
    StopAllContinuousActivities();
    OnStaminaEmpty.Broadcast();
    OnStaminaEmptyBP();
}

void UStaminaComponent::StartContinuousActivity(const FStaminaCostInfo& ActivityInfo)
{
    // Check if we already have this activity running
    for (const FStaminaCostInfo& ExistingActivity : ActiveContinuousActivities)
    {
        if (ExistingActivity.Activity == ActivityInfo.Activity)
        {
            // Activity already running
            return;
        }
    }
    
    // Add the new continuous activity
    ActiveContinuousActivities.Add(ActivityInfo);
    
    UE_LOG(LogTemp, Log, TEXT("%s started continuous %s activity"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
           *UEnum::GetValueAsString(ActivityInfo.Activity));
}

void UStaminaComponent::StopContinuousActivity(EStaminaActivity Activity)
{
    int32 RemovedCount = ActiveContinuousActivities.RemoveAll([Activity](const FStaminaCostInfo& ActivityInfo)
    {
        return ActivityInfo.Activity == Activity;
    });
    
    if (RemovedCount > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("%s stopped continuous %s activity"), 
               OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
               *UEnum::GetValueAsString(Activity));
    }
}

void UStaminaComponent::StopAllContinuousActivities()
{
    if (ActiveContinuousActivities.Num() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("%s stopped all continuous activities"), 
               OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"));
        
        ActiveContinuousActivities.Empty();
    }
}

float UStaminaComponent::GetStaminaPercent() const
{
    return CurrentMaxStamina > 0.0f ? CurrentStamina / CurrentMaxStamina : 0.0f;
}

bool UStaminaComponent::IsFullStamina() const
{
    return FMath::IsNearlyEqual(CurrentStamina, CurrentMaxStamina, 0.1f);
}

bool UStaminaComponent::CanPerformActivity(const FStaminaCostInfo& ActivityInfo) const
{
    if (!bHasStamina || CurrentMaxStamina <= 0.0f)
        return false;
        
    float TotalCost = CalculateActivityCost(ActivityInfo);
    return CurrentStamina >= TotalCost;
}

bool UStaminaComponent::CanPerformActivitySimple(float StaminaCost) const
{
    return bHasStamina && CurrentStamina >= StaminaCost;
}

float UStaminaComponent::CalculateActivityCost(const FStaminaCostInfo& ActivityInfo) const
{
    float Cost = ActivityInfo.BaseCost;
    
    // Apply activity modifiers
    float Multiplier = GetActivityCostMultiplier(ActivityInfo.Activity);
    Cost *= Multiplier;
    
    return FMath::Max(0.0f, Cost);
}

void UStaminaComponent::SetActivityModifier(EStaminaActivity Activity, float CostMultiplier)
{
    CostMultiplier = FMath::Clamp(CostMultiplier, 0.1f, 5.0f);
    
    // Find existing modifier or add new one
    for (FActivityCostModifier& Modifier : ActivityModifiers)
    {
        if (Modifier.Activity == Activity)
        {
            Modifier.CostMultiplier = CostMultiplier;
            return;
        }
    }
    
    // Add new modifier
    FActivityCostModifier NewModifier;
    NewModifier.Activity = Activity;
    NewModifier.CostMultiplier = CostMultiplier;
    ActivityModifiers.Add(NewModifier);
    
    UE_LOG(LogTemp, Log, TEXT("%s gained %s activity modifier with %.2fx cost multiplier"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
           *UEnum::GetValueAsString(Activity), CostMultiplier);
}

void UStaminaComponent::RemoveActivityModifier(EStaminaActivity Activity)
{
    ActivityModifiers.RemoveAll([Activity](const FActivityCostModifier& Modifier)
    {
        return Modifier.Activity == Activity;
    });
}

float UStaminaComponent::GetActivityCostMultiplier(EStaminaActivity Activity) const
{
    for (const FActivityCostModifier& Modifier : ActivityModifiers)
    {
        if (Modifier.Activity == Activity)
        {
            return Modifier.CostMultiplier;
        }
    }
    return 1.0f; // Default multiplier
}

void UStaminaComponent::CheckStaminaState()
{
    bool bPreviousHasStamina = bHasStamina;
    bHasStamina = CurrentMaxStamina > 0.0f;
    
    // Check for stamina empty state
    if (CurrentStamina <= 0.0f && bHasStamina)
    {
        OnStaminaEmpty.Broadcast();
        OnStaminaEmptyBP();
        
        // Stop all continuous activities when out of stamina
        StopAllContinuousActivities();
    }
}

void UStaminaComponent::BroadcastStaminaChanged()
{
    OnStaminaChanged.Broadcast(CurrentStamina, CurrentMaxStamina);
    OnStaminaChangedBP(CurrentStamina, CurrentMaxStamina);
}

bool UStaminaComponent::IsActivityActive(EStaminaActivity Activity) const
{
    for (const FStaminaCostInfo& ActivityInfo : ActiveContinuousActivities)
    {
        if (ActivityInfo.Activity == Activity)
        {
            return true;
        }
    }
    return false;
}

int32 UStaminaComponent::GetActiveActivityCount() const
{
    return ActiveContinuousActivities.Num();
}

TArray<EStaminaActivity> UStaminaComponent::GetActiveActivityTypes() const
{
    TArray<EStaminaActivity> ActivityTypes;
    ActivityTypes.Reserve(ActiveContinuousActivities.Num());
    
    for (const FStaminaCostInfo& ActivityInfo : ActiveContinuousActivities)
    {
        ActivityTypes.Add(ActivityInfo.Activity);
    }
    
    return ActivityTypes;
}

bool UStaminaComponent::HasActiveActivity(EStaminaActivity Activity) const
{
    return IsActivityActive(Activity);
}

float UStaminaComponent::GetActivityCurrentCost(EStaminaActivity Activity) const
{
    for (const FStaminaCostInfo& ActivityInfo : ActiveContinuousActivities)
    {
        if (ActivityInfo.Activity == Activity)
        {
            return CalculateActivityCost(ActivityInfo);
        }
    }
    return 0.0f;
}