// Public/Components/NeedsComponent.h
// Base needs system for all characters in RadiantRPG

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Types/ARPG_AIDataTableTypes.h"
#include "Types/ARPG_AITypes.h"
#include "Types/ARPG_NPCTypes.h"
#include "Types/ARPG_NeedsDataTypes.h"
#include "NeedsComponent.generated.h"

class ABaseCharacter;

// Need-related events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNeedChanged, EARPG_NeedType, NeedType, float, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNeedCritical, EARPG_NeedType, NeedType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNeedSatisfied, EARPG_NeedType, NeedType);

/**
 * Base NeedsComponent - Manages character needs for both players and NPCs
 * 
 * Handles hunger, fatigue, safety, and other basic needs.
 * AI-specific functionality should be implemented in derived UARPG_AINeedsComponent.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
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
    TMap<EARPG_NeedType, FARPG_AINeed> CurrentNeeds;

    /** Configuration for needs system */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FARPG_NeedsConfiguration NeedsConfig;

    /** Data table containing creature needs profiles */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    UDataTable* CreatureNeedsDataTable;

    /** Creature archetype to use for needs configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    EARPG_CreatureArchetype CreatureArchetype = EARPG_CreatureArchetype::Human;

    /** Row name to use from the data table (if empty, uses archetype enum name) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FName DataTableRowName;

    /** Whether needs system is active for this character */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Needs")
    bool bNeedsActive;

    /** Whether needs updates are paused */
    UPROPERTY(BlueprintReadWrite, Category = "State")
    bool bNeedsUpdatePaused;

    /** Global decay multiplier for all needs */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Needs", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float GlobalDecayMultiplier;

    /** Whether to auto-populate default needs if CurrentNeeds is empty */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    bool bAutoPopulateDefaults = true;

    /** Cached owner reference */
    UPROPERTY()
    ABaseCharacter* OwnerCharacter;

    /** Time tracking */
    float TimeSinceLastUpdate;
    float LastLogTime;

public:
    // === EVENTS ===
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnNeedChanged OnNeedChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnNeedCritical OnNeedCritical;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnNeedSatisfied OnNeedSatisfied;

    // === Core Interface ===
    
    /** Get current level of a specific need */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Needs")
    virtual float GetNeedLevel(EARPG_NeedType NeedType) const;

    /** Set the level of a specific need */
    UFUNCTION(BlueprintCallable, Category = "Needs")
    virtual void SetNeedLevel(EARPG_NeedType NeedType, float NewLevel);

    /** Modify a need by a delta amount */
    UFUNCTION(BlueprintCallable, Category = "Needs")
    virtual void ModifyNeedLevel(EARPG_NeedType NeedType, float DeltaAmount);

    /** Get all current needs */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Needs")
    virtual TMap<EARPG_NeedType, FARPG_AINeed> GetAllNeeds() const { return CurrentNeeds; }

    /** Check if a need is in critical state */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Needs")
    virtual bool IsNeedCritical(EARPG_NeedType NeedType) const;

    /** Get all needs that are currently critical */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Needs")
    virtual TArray<EARPG_NeedType> GetCriticalNeeds() const;

    /** Get all needs that are currently urgent (above urgency threshold) */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Needs")
    virtual TArray<EARPG_NeedType> GetUrgentNeeds() const;

    /** Initialize needs with custom configuration */
    UFUNCTION(BlueprintCallable, Category = "Needs")
    virtual void InitializeNeeds(const FARPG_NeedsConfiguration& Config, const TArray<FARPG_AINeed>& InitialNeeds);

    /** Initialize needs from creature archetype data table */
    UFUNCTION(BlueprintCallable, Category = "Needs")
    virtual void InitializeNeedsFromArchetype(EARPG_CreatureArchetype Archetype, UDataTable* DataTable = nullptr);

    /** Load needs configuration from data table by row name */
    UFUNCTION(BlueprintCallable, Category = "Needs")
    virtual bool LoadNeedsFromDataTable(FName RowName, UDataTable* DataTable = nullptr);

    /** Update needs configuration */
    UFUNCTION(BlueprintCallable, Category = "Needs")
    virtual void UpdateNeedsConfiguration(const FARPG_NeedsConfiguration& NewConfig);

    /** Pause/unpause needs updates */
    UFUNCTION(BlueprintCallable, Category = "Needs")
    virtual void SetNeedsUpdatePaused(bool bPaused) { bNeedsUpdatePaused = bPaused; }

    /** Check if needs updates are paused */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Needs")
    virtual bool AreNeedsUpdatesPaused() const { return bNeedsUpdatePaused; }

    /** Get the creature archetype being used */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Configuration")
    EARPG_CreatureArchetype  GetCreatureArchetype() const { return CreatureArchetype; }
    
    /** Set the creature archetype and optionally reload needs */
    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void SetCreatureArchetype(EARPG_CreatureArchetype  NewArchetype, bool bReloadNeeds = false);

protected:
    // === Internal Functions ===

    /** Initialize default needs if none are configured */
    virtual void InitializeDefaultNeeds();

    /** Auto-populate needs based on current creature archetype */
    virtual void AutoPopulateNeedsFromArchetype();

    /** Create a default need configuration for a specific need type */
    virtual FARPG_AINeed CreateDefaultNeedConfig(EARPG_NeedType NeedType) const;

    /** Update all needs over time based on decay rates */
    virtual void UpdateNeedsOverTime(float DeltaTime);

    /** Update the critical status of a specific need and trigger events */
    virtual void UpdateNeedCriticalStatus(EARPG_NeedType NeedType);

    /** Get the priority order for needs (higher values = higher priority) */
    virtual float GetNeedPriority(EARPG_NeedType NeedType) const;

    /** Called when a need value changes - override for custom behavior */
    virtual void OnNeedValueChanged(EARPG_NeedType NeedType, float OldValue, float NewValue);

    /** Called when a need becomes critical - override for custom behavior */
    virtual void OnNeedBecomeCritical(EARPG_NeedType NeedType);

    /** Called when a need is no longer critical - override for custom behavior */
    virtual void OnNeedNoLongerCritical(EARPG_NeedType NeedType);

    /** Get default decay rate for a need type */
    virtual float GetDefaultDecayRateForNeed(EARPG_NeedType NeedType) const;
    void InitializeNeedsFromDataTable();
    float GetDecayRateFromConfig(EARPG_NeedType NeedType, const FARPG_AINeedsConfigRow& ConfigRow) const;

    /** Get archetype-specific need configuration */
    virtual TArray<FARPG_CreatureNeedConfig> GetArchetypeNeedConfigs(EARPG_CreatureArchetype Archetype) const;

    // === Blueprint Events ===

    /** Blueprint event for need value changes */
    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void BP_OnNeedChanged(EARPG_NeedType NeedType, float OldValue, float NewValue);

    /** Blueprint event for critical need state */
    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void BP_OnNeedCritical(EARPG_NeedType NeedType);

    /** Blueprint event for need satisfaction */
    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void BP_OnNeedSatisfied(EARPG_NeedType NeedType);
};