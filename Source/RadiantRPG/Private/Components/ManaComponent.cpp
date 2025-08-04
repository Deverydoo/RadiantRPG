// Private/Components/ManaComponent.cpp

#include "Components/ManaComponent.h"
#include "Characters/BaseCharacter.h"
#include "Engine/World.h"

UManaComponent::UManaComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;

    // Default settings
    BaseMaxMana = 100.0f;
    CurrentMana = 100.0f; // Start with full mana
    CurrentMaxMana = 100.0f;
    
    // Regeneration settings
    bCanRegenerate = true;
    ManaRegenRate = 5.0f; // 5 mana per second
    RegenDelay = 3.0f; // 3 seconds after casting spell
    TimeSinceLastSpell = 0.0f;

    // State
    bHasMana = true;
    OwnerCharacter = nullptr;
}

void UManaComponent::BeginPlay()
{
    Super::BeginPlay();

    // Cache owner reference
    OwnerCharacter = Cast<ABaseCharacter>(GetOwner());
    
    // Initialize mana to max
    CurrentMaxMana = BaseMaxMana;
    CurrentMana = CurrentMaxMana;
    
    CheckManaState();
    BroadcastManaChanged();
    
    UE_LOG(LogTemp, Log, TEXT("ManaComponent initialized for %s with %f/%f mana"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"), 
           CurrentMana, CurrentMaxMana);
}

void UManaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Update time since last spell
    TimeSinceLastSpell += DeltaTime;
    
    // Update regeneration
    UpdateRegeneration(DeltaTime);
}

void UManaComponent::UpdateRegeneration(float DeltaTime)
{
    if (!bCanRegenerate || IsFullMana())
        return;

    // Only regen if enough time has passed since last spell
    if (TimeSinceLastSpell >= RegenDelay)
    {
        float RegenAmount = ManaRegenRate * DeltaTime;
        RestoreMana(RegenAmount);
    }
}

bool UManaComponent::TrySpendMana(const FSpellCostInfo& SpellInfo)
{
    float TotalCost = CalculateSpellCost(SpellInfo);
    
    if (CurrentMana < TotalCost)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s insufficient mana: %f needed, %f available"), 
               OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"), 
               TotalCost, CurrentMana);
        return false;
    }

    // Spend the mana
    CurrentMana = FMath::Max(0.0f, CurrentMana - TotalCost);
    
    // Reset spell timer for regeneration
    TimeSinceLastSpell = 0.0f;
    
    // Broadcast events
    OnManaSpent.Broadcast(TotalCost, SpellInfo.SpellCaster);
    OnManaSpentBP(TotalCost, SpellInfo.SpellCaster);
    
    CheckManaState();
    BroadcastManaChanged();
    
    UE_LOG(LogTemp, Log, TEXT("%s spent %f mana, %f remaining"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"), 
           TotalCost, CurrentMana);
    
    return true;
}

bool UManaComponent::TrySpendManaSimple(float ManaCost, AActor* SpellCaster)
{
    FSpellCostInfo SpellInfo;
    SpellInfo.BaseCost = ManaCost;
    SpellInfo.School = EManaSchool::Arcane;
    SpellInfo.SpellCaster = SpellCaster;
    
    return TrySpendMana(SpellInfo);
}

void UManaComponent::RestoreMana(float ManaAmount)
{
    if (ManaAmount <= 0.0f)
        return;

    float PreviousMana = CurrentMana;
    CurrentMana = FMath::Clamp(CurrentMana + ManaAmount, 0.0f, CurrentMaxMana);
    
    if (CurrentMana != PreviousMana)
    {
        CheckManaState();
        BroadcastManaChanged();
        OnManaRestoredBP(ManaAmount);
        
        UE_LOG(LogTemp, Verbose, TEXT("%s restored %f mana, now %f/%f"), 
               OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"), 
               ManaAmount, CurrentMana, CurrentMaxMana);
    }
}

void UManaComponent::SetMana(float NewMana)
{
    CurrentMana = FMath::Clamp(NewMana, 0.0f, CurrentMaxMana);
    CheckManaState();
    BroadcastManaChanged();
}

void UManaComponent::SetMaxMana(float NewMaxMana, bool bRestoreToFull)
{
    float OldMaxMana = CurrentMaxMana;
    CurrentMaxMana = FMath::Max(0.0f, NewMaxMana);
    
    if (bRestoreToFull)
    {
        CurrentMana = CurrentMaxMana;
    }
    else
    {
        // Adjust current mana proportionally
        if (OldMaxMana > 0.0f)
        {
            float ManaRatio = CurrentMana / OldMaxMana;
            CurrentMana = CurrentMaxMana * ManaRatio;
        }
        else
        {
            CurrentMana = CurrentMaxMana;
        }
    }
    
    CheckManaState();
    BroadcastManaChanged();
}

void UManaComponent::AddMaxMana(float ManaToAdd, bool bRestoreToFull)
{
    SetMaxMana(CurrentMaxMana + ManaToAdd, bRestoreToFull);
}

void UManaComponent::DrainAllMana()
{
    SetMana(0.0f);
    OnManaEmpty.Broadcast();
    OnManaEmptyBP();
}

float UManaComponent::GetManaPercent() const
{
    return CurrentMaxMana > 0.0f ? CurrentMana / CurrentMaxMana : 0.0f;
}

bool UManaComponent::IsFullMana() const
{
    return FMath::IsNearlyEqual(CurrentMana, CurrentMaxMana, 0.1f);
}

bool UManaComponent::CanCastSpell(const FSpellCostInfo& SpellInfo) const
{
    if (!bHasMana || CurrentMaxMana <= 0.0f)
        return false;
        
    float TotalCost = CalculateSpellCost(SpellInfo);
    return CurrentMana >= TotalCost;
}

bool UManaComponent::CanCastSpellSimple(float ManaCost) const
{
    return bHasMana && CurrentMana >= ManaCost;
}

float UManaComponent::CalculateSpellCost(const FSpellCostInfo& SpellInfo) const
{
    float Cost = SpellInfo.BaseCost;
    
    // Apply school affinity modifiers
    float Multiplier = GetSchoolCostMultiplier(SpellInfo.School);
    Cost *= Multiplier;
    
    return FMath::Max(0.0f, Cost);
}

void UManaComponent::SetSchoolAffinity(EManaSchool School, float CostMultiplier)
{
    CostMultiplier = FMath::Clamp(CostMultiplier, 0.1f, 5.0f);
    
    // Find existing affinity or add new one
    for (FManaAffinityModifier& Affinity : SchoolAffinities)
    {
        if (Affinity.School == School)
        {
            Affinity.CostMultiplier = CostMultiplier;
            return;
        }
    }
    
    // Add new affinity
    FManaAffinityModifier NewAffinity;
    NewAffinity.School = School;
    NewAffinity.CostMultiplier = CostMultiplier;
    SchoolAffinities.Add(NewAffinity);
    
    UE_LOG(LogTemp, Log, TEXT("%s gained %s affinity with %.2fx cost multiplier"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
           *UEnum::GetValueAsString(School), CostMultiplier);
}

void UManaComponent::RemoveSchoolAffinity(EManaSchool School)
{
    SchoolAffinities.RemoveAll([School](const FManaAffinityModifier& Affinity)
    {
        return Affinity.School == School;
    });
}

float UManaComponent::GetSchoolCostMultiplier(EManaSchool School) const
{
    for (const FManaAffinityModifier& Affinity : SchoolAffinities)
    {
        if (Affinity.School == School)
        {
            return Affinity.CostMultiplier;
        }
    }
    return 1.0f; // Default multiplier
}

void UManaComponent::CheckManaState()
{
    bool bPreviousHasMana = bHasMana;
    bHasMana = CurrentMaxMana > 0.0f;
    
    // Check for mana empty state
    if (CurrentMana <= 0.0f && bHasMana)
    {
        OnManaEmpty.Broadcast();
        OnManaEmptyBP();
    }
}

void UManaComponent::BroadcastManaChanged()
{
    OnManaChanged.Broadcast(CurrentMana, CurrentMaxMana);
    OnManaChangedBP(CurrentMana, CurrentMaxMana);
}