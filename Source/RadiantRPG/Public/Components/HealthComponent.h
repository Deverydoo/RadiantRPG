// Public/Components/HealthComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Types/CoreTypes.h"
#include "HealthComponent.generated.h"

class ABaseCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, NewHealth, float, NewMaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeath, ABaseCharacter*, DeadCharacter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDamageTaken, float, DamageAmount, AActor*, DamageSource, const FVector&, HitLocation);

/**
 * Component that manages character health, damage, death, and regeneration
 * Can be attached to any actor that needs health functionality
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RADIANTRPG_API UHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UHealthComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Base Health Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (ClampMin = "1.0"))
    float BaseMaxHealth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    float CurrentHealth;

    UPROPERTY(BlueprintReadOnly, Category = "Health")
    float CurrentMaxHealth;

    // Regeneration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regeneration")
    bool bCanRegenerate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regeneration", meta = (ClampMin = "0.0"))
    float HealthRegenRate; // Health per second

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regeneration", meta = (ClampMin = "0.0"))
    float RegenDelay; // Delay after taking damage before regen starts

    UPROPERTY(BlueprintReadOnly, Category = "Regeneration")
    float TimeSinceLastDamage;

    // Damage Resistances
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resistances")
    TArray<FDamageResistance> DamageResistances;

    // State
    UPROPERTY(BlueprintReadOnly, Category = "Health")
    bool bIsDead;

    UPROPERTY(BlueprintReadOnly, Category = "Health")
    bool bIsInvulnerable;

    // Cached owner reference
    UPROPERTY()
    ABaseCharacter* OwnerCharacter;

public:
    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnHealthChanged OnHealthChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnDeath OnDeath;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnDamageTaken OnDamageTaken;

    // Public functions
    UFUNCTION(BlueprintCallable, Category = "Health")
    void TakeDamage(const FDamageInfo& DamageInfo);

    UFUNCTION(BlueprintCallable, Category = "Health")
    void TakeDamageSimple(float DamageAmount, AActor* DamageSource = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Health")
    void Heal(float HealAmount, bool bCanOverheal = false);

    UFUNCTION(BlueprintCallable, Category = "Health")
    void SetHealth(float NewHealth);

    UFUNCTION(BlueprintCallable, Category = "Health")
    void SetMaxHealth(float NewMaxHealth, bool bHealToFull = false);

    UFUNCTION(BlueprintCallable, Category = "Health")
    void AddMaxHealth(float HealthToAdd, bool bHealToFull = false);

    UFUNCTION(BlueprintCallable, Category = "Health")
    void SetInvulnerable(bool bNewInvulnerable);

    UFUNCTION(BlueprintCallable, Category = "Health")
    void Kill();

    UFUNCTION(BlueprintCallable, Category = "Health")
    void Revive(float HealthPercent = 1.0f);

    // Getters
    UFUNCTION(BlueprintPure, Category = "Health")
    float GetHealth() const { return CurrentHealth; }

    UFUNCTION(BlueprintPure, Category = "Health")
    float GetMaxHealth() const { return CurrentMaxHealth; }

    UFUNCTION(BlueprintPure, Category = "Health")
    float GetHealthPercent() const;

    UFUNCTION(BlueprintPure, Category = "Health")
    bool IsDead() const { return bIsDead; }

    UFUNCTION(BlueprintPure, Category = "Health")
    bool IsFullHealth() const;

    UFUNCTION(BlueprintPure, Category = "Health")
    bool IsInvulnerable() const { return bIsInvulnerable; }

    // Resistance functions
    UFUNCTION(BlueprintCallable, Category = "Resistances")
    void AddResistance(EDamageType DamageType, float ResistancePercent);

    UFUNCTION(BlueprintCallable, Category = "Resistances")
    void RemoveResistance(EDamageType DamageType);

    UFUNCTION(BlueprintPure, Category = "Resistances")
    float GetResistance(EDamageType DamageType) const;

protected:
    // Internal functions
    void UpdateRegeneration(float DeltaTime);
    float CalculateDamageAfterResistance(const FDamageInfo& DamageInfo) const;
    void HandleDeath();
    void BroadcastHealthChanged();

    // Blueprint events
    UFUNCTION(BlueprintImplementableEvent, Category = "Health")
    void OnHealthChangedBP(float NewHealth, float NewMaxHealth);

    UFUNCTION(BlueprintImplementableEvent, Category = "Health")
    void OnDeathBP();

    UFUNCTION(BlueprintImplementableEvent, Category = "Health")
    void OnDamageTakenBP(float DamageAmount, AActor* DamageSource, const FVector& HitLocation);

    UFUNCTION(BlueprintImplementableEvent, Category = "Health")
    void OnHealedBP(float HealAmount);
};