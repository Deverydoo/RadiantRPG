// Source/RadiantRPG/Private/AI/Core/ARPG_AIEventManager.cpp

#include "AI/Core/ARPG_AIEventManager.h"
#include "AI/Core/ARPG_AIBrainComponent.h"
#include "AI/Interfaces/IARPG_EventSubscriber.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "RadiantRPG.h"

void UARPG_AIEventManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    // Start cleanup timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(CleanupTimerHandle, this, 
            &UARPG_AIEventManager::CleanupExpiredEvents, EventCleanupInterval, true);
    }
    
    if (bDebugLogging)
    {
        UE_LOG(LogARPG, Log, TEXT("AI Event Manager initialized"));
    }
}

void UARPG_AIEventManager::Deinitialize()
{
    // Clear timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(CleanupTimerHandle);
    }
    
    // Clear all data
    ActiveEvents.Empty();
    EventsByType.Empty();
    RegisteredBrains.Empty();
    BrainSubscriptions.Empty();
    InterfaceSubscribers.Empty();
    
    if (bDebugLogging)
    {
        UE_LOG(LogARPG, Log, TEXT("AI Event Manager deinitialized"));
    }
    
    Super::Deinitialize();
}

bool UARPG_AIEventManager::ShouldCreateSubsystem(UObject* Outer) const
{
    // Only create in game worlds, not editor
    if (UWorld* World = Cast<UWorld>(Outer))
    {
        return World->IsGameWorld();
    }
    return false;
}

void UARPG_AIEventManager::BroadcastAIEvent(const FARPG_AIEvent& Event)
{
    if (!bEventProcessingEnabled || !ValidateEvent(Event))
    {
        return;
    }
    
    // Add to storage
    AddEventToStorage(Event);
    
    // Process for propagation
    ProcessEventForPropagation(Event);
    
    if (bDebugLogging)
    {
        UE_LOG(LogARPG, VeryVerbose, TEXT("Broadcasted AI Event: %s (Strength: %.2f, Global: %s)"), 
               *Event.EventType.ToString(), Event.EventStrength, Event.bGlobal ? TEXT("Yes") : TEXT("No"));
    }
}

void UARPG_AIEventManager::BroadcastStimulusEvent(const FARPG_AIStimulus& Stimulus, bool bCreateMemory)
{
    FARPG_AIEvent Event = ConvertStimulusToEvent(Stimulus, bCreateMemory);
    BroadcastAIEvent(Event);
}

void UARPG_AIEventManager::BroadcastWorldStateEvent(FGameplayTag EventType, FVector Location, AActor* Instigator, 
                                                    float EventStrength, float EventRadius, bool bGlobal)
{
    FARPG_AIEvent Event;
    Event.EventID = GenerateEventID();
    Event.EventType = EventType;
    Event.EventLocation = Location;
    Event.EventInstigator = Instigator;
    Event.EventStrength = FMath::Clamp(EventStrength, 0.0f, 1.0f);
    Event.EventRadius = EventRadius;
    Event.bGlobal = bGlobal;
    Event.Timestamp = GetWorld()->GetTimeSeconds();
    
    BroadcastAIEvent(Event);
}

void UARPG_AIEventManager::BroadcastFactionEvent(FGameplayTag EventType, FGameplayTag FactionTag, FVector Location, 
                                                 AActor* Instigator, float EventStrength)
{
    FARPG_AIEvent Event;
    Event.EventID = GenerateEventID();
    Event.EventType = EventType;
    Event.EventLocation = Location;
    Event.EventInstigator = Instigator;
    Event.EventStrength = FMath::Clamp(EventStrength, 0.0f, 1.0f);
    Event.EventRadius = 2000.0f; // Faction events have larger radius
    Event.bGlobal = false;
    Event.Timestamp = GetWorld()->GetTimeSeconds();
    Event.EventData.Add(TEXT("FactionTag"), FactionTag.ToString());
    
    BroadcastAIEvent(Event);
}

void UARPG_AIEventManager::BroadcastCombatEvent(AActor* Attacker, AActor* Target, FVector Location, float Damage, bool bKillingBlow)
{
    FGameplayTag EventType = bKillingBlow ? 
        FGameplayTag::RequestGameplayTag(TEXT("AI.Event.Combat.Death")) : 
        FGameplayTag::RequestGameplayTag(TEXT("AI.Event.Combat.Damage"));
    
    FARPG_AIEvent Event;
    Event.EventID = GenerateEventID();
    Event.EventType = EventType;
    Event.EventLocation = Location;
    Event.EventInstigator = Attacker;
    Event.EventTarget = Target;
    Event.EventStrength = bKillingBlow ? 1.0f : FMath::Clamp(Damage / 100.0f, 0.1f, 0.8f);
    Event.EventRadius = bKillingBlow ? 1500.0f : 800.0f;
    Event.bGlobal = false;
    Event.Timestamp = GetWorld()->GetTimeSeconds();
    Event.EventData.Add(TEXT("Damage"), FString::SanitizeFloat(Damage));
    Event.EventData.Add(TEXT("KillingBlow"), bKillingBlow ? TEXT("true") : TEXT("false"));
    
    BroadcastAIEvent(Event);
}

void UARPG_AIEventManager::BroadcastDeathEvent(AActor* DeadActor, AActor* Killer, FVector Location)
{
    FARPG_AIEvent Event;
    Event.EventID = GenerateEventID();
    Event.EventType = FGameplayTag::RequestGameplayTag(TEXT("AI.Event.Death"));
    Event.EventLocation = Location;
    Event.EventInstigator = Killer;
    Event.EventTarget = DeadActor;
    Event.EventStrength = 1.0f; // Death is always maximum strength
    Event.EventRadius = 2000.0f; // Death events have large impact radius
    Event.bGlobal = false;
    Event.Timestamp = GetWorld()->GetTimeSeconds();
    
    if (IsValid(DeadActor))
    {
        Event.EventData.Add(TEXT("DeadActorClass"), DeadActor->GetClass()->GetName());
    }
    if (IsValid(Killer))
    {
        Event.EventData.Add(TEXT("KillerClass"), Killer->GetClass()->GetName());
    }
    
    BroadcastAIEvent(Event);
}

void UARPG_AIEventManager::BroadcastTradeEvent(AActor* Buyer, AActor* Seller, FGameplayTag ItemTag, int32 Quantity, float Value)
{
    FARPG_AIEvent Event;
    Event.EventID = GenerateEventID();
    Event.EventType = FGameplayTag::RequestGameplayTag(TEXT("AI.Event.Trade"));
    Event.EventLocation = IsValid(Seller) ? Seller->GetActorLocation() : FVector::ZeroVector;
    Event.EventInstigator = Buyer;
    Event.EventTarget = Seller;
    Event.EventStrength = FMath::Clamp(Value / 1000.0f, 0.1f, 0.5f); // Trade events are moderate strength
    Event.EventRadius = 500.0f; // Local trade event
    Event.bGlobal = false;
    Event.Timestamp = GetWorld()->GetTimeSeconds();
    Event.EventData.Add(TEXT("ItemTag"), ItemTag.ToString());
    Event.EventData.Add(TEXT("Quantity"), FString::FromInt(Quantity));
    Event.EventData.Add(TEXT("Value"), FString::SanitizeFloat(Value));
    
    BroadcastAIEvent(Event);
}

void UARPG_AIEventManager::RegisterBrainComponent(UARPG_AIBrainComponent* BrainComponent)
{
    if (!IsValid(BrainComponent))
    {
        return;
    }
    
    RegisteredBrains.Add(BrainComponent);
    
    // Initialize empty subscription set
    if (!BrainSubscriptions.Contains(BrainComponent))
    {
        BrainSubscriptions.Add(BrainComponent, TSet<FGameplayTag>());
    }
    
    if (bDebugLogging)
    {
        UE_LOG(LogARPG, Log, TEXT("Registered brain component: %s"), *BrainComponent->GetOwner()->GetName());
    }
}

void UARPG_AIEventManager::UnregisterBrainComponent(UARPG_AIBrainComponent* BrainComponent)
{
    if (!IsValid(BrainComponent))
    {
        return;
    }
    
    RegisteredBrains.Remove(BrainComponent);
    BrainSubscriptions.Remove(BrainComponent);
    
    if (bDebugLogging)
    {
        UE_LOG(LogARPG, Log, TEXT("Unregistered brain component: %s"), *BrainComponent->GetOwner()->GetName());
    }
}

void UARPG_AIEventManager::SubscribeToEvents(UARPG_AIBrainComponent* BrainComponent, const TArray<FGameplayTag>& EventTypes)
{
    if (!IsValid(BrainComponent))
    {
        return;
    }
    
    if (!BrainSubscriptions.Contains(BrainComponent))
    {
        RegisterBrainComponent(BrainComponent);
    }
    
    TSet<FGameplayTag>& Subscriptions = BrainSubscriptions[BrainComponent];
    for (const FGameplayTag& EventType : EventTypes)
    {
        Subscriptions.Add(EventType);
    }
    
    if (bDebugLogging)
    {
        UE_LOG(LogARPG, Log, TEXT("Brain %s subscribed to %d event types"), 
               *BrainComponent->GetOwner()->GetName(), EventTypes.Num());
    }
}

void UARPG_AIEventManager::UnsubscribeFromEvents(UARPG_AIBrainComponent* BrainComponent, const TArray<FGameplayTag>& EventTypes)
{
    if (!IsValid(BrainComponent) || !BrainSubscriptions.Contains(BrainComponent))
    {
        return;
    }
    
    TSet<FGameplayTag>& Subscriptions = BrainSubscriptions[BrainComponent];
    for (const FGameplayTag& EventType : EventTypes)
    {
        Subscriptions.Remove(EventType);
    }
}

void UARPG_AIEventManager::SubscribeEventListener(TScriptInterface<IARPG_EventSubscriber> Subscriber, FGameplayTag EventType)
{
    if (!Subscriber.GetInterface())
    {
        return;
    }
    
    InterfaceSubscribers.FindOrAdd(EventType).AddUnique(Subscriber);
}

void UARPG_AIEventManager::UnsubscribeEventListener(TScriptInterface<IARPG_EventSubscriber> Subscriber, FGameplayTag EventType)
{
    if (InterfaceSubscribers.Contains(EventType))
    {
        InterfaceSubscribers[EventType].Remove(Subscriber);
        if (InterfaceSubscribers[EventType].Num() == 0)
        {
            InterfaceSubscribers.Remove(EventType);
        }
    }
}

TArray<FARPG_AIEvent> UARPG_AIEventManager::GetRecentEvents(FGameplayTag EventType, float TimeWindow, FVector SearchLocation, float SearchRadius) const
{
    TArray<FARPG_AIEvent> Results;
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    // Get events of specific type
    const TArray<int32>* EventIndices = EventsByType.Find(EventType);
    if (!EventIndices)
    {
        return Results;
    }
    
    for (int32 Index : *EventIndices)
    {
        if (ActiveEvents.IsValidIndex(Index))
        {
            const FARPG_AIEvent& Event = ActiveEvents[Index];
            
            // Check time window
            if ((CurrentTime - Event.Timestamp) > TimeWindow)
            {
                continue;
            }
            
            // Check location if specified
            if (SearchRadius > 0.0f)
            {
                float Distance = FVector::Dist(Event.EventLocation, SearchLocation);
                if (Distance > SearchRadius)
                {
                    continue;
                }
            }
            
            Results.Add(Event);
        }
    }
    
    // Sort by most recent first
    Results.Sort([](const FARPG_AIEvent& A, const FARPG_AIEvent& B) {
        return A.Timestamp > B.Timestamp;
    });
    
    return Results;
}

TArray<FARPG_AIEvent> UARPG_AIEventManager::GetActiveEvents() const
{
    return ActiveEvents;
}

bool UARPG_AIEventManager::IsEventTypeActive(FGameplayTag EventType) const
{
    const TArray<int32>* EventIndices = EventsByType.Find(EventType);
    return EventIndices && EventIndices->Num() > 0;
}

void UARPG_AIEventManager::ExpireEvent(const FGuid& EventID)
{
    for (int32 i = ActiveEvents.Num() - 1; i >= 0; i--)
    {
        if (ActiveEvents[i].EventID == EventID)
        {
            RemoveEventFromStorage(i);
            break;
        }
    }
}

void UARPG_AIEventManager::ClearAllEvents()
{
    ActiveEvents.Empty();
    EventsByType.Empty();
    
    if (bDebugLogging)
    {
        UE_LOG(LogARPG, Log, TEXT("Cleared all AI events"));
    }
}

FString UARPG_AIEventManager::GetSubscriberDebugInfo() const
{
    FString Info = FString::Printf(TEXT("AI Event Manager Debug Info:\n"));
    Info += FString::Printf(TEXT("Registered Brains: %d\n"), RegisteredBrains.Num());
    Info += FString::Printf(TEXT("Active Events: %d\n"), ActiveEvents.Num());
    Info += FString::Printf(TEXT("Event Types Tracked: %d\n"), EventsByType.Num());
    
    Info += TEXT("Brain Subscriptions:\n");
    for (const auto& Pair : BrainSubscriptions)
    {
        if (IsValid(Pair.Key.Get()))
        {
            Info += FString::Printf(TEXT("  %s: %d subscriptions\n"), 
                   *Pair.Key->GetOwner()->GetName(), Pair.Value.Num());
        }
    }
    
    return Info;
}

void UARPG_AIEventManager::CleanupExpiredEvents()
{
    if (ActiveEvents.Num() == 0)
    {
        return;
    }
    
    float CurrentTime = GetWorld()->GetTimeSeconds();
    int32 RemovedCount = 0;
    
    // Remove expired events
    for (int32 i = ActiveEvents.Num() - 1; i >= 0; i--)
    {
        const FARPG_AIEvent& Event = ActiveEvents[i];
        float EventAge = CurrentTime - Event.Timestamp;
        
        if (EventAge > EventHistoryDuration)
        {
            RemoveEventFromStorage(i);
            RemovedCount++;
        }
    }
    
    // Limit total event count
    if (ActiveEvents.Num() > MaxEventHistory)
    {
        // Remove oldest events first
        TArray<int32> SortedIndices;
        for (int32 i = 0; i < ActiveEvents.Num(); i++)
        {
            SortedIndices.Add(i);
        }
        
        SortedIndices.Sort([this](int32 A, int32 B) {
            return ActiveEvents[A].Timestamp < ActiveEvents[B].Timestamp;
        });
        
        int32 ToRemove = ActiveEvents.Num() - MaxEventHistory;
        for (int32 i = 0; i < ToRemove; i++)
        {
            RemoveEventFromStorage(SortedIndices[i]);
            RemovedCount++;
        }
    }
    
    if (bDebugLogging && RemovedCount > 0)
    {
        UE_LOG(LogARPG, VeryVerbose, TEXT("Cleaned up %d expired events"), RemovedCount);
    }
}

void UARPG_AIEventManager::ProcessEventForPropagation(const FARPG_AIEvent& Event)
{
    // Broadcast to main delegate (anyone can listen to this)
    OnAnyEvent.Broadcast(Event.EventType, Event);
    
    // Broadcast to registered brain components
    for (auto It = RegisteredBrains.CreateIterator(); It; ++It)
    {
        if (!IsValid(It->Get()))
        {
            It.RemoveCurrent();
            continue;
        }
        
        UARPG_AIBrainComponent* Brain = It->Get();
        if (ShouldBroadcastToBrain(Event, Brain))
        {
            // Call the brain's event handler directly
            // This would need to be exposed as a public function in the brain component
            if (Brain->GetClass()->ImplementsInterface(UARPG_EventSubscriber::StaticClass()))
            {
                IARPG_EventSubscriber::Execute_OnEventReceived(Brain, Event.EventType, Event);
            }
        }
    }
    
    // Broadcast to interface subscribers
    if (InterfaceSubscribers.Contains(Event.EventType))
    {
        TArray<TScriptInterface<IARPG_EventSubscriber>>& Subscribers = InterfaceSubscribers[Event.EventType];
        for (auto It = Subscribers.CreateIterator(); It; ++It)
        {
            if (It->GetInterface())
            {
                IARPG_EventSubscriber::Execute_OnEventReceived(It->GetObject(), Event.EventType, Event);
            }
            else
            {
                It.RemoveCurrent();
            }
        }
    }
}

bool UARPG_AIEventManager::ShouldBroadcastToBrain(const FARPG_AIEvent& Event, UARPG_AIBrainComponent* Brain) const
{
    if (!IsValid(Brain) || !Brain->GetOwner())
    {
        return false;
    }
    
    // Check if brain is subscribed to this event type or its parents
    const TSet<FGameplayTag>* Subscriptions = BrainSubscriptions.Find(Brain);
    if (Subscriptions)
    {
        for (const FGameplayTag& SubscribedTag : *Subscriptions)
        {
            if (Event.EventType.MatchesTag(SubscribedTag))
            {
                // Additional distance check for local events
                if (!Event.bGlobal && Event.EventRadius > 0.0f)
                {
                    float Distance = FVector::Dist(Event.EventLocation, Brain->GetOwner()->GetActorLocation());
                    return Distance <= Event.EventRadius;
                }
                return true;
            }
        }
    }
    
    // If no specific subscriptions, only broadcast global events
    return Event.bGlobal;
}

float UARPG_AIEventManager::CalculateEventRelevance(const FARPG_AIEvent& Event, FVector ListenerLocation) const
{
    if (Event.bGlobal)
    {
        return 1.0f;
    }
    
    if (Event.EventRadius <= 0.0f)
    {
        return Event.EventStrength;
    }
    
    float Distance = FVector::Dist(Event.EventLocation, ListenerLocation);
    if (Distance > Event.EventRadius)
    {
        return 0.0f;
    }
    
    // Linear falloff based on distance
    float DistanceRatio = 1.0f - (Distance / Event.EventRadius);
    return Event.EventStrength * DistanceRatio;
}

FGuid UARPG_AIEventManager::GenerateEventID() const
{
    return FGuid::NewGuid();
}

void UARPG_AIEventManager::AddEventToStorage(const FARPG_AIEvent& Event)
{
    int32 NewIndex = ActiveEvents.Add(Event);
    
    // Add to type index
    EventsByType.FindOrAdd(Event.EventType).Add(NewIndex);
}

void UARPG_AIEventManager::RemoveEventFromStorage(int32 EventIndex)
{
    if (!ActiveEvents.IsValidIndex(EventIndex))
    {
        return;
    }
    
    const FARPG_AIEvent& Event = ActiveEvents[EventIndex];
    
    // Remove from type index
    if (TArray<int32>* TypeEvents = EventsByType.Find(Event.EventType))
    {
        TypeEvents->Remove(EventIndex);
        if (TypeEvents->Num() == 0)
        {
            EventsByType.Remove(Event.EventType);
        }
    }
    
    // Remove from main array
    ActiveEvents.RemoveAtSwap(EventIndex);
    
    // Update indices in type maps (since we used RemoveAtSwap)
    if (EventIndex < ActiveEvents.Num())
    {
        const FARPG_AIEvent& MovedEvent = ActiveEvents[EventIndex];
        if (TArray<int32>* TypeEvents = EventsByType.Find(MovedEvent.EventType))
        {
            // Find and update the old index reference
            int32* FoundIndex = TypeEvents->FindByPredicate([this, EventIndex](int32 Index) {
                return Index == ActiveEvents.Num(); // This was the last index before swap
            });
            if (FoundIndex)
            {
                *FoundIndex = EventIndex;
            }
        }
    }
}

bool UARPG_AIEventManager::ValidateEvent(const FARPG_AIEvent& Event) const
{
    if (!Event.EventType.IsValid())
    {
        UE_LOG(LogARPG, Warning, TEXT("AI Event has invalid EventType"));
        return false;
    }
    
    if (Event.EventStrength < 0.0f || Event.EventStrength > 1.0f)
    {
        UE_LOG(LogARPG, Warning, TEXT("AI Event has invalid EventStrength: %.2f"), Event.EventStrength);
        return false;
    }
    
    return true;
}

FARPG_AIEvent UARPG_AIEventManager::ConvertStimulusToEvent(const FARPG_AIStimulus& Stimulus, bool bCreateMemory) const
{
    FARPG_AIEvent Event;
    Event.EventID = GenerateEventID();
    Event.EventType = Stimulus.StimulusTag;
    Event.EventLocation = Stimulus.Location;
    Event.EventInstigator = Stimulus.SourceActor.Get();
    Event.EventStrength = Stimulus.Intensity;
    Event.EventRadius = 500.0f; // Default stimulus radius
    Event.bGlobal = false;
    Event.Timestamp = GetWorld()->GetTimeSeconds();
    
    // Copy stimulus data
    for (const auto& DataPair : Stimulus.StimulusData)
    {
        Event.EventData.Add(DataPair.Key, DataPair.Value);
    }
    
    Event.EventData.Add(TEXT("StimulusType"), StaticEnum<EARPG_StimulusType>()->GetValueAsString(Stimulus.StimulusType));
    Event.EventData.Add(TEXT("CreateMemory"), bCreateMemory ? TEXT("true") : TEXT("false"));
    
    return Event;
}

void UARPG_AIEventManager::RegisterSubscriber(TScriptInterface<IARPG_EventSubscriber> Subscriber)
{
    if (!Subscriber.GetInterface())
    {
        return;
    }
    
    // Subscribe to all events - the subscriber can filter what it cares about
    FGameplayTag AllEventsTag = FGameplayTag::RequestGameplayTag(TEXT("AI.Event"));
    InterfaceSubscribers.FindOrAdd(AllEventsTag).AddUnique(Subscriber);
    
    if (bDebugLogging)
    {
        UE_LOG(LogARPG, Log, TEXT("Registered event subscriber"));
    }
}

void UARPG_AIEventManager::UnregisterSubscriber(TScriptInterface<IARPG_EventSubscriber> Subscriber)
{
    if (!Subscriber.GetInterface())
    {
        return;
    }
    
    // Remove from all subscription lists
    for (auto& SubscriptionPair : InterfaceSubscribers)
    {
        SubscriptionPair.Value.Remove(Subscriber);
    }
    
    // Clean up empty subscription lists
    for (auto It = InterfaceSubscribers.CreateIterator(); It; ++It)
    {
        if (It->Value.Num() == 0)
        {
            It.RemoveCurrent();
        }
    }
    
    if (bDebugLogging)
    {
        UE_LOG(LogARPG, Log, TEXT("Unregistered event subscriber"));
    }
}