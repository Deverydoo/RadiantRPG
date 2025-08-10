// Public/Components/NeedsComponent.h
// Manages basic character needs system for RadiantRPG

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Types/ARPG_AITypes.h"
#include "NeedsComponent.generated.h"

class ABaseCharacter;

// Need-related events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNeedChanged, EARPG_NeedType, NeedType, float, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNeedCritical, EARPG_NeedType, NeedType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNeedSatisfied, EARPG_NeedType, NeedType);

/**
 * NeedsComponent - Manages basic character needs
 * 
 * Handles hunger, fatigue, safety, and other basic needs that drive NPC behavior
 * and can optionally affect player character as well.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RADIANTRPG_API UNeedsComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UNeedsComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** Map of all needs for this character */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Needs")
    TMap<EARPG_NeedType, FARPG_AINeed> Needs;

    /** Whether needs system is active for this character */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Needs")
    bool bNeedsActive;

    /** Global decay multiplier for all needs */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Needs", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float GlobalDecayMultiplier;

    /** Cached owner reference */
    UPROPERTY()
    ABaseCharacter* OwnerCharacter;

    /** Last time we logged needs update for throttling */
    float LastLogTime;

public:
    // === EVENTS ===
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnNeedChanged OnNeedChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnNeedCritical OnNeedCritical;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnNeedSatisfied OnNeedSatisfied;

    // === BLUEPRINT FUNCTIONS ===
    UFUNCTION(BlueprintCallable, Category = "Needs")
    float GetNeedLevel(EARPG_NeedType NeedType) const;

    UFUNCTION(BlueprintCallable, Category = "Needs")
    void SetNeedLevel(EARPG_NeedType NeedType, float NewLevel);

    UFUNCTION(BlueprintCallable, Category = "Needs")
    void ModifyNeedLevel(EARPG_NeedType NeedType, float Amount);

    UFUNCTION(BlueprintCallable, Category = "Needs")
    bool IsNeedCritical(EARPG_NeedType NeedType) const;

    UFUNCTION(BlueprintCallable, Category = "Needs")
    TArray<EARPG_NeedType> GetCriticalNeeds() const;

    UFUNCTION(BlueprintCallable, Category = "Needs")
    EARPG_NeedType GetMostCriticalNeed() const;

    UFUNCTION(BlueprintCallable, Category = "Needs")
    bool HasCriticalNeeds() const;

    UFUNCTION(BlueprintCallable, Category = "Needs")
    void SatisfyNeed(EARPG_NeedType NeedType);

    UFUNCTION(BlueprintCallable, Category = "Needs")
    void SatisfyAllNeeds();

    UFUNCTION(BlueprintCallable, Category = "Needs")
    void SetNeedsActive(bool bActive);

    UFUNCTION(BlueprintCallable, Category = "Needs")
    void AddNeed(EARPG_NeedType NeedType, float StartingLevel = 0.5f, float DecayRate = 0.001f);

    UFUNCTION(BlueprintCallable, Category = "Needs")
    void RemoveNeed(EARPG_NeedType NeedType);

    // === BLUEPRINT EVENTS ===
    UFUNCTION(BlueprintImplementableEvent, Category = "Needs Events")
    void OnNeedChangedBP(EARPG_NeedType NeedType, float NewLevel);

    UFUNCTION(BlueprintImplementableEvent, Category = "Needs Events")
    void OnNeedCriticalBP(EARPG_NeedType NeedType);

    UFUNCTION(BlueprintImplementableEvent, Category = "Needs Events")
    void OnNeedSatisfiedBP(EARPG_NeedType NeedType);

protected:
    // === CORE SYSTEM FUNCTIONS ===
    
    /** Initialize default needs for this character */
    void InitializeDefaultNeeds();
    
    /** Update all needs based on time passage */
    void UpdateNeeds(float DeltaTime);
    
    /** Update critical status for a specific need and broadcast events */
    void UpdateNeedCriticalStatus(EARPG_NeedType NeedType);
    
    /** Broadcast need changed event to both C++ and Blueprint listeners */
    void BroadcastNeedChanged(EARPG_NeedType NeedType, float NewLevel);

    // === HELPER FUNCTIONS ===
    
    /** Log needs update information with throttling */
    void LogNeedsUpdate(bool bAnyNeedChanged, int32 CriticalCount);
    
    /** Set appropriate critical threshold based on need type */
    void SetCriticalThresholdForNeedType(FARPG_AINeed& Need);
    
    /** Log when a new need is added */
    void LogNeedAddition(EARPG_NeedType NeedType, const FARPG_AINeed& Need);
};