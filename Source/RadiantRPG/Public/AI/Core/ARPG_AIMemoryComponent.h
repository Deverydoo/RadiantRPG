// Public/AI/Core/ARPG_AIMemoryComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTags.h"
#include "Engine/DataTable.h"
#include "Types/ARPG_AIEventTypes.h"
#include "Types/ARPG_AIDataTableTypes.h"
#include "AI/Interfaces/IARPG_EventSubscriber.h"
#include "World/RadiantZoneManager.h"
#include "ARPG_AIMemoryComponent.generated.h"

class UARPG_AIBrainComponent;
class UARPG_AIEventManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMemoryFormed, const FARPG_MemoryEntry&, Memory, EARPG_MemoryType, MemoryType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMemoryForgotten, const FARPG_MemoryEntry&, Memory, EARPG_MemoryType, MemoryType);

/**
 * Memory configuration settings
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_MemoryConfiguration
{
    GENERATED_BODY()

    /** Maximum number of short-term memories per type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capacity", meta = (ClampMin = "1", ClampMax = "1000"))
    int32 MaxShortTermMemories = 50;

    /** Maximum number of long-term memories per type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capacity", meta = (ClampMin = "1", ClampMax = "10000"))
    int32 MaxLongTermMemories = 200;

    /** How long memories stay in short-term storage (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transfer", meta = (ClampMin = "30.0", ClampMax = "3600.0"))
    float ShortTermDuration = 300.0f;

    /** Minimum strength required for long-term transfer */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transfer", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float LongTermThreshold = 0.6f;

    /** Strength threshold below which memories are forgotten */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forgetting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ForgetThreshold = 0.1f;

    /** How often to update memory decay (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance", meta = (ClampMin = "0.1", ClampMax = "60.0"))
    float DecayUpdateFrequency = 5.0f;

    /** Whether to enable debug logging */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bEnableDebugLogging = false;

    FARPG_MemoryConfiguration()
    {
        MaxShortTermMemories = 50;
        MaxLongTermMemories = 200;
        ShortTermDuration = 300.0f;
        LongTermThreshold = 0.6f;
        ForgetThreshold = 0.1f;
        DecayUpdateFrequency = 5.0f;
        bEnableDebugLogging = false;
    }
};

/**
 * AI Memory Component
 * Manages short-term and long-term memory storage for AI systems
 * Supports species-specific configuration through data tables
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class RADIANTRPG_API UARPG_AIMemoryComponent : public UActorComponent, public IARPG_EventSubscriber
{
    GENERATED_BODY()

public:
    UARPG_AIMemoryComponent();

    // === CONFIGURATION METHODS ===
    virtual UObject* GetSubscriberObject() override { return this; }
    
    /** Load configuration from data table or use blueprint defaults */
    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void LoadMemoryConfiguration();
    
    /** Set data table config (call this before BeginPlay) */
    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void SetDataTableConfig(UDataTable* DataTable, FName RowName);
    
    /** Get current effective configuration */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Configuration")
    FARPG_MemoryConfiguration GetEffectiveConfiguration() const;

    // === UActorComponent Interface ===
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // === IARPG_EventSubscriber Interface ===
    virtual void OnAIEventReceived(FGameplayTag EventType, const FARPG_AIEvent& EventData);

    // === Memory Formation ===

    /** Form a new memory from provided data */
    UFUNCTION(BlueprintCallable, Category = "Memory Formation")
    void FormMemory(const FARPG_MemoryEntry& MemoryEntry);

    /** Form memory from an AI event */
    UFUNCTION(BlueprintCallable, Category = "Memory Formation")
    void FormEventMemory(const FARPG_AIEvent& Event, EARPG_MemoryRelevance Relevance = EARPG_MemoryRelevance::Medium, float EmotionalWeight = 0.0f);
    float GetRelevanceStrength(EARPG_MemoryRelevance Relevance) const;

    /** Form location-based memory */
    UFUNCTION(BlueprintCallable, Category = "Memory Formation")
    void FormLocationMemory(FVector Location, FGameplayTag LocationTag, const FString& Description, EARPG_MemoryRelevance Relevance = EARPG_MemoryRelevance::Medium);
    ARadiantZoneManager* FindZoneAtLocation(FVector Location) const;
    float CalculateLocationDecayRate(FGameplayTag LocationTag, EARPG_MemoryRelevance Relevance) const;

    void ReinforceLocationMemory(const FARPG_MemoryEntry& ExistingMemory, float StrengthBoost);
    /** Form entity memory (about NPCs, creatures, etc.) */
    UFUNCTION(BlueprintCallable, Category = "Memory Formation")
    void FormEntityMemory(AActor* Entity, FGameplayTag EntityTag, const FString& Description, float EmotionalWeight = 0.0f);
    EARPG_MemoryRelevance DetermineEntityRelevance(AActor* Entity, float EmotionalWeight) const;
    float CalculateEntityMemoryStrength(AActor* Entity, float EmotionalWeight) const;
    float CalculateEntityDecayRate(AActor* Entity, float EmotionalWeight) const;
    void ReinforceEntityMemory(AActor* Entity, float StrengthBoost, float NewEmotionalWeight);

    // === Memory Querying ===

    /** Query memories with specific criteria */
    UFUNCTION(BlueprintCallable, Category = "Memory Queries")
    TArray<FARPG_MemoryEntry> QueryMemories(const FARPG_MemoryQuery& Query) const;

    /** Get recent memories of a specific type */
    UFUNCTION(BlueprintCallable, Category = "Memory Queries")
    TArray<FARPG_MemoryEntry> GetRecentMemories(EARPG_MemoryType MemoryType = EARPG_MemoryType::Any, float TimeWindow = 300.0f, int32 MaxResults = 10) const;

    /** Get all memories about a specific actor */
    UFUNCTION(BlueprintCallable, Category = "Memory Queries")
    TArray<FARPG_MemoryEntry> GetMemoriesAboutActor(AActor* Actor, int32 MaxResults = 10) const;

    /** Get memories near a location */
    UFUNCTION(BlueprintCallable, Category = "Memory Queries")
    TArray<FARPG_MemoryEntry> GetMemoriesNearLocation(FVector Location, float Radius = 1000.0f, int32 MaxResults = 10) const;

    /** Get emotionally charged memories */
    UFUNCTION(BlueprintCallable, Category = "Memory Queries")
    TArray<FARPG_MemoryEntry> GetEmotionalMemories(float MinEmotionalWeight = 0.5f, int32 MaxResults = 10) const;

    /** Get strongest memories of a type */
    UFUNCTION(BlueprintCallable, Category = "Memory Queries")
    TArray<FARPG_MemoryEntry> GetStrongestMemories(EARPG_MemoryType MemoryType = EARPG_MemoryType::Any, int32 MaxResults = 10) const;

    /** Get average memory strength for a tag or category */
    UFUNCTION(BlueprintCallable, Category = "Memory Analysis")
    float GetMemoryStrengthFor(FGameplayTag MemoryTag) const;

    // === Memory Management ===

    /** Forget a specific memory by index */
    UFUNCTION(BlueprintCallable, Category = "Memory Management")
    void ForgetMemory(int32 MemoryIndex);

    /** Forget all memories about a specific actor */
    UFUNCTION(BlueprintCallable, Category = "Memory Management")
    void ForgetMemoriesAboutActor(AActor* Actor);

    /** Forget all memories of a specific type */
    UFUNCTION(BlueprintCallable, Category = "Memory Management")
    void ForgetMemoriesOfType(EARPG_MemoryType MemoryType);

    /** Clear all memories */
    UFUNCTION(BlueprintCallable, Category = "Memory Management")
    void ClearAllMemories();

    // === Memory Statistics ===

    /** Get total number of memories */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Memory Statistics")
    int32 GetMemoryCount(EARPG_MemoryType MemoryType = EARPG_MemoryType::Any) const;

    /** Get number of short-term memories */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Memory Statistics")
    int32 GetShortTermMemoryCount() const;

    /** Get number of long-term memories */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Memory Statistics")
    int32 GetLongTermMemoryCount() const;

    /** Check if we have any memory about an actor */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Memory Statistics")
    bool HasMemoryAboutActor(AActor* Actor) const;

    /** Check if we have memories of a specific type/tag */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Memory Statistics")
    bool HasMemoryOfType(EARPG_MemoryType MemoryType, FGameplayTag SpecificTag = FGameplayTag()) const;

    // === Advanced Memory Operations ===

    /** Reinforce a memory (increase its strength) */
    UFUNCTION(BlueprintCallable, Category = "Memory Operations")
    void ReinforceMemory(int32 MemoryIndex, float StrengthBoost = 0.2f);

    /** Make a memory more vivid/detailed */
    UFUNCTION(BlueprintCallable, Category = "Memory Operations")
    void MakeMemoryVivid(int32 MemoryIndex);

    /** Make a memory permanent (won't be forgotten) */
    UFUNCTION(BlueprintCallable, Category = "Memory Operations")
    void MakeMemoryPermanent(int32 MemoryIndex);

    /** Update memory configuration at runtime */
    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void SetMemoryConfiguration(const FARPG_MemoryConfiguration& NewConfig);

    /** Get effective memory config (from data table if available, otherwise blueprint defaults) */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Configuration")
    FARPG_MemoryConfiguration GetMemoryConfiguration() const { return EffectiveConfig; }

    // === AI Brain Integration ===

    /** Contribute memory-based inputs to AI brain processing */
    UFUNCTION(BlueprintCallable, Category = "AI Integration")
    void ContributeToInputs(struct FARPG_AIInputVector& Inputs) const;

    // === Blueprint Events ===

    /** Called when a new memory is formed */
    UFUNCTION(BlueprintImplementableEvent, Category = "Memory Events")
    void BP_OnMemoryFormed(const FARPG_MemoryEntry& Memory);

    /** Called when a memory is forgotten */
    UFUNCTION(BlueprintImplementableEvent, Category = "Memory Events")
    void BP_OnMemoryForgotten(const FARPG_MemoryEntry& Memory);

    /** Determine event relevance for memory formation (Blueprint override) */
    UFUNCTION(BlueprintImplementableEvent, Category = "Memory Formation")
    EARPG_MemoryRelevance BP_DetermineEventRelevance(const FARPG_AIEvent& Event) const;

    /** Calculate emotional weight for events (Blueprint override) */
    UFUNCTION(BlueprintImplementableEvent, Category = "Memory Formation")
    float BP_CalculateEmotionalWeight(const FARPG_AIEvent& Event) const;

    // === Delegates ===

    /** Broadcast when a memory is formed */
    UPROPERTY(BlueprintAssignable, Category = "Memory Events")
    FOnMemoryFormed OnMemoryFormed;

    /** Broadcast when a memory is forgotten */
    UPROPERTY(BlueprintAssignable, Category = "Memory Events")
    FOnMemoryForgotten OnMemoryForgotten;

protected:
    // === Configuration ===

    /** Memory system configuration (Blueprint defaults) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FARPG_MemoryConfiguration MemoryConfig;

    /** Data table containing species-specific memory configurations */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Table Configuration")
    UDataTable* MemoryDataTable;

    /** Row name to use from the data table (usually species name) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Table Configuration")
    FName DataTableRowName;

    /** Whether to use data table config over blueprint config */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Configuration")
    bool bUseDataTableConfig;

    /** Effective configuration (resolved from data table or blueprint) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Configuration")
    FARPG_MemoryConfiguration EffectiveConfig;

    // === Component References ===

    /** Reference to the AI brain component */
    UPROPERTY(BlueprintReadOnly, Category = "Components")
    TWeakObjectPtr<UARPG_AIBrainComponent> BrainComponent;

private:
    // === Memory Storage ===

    /** Short-term memories organized by type */
    TMap<EARPG_MemoryType, TArray<FARPG_MemoryEntry>> ShortTermMemories;

    /** Long-term memories organized by type */
    TMap<EARPG_MemoryType, TArray<FARPG_MemoryEntry>> LongTermMemories;

    /** Last time memory decay was updated */
    float LastDecayUpdate;

    // === Initialization ===

    void InitializeMemoryStorage();
    void InitializeComponentReferences();
    void RegisterWithEventManager();
    void UnregisterFromEventManager();

    // === Configuration Loading ===

    bool LoadConfigurationFromDataTable(FARPG_MemoryConfiguration& OutConfig) const;

    // === Memory Management ===

    void UpdateMemoryDecay();
    void ProcessMemoryTransfer();
    bool IsSystemUnderMemoryPressure() const;
    void PerformEmergencyMemoryCleanup();
    void CleanupForgottenMemories();
    void AddMemoryToStorage(const FARPG_MemoryEntry& Memory, bool bIsLongTerm);

    // === Event Processing ===

    bool ShouldFormMemoryFromEvent(const FARPG_AIEvent& Event) const;
    EARPG_MemoryRelevance DetermineEventRelevance(const FARPG_AIEvent& Event) const;
    float CalculateEmotionalWeight(const FARPG_AIEvent& Event) const;

    // === Query Helpers ===

    bool DoesMemoryMatchQuery(const FARPG_MemoryEntry& Memory, const FARPG_MemoryQuery& Query) const;
    void SortMemoriesByRelevance(TArray<FARPG_MemoryEntry>& Memories) const;
    void SortMemoriesByTime(TArray<FARPG_MemoryEntry>& Memories, bool bMostRecentFirst = true) const;
};