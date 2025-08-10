// Source/RadiantRPG/Public/Characters/ARPG_BaseNPCCharacter.h

#pragma once

#include "CoreMinimal.h"
#include "Characters/BaseCharacter.h"
#include "GameplayTags.h"
#include "AI/Interfaces/IARPG_AIBrainInterface.h"
#include "Types/ARPG_AITypes.h"
#include "Types/ARPG_NPCTypes.h"
#include "ARPG_BaseNPCCharacter.generated.h"

class UARPG_AIBrainComponent;
class UARPG_AIMemoryComponent;
class UARPG_AIPerceptionComponent;
class UARPG_AINeedsComponent;
class UARPG_AIPersonalityComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNPCBehaviorChanged, AActor*, NPC, FGameplayTag, NewBehavior);

/**
 * Base NPC Character with modular AI system
 * Provides core AI components and interfaces for all NPCs
 */
UCLASS(BlueprintType, Blueprintable)
class RADIANTRPG_API AARPG_BaseNPCCharacter : public ABaseCharacter, public IARPG_AIBrainInterface
{
    GENERATED_BODY()

public:
    AARPG_BaseNPCCharacter();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void Tick(float DeltaTime) override;

    // === IARPG_AIBrainInterface Implementation ===
    
    UFUNCTION(BlueprintCallable, Category = "AI")
    virtual void InitializeBrain(const FARPG_AIBrainConfiguration& Config) override;

    UFUNCTION(BlueprintCallable, Category = "AI")
    virtual void ProcessStimulus(const FARPG_AIStimulus& Stimulus) override;

    UFUNCTION(BlueprintCallable, Category = "AI")
    virtual FARPG_AIIntent GenerateIntent() override;

    UFUNCTION(BlueprintCallable, Category = "AI")
    virtual void UpdateBrain(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "AI")
    virtual bool ShouldActivateCuriosity() const override;

    UFUNCTION(BlueprintCallable, Category = "AI")
    virtual FARPG_AIBrainState GetBrainState() const override;

    UFUNCTION(BlueprintCallable, Category = "AI")
    virtual void SetBrainEnabled(bool bEnabled) override;

    UFUNCTION(BlueprintCallable, Category = "AI")
    virtual float GetIntentConfidence() const override;

    UFUNCTION(BlueprintCallable, Category = "AI")
    virtual FARPG_AIIntent GetCurrentIntent() const ;

    UFUNCTION(BlueprintCallable, Category = "AI")
    virtual void ExecuteIntent(const FARPG_AIIntent& Intent) ;

    // === NPC Interface ===

    /** Initialize NPC with configuration data */
    UFUNCTION(BlueprintCallable, Category = "NPC")
    virtual void InitializeNPC(const FARPG_NPCConfiguration& Config);

    /** Get NPC type identifier */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "NPC")
    FGameplayTag GetNPCType() const { return NPCType; }

    /** Get NPC faction */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "NPC")
    FGameplayTag GetFaction() const { return Faction; }

    /** Set NPC faction */
    UFUNCTION(BlueprintCallable, Category = "NPC")
    void SetFaction(FGameplayTag NewFaction);

    /** Check if this NPC is hostile to target */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "NPC")
    bool IsHostileTo(AActor* Target) const;

    /** Check if this NPC is friendly to target */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "NPC")
    bool IsFriendlyTo(AActor* Target) const;

    /** Get current behavior state */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "NPC")
    FGameplayTag GetCurrentBehavior() const { return CurrentBehavior; }

    // === Component Access ===

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Components")
    UARPG_AIBrainComponent* GetBrainComponent() const { return BrainComponent; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Components")
    UARPG_AIMemoryComponent* GetMemoryComponent() const { return MemoryComponent; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Components")
    UARPG_AIPerceptionComponent* GetPerceptionComponent() const { return PerceptionComponent; }

    //UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Components")
    UARPG_AINeedsComponent* GetNeedsComponent() const { return NeedsComponent; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Components")
    UARPG_AIPersonalityComponent* GetPersonalityComponent() const { return PersonalityComponent; }

    // === Events ===

    /** Called when behavior changes */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnNPCBehaviorChanged OnBehaviorChanged;

protected:
    // === Core AI Components ===

    /** Central AI brain */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components")
    UARPG_AIBrainComponent* BrainComponent;

    /** Memory system */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components")
    UARPG_AIMemoryComponent* MemoryComponent;

    /** Perception system */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components")
    UARPG_AIPerceptionComponent* PerceptionComponent;

    /** Needs system */
    //UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components")
    UARPG_AINeedsComponent* NeedsComponent;

    /** Personality system */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components")
    UARPG_AIPersonalityComponent* PersonalityComponent;

    // === Configuration ===

    /** NPC type identifier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FGameplayTag NPCType;

    /** Faction this NPC belongs to */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FGameplayTag Faction;

    /** Default brain configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FARPG_AIBrainConfiguration DefaultBrainConfig;

    /** Default needs configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FARPG_NeedsConfiguration DefaultNeedsConfig;

    /** Default personality traits */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    TArray<FARPG_AIPersonalityTrait> DefaultPersonalityTraits;

    // === State ===

    /** Current behavior tag */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    FGameplayTag CurrentBehavior;

    /** Whether this NPC is currently active */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bIsActive = true;

    // === Blueprint Events ===

    /** Called when NPC behavior changes */
    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void BP_OnBehaviorChanged(FGameplayTag OldBehavior, FGameplayTag NewBehavior);

    /** Called when NPC is initialized */
    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void BP_OnNPCInitialized();

    /** Called when faction changes */
    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void BP_OnFactionChanged(FGameplayTag OldFaction, FGameplayTag NewFaction);

private:
    // === Internal Functions ===

    /** Initialize all AI components */
    void InitializeAIComponents();

    /** Setup component references */
    void SetupComponentReferences();

    /** Handle behavior change */
    UFUNCTION()
    void OnIntentChanged(const FARPG_AIIntent& NewIntent);

    /** Update current behavior based on intent */
    void UpdateBehaviorFromIntent(const FARPG_AIIntent& Intent);

    /** Validate faction relationships */
    bool IsValidFactionTarget(AActor* Target, bool bCheckHostile) const;
};