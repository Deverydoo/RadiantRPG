// Source/RadiantRPG/Private/World/WorldEventManager.cpp

#include "World/WorldEventManager.h"
#include "World/EventListenerComponent.h"
#include "World/RadiantZoneManager.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "CollisionQueryParams.h"

void UWorldEventManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    UE_LOG(LogTemp, Log, TEXT("WorldEventManager initialized"));
}

void UWorldEventManager::Deinitialize()
{
    if (GetWorld() && UpdateTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(UpdateTimerHandle);
    }
    
    RegisteredListeners.Empty();
    RegisteredZones.Empty();
    ActiveEvents.Empty();
    EventHistory.Empty();
    
    Super::Deinitialize();
}

void UWorldEventManager::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    
    // Start the update timer
    InWorld.GetTimerManager().SetTimer(
        UpdateTimerHandle,
        this,
        &UWorldEventManager::UpdateActiveEvents,
        UpdateInterval,
        true
    );
    
    UE_LOG(LogTemp, Log, TEXT("WorldEventManager started update timer"));
}

bool UWorldEventManager::ShouldCreateSubsystem(UObject* Outer) const
{
    // Only create in game worlds
    UWorld* World = Cast<UWorld>(Outer);
    return World && (World->IsGameWorld() || World->IsPlayInEditor());
}

void UWorldEventManager::BroadcastEvent(const FWorldEvent& Event)
{
    if (!GetWorld())
    {
        return;
    }
    
    // Set timestamp if not already set
    FWorldEvent ProcessedEvent = Event;
    if (ProcessedEvent.Timestamp == 0.0f)
    {
        ProcessedEvent.Timestamp = GetWorld()->GetTimeSeconds();
    }
    
    // Process the event
    ProcessEvent(ProcessedEvent);
    
    // Add to active events if it has duration
    if (ProcessedEvent.Duration > 0.0f)
    {
        ActiveEvents.Add(ProcessedEvent);
        
        // Maintain max active events
        if (ActiveEvents.Num() > MaxActiveEvents)
        {
            ActiveEvents.RemoveAt(0);
        }
    }
    
    // Add to history
    FEventMemory Memory;
    Memory.Event = ProcessedEvent;
    Memory.RecordedTime = GetWorld()->GetTimeSeconds();
    Memory.Importance = static_cast<float>(ProcessedEvent.Priority);
    EventHistory.Add(Memory);
    
    // Maintain max history
    if (EventHistory.Num() > MaxHistoryEntries)
    {
        EventHistory.RemoveAt(0);
    }
    
    // Notify listeners
    NotifyListenersInRange(ProcessedEvent);
    NotifyZones(ProcessedEvent);
    
    // Broadcast delegates
    OnEventBroadcast.Broadcast(ProcessedEvent);
    OnEventBroadcastBP.Broadcast(ProcessedEvent);
    
    UE_LOG(LogTemp, Verbose, TEXT("Broadcasted event: %s at %s with scope %d"),
        *ProcessedEvent.EventTag.ToString(),
        *ProcessedEvent.Location.ToString(),
        (int32)ProcessedEvent.Scope);
}

void UWorldEventManager::BroadcastEventAtLocation(FGameplayTag EventTag, FVector Location, 
    float Radius, AActor* Instigator, EEventPriority Priority)
{
    FWorldEvent Event;
    Event.EventTag = EventTag;
    Event.Location = Location;
    Event.Radius = Radius;
    Event.Instigator = Instigator;
    Event.Priority = Priority;
    Event.Scope = EEventScope::Local;
    
    BroadcastEvent(Event);
}

void UWorldEventManager::BroadcastGlobalEvent(FGameplayTag EventTag, AActor* Instigator)
{
    FWorldEvent Event;
    Event.EventTag = EventTag;
    Event.Instigator = Instigator;
    Event.Scope = EEventScope::Global;
    Event.Priority = EEventPriority::High;
    
    if (Instigator)
    {
        Event.Location = Instigator->GetActorLocation();
    }
    
    BroadcastEvent(Event);
}

void UWorldEventManager::BroadcastZoneEvent(FGameplayTag EventTag, FGameplayTag ZoneTag, AActor* Instigator)
{
    FWorldEvent Event;
    Event.EventTag = EventTag;
    Event.Instigator = Instigator;
    Event.Scope = EEventScope::Zone;
    Event.Metadata.Add("ZoneTag", ZoneTag.ToString());
    
    // Find the zone and set location
    for (const auto& ZonePtr : RegisteredZones)
    {
        if (ARadiantZoneManager* Zone = ZonePtr.Get())
        {
            if (Zone->GetZoneTag() == ZoneTag)
            {
                Event.Location = Zone->GetActorLocation();
                Event.Radius = Zone->GetZoneRadius();
                break;
            }
        }
    }
    
    BroadcastEvent(Event);
}

void UWorldEventManager::BroadcastStimulus(const FStimulus& Stimulus)
{
    if (!GetWorld())
    {
        return;
    }
    
    // Convert stimulus to event for general processing
    FWorldEvent Event;
    Event.EventTag = Stimulus.StimulusTag;
    Event.Location = Stimulus.Location;
    Event.Instigator = Stimulus.Source;
    Event.Intensity = Stimulus.Intensity;
    Event.Category = EEventCategory::AI;
    Event.Scope = EEventScope::Local;
    
    // Set radius based on stimulus type
    switch (Stimulus.Type)
    {
        case EAIStimulusType::Visual:
            Event.Radius = 3000.0f * Stimulus.Intensity;
            break;
        case EAIStimulusType::Sound:
            Event.Radius = 2000.0f * Stimulus.Intensity;
            break;
        case EAIStimulusType::Touch:
            Event.Radius = 100.0f;
            break;
        default:
            Event.Radius = 1000.0f * Stimulus.Intensity;
            break;
    }
    
    // Broadcast the event
    BroadcastEvent(Event);
    
    // Also notify AI-specific delegates
    if (Stimulus.Source)
    {
        OnStimulusCreated.Broadcast(Stimulus.Source, Stimulus);
        OnStimulusCreatedBP.Broadcast(Stimulus.Source, Stimulus);
    }
}

void UWorldEventManager::CreateVisualStimulus(AActor* Source, float Intensity)
{
    if (!Source)
    {
        return;
    }
    
    FStimulus Stimulus;
    Stimulus.Type = EAIStimulusType::Visual;
    Stimulus.Source = Source;
    Stimulus.Location = Source->GetActorLocation();
    Stimulus.Intensity = Intensity;
    Stimulus.Timestamp = GetWorld()->GetTimeSeconds();
    Stimulus.StimulusTag = FGameplayTag::RequestGameplayTag("AI.Stimulus.Visual");
    
    BroadcastStimulus(Stimulus);
}

void UWorldEventManager::CreateAuditoryStimulus(AActor* Source, float Intensity, float Radius)
{
    if (!Source)
    {
        return;
    }
    
    FStimulus Stimulus;
    Stimulus.Type = EAIStimulusType::Sound;
    Stimulus.Source = Source;
    Stimulus.Location = Source->GetActorLocation();
    Stimulus.Intensity = Intensity;
    Stimulus.Timestamp = GetWorld()->GetTimeSeconds();
    Stimulus.StimulusTag = FGameplayTag::RequestGameplayTag("AI.Stimulus.Auditory");
    Stimulus.AdditionalData.Add("Radius", FString::SanitizeFloat(Radius));
    
    BroadcastStimulus(Stimulus);
}

void UWorldEventManager::RegisterListener(UEventListenerComponent* Listener)
{
    if (Listener && !RegisteredListeners.Contains(Listener))
    {
        RegisteredListeners.Add(Listener);
        UE_LOG(LogTemp, Verbose, TEXT("Registered event listener: %s"), 
            *GetNameSafe(Listener->GetOwner()));
    }
}

void UWorldEventManager::UnregisterListener(UEventListenerComponent* Listener)
{
    RegisteredListeners.RemoveAll([Listener](const TWeakObjectPtr<UEventListenerComponent>& WeakListener)
    {
        return !WeakListener.IsValid() || WeakListener.Get() == Listener;
    });
}

void UWorldEventManager::RegisterZone(ARadiantZoneManager* Zone)
{
    if (Zone && !RegisteredZones.Contains(Zone))
    {
        RegisteredZones.Add(Zone);
        UE_LOG(LogTemp, Log, TEXT("Registered zone: %s"), *Zone->GetName());
    }
}

void UWorldEventManager::UnregisterZone(ARadiantZoneManager* Zone)
{
    RegisteredZones.RemoveAll([Zone](const TWeakObjectPtr<ARadiantZoneManager>& WeakZone)
    {
        return !WeakZone.IsValid() || WeakZone.Get() == Zone;
    });
}

ARadiantZoneManager* UWorldEventManager::GetZoneAtLocation(FVector Location) const
{
    for (const auto& ZonePtr : RegisteredZones)
    {
        if (ARadiantZoneManager* Zone = ZonePtr.Get())
        {
            if (Zone->IsLocationInZone(Location))
            {
                return Zone;
            }
        }
    }
    return nullptr;
}

TArray<FWorldEvent> UWorldEventManager::GetActiveEventsInRadius(FVector Location, float Radius) const
{
    TArray<FWorldEvent> Result;
    
    for (const FWorldEvent& Event : ActiveEvents)
    {
        float Distance = FVector::Dist(Event.Location, Location);
        if (Distance <= Radius + Event.Radius)
        {
            Result.Add(Event);
        }
    }
    
    return Result;
}

TArray<FWorldEvent> UWorldEventManager::GetActiveEventsByCategory(EEventCategory Category) const
{
    TArray<FWorldEvent> Result;
    
    for (const FWorldEvent& Event : ActiveEvents)
    {
        if (Event.Category == Category)
        {
            Result.Add(Event);
        }
    }
    
    return Result;
}

TArray<FWorldEvent> UWorldEventManager::GetActiveEventsByTag(FGameplayTag Tag) const
{
    TArray<FWorldEvent> Result;
    
    for (const FWorldEvent& Event : ActiveEvents)
    {
        if (Event.EventTag.MatchesTag(Tag))
        {
            Result.Add(Event);
        }
    }
    
    return Result;
}

bool UWorldEventManager::IsEventActiveAtLocation(FGameplayTag EventTag, FVector Location, float CheckRadius) const
{
    for (const FWorldEvent& Event : ActiveEvents)
    {
        if (Event.EventTag.MatchesTag(EventTag))
        {
            float Distance = FVector::Dist(Event.Location, Location);
            if (Distance <= CheckRadius + Event.Radius)
            {
                return true;
            }
        }
    }
    return false;
}

TArray<FEventMemory> UWorldEventManager::GetEventHistory(float TimeWindow) const
{
    if (!GetWorld())
    {
        return TArray<FEventMemory>();
    }
    
    float CurrentTime = GetWorld()->GetTimeSeconds();
    float CutoffTime = CurrentTime - TimeWindow;
    
    TArray<FEventMemory> Result;
    for (const FEventMemory& Memory : EventHistory)
    {
        if (Memory.RecordedTime >= CutoffTime)
        {
            Result.Add(Memory);
        }
    }
    
    return Result;
}

TArray<FEventMemory> UWorldEventManager::GetEventHistoryForActor(AActor* Actor, float TimeWindow) const
{
    if (!Actor || !GetWorld())
    {
        return TArray<FEventMemory>();
    }
    
    float CurrentTime = GetWorld()->GetTimeSeconds();
    float CutoffTime = CurrentTime - TimeWindow;
    
    TArray<FEventMemory> Result;
    for (const FEventMemory& Memory : EventHistory)
    {
        if (Memory.RecordedTime >= CutoffTime)
        {
            if (Memory.Event.Instigator == Actor || Memory.Event.Target == Actor)
            {
                Result.Add(Memory);
            }
        }
    }
    
    return Result;
}

void UWorldEventManager::ProcessEvent(const FWorldEvent& Event)
{
    // Additional event processing logic can go here
    // For example, validating events, applying zone rules, etc.
}

void UWorldEventManager::NotifyListenersInRange(const FWorldEvent& Event)
{
    // Clean up invalid listeners
    RegisteredListeners.RemoveAll([](const TWeakObjectPtr<UEventListenerComponent>& WeakListener)
    {
        return !WeakListener.IsValid();
    });
    
    // Sort listeners by relevance for this event
    TArray<TPair<UEventListenerComponent*, float>> SortedListeners;
    
    for (const auto& ListenerPtr : RegisteredListeners)
    {
        if (UEventListenerComponent* Listener = ListenerPtr.Get())
        {
            if (ShouldListenerReceiveEvent(Listener, Event))
            {
                float Relevance = CalculateEventRelevance(Listener, Event);
                SortedListeners.Add(TPair<UEventListenerComponent*, float>(Listener, Relevance));
            }
        }
    }
    
    // Sort by relevance (highest first)
    SortedListeners.Sort([](const TPair<UEventListenerComponent*, float>& A, 
                           const TPair<UEventListenerComponent*, float>& B)
    {
        return A.Value > B.Value;
    });
    
    // Notify listeners
    for (const auto& Pair : SortedListeners)
    {
        Pair.Key->OnEventReceived(Event);
    }
}

void UWorldEventManager::NotifyZones(const FWorldEvent& Event)
{
    // Clean up invalid zones
    RegisteredZones.RemoveAll([](const TWeakObjectPtr<ARadiantZoneManager>& WeakZone)
    {
        return !WeakZone.IsValid();
    });
    
    // Notify relevant zones
    for (const auto& ZonePtr : RegisteredZones)
    {
        if (ARadiantZoneManager* Zone = ZonePtr.Get())
        {
            // Check if event is within zone or is a zone-wide event
            if (Event.Scope == EEventScope::Global ||
                (Event.Scope == EEventScope::Zone && 
                 Event.Metadata.Contains("ZoneTag") && 
                 Event.Metadata["ZoneTag"] == Zone->GetZoneTag().ToString()) ||
                Zone->IsLocationInZone(Event.Location))
            {
                Zone->OnEventOccurred(Event);
            }
        }
    }
}

void UWorldEventManager::UpdateActiveEvents()
{
    if (!GetWorld())
    {
        return;
    }
    
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    // Remove expired events
    ActiveEvents.RemoveAll([CurrentTime](const FWorldEvent& Event)
    {
        return Event.Duration > 0.0f && 
               (CurrentTime - Event.Timestamp) > Event.Duration;
    });
    
    // Clean up old history
    CleanupExpiredEvents();
}

void UWorldEventManager::CleanupExpiredEvents()
{
    if (!GetWorld())
    {
        return;
    }
    
    float CurrentTime = GetWorld()->GetTimeSeconds();
    float CutoffTime = CurrentTime - EventHistoryDuration;
    
    EventHistory.RemoveAll([CutoffTime](const FEventMemory& Memory)
    {
        return Memory.RecordedTime < CutoffTime;
    });
}

bool UWorldEventManager::ShouldListenerReceiveEvent(UEventListenerComponent* Listener, 
    const FWorldEvent& Event) const
{
    if (!Listener || !Listener->GetOwner())
    {
        return false;
    }
    
    // Check if listener is interested in this event
    if (!Listener->IsInterestedInEvent(Event))
    {
        return false;
    }
    
    // Global events reach everyone
    if (Event.Scope == EEventScope::Global)
    {
        return true;
    }
    
    // Check distance for local events
    if (Event.Scope == EEventScope::Local)
    {
        float Distance = FVector::Dist(
            Listener->GetOwner()->GetActorLocation(),
            Event.Location
        );
        
        if (Distance > Event.Radius)
        {
            return false;
        }
        
        // Check line of sight if required
        if (Listener->RequiresLineOfSight())
        {
            return HasLineOfSight(
                Listener->GetOwner()->GetActorLocation(),
                Event.Location
            );
        }
    }
    
    // Zone events
    if (Event.Scope == EEventScope::Zone)
    {
        ARadiantZoneManager* ListenerZone = GetZoneAtLocation(
            Listener->GetOwner()->GetActorLocation()
        );
        
        if (!ListenerZone)
        {
            return false;
        }
        
        // Check if it's the correct zone
        if (Event.Metadata.Contains("ZoneTag"))
        {
            return Event.Metadata["ZoneTag"] == ListenerZone->GetZoneTag().ToString();
        }
    }
    
    return true;
}

float UWorldEventManager::CalculateEventRelevance(UEventListenerComponent* Listener, 
    const FWorldEvent& Event) const
{
    if (!Listener || !Listener->GetOwner())
    {
        return 0.0f;
    }
    
    float Relevance = 1.0f;
    
    // Priority affects relevance
    Relevance *= static_cast<float>(Event.Priority) + 1.0f;
    
    // Distance affects relevance (closer = more relevant)
    float Distance = FVector::Dist(
        Listener->GetOwner()->GetActorLocation(),
        Event.Location
    );
    
    if (Event.Radius > 0.0f)
    {
        float DistanceFactor = 1.0f - (Distance / Event.Radius);
        Relevance *= FMath::Clamp(DistanceFactor, 0.1f, 1.0f);
    }
    
    // Intensity affects relevance
    Relevance *= Event.Intensity;
    
    // Category preference (can be extended based on listener preferences)
    float CategoryMultiplier = Listener->GetCategoryInterestLevel(Event.Category);
    Relevance *= CategoryMultiplier;
    
    return Relevance;
}

bool UWorldEventManager::HasLineOfSight(FVector From, FVector To) const
{
    if (!GetWorld())
    {
        return false;
    }
    
    FCollisionQueryParams QueryParams;
    QueryParams.bTraceComplex = false;
    QueryParams.AddIgnoredActor(nullptr); // Could add specific actors to ignore
    
    FHitResult Hit;
    return !GetWorld()->LineTraceSingleByChannel(
        Hit,
        From,
        To,
        ECC_Visibility,
        QueryParams
    );
}