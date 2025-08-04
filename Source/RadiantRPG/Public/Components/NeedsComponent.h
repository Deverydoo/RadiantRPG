// Public/Components/NeedsComponent.h
// Basic needs system for characters (hunger, thirst, sleep, etc.)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "NeedsComponent.generated.h"

class ABaseCharacter;

UENUM(BlueprintType)
enum class ENeedType : uint8
{
    Hunger UMETA(DisplayName = "Hunger"),
    Thirst UMETA(DisplayName = "Thirst"),
    Sleep UMETA(DisplayName = "Sleep"),
    Comfort UMETA(DisplayName = "Comfort"),
    Social UMETA(DisplayName = "Social"),
    Safety UMETA(DisplayName = "Safety"),
    Cleanliness UMETA(DisplayName = "Cleanliness")
};

USTRUCT(BlueprintType)
struct FNeedData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need")
    ENeedType NeedType;

    /** Current need level (0.0 = desperate, 1.0 = fully satisfied) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CurrentLevel;

    /** Rate at which this need decays per second */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need", meta = (ClampMin = "0.0"))
    float DecayRate;

    /** Threshold below which this need becomes critical */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CriticalThreshold;

    /** Whether this need is currently critical */
    UPROPERTY(BlueprintReadOnly, Category = "Need")
    bool bIsCritical;

    /** Whether this need affects character behavior */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need")
    bool bIsActive;

    FNeedData()
    {
        NeedType = ENeedType::Hunger;
        CurrentLevel = 1.0f;
        DecayRate = 0.001f; // Very slow decay by default
        CriticalThreshold = 0.2f;
        bIsCritical = false;
        bIsActive = true;
    }
};

// Need-related events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNeedChanged, ENeedType, NeedType, float, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNeedCritical, ENeedType, NeedType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNeedSatisfied, ENeedType, NeedType);

/**
 * NeedsComponent - Manages basic character needs
 * 
 * Handles hunger, thirst, sleep, and other basic needs that drive NPC behavior
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
    TMap<ENeedType, FNeedData> Needs;

    /** Whether needs system is active for this character */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Needs")
    bool bNeedsActive;

    /** Global decay multiplier for all needs */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Needs", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float GlobalDecayMultiplier;

    /** Cached owner reference */
    UPROPERTY()
    ABaseCharacter* OwnerCharacter;

public:
    // === EVENTS ===
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnNeedChanged OnNeedChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnNeedCritical OnNeedCritical;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnNeedSatisfied OnNeedSatisfied;

    // === PUBLIC INTERFACE ===
    
    /** Get current level of a specific need */
    UFUNCTION(BlueprintPure, Category = "Needs")
    float GetNeedLevel(ENeedType NeedType) const;

    /** Set need level directly */
    UFUNCTION(BlueprintCallable, Category = "Needs")
    void SetNeedLevel(ENeedType NeedType, float NewLevel);

    /** Modify need level by an amount */
    UFUNCTION(BlueprintCallable, Category = "Needs")
    void ModifyNeedLevel(ENeedType NeedType, float Amount);

    /** Check if a need is critical */
    UFUNCTION(BlueprintPure, Category = "Needs")
    bool IsNeedCritical(ENeedType NeedType) const;

    /** Get all critical needs */
    UFUNCTION(BlueprintPure, Category = "Needs")
    TArray<ENeedType> GetCriticalNeeds() const;

    /** Get the most critical need */
    UFUNCTION(BlueprintPure, Category = "Needs")
    ENeedType GetMostCriticalNeed() const;

    /** Check if any needs are critical */
    UFUNCTION(BlueprintPure, Category = "Needs")
    bool HasCriticalNeeds() const;

    /** Satisfy a need (set to maximum) */
    UFUNCTION(BlueprintCallable, Category = "Needs")
    void SatisfyNeed(ENeedType NeedType);

    /** Satisfy all needs */
    UFUNCTION(BlueprintCallable, Category = "Needs")
    void SatisfyAllNeeds();

    /** Enable/disable the needs system */
    UFUNCTION(BlueprintCallable, Category = "Needs")
    void SetNeedsActive(bool bActive);

    /** Add a new need type */
    UFUNCTION(BlueprintCallable, Category = "Needs")
    void AddNeed(ENeedType NeedType, float StartingLevel = 1.0f, float DecayRate = 0.001f);

    /** Remove a need type */
    UFUNCTION(BlueprintCallable, Category = "Needs")
    void RemoveNeed(ENeedType NeedType);

protected:
    // === INTERNAL FUNCTIONS ===
    
    /** Initialize default needs */
    void InitializeDefaultNeeds();

    /** Update all needs over time */
    void UpdateNeeds(float DeltaTime);

    /** Check and update critical status for a need */
    void UpdateNeedCriticalStatus(ENeedType NeedType);

    /** Broadcast need change events */
    void BroadcastNeedChanged(ENeedType NeedType, float NewLevel);

    // === BLUEPRINT EVENTS ===
    
    UFUNCTION(BlueprintImplementableEvent, Category = "Needs Events")
    void OnNeedChangedBP(ENeedType NeedType, float NewLevel);

    UFUNCTION(BlueprintImplementableEvent, Category = "Needs Events")
    void OnNeedCriticalBP(ENeedType NeedType);

    UFUNCTION(BlueprintImplementableEvent, Category = "Needs Events")
    void OnNeedSatisfiedBP(ENeedType NeedType);
};