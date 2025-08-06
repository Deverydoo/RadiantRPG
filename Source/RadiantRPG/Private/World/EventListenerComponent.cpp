// Source/RadiantRPG/Private/World/EventListenerComponent.cpp

#include "World/EventListenerComponent.h"
#include "World/WorldEventManager.h"
#include "Engine/World.h"
#include "TimerManager.h"

UEventListenerComponent::UEventListenerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    // Default category interest levels
    CategoryInterestLevels.Add(EEventCategory::Combat, 1.0f);
    CategoryInterestLevels.Add(EEventCategory::Social, 0.5f);
    CategoryInterestLevels.Add(EEventCategory::Environmental, 0.3f);
    CategoryInterestLevels.Add(EEventCategory::Economic, 0.2f);
    CategoryInterestLevels.Add(EEventCategory::Faction, 0.7f);
    CategoryInterestLevels.Add(EEventCategory::Discovery, 0.6f);
    CategoryInterestLevels.Add(EEventCategory::Quest, 0.8f);
    CategoryInterestLevels.Add(EEventCategory::Resource, 0.4f);
    CategoryInterestLevels.Add(EEventCategory::AI, 1.0f);
    CategoryInterestLevels.Add(EEventCategory::System, 0.1f);

    // Default stimulus mapping
    CategoryToStimulusMapping.Add(EEventCategory::Combat, EAIStimulusType::Danger);
    CategoryToStimulusMapping.Add(EEventCategory::Social, EAIStimulusType::Social);
    CategoryToStimulusMapping.Add(EEventCategory::Environmental, EAIStimulusType::Visual);
    CategoryToStimulusMapping.Add(EEventCategory::Resource, EAIStimulusType::Opportunity);
}

void UEventListenerComponent::BeginPlay()
{
    Super::BeginPlay();

    // Get the event manager
    if (UWorld* World = GetWorld())
    {
        EventManager = World->GetSubsystem<UWorldEventManager>();
        if (EventManager)
        {
            EventManager->RegisterListener(this);
            
            // Auto-subscribe to all categories if configured
            if (bAutoSubscribeToAllCategories)
            {
                for (const auto& Pair : CategoryInterestLevels)
                {
                    SubscribeToCategory(Pair.Key, DefaultListenRadius);
                }
            }
            
            UE_LOG(LogEventSystem, Log, TEXT("EventListener registered for %s"), 
                *GetNameSafe(GetOwner()));
        }
        else
        {
            UE_LOG(LogEventSystem, Warning, TEXT("Failed to find WorldEventManager for %s"), 
                *GetNameSafe(GetOwner()));
        }

        // Start memory update timer
        World->GetTimerManager().SetTimer(
            MemoryUpdateTimer,
            this,
            &UEventListenerComponent::UpdateMemory,
            5.0f,
            true
        );
    }
}

void UEventListenerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Unregister from event manager
    if (EventManager)
    {
        EventManager->UnregisterListener(this);
    }

    // Clear timer
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(MemoryUpdateTimer);
    }

    Super::EndPlay(EndPlayReason);
}

void UEventListenerComponent::OnEventReceived(const FWorldEvent& Event)
{
    // Check if we should process this event
    if (!IsInterestedInEvent(Event))
    {
        return;
    }

    UE_LOG(LogEventSystem, Verbose, TEXT("%s received event: %s"), 
        *GetNameSafe(GetOwner()), *Event.EventTag.ToString());

    // Process the event
    ProcessReceivedEvent(Event);

    // Broadcast to Blueprint
    OnEventReceivedBP(Event);

    // Check for high priority
    if (Event.Priority >= EEventPriority::High)
    {
        OnHighPriorityEvent(Event);
    }

    // Broadcast delegates
    OnEventProcessed.Broadcast(Event);
    OnEventProcessedBP.Broadcast(Event);

    // Convert to stimulus if needed
    if (bConvertEventsToStimuli)
    {
        ConvertEventToStimulus(Event);
    }

    // Remember the event if important enough
    if (ShouldRememberEvent(Event))
    {
        RememberEvent(Event, CalculateEventImportance(Event));
    }
}

void UEventListenerComponent::SubscribeToEvent(FGameplayTag EventTag, float MaxDistance)
{
    FEventSubscription Sub;
    Sub.EventTag = EventTag;
    Sub.MaxDistance = MaxDistance;
    Sub.MinPriority = MinimumPriority;
    Sub.bRequiresLineOfSight = bRequiresLineOfSight;
    
    Subscriptions.Add(Sub);
    
    UE_LOG(LogEventSystem, Log, TEXT("%s subscribed to event: %s"), 
        *GetNameSafe(GetOwner()), *EventTag.ToString());
}

void UEventListenerComponent::SubscribeToCategory(EEventCategory Category, float MaxDistance)
{
    FEventSubscription Sub;
    Sub.Category = Category;
    Sub.MaxDistance = MaxDistance;
    Sub.MinPriority = MinimumPriority;
    Sub.bRequiresLineOfSight = bRequiresLineOfSight;
    
    Subscriptions.Add(Sub);
    
    UE_LOG(LogEventSystem, Log, TEXT("%s subscribed to category: %d"), 
        *GetNameSafe(GetOwner()), (int32)Category);
}

void UEventListenerComponent::UnsubscribeFromEvent(FGameplayTag EventTag)
{
    Subscriptions.RemoveAll([EventTag](const FEventSubscription& Sub)
    {
        return Sub.EventTag == EventTag;
    });
}

void UEventListenerComponent::UnsubscribeFromCategory(EEventCategory Category)
{
    Subscriptions.RemoveAll([Category](const FEventSubscription& Sub)
    {
        return Sub.Category == Category;
    });
}

void UEventListenerComponent::SetRequiresLineOfSight(bool bRequire)
{
    bRequiresLineOfSight = bRequire;
    
    // Update existing subscriptions
    for (FEventSubscription& Sub : Subscriptions)
    {
        Sub.bRequiresLineOfSight = bRequire;
    }
}

void UEventListenerComponent::SetMinimumPriority(EEventPriority MinPriority)
{
    MinimumPriority = MinPriority;
    
    // Update existing subscriptions
    for (FEventSubscription& Sub : Subscriptions)
    {
        Sub.MinPriority = MinPriority;
    }
}

bool UEventListenerComponent::IsInterestedInEvent(const FWorldEvent& Event) const
{
    // Check priority
    if (Event.Priority < MinimumPriority)
    {
        return false;
    }

    // Check subscriptions
    for (const FEventSubscription& Sub : Subscriptions)
    {
        // Check event tag match
        if (Sub.EventTag.IsValid() && Event.EventTag.MatchesTag(Sub.EventTag))
        {
            return true;
        }
        
        // Check category match
        if (Sub.Category == Event.Category)
        {
            return true;
        }
    }

    // If no specific subscriptions, check if we're interested in the category
    if (Subscriptions.Num() == 0 && bAutoSubscribeToAllCategories)
    {
        return GetCategoryInterestLevel(Event.Category) > 0.0f;
    }

    return false;
}

float UEventListenerComponent::GetCategoryInterestLevel(EEventCategory Category) const
{
    if (const float* Interest = CategoryInterestLevels.Find(Category))
    {
        return *Interest;
    }
    return 0.0f;
}

void UEventListenerComponent::RememberEvent(const FWorldEvent& Event, float Importance)
{
    if (!GetWorld())
    {
        return;
    }

    FEventMemory Memory;
    Memory.Event = Event;
    Memory.RecordedTime = GetWorld()->GetTimeSeconds();
    Memory.Importance = Importance;
    
    // Check if we were a participant
    Memory.bWasParticipant = (Event.Instigator == GetOwner() || Event.Target == GetOwner());
    Memory.bWasWitness = !Memory.bWasParticipant;

    RememberedEvents.Add(Memory);

    // Maintain max memory size
    if (RememberedEvents.Num() > MaxMemorySize)
    {
        // Remove least important old memory
        RememberedEvents.Sort([](const FEventMemory& A, const FEventMemory& B)
        {
            return A.Importance > B.Importance;
        });
        RememberedEvents.SetNum(MaxMemorySize);
    }
}

TArray<FEventMemory> UEventListenerComponent::GetRecentEvents(float TimeWindow) const
{
    if (!GetWorld())
    {
        return TArray<FEventMemory>();
    }

    float CurrentTime = GetWorld()->GetTimeSeconds();
    float CutoffTime = CurrentTime - TimeWindow;

    TArray<FEventMemory> RecentEvents;
    for (const FEventMemory& Memory : RememberedEvents)
    {
        if (Memory.RecordedTime >= CutoffTime)
        {
            RecentEvents.Add(Memory);
        }
    }

    return RecentEvents;
}

FEventMemory UEventListenerComponent::GetMostRecentEventByTag(FGameplayTag Tag) const
{
    FEventMemory MostRecent;
    float LatestTime = 0.0f;

    for (const FEventMemory& Memory : RememberedEvents)
    {
        if (Memory.Event.EventTag.MatchesTag(Tag) && Memory.RecordedTime > LatestTime)
        {
            MostRecent = Memory;
            LatestTime = Memory.RecordedTime;
        }
    }

    return MostRecent;
}

void UEventListenerComponent::ClearMemory()
{
    RememberedEvents.Empty();
}

void UEventListenerComponent::ProcessReceivedEvent(const FWorldEvent& Event)
{
    // Additional processing can be added here
    // This is where derived classes or specific implementations
    // can add custom logic
}

void UEventListenerComponent::UpdateMemory()
{
    if (!GetWorld())
    {
        return;
    }

    float CurrentTime = GetWorld()->GetTimeSeconds();
    float CutoffTime = CurrentTime - MemoryDuration;

    // Remove old memories
    RememberedEvents.RemoveAll([CutoffTime](const FEventMemory& Memory)
    {
        return Memory.RecordedTime < CutoffTime;
    });

    // Sort by importance and recency
    RememberedEvents.Sort([CurrentTime](const FEventMemory& A, const FEventMemory& B)
    {
        // Weight both importance and recency
        float AScore = A.Importance * (1.0f + (A.RecordedTime / CurrentTime));
        float BScore = B.Importance * (1.0f + (B.RecordedTime / CurrentTime));
        return AScore > BScore;
    });
}

bool UEventListenerComponent::ShouldRememberEvent(const FWorldEvent& Event) const
{
    // Check importance threshold
    float Importance = CalculateEventImportance(Event);
    if (Importance < MinimumImportanceToRemember)
    {
        return false;
    }

    // Always remember events we participated in
    if (Event.Instigator == GetOwner() || Event.Target == GetOwner())
    {
        return true;
    }

    // Remember high priority events
    if (Event.Priority >= EEventPriority::High)
    {
        return true;
    }

    // Remember events we're highly interested in
    float InterestLevel = GetCategoryInterestLevel(Event.Category);
    return InterestLevel >= 0.7f;
}

float UEventListenerComponent::CalculateEventImportance(const FWorldEvent& Event) const
{
    float Importance = 0.0f;

    // Base importance from priority
    switch (Event.Priority)
    {
        case EEventPriority::Low:
            Importance = 0.25f;
            break;
        case EEventPriority::Normal:
            Importance = 0.5f;
            break;
        case EEventPriority::High:
            Importance = 0.75f;
            break;
        case EEventPriority::Critical:
            Importance = 1.0f;
            break;
    }

    // Modify by category interest
    Importance *= GetCategoryInterestLevel(Event.Category);

    // Increase if we were involved
    if (Event.Instigator == GetOwner() || Event.Target == GetOwner())
    {
        Importance *= 2.0f;
    }

    // Factor in intensity
    Importance *= Event.Intensity;

    return FMath::Clamp(Importance, 0.0f, 1.0f);
}

void UEventListenerComponent::ConvertEventToStimulus(const FWorldEvent& Event)
{
    FStimulus Stimulus;
    Stimulus.Location = Event.Location;
    Stimulus.Source = Event.Instigator;
    Stimulus.Intensity = Event.Intensity;
    Stimulus.Timestamp = Event.Timestamp;
    Stimulus.StimulusTag = Event.EventTag;

    // Map category to stimulus type
    if (const EAIStimulusType* Type = CategoryToStimulusMapping.Find(Event.Category))
    {
        Stimulus.Type = *Type;
    }
    else
    {
        // Default mapping based on category
        switch (Event.Category)
        {
            case EEventCategory::Combat:
                Stimulus.Type = EAIStimulusType::Danger;
                break;
            case EEventCategory::Social:
                Stimulus.Type = EAIStimulusType::Social;
                break;
            case EEventCategory::Environmental:
                Stimulus.Type = EAIStimulusType::Visual;
                break;
            case EEventCategory::Resource:
                Stimulus.Type = EAIStimulusType::Opportunity;
                break;
            default:
                Stimulus.Type = EAIStimulusType::Visual;
                break;
        }
    }

    // Copy metadata
    Stimulus.AdditionalData = Event.Metadata;

    // Send to Blueprint
    OnStimulusReceived(Stimulus);
}