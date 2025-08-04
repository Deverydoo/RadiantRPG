// Public/Components/StaminaComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StaminaComponent.generated.h"

class ABaseCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStaminaChanged, float, NewStamina, float, NewMaxStamina);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStaminaSpent, float, StaminaCost, AActor*, Actor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaminaEmpty);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnExhaustionStateChanged, bool, bIsExhausted);

UENUM(BlueprintType)
enum class EStaminaActivity : uint8
{
    Walking UMETA(DisplayName = "Walking"),
    Running UMETA(DisplayName = "Running"),
    Sprinting UMETA(DisplayName = "Sprinting"),
    Jumping UMETA(DisplayName = "Jumping"),
    Attacking UMETA(DisplayName = "Attacking"),
    Blocking UMETA(DisplayName = "Blocking"),
    Dodging UMETA(DisplayName = "Dodging"),
    Climbing UMETA(DisplayName = "Climbing"),
    Swimming UMETA(DisplayName = "Swimming"),
    HeavyLifting UMETA(DisplayName = "Heavy Lifting")
};

USTRUCT(BlueprintType)
struct FStaminaCostInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
    float BaseCost;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
    EStaminaActivity Activity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
    AActor* Actor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
    bool bIsContinuous; // For activities like running

    FStaminaCostInfo()
    {
        BaseCost = 0.0f;
        Activity = EStaminaActivity::Walking;
        Actor = nullptr;
        bIsContinuous = false;
    }
};

USTRUCT(BlueprintType)
struct FActivityCostModifier
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Activity")
    EStaminaActivity Activity;

    // Cost multiplier for this activity (0.5 = half cost, 2.0 = double cost)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Activity", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float CostMultiplier;

    FActivityCostModifier()
    {
        Activity = EStaminaActivity::Walking;
        CostMultiplier = 1.0f;
    }
};

/**
 * Component that manages character stamina for physical activities
 * Handles stamina consumption, regeneration, exhaustion, and activity-based modifiers
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RADIANTRPG_API UStaminaComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UStaminaComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Base Stamina Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = "1.0"))
    float BaseMaxStamina;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
    float CurrentStamina;

    UPROPERTY(BlueprintReadOnly, Category = "Stamina")
    float CurrentMaxStamina;

    // Regeneration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regeneration")
    bool bCanRegenerate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regeneration", meta = (ClampMin = "0.0"))
    float StaminaRegenRate; // Stamina per second

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regeneration", meta = (ClampMin = "0.0"))
    float RegenDelay; // Delay after spending stamina before regen starts

    UPROPERTY(BlueprintReadOnly, Category = "Regeneration")
    float TimeSinceLastActivity;

    // Exhaustion
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exhaustion", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ExhaustionThreshold; // Percentage of stamina at which exhaustion sets in

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exhaustion", meta = (ClampMin = "0.1", ClampMax = "1.0"))
    float ExhaustedRegenMultiplier; // Regen rate multiplier when exhausted

    UPROPERTY(BlueprintReadOnly, Category = "Exhaustion")
    bool bIsExhausted;

    // Activity Modifiers
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Activities")
    TArray<FActivityCostModifier> ActivityModifiers;

    // Continuous Activities
    UPROPERTY(BlueprintReadOnly, Category = "Activities")
    TArray<FStaminaCostInfo> ActiveContinuousActivities;

    // State
    UPROPERTY(BlueprintReadOnly, Category = "Stamina")
    bool bHasStamina;

    // Cached owner reference
    UPROPERTY()
    ABaseCharacter* OwnerCharacter;

public:
    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnStaminaChanged OnStaminaChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnStaminaSpent OnStaminaSpent;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnStaminaEmpty OnStaminaEmpty;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnExhaustionStateChanged OnExhaustionStateChanged;

    // Public functions
    UFUNCTION(BlueprintCallable, Category = "Stamina")
    bool TrySpendStamina(const FStaminaCostInfo& ActivityInfo);

    UFUNCTION(BlueprintCallable, Category = "Stamina")
    bool TrySpendStaminaSimple(float StaminaCost, AActor* Actor = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Stamina")
    void RestoreStamina(float StaminaAmount);

    UFUNCTION(BlueprintCallable, Category = "Stamina")
    void SetStamina(float NewStamina);

    UFUNCTION(BlueprintCallable, Category = "Stamina")
    void SetMaxStamina(float NewMaxStamina, bool bRestoreToFull = false);

    UFUNCTION(BlueprintCallable, Category = "Stamina")
    void AddMaxStamina(float StaminaToAdd, bool bRestoreToFull = false);

    UFUNCTION(BlueprintCallable, Category = "Stamina")
    void DrainAllStamina();

    // Continuous activity management
    UFUNCTION(BlueprintCallable, Category = "Activities")
    void StartContinuousActivity(const FStaminaCostInfo& ActivityInfo);

    UFUNCTION(BlueprintCallable, Category = "Activities")
    void StopContinuousActivity(EStaminaActivity Activity);

    UFUNCTION(BlueprintCallable, Category = "Activities")
    void StopAllContinuousActivities();

    // Getters
    UFUNCTION(BlueprintPure, Category = "Stamina")
    float GetStamina() const { return CurrentStamina; }

    UFUNCTION(BlueprintPure, Category = "Stamina")
    float GetMaxStamina() const { return CurrentMaxStamina; }

    UFUNCTION(BlueprintPure, Category = "Stamina")
    float GetStaminaPercent() const;

    UFUNCTION(BlueprintPure, Category = "Stamina")
    bool HasStamina() const { return bHasStamina && CurrentStamina > 0.0f; }

    UFUNCTION(BlueprintPure, Category = "Stamina")
    bool IsFullStamina() const;

    UFUNCTION(BlueprintPure, Category = "Stamina")
    bool IsExhausted() const { return bIsExhausted; }

    UFUNCTION(BlueprintPure, Category = "Stamina")
    bool CanPerformActivity(const FStaminaCostInfo& ActivityInfo) const;

    UFUNCTION(BlueprintPure, Category = "Stamina")
    bool CanPerformActivitySimple(float StaminaCost) const;

    UFUNCTION(BlueprintPure, Category = "Stamina")
    float CalculateActivityCost(const FStaminaCostInfo& ActivityInfo) const;

    // Activity modifier functions
    UFUNCTION(BlueprintCallable, Category = "Activities")
    void SetActivityModifier(EStaminaActivity Activity, float CostMultiplier);

    UFUNCTION(BlueprintCallable, Category = "Activities")
    void RemoveActivityModifier(EStaminaActivity Activity);

    UFUNCTION(BlueprintPure, Category = "Activities")
    float GetActivityCostMultiplier(EStaminaActivity Activity) const;

protected:
    // Internal functions
    void UpdateRegeneration(float DeltaTime);
    void UpdateContinuousActivities(float DeltaTime);
    void UpdateExhaustionState();
    void BroadcastStaminaChanged();
    void CheckStaminaState();

    // Blueprint events
    UFUNCTION(BlueprintImplementableEvent, Category = "Stamina")
    void OnStaminaChangedBP(float NewStamina, float NewMaxStamina);

    UFUNCTION(BlueprintImplementableEvent, Category = "Stamina")
    void OnStaminaSpentBP(float StaminaCost, AActor* Actor);

    UFUNCTION(BlueprintImplementableEvent, Category = "Stamina")
    void OnStaminaEmptyBP();

    UFUNCTION(BlueprintImplementableEvent, Category = "Stamina")
    void OnExhaustionStateChangedBP(bool bNewExhausted);

    UFUNCTION(BlueprintImplementableEvent, Category = "Stamina")
    void OnStaminaRestoredBP(float StaminaAmount);
};