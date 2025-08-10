// Source/RadiantRPG/Public/AI/Core/ARPG_AIMemoryComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTags.h"
#include "Types/ARPG_AIEventTypes.h"
#include "AI/Interfaces/IARPG_EventSubscriber.h"
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
 * Manages short-term and long-term memory for AI entities
 * Automatically forms memories from AI events and provides memory querying
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class RADIANTRPG_API UARPG_AIMemoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UARPG_AIMemoryComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // === Memory Formation ===

    /** Form a new memory from provided entry */
    UFUNCTION(BlueprintCallable, Category = "Memory Formation")
    void FormMemory(const FARPG_MemoryEntry& MemoryEntry);

    /** Form a memory about an event */
    UFUNCTION(BlueprintCallable, Category = "Memory Formation")
    void FormEventMemory(const FARPG_AIEvent& Event, EARPG_MemoryRelevance Relevance = EARPG_MemoryRelevance::Medium, float EmotionalWeight = 0.0f);

    /** Form a memory about a location */
    UFUNCTION(BlueprintCallable, Category = "Memory Formation")
    void FormLocationMemory(FVector Location, FGameplayTag LocationTag, const FString& Description = "", EARPG_MemoryRelevance Relevance = EARPG_MemoryRelevance::Medium);

    /** Form a memory about an entity/actor */
    UFUNCTION(BlueprintCallable, Category = "Memory Formation")
    void FormEntityMemory(AActor* Entity, FGameplayTag EntityTag, const FString& Description = "", EARPG_MemoryRelevance Relevance = EARPG_MemoryRelevance::Medium, float EmotionalWeight = 0.0f);

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

    /** Reinforce a memory (increase its strength) */
    UFUNCTION(BlueprintCallable, Category = "Memory Management")
    void ReinforceMemory(int32 MemoryIndex, float StrengthBoost = 0.1f);

    /** Make a memory vivid (slower decay) */
    UFUNCTION(BlueprintCallable, Category = "Memory Management")
    void MakeMemoryVivid(int32 MemoryIndex);

    /** Make a memory permanent (no decay) */
    UFUNCTION(BlueprintCallable, Category = "Memory Management")
    void MakeMemoryPermanent(int32 MemoryIndex);

    // === Memory Statistics ===

    /** Get total count of memories */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Memory Statistics")
    int32 GetMemoryCount(EARPG_MemoryType MemoryType = EARPG_MemoryType::Any) const;

    /** Get count of short-term memories */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Memory Statistics")
    int32 GetShortTermMemoryCount() const;

    /** Get count of long-term memories */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Memory Statistics")
    int32 GetLongTermMemoryCount() const;

    /** Check if has any memories about an actor */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Memory Statistics")
    bool HasMemoryAboutActor(AActor* Actor) const;

    /** Check if has memories of a specific type */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Memory Statistics")
    bool HasMemoryOfType(EARPG_MemoryType MemoryType, FGameplayTag SpecificTag = FGameplayTag()) const;

    // === Configuration ===

    /** Update memory configuration */
    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void SetMemoryConfiguration(const FARPG_MemoryConfiguration& NewConfig);

    /** Get current memory configuration */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Configuration")
    FARPG_MemoryConfiguration GetMemoryConfiguration() const { return MemoryConfig; }

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

    /** Memory system configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FARPG_MemoryConfiguration MemoryConfig;

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

    /** Last time decay was updated */
    float LastDecayUpdate = 0.0f;

    // === Internal Processing ===

    /** Initialize memory storage maps */
    void InitializeMemoryStorage();

    /** Update memory decay for all memories */
    void UpdateMemoryDecay();

    /** Transfer qualifying short-term memories to long-term */
    void ProcessMemoryTransfer();

    /** Clean up memories that have decayed below threshold */
    void CleanupForgottenMemories();

    /** Add memory to appropriate storage */
    void AddMemoryToStorage(const FARPG_MemoryEntry& Memory, bool bIsLongTerm);

    // === Event Processing ===

    /** Register with AI Event Manager for automatic memory formation */
    void RegisterWithEventManager();

    /** Unregister from AI Event Manager */
    void UnregisterFromEventManager();

    /** Handle AI events for automatic memory formation */
    UFUNCTION()
    void OnAIEventReceived(FGameplayTag EventType, const FARPG_AIEvent& EventData);

    /** Check if we should form a memory from an event */
    bool ShouldFormMemoryFromEvent(const FARPG_AIEvent& Event) const;

    /** Determine how relevant an event is for memory formation */
    EARPG_MemoryRelevance DetermineEventRelevance(const FARPG_AIEvent& Event) const;

    /** Calculate emotional weight for an event */
    float CalculateEmotionalWeight(const FARPG_AIEvent& Event) const;

    // === Memory Query Helpers ===

    /** Check if a memory matches query criteria */
    bool DoesMemoryMatchQuery(const FARPG_MemoryEntry& Memory, const FARPG_MemoryQuery& Query) const;

    /** Sort memories by relevance (highest first) */
    void SortMemoriesByRelevance(TArray<FARPG_MemoryEntry>& Memories) const;

    /** Sort memories by time (most recent first) */
    void SortMemoriesByTime(TArray<FARPG_MemoryEntry>& Memories, bool bMostRecentFirst = true) const;

    /** Sort memories by strength (strongest first) */
    void SortMemoriesByStrength(TArray<FARPG_MemoryEntry>& Memories, float CurrentTime) const;
};