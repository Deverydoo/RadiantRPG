// Public/AI/Interfaces/IARPG_AIManager.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "Types/ARPG_AITypes.h"
#include "Types/ARPG_AIEventTypes.h"
#include "IARPG_AIManagerInterface.generated.h"

class AARPG_BaseNPCCharacter;
class UARPG_AIBrainComponent;

UINTERFACE(MinimalAPI, Blueprintable)
class UARPG_AIManagerInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * Interface for AI Manager
 * Provides abstraction for AI system management
 */
class RADIANTRPG_API IARPG_AIManagerInterface
{
    GENERATED_BODY()

public:
    // === NPC Management ===
    // System lifecycle methods
    virtual void OnSystemInitialized() = 0;
    virtual void OnSystemShutdown() = 0;
    
    // Periodic update for AI processing
    virtual void PeriodicUpdate();

    // AI Management methods
    virtual void RegisterAI(class AActor* AIActor) = 0;
    virtual void UnregisterAI(class AActor* AIActor) = 0;
    
    // Event broadcasting
    virtual void BroadcastStimulus(const struct FARPG_AIStimulus& Stimulus, FVector Location, float Radius) = 0;
    virtual void BroadcastGlobalEvent(const struct FARPG_AIEvent& Event) = 0;
    
    virtual int32 GetActiveAICount() const = 0;
    
    /** Register an NPC with the AI manager */
    virtual void RegisterNPC(AARPG_BaseNPCCharacter* NPC) = 0;
    
    /** Unregister an NPC from the AI manager */
    virtual void UnregisterNPC(AARPG_BaseNPCCharacter* NPC) = 0;
    
    /** Get all registered NPCs */
    virtual TArray<AARPG_BaseNPCCharacter*> GetAllNPCs() const = 0;
    
    /** Get NPCs within a certain radius */
    virtual TArray<AARPG_BaseNPCCharacter*> GetNPCsInRadius(FVector Location, float Radius) const = 0;
    
    /** Get NPCs by faction */
    virtual TArray<AARPG_BaseNPCCharacter*> GetNPCsByFaction(FGameplayTag FactionTag) const = 0;
    
    /** Get NPCs by type */
    virtual TArray<AARPG_BaseNPCCharacter*> GetNPCsByType(FGameplayTag NPCType) const = 0;
    
    // === Brain Component Management ===
    
    /** Register a brain component */
    virtual void RegisterBrainComponent(UARPG_AIBrainComponent* Brain) = 0;
    
    /** Unregister a brain component */
    virtual void UnregisterBrainComponent(UARPG_AIBrainComponent* Brain) = 0;
    
    /** Get all active brain components */
    virtual TArray<UARPG_AIBrainComponent*> GetActiveBrains() const = 0;
    
    // === AI System Control ===
    
    /** Enable or disable the entire AI system */
    virtual void SetAISystemEnabled(bool bEnabled) = 0;
    
    /** Check if AI system is enabled */
    virtual bool IsAISystemEnabled() const = 0;
    
    /** Set global AI update rate */
    virtual void SetGlobalAIUpdateRate(float UpdateRate) = 0;
    
    /** Get current global AI update rate */
    virtual float GetGlobalAIUpdateRate() const = 0;
    
    // === Curiosity System ===
    
    /** Trigger global curiosity event */
    virtual void TriggerGlobalCuriosity(float Intensity = 1.0f) = 0;
    
    /** Set global curiosity multiplier */
    virtual void SetGlobalCuriosityMultiplier(float Multiplier) = 0;
    
    // === Performance Management ===
    
    /** Get current AI load */
    virtual float GetAISystemLoad() const = 0;
    
    /** Set max active NPCs */
    virtual void SetMaxActiveNPCs(int32 MaxNPCs) = 0;
    
    /** Get max active NPCs */
    virtual int32 GetMaxActiveNPCs() const = 0;
    
    // === Event Broadcasting ===
    
    /** Broadcast AI event to all relevant NPCs */
    virtual void BroadcastAIEvent(const FARPG_AIEvent& Event) = 0;
    
    // === Zone Management ===
    
    /** Notify AI manager of zone change */
    virtual void OnZoneTransition(FGameplayTag FromZone, FGameplayTag ToZone) = 0;
    
    /** Update NPCs for zone */
    virtual void UpdateNPCsForZone(FGameplayTag ZoneTag) = 0;
    
    // === Debug ===
    
    /** Toggle AI debug display */
    virtual void SetAIDebugEnabled(bool bEnabled) = 0;
    
    /** Check if AI debug is enabled */
    virtual bool IsAIDebugEnabled() const = 0;
};