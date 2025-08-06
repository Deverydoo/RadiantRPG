// Private/Components/HealthComponent.cpp

#include "Components/HealthComponent.h"
#include "Characters/BaseCharacter.h"
#include "Engine/World.h"
#include "Types/CoreTypes.h"

UHealthComponent::UHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;

    // Default settings
    BaseMaxHealth = 100.0f;
    CurrentHealth = 100.0f; // Start with full health
    CurrentMaxHealth = 100.0f;
    
    // Regeneration settings
    bCanRegenerate = true;
    HealthRegenRate = 2.0f; // 2 health per second
    RegenDelay = 5.0f; // 5 seconds after taking damage
    TimeSinceLastDamage = 0.0f;

    // State
    bIsDead = false;
    bIsInvulnerable = false;
    OwnerCharacter = nullptr;
}

void UHealthComponent::BeginPlay()
{
    Super::BeginPlay();

    // Cache owner reference
    OwnerCharacter = Cast<ABaseCharacter>(GetOwner());
    
    // Initialize health to max
    CurrentMaxHealth = BaseMaxHealth;
    CurrentHealth = CurrentMaxHealth;
    
    BroadcastHealthChanged();
    
    UE_LOG(LogTemp, Log, TEXT("HealthComponent initialized for %s with %f/%f health"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"), 
           CurrentHealth, CurrentMaxHealth);
}

void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bIsDead)
    {
        // Update time since last damage
        TimeSinceLastDamage += DeltaTime;
        
        // Update regeneration
        UpdateRegeneration(DeltaTime);
    }
}

void UHealthComponent::UpdateRegeneration(float DeltaTime)
{
    if (!bCanRegenerate || bIsDead || IsFullHealth())
        return;

    // Only regen if enough time has passed since last damage
    if (TimeSinceLastDamage >= RegenDelay)
    {
        float RegenAmount = HealthRegenRate * DeltaTime;
        Heal(RegenAmount, false);
    }
}

void UHealthComponent::TakeDamage(const FDamageInfo& DamageInfo)
{
    if (bIsDead || bIsInvulnerable || DamageInfo.Amount <= 0.0f)
        return;

    // Calculate damage after resistance
    float FinalDamage = CalculateDamageAfterResistance(DamageInfo);
    
    if (FinalDamage <= 0.0f)
        return;

    // Apply damage
    CurrentHealth = FMath::Clamp(CurrentHealth - FinalDamage, 0.0f, CurrentMaxHealth);
    
    // Reset damage timer for regeneration
    TimeSinceLastDamage = 0.0f;
    
    // Broadcast events
    OnDamageTaken.Broadcast(FinalDamage, DamageInfo.DamageSource, DamageInfo.HitLocation);
    OnDamageTakenBP(FinalDamage, DamageInfo.DamageSource, DamageInfo.HitLocation);
    
    BroadcastHealthChanged();
    
    // Check for death
    if (CurrentHealth <= 0.0f)
    {
        HandleDeath();
    }
    
    UE_LOG(LogTemp, Log, TEXT("%s took %f damage, health now %f/%f"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"), 
           FinalDamage, CurrentHealth, CurrentMaxHealth);
}

void UHealthComponent::TakeDamageSimple(float DamageAmount, AActor* DamageSource)
{
    FDamageInfo DamageInfo;
    DamageInfo.Amount = DamageAmount;
    DamageInfo.DamageType = EDamageType::Physical;
    DamageInfo.DamageSource = DamageSource;
    DamageInfo.HitLocation = OwnerCharacter ? OwnerCharacter->GetActorLocation() : FVector::ZeroVector;
    
    TakeDamage(DamageInfo);
}

void UHealthComponent::Heal(float HealAmount, bool bCanOverheal)
{
    if (bIsDead || HealAmount <= 0.0f)
        return;

    float PreviousHealth = CurrentHealth;
    
    if (bCanOverheal)
    {
        CurrentHealth += HealAmount;
    }
    else
    {
        CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0.0f, CurrentMaxHealth);
    }
    
    if (CurrentHealth != PreviousHealth)
    {
        BroadcastHealthChanged();
        OnHealedBP(HealAmount);
        
        UE_LOG(LogTemp, Log, TEXT("%s healed for %f, health now %f/%f"), 
               OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"), 
               HealAmount, CurrentHealth, CurrentMaxHealth);
    }
}

void UHealthComponent::SetHealth(float NewHealth)
{
    CurrentHealth = FMath::Max(0.0f, NewHealth);
    BroadcastHealthChanged();
    
    if (CurrentHealth <= 0.0f && !bIsDead)
    {
        HandleDeath();
    }
}

void UHealthComponent::SetMaxHealth(float NewMaxHealth, bool bHealToFull)
{
    float OldMaxHealth = CurrentMaxHealth;
    CurrentMaxHealth = FMath::Max(1.0f, NewMaxHealth);
    
    if (bHealToFull)
    {
        CurrentHealth = CurrentMaxHealth;
    }
    else
    {
        // Adjust current health proportionally
        if (OldMaxHealth > 0.0f)
        {
            float HealthRatio = CurrentHealth / OldMaxHealth;
            CurrentHealth = CurrentMaxHealth * HealthRatio;
        }
    }
    
    BroadcastHealthChanged();
}

void UHealthComponent::AddMaxHealth(float HealthToAdd, bool bHealToFull)
{
    SetMaxHealth(CurrentMaxHealth + HealthToAdd, bHealToFull);
}

void UHealthComponent::SetInvulnerable(bool bNewInvulnerable)
{
    bIsInvulnerable = bNewInvulnerable;
    
    UE_LOG(LogTemp, Log, TEXT("%s invulnerability set to: %s"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"), 
           bIsInvulnerable ? TEXT("true") : TEXT("false"));
}

void UHealthComponent::Kill()
{
    if (bIsDead)
        return;
        
    CurrentHealth = 0.0f;
    HandleDeath();
}

void UHealthComponent::Revive(float HealthPercent)
{
    if (!bIsDead)
        return;
        
    bIsDead = false;
    CurrentHealth = CurrentMaxHealth * FMath::Clamp(HealthPercent, 0.0f, 1.0f);
    
    BroadcastHealthChanged();
    
    UE_LOG(LogTemp, Log, TEXT("%s revived with %f health"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"), 
           CurrentHealth);
}

float UHealthComponent::GetHealthPercent() const
{
    return CurrentMaxHealth > 0.0f ? CurrentHealth / CurrentMaxHealth : 0.0f;
}

bool UHealthComponent::IsFullHealth() const
{
    return FMath::IsNearlyEqual(CurrentHealth, CurrentMaxHealth, 0.1f);
}

void UHealthComponent::AddResistance(EDamageType DamageType, float ResistancePercent)
{
    ResistancePercent = FMath::Clamp(ResistancePercent, 0.0f, 1.0f);
    
    // Find existing resistance or add new one
    for (FDamageResistance& Resistance : DamageResistances)
    {
        if (Resistance.DamageType == DamageType)
        {
            Resistance.ResistancePercent = ResistancePercent;
            return;
        }
    }
    
    // Add new resistance
    FDamageResistance NewResistance;
    NewResistance.DamageType = DamageType;
    NewResistance.ResistancePercent = ResistancePercent;
    DamageResistances.Add(NewResistance);
}

void UHealthComponent::RemoveResistance(EDamageType DamageType)
{
    DamageResistances.RemoveAll([DamageType](const FDamageResistance& Resistance)
    {
        return Resistance.DamageType == DamageType;
    });
}

float UHealthComponent::GetResistance(EDamageType DamageType) const
{
    for (const FDamageResistance& Resistance : DamageResistances)
    {
        if (Resistance.DamageType == DamageType)
        {
            return Resistance.ResistancePercent;
        }
    }
    return 0.0f;
}

float UHealthComponent::CalculateDamageAfterResistance(const FDamageInfo& DamageInfo) const
{
    float Resistance = GetResistance(DamageInfo.DamageType);
    return DamageInfo.Amount * (1.0f - Resistance);
}

void UHealthComponent::HandleDeath()
{
    if (bIsDead)
        return;
        
    bIsDead = true;
    CurrentHealth = 0.0f;
    
    // Broadcast death events
    OnDeath.Broadcast(OwnerCharacter);
    OnDeathBP();
    
    BroadcastHealthChanged();
    
    UE_LOG(LogTemp, Log, TEXT("%s has died"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"));
}

void UHealthComponent::BroadcastHealthChanged()
{
    OnHealthChanged.Broadcast(CurrentHealth, CurrentMaxHealth);
    OnHealthChangedBP(CurrentHealth, CurrentMaxHealth);
}