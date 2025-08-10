// Source/RadiantRPG/Public/AI/Core/ARPG_AIEventManager.h

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTags.h"
#include "Types/ARPG_AITypes.h"
#include "Types/ARPG_AIEventTypes.h"
#include "Types/EventTypes.h"
#include "AI/Interfaces/IARPG_EventSubscriber.h"
#include "ARPG_AIEventManager.generated.h"

class UARPG_AIBrainComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIEventBroadcast, FGameplayTag, EventType, const struct FARPG_AIEvent&, EventData);

/**
 * AI Event Manager - World Subsystem
 * Handles AI-specific event broadcasting and subscription
 * Manages event propagation, filtering, and lifecycle
 */
UCLASS()
class RADIANTRPG_API UARPG_AIEventManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // === UWorldSubsystem Interface ===
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    // === Event Broadcasting ===

    /** Broadcast an AI event to all subscribers */
    UFUNCTION(BlueprintCallable, Category = "AI Events")
    void BroadcastAIEvent(const FARPG_AIEvent& Event);

    /** Create and broadcast a stimulus event */
    UFUNCTION(BlueprintCallable, Category = "AI Events")
    void BroadcastStimulusEvent(const FARPG_AIStimulus& Stimulus, bool bCreateMemory = true);

    /** Create and broadcast a world state change event */
    UFUNCTION(BlueprintCallable, Category = "AI Events")
    void BroadcastWorldStateEvent(FGameplayTag EventType, FVector Location, AActor* Instigator = nullptr, 
                                  float EventStrength = 0.5f, float EventRadius = 1000.0f, bool bGlobal = false);

    /** Broadcast faction-related event */
    UFUNCTION(BlueprintCallable, Category = "AI Events")
    void BroadcastFactionEvent(FGameplayTag EventType, FGameplayTag FactionTag, FVector Location, 
                               AActor* Instigator = nullptr, float EventStrength = 0.7f);

    /** Broadcast combat event */
    UFUNCTION(BlueprintCallable, Category = "AI Events")
    void BroadcastCombatEvent(AActor* Attacker, AActor* Target, FVector Location, float Damage, bool bKillingBlow = false);

    /** Broadcast death event */
    UFUNCTION(BlueprintCallable, Category = "AI Events")
    void BroadcastDeathEvent(AActor* DeadActor, AActor* Killer, FVector Location);

    /** Broadcast trade/economic event */
    UFUNCTION(BlueprintCallable, Category = "AI Events")
    void BroadcastTradeEvent(AActor* Buyer, AActor* Seller, FGameplayTag ItemTag, int32 Quantity, float Value);

    // === Event Subscription ===

    /** Register an AI brain component to receive events */
    UFUNCTION(BlueprintCallable, Category = "AI Events")
    void RegisterBrainComponent(UARPG_AIBrainComponent* BrainComponent);

    /** Unregister an AI brain component */
    UFUNCTION(BlueprintCallable, Category = "AI Events")
    void UnregisterBrainComponent(UARPG_AIBrainComponent* BrainComponent);

    /** Subscribe to specific event types */
    UFUNCTION(BlueprintCallable, Category = "AI Events")
    void SubscribeToEvents(UARPG_AIBrainComponent* BrainComponent, const TArray<FGameplayTag>& EventTypes);

    /** Unsubscribe from specific event types */
    UFUNCTION(BlueprintCallable, Category = "AI Events")
    void UnsubscribeFromEvents(UARPG_AIBrainComponent* BrainComponent, const TArray<FGameplayTag>& EventTypes);

    /** Subscribe using interface */
    UFUNCTION(BlueprintCallable, Category = "AI Events")
    void SubscribeEventListener(TScriptInterface<IARPG_EventSubscriber> Subscriber, FGameplayTag EventType);

    /** Unsubscribe using interface */
    UFUNCTION(BlueprintCallable, Category = "AI Events")
    void UnsubscribeEventListener(TScriptInterface<IARPG_EventSubscriber> Subscriber, FGameplayTag EventType);

    // === Event Queries ===

    /** Get recent events of a specific type within range */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Events")
    TArray<FARPG_AIEvent> GetRecentEvents(FGameplayTag EventType, float TimeWindow = 30.0f, 
                                          FVector SearchLocation = FVector::ZeroVector, float SearchRadius = -1.0f) const;

    /** Get all active events */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Events")
    TArray<FARPG_AIEvent> GetActiveEvents() const;

    /** Check if event type is currently active */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Events")
    bool IsEventTypeActive(FGameplayTag EventType) const;

    /** Get event count for debugging */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI Events")
    int32 GetActiveEventCount() const { return ActiveEvents.Num(); }

    // === Event Management ===

    /** Manually expire an event */
    UFUNCTION(BlueprintCallable, Category = "AI Events")
    void ExpireEvent(const FGuid& EventID);

    /** Clear all events (useful for level transitions) */
    UFUNCTION(BlueprintCallable, Category = "AI Events")
    void ClearAllEvents();

    /** Enable/disable event processing */
    UFUNCTION(BlueprintCallable, Category = "AI Events")
    void SetEventProcessingEnabled(bool bEnabled) { bEventProcessingEnabled = bEnabled; }

    // === Debug ===

    /** Enable debug logging for AI events */
    UFUNCTION(BlueprintCallable, Category = "Debug")
    void SetDebugLogging(bool bEnabled) { bDebugLogging = bEnabled; }

    /** Get debug info about subscribers */
    UFUNCTION(BlueprintCallable, Category = "Debug")
    FString GetSubscriberDebugInfo() const;

public:
    /** Main event broadcast delegate - all AI systems can bind to this */
    UPROPERTY(BlueprintAssignable, Category = "AI Events")
    FOnAIEventBroadcast OnAnyEvent;

protected:
    // === Configuration ===

    /** Maximum number of events to keep in history */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "10", ClampMax = "10000"))
    int32 MaxEventHistory = 1000;

    /** How long to keep events in history (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "10.0", ClampMax = "3600.0"))
    float EventHistoryDuration = 300.0f;

    /** How often to clean up expired events (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "1.0", ClampMax = "60.0"))
    float EventCleanupInterval = 10.0f;

    /** Whether event processing is enabled */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    bool bEventProcessingEnabled = true;

    /** Enable debug logging */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bDebugLogging = false;

private:
    // === Event Storage ===

    /** All active/recent events */
    TArray<FARPG_AIEvent> ActiveEvents;

    /** Events indexed by type for fast lookup */
    TMap<FGameplayTag, TArray<int32>> EventsByType;

    /** Next event ID to assign */
    int32 NextEventID = 1;

    // === Subscriber Management ===

    /** All registered brain components */
    TSet<TWeakObjectPtr<UARPG_AIBrainComponent>> RegisteredBrains;

    /** Event type subscriptions for brain components */
    TMap<TWeakObjectPtr<UARPG_AIBrainComponent>, TSet<FGameplayTag>> BrainSubscriptions;

    /** Interface-based subscribers */
    TMap<FGameplayTag, TArray<TScriptInterface<IARPG_EventSubscriber>>> InterfaceSubscribers;

    // === Timer Management ===

    /** Handle for cleanup timer */
    FTimerHandle CleanupTimerHandle;

    // === Internal Processing ===

    /** Cleanup expired events */
    void CleanupExpiredEvents();

    /** Process event for propagation */
    void ProcessEventForPropagation(const FARPG_AIEvent& Event);

    /** Filter events for specific brain component */
    bool ShouldBroadcastToBrain(const FARPG_AIEvent& Event, UARPG_AIBrainComponent* Brain) const;

    /** Calculate event relevance for distance-based filtering */
    float CalculateEventRelevance(const FARPG_AIEvent& Event, FVector ListenerLocation) const;

    /** Generate unique event ID */
    FGuid GenerateEventID() const;

    /** Add event to storage */
    void AddEventToStorage(const FARPG_AIEvent& Event);

    /** Remove event from storage */
    void RemoveEventFromStorage(int32 EventIndex);

    /** Validate event before broadcasting */
    bool ValidateEvent(const FARPG_AIEvent& Event) const;

    /** Convert stimulus to AI event */
    FARPG_AIEvent ConvertStimulusToEvent(const FARPG_AIStimulus& Stimulus, bool bCreateMemory) const;
};