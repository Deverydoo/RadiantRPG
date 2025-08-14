// Public/AI/Core/ARPG_AIManager.h

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "AI/Interfaces/IARPG_AIManagerInterface.h"
#include "Types/ARPG_AITypes.h"
#include "Types/ARPG_AIEventTypes.h"
#include "Types/SystemTypes.h"
#include "ARPG_AIManager.generated.h"

class AARPG_BaseNPCCharacter;
class UARPG_AIBrainComponent;
class UARPG_AIEventManager;
class UWorldEventManager;
class URadiantGameManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNPCRegistered, AARPG_BaseNPCCharacter*, NPC);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNPCUnregistered, AARPG_BaseNPCCharacter*, NPC);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBrainRegistered, UARPG_AIBrainComponent*, Brain);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBrainUnregistered, UARPG_AIBrainComponent*, Brain);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAISystemStateChanged, bool, bEnabled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnZoneTransition, FGameplayTag, FromZone, FGameplayTag, ToZone);

/**
 * AI Manager Configuration
 */
USTRUCT(BlueprintType)
struct FARPG_AIManagerConfig
{
    GENERATED_BODY()

    /** Maximum number of active NPCs */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    int32 MaxActiveNPCs = 100;

    /** Global AI update rate (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float GlobalUpdateRate = 0.1f;

    /** Distance for LOD calculations */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float LODDistance = 5000.0f;

    /** Enable AI debug visualization */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bEnableDebugVisualization = false;

    /** Enable detailed AI logging */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bEnableDetailedLogging = false;

    /** Global curiosity multiplier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
    float GlobalCuriosityMultiplier = 1.0f;

    /** Default idle intent tag */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
    FGameplayTag DefaultIdleIntent;

    FARPG_AIManagerConfig()
    {
        DefaultIdleIntent = FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Idle"));
    }
};

/**
 * AI Manager - World Subsystem
 * Central manager for all AI entities and behaviors
 * Implements push-based event system for AI coordination
 */
UCLASS(BlueprintType, Blueprintable)
class RADIANTRPG_API UARPG_AIManager : public UWorldSubsystem, public IARPG_AIManagerInterface
{
    GENERATED_BODY()

public:
    UARPG_AIManager();

    // === UWorldSubsystem Interface ===
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    // === IARPG_AIManagerInterface Implementation ===
    
    // System lifecycle methods
    virtual void OnSystemInitialized() override;
    virtual void OnSystemShutdown() override;
    
    // AI Management methods
    virtual void RegisterAI(class AActor* AIActor) override;
    virtual void UnregisterAI(class AActor* AIActor) override;
    virtual void BroadcastGlobalEvent(const struct FARPG_AIEvent& Event) override;
    virtual int32 GetActiveAICount() const override;
    
    // NPC Management
    virtual void RegisterNPC(AARPG_BaseNPCCharacter* NPC) override;
    virtual void UnregisterNPC(AARPG_BaseNPCCharacter* NPC) override;
    virtual TArray<AARPG_BaseNPCCharacter*> GetAllNPCs() const override;
    virtual TArray<AARPG_BaseNPCCharacter*> GetNPCsInRadius(FVector Location, float Radius) const override;
    virtual TArray<AARPG_BaseNPCCharacter*> GetNPCsByFaction(FGameplayTag FactionTag) const override;
    virtual TArray<AARPG_BaseNPCCharacter*> GetNPCsByType(FGameplayTag NPCType) const override;
    
    // Brain Component Management
    virtual void RegisterBrainComponent(UARPG_AIBrainComponent* Brain) override;
    virtual void UnregisterBrainComponent(UARPG_AIBrainComponent* Brain) override;
    virtual TArray<UARPG_AIBrainComponent*> GetActiveBrains() const override;
    
    // AI System Control
    virtual void SetAISystemEnabled(bool bEnabled) override;
    virtual bool IsAISystemEnabled() const override;
    virtual void SetGlobalUpdateRate(float UpdateRate);
    virtual float GetGlobalUpdateRate() const ;
    virtual void SetGlobalAIUpdateRate(float UpdateRate) override;
    virtual float GetGlobalAIUpdateRate() const override;
    
    // AI Processing
    virtual void UpdateAIProcessing(float DeltaTime);
    virtual void ForceUpdateAllAI() ;
    
    // Curiosity System
    virtual void TriggerGlobalCuriosity(float Intensity = 1.0f) override;
    virtual void SetGlobalCuriosityMultiplier(float Multiplier) override;
    
    // Performance Management
    virtual float GetAISystemLoad() const override;
    virtual void SetMaxActiveNPCs(int32 MaxNPCs) override;
    virtual int32 GetMaxActiveNPCs() const override;
    
    // Event Broadcasting
    virtual void BroadcastAIEvent(const FARPG_AIEvent& Event) override;
    virtual void BroadcastStimulus(const struct FARPG_AIStimulus& Stimulus, FVector Location, float Radius) override;
    
    // Zone Management
    virtual void OnZoneTransition(FGameplayTag FromZone, FGameplayTag ToZone) override;
    virtual void UpdateNPCsForZone(FGameplayTag ZoneTag) override;
    
    // Debug & Visualization
    virtual void EnableDebugVisualization(bool bEnabled) ;
    virtual bool IsDebugVisualizationEnabled() const ;
    virtual void SetAIDebugEnabled(bool bEnabled) override;
    virtual bool IsAIDebugEnabled() const override;

    // === Configuration ===
    
    /** Get the current AI manager configuration */
    UFUNCTION(BlueprintCallable, Category = "AI Manager")
    const FARPG_AIManagerConfig& GetConfiguration() const { return Configuration; }

    /** Update AI manager configuration */
    UFUNCTION(BlueprintCallable, Category = "AI Manager")
    void UpdateConfiguration(const FARPG_AIManagerConfig& NewConfig);

    // === Events ===
    
    /** Called when an NPC is registered */
    UPROPERTY(BlueprintAssignable, Category = "AI Events")
    FOnNPCRegistered OnNPCRegistered;

    /** Called when an NPC is unregistered */
    UPROPERTY(BlueprintAssignable, Category = "AI Events")
    FOnNPCUnregistered OnNPCUnregistered;

    /** Called when a brain component is registered */
    UPROPERTY(BlueprintAssignable, Category = "AI Events")
    FOnBrainRegistered OnBrainRegistered;

    /** Called when a brain component is unregistered */
    UPROPERTY(BlueprintAssignable, Category = "AI Events")
    FOnBrainUnregistered OnBrainUnregistered;

    /** Called when AI system state changes */
    UPROPERTY(BlueprintAssignable, Category = "AI Events")
    FOnAISystemStateChanged OnAISystemStateChanged;

    /** Called when zone transition occurs */
    UPROPERTY(BlueprintAssignable, Category = "AI Events")
    FOnZoneTransition OnZoneTransitionDelegate;
    bool bAISystemEnabled;
    float GlobalUpdateRate;
    int MaxActiveNPCs;
    TArray<TObjectPtr<AActor>> RegisteredAIs;


    // === Public Utilities ===
    
    /** Get the AI event manager subsystem */
    UFUNCTION(BlueprintCallable, Category = "AI Manager")
    UARPG_AIEventManager* GetEventManager() const { return EventManager; }

    /** Get the world event manager */
    UFUNCTION(BlueprintCallable, Category = "AI Manager")
    UWorldEventManager* GetWorldEventManager() const { return WorldEventManager; }

    /** Get current AI system load (0-1) */
    UFUNCTION(BlueprintCallable, Category = "AI Manager")
    float GetCurrentAILoad() const { return CurrentAILoad; }

protected:
    // === Internal Methods ===
    
    /** Initialize subsystem references */
    void InitializeSubsystemReferences();

    /** Register with other game systems */
    void RegisterWithOtherSystems();

    /** Unregister from other game systems */
    void UnregisterFromOtherSystems();

    /** Start the periodic update timer */
    void StartUpdateTimer();
    void UpdateAISystems();
    void CleanupInvalidReferences();

    void DebugLogAIStats();
    /** Stop the periodic update timer */
    void StopUpdateTimer();

    /** Periodic update function */
    void PeriodicUpdate();

    /** Process AI updates based on LOD */
    void ProcessAIUpdates(float DeltaTime);

    /** Update AI load metrics */
    void UpdateAILoadMetrics();

    /** Clean up inactive AI entities */
    void CleanupInactiveAI();

    /** Debug visualization update */
    void UpdateDebugVisualization();

    /** Handle zone transition for NPCs */
    void HandleZoneTransition(AARPG_BaseNPCCharacter* NPC, FGameplayTag FromZone, FGameplayTag ToZone);

private:
    // === Configuration ===
    
    /** Current configuration */
    UPROPERTY()
    FARPG_AIManagerConfig Configuration;

    // === Registered Entities ===
    
    /** All registered NPCs */
    UPROPERTY()
    TArray<AARPG_BaseNPCCharacter*> RegisteredNPCs;

    /** All registered brain components */
    UPROPERTY()
    TArray<UARPG_AIBrainComponent*> RegisteredBrains;

    /** NPCs organized by faction */
    TMap<FGameplayTag, TArray<AARPG_BaseNPCCharacter*>> NPCsByFaction;

    /** NPCs organized by type */
    TMap<FGameplayTag, TArray<AARPG_BaseNPCCharacter*>> NPCsByType;

    /** Generic AI actors (for backwards compatibility) */
    UPROPERTY()
    TArray<AActor*> RegisteredAIActors;

    // === Subsystem References ===
    
    /** Reference to AI event manager */
    UPROPERTY()
    UARPG_AIEventManager* EventManager;

    /** Reference to world event manager */
    UPROPERTY()
    UWorldEventManager* WorldEventManager;

    /** Reference to game manager */
    UPROPERTY()
    URadiantGameManager* GameManager;

    // === State ===
    
    /** Is the AI system currently enabled */
    bool bIsAISystemEnabled;

    /** Has the system been initialized */
    bool bIsInitialized;

    /** Is debug visualization enabled */
    bool bDebugVisualizationEnabled;

    /** Current AI system load (0-1) */
    float CurrentAILoad;

    /** Last update timestamp */
    float LastUpdateTime;

    /** Update timer handle */
    FTimerHandle UpdateTimerHandle;

    // === Optimization ===
    
    /** NPCs that need immediate updates */
    TSet<AARPG_BaseNPCCharacter*> HighPriorityNPCs;

    /** NPCs in player vicinity */
    TSet<AARPG_BaseNPCCharacter*> NearbyNPCs;

    /** Cache for spatial queries */
    mutable TMap<FVector, TPair<float, TArray<AARPG_BaseNPCCharacter*>>> SpatialQueryCache;

    /** Time to clear spatial cache */
    float SpatialCacheClearTime;
};