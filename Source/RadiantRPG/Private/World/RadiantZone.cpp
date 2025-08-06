// Source/RadiantRPG/Private/World/RadiantZone.cpp

#include "World/RadiantZone.h"
#include "World/WorldEventManager.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

ARadiantZone::ARadiantZone()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 1.0f;

    // Create root component
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    // Create zone bounds
    ZoneBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneBounds"));
    ZoneBounds->SetupAttachment(RootComponent);
    ZoneBounds->SetBoxExtent(FVector(5000.0f, 5000.0f, 2000.0f));
    ZoneBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ZoneBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
    ZoneBounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    // Create zone influence sphere
    ZoneInfluence = CreateDefaultSubobject<USphereComponent>(TEXT("ZoneInfluence"));
    ZoneInfluence->SetupAttachment(RootComponent);
    ZoneInfluence->SetSphereRadius(7500.0f);
    ZoneInfluence->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ZoneInfluence->SetCollisionResponseToAllChannels(ECR_Ignore);

    // Default weather types
    PossibleWeatherTypes = { EZoneWeather::Clear, EZoneWeather::Cloudy, EZoneWeather::Rain };
}

void ARadiantZone::BeginPlay()
{
    Super::BeginPlay();

    // Get event manager
    if (UWorld* World = GetWorld())
    {
        EventManager = World->GetSubsystem<UWorldEventManager>();
        if (EventManager)
        {
            EventManager->RegisterZone(this);
        }

        // Setup timers
        if (WeatherUpdateInterval > 0.0f)
        {
            World->GetTimerManager().SetTimer(
                WeatherUpdateTimer,
                this,
                &ARadiantZone::UpdateWeather,
                WeatherUpdateInterval,
                true
            );
        }

        World->GetTimerManager().SetTimer(
            ResourceUpdateTimer,
            this,
            &ARadiantZone::UpdateResources,
            10.0f,
            true
        );

        World->GetTimerManager().SetTimer(
            EventProcessTimer,
            this,
            &ARadiantZone::ProcessZoneEvents,
            5.0f,
            true
        );
    }

    // Bind overlap events
    ZoneBounds->OnComponentBeginOverlap.AddDynamic(this, &ARadiantZone::OnZoneBeginOverlap);
    ZoneBounds->OnComponentEndOverlap.AddDynamic(this, &ARadiantZone::OnZoneEndOverlap);

    UE_LOG(LogEventSystem, Log, TEXT("Zone %s initialized"), *ZoneName);
}

void ARadiantZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Unregister from event manager
    if (EventManager)
    {
        EventManager->UnregisterZone(this);
    }

    // Clear timers
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(WeatherUpdateTimer);
        World->GetTimerManager().ClearTimer(ResourceUpdateTimer);
        World->GetTimerManager().ClearTimer(EventProcessTimer);
    }

    Super::EndPlay(EndPlayReason);
}

void ARadiantZone::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsActive)
    {
        CheckFactionStatus();
    }
}

float ARadiantZone::GetZoneRadius() const
{
    if (ZoneInfluence)
    {
        return ZoneInfluence->GetScaledSphereRadius();
    }
    return 0.0f;
}

bool ARadiantZone::IsLocationInZone(FVector Location) const
{
    if (!ZoneBounds)
    {
        return false;
    }

    FVector LocalLocation = GetActorTransform().InverseTransformPosition(Location);
    FVector BoxExtent = ZoneBounds->GetScaledBoxExtent();
    
    return FMath::Abs(LocalLocation.X) <= BoxExtent.X &&
           FMath::Abs(LocalLocation.Y) <= BoxExtent.Y &&
           FMath::Abs(LocalLocation.Z) <= BoxExtent.Z;
}

void ARadiantZone::OnEventOccurred(const FWorldEvent& Event)
{
    if (!bIsActive)
    {
        return;
    }

    // Check if this event type is allowed
    if (BlockedEventTypes.Contains(Event.EventTag))
    {
        return;
    }

    // Add to active zone events
    ActiveZoneEvents.Add(Event);

    // Apply zone-specific event modifications
    FWorldEvent ModifiedEvent = Event;
    ModifiedEvent.Intensity *= EventFrequencyMultiplier;

    // Notify Blueprint
    OnZoneEventTriggered(ModifiedEvent);

    UE_LOG(LogEventSystem, Verbose, TEXT("Zone %s processed event: %s"), 
        *ZoneName, *Event.EventTag.ToString());
}

void ARadiantZone::TriggerZoneEvent(FGameplayTag EventTag, AActor* InInstigator)
{
    if (!EventManager || !bIsActive)
    {
        return;
    }

    EventManager->BroadcastZoneEvent(EventTag, ZoneTag, InInstigator);
}

void ARadiantZone::BroadcastZoneAnnouncement(const FString& Message, EEventPriority Priority)
{
    if (!EventManager || !bIsActive)
    {
        return;
    }

    FWorldEvent Event;
    Event.EventTag = FGameplayTag::RequestGameplayTag("Zone.Announcement");
    Event.Category = EEventCategory::System;
    Event.Scope = EEventScope::Zone;
    Event.Priority = Priority;
    Event.Location = GetActorLocation();
    Event.Radius = GetZoneRadius();
    Event.Metadata.Add("Message", Message);
    Event.Metadata.Add("ZoneName", ZoneName);

    EventManager->BroadcastEvent(Event);
}

void ARadiantZone::SetWeather(EZoneWeather NewWeather)
{
    if (CurrentWeather != NewWeather)
    {
        EZoneWeather OldWeather = CurrentWeather;
        CurrentWeather = NewWeather;

        // Play weather sound
        if (USoundBase** SoundPtr = WeatherSounds.Find(NewWeather))
        {
            if (*SoundPtr)
            {
                UGameplayStatics::PlaySoundAtLocation(this, *SoundPtr, GetActorLocation());
            }
        }

        // Broadcast weather change event
        if (EventManager)
        {
            FWorldEvent Event;
            Event.EventTag = FGameplayTag::RequestGameplayTag("Zone.Weather.Changed");
            Event.Category = EEventCategory::Environmental;
            Event.Scope = EEventScope::Zone;
            Event.Location = GetActorLocation();
            Event.Radius = GetZoneRadius();
            Event.Metadata.Add("OldWeather", FString::FromInt((int32)OldWeather));
            Event.Metadata.Add("NewWeather", FString::FromInt((int32)NewWeather));

            EventManager->BroadcastEvent(Event);
        }

        // Notify Blueprint
        OnWeatherChanged(NewWeather);

        UE_LOG(LogEventSystem, Log, TEXT("Zone %s weather changed to %d"), 
            *ZoneName, (int32)NewWeather);
    }
}

void ARadiantZone::StartWeatherCycle()
{
    if (GetWorld() && !WeatherUpdateTimer.IsValid())
    {
        GetWorld()->GetTimerManager().SetTimer(
            WeatherUpdateTimer,
            this,
            &ARadiantZone::UpdateWeather,
            WeatherUpdateInterval,
            true
        );
    }
}

void ARadiantZone::StopWeatherCycle()
{
    if (GetWorld() && WeatherUpdateTimer.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(WeatherUpdateTimer);
    }
}

void ARadiantZone::SetControllingFaction(FGameplayTag FactionTag)
{
    if (ControllingFaction != FactionTag)
    {
        FGameplayTag OldFaction = ControllingFaction;
        ControllingFaction = FactionTag;
        FactionControlStrength = 1.0f;

        // Broadcast faction change event
        if (EventManager)
        {
            FWorldEvent Event;
            Event.EventTag = FGameplayTag::RequestGameplayTag("Zone.Faction.ControlChanged");
            Event.Category = EEventCategory::Faction;
            Event.Scope = EEventScope::Zone;
            Event.Priority = EEventPriority::High;
            Event.Location = GetActorLocation();
            Event.Radius = GetZoneRadius();
            Event.Metadata.Add("OldFaction", OldFaction.ToString());
            Event.Metadata.Add("NewFaction", FactionTag.ToString());

            EventManager->BroadcastEvent(Event);
        }

        // Notify Blueprint
        OnFactionControlChanged(FactionTag);

        UE_LOG(LogEventSystem, Log, TEXT("Zone %s control changed to faction: %s"), 
            *ZoneName, *FactionTag.ToString());
    }
}

void ARadiantZone::StartFactionConflict(FGameplayTag AttackingFaction)
{
    if (!AttackingFaction.IsValid() || AttackingFaction == ControllingFaction)
    {
        return;
    }

    ContestedByFaction = AttackingFaction;

    // Broadcast conflict event
    if (EventManager)
    {
        FWorldEvent Event;
        Event.EventTag = FGameplayTag::RequestGameplayTag("Zone.Faction.ConflictStarted");
        Event.Category = EEventCategory::Faction;
        Event.Scope = EEventScope::Zone;
        Event.Priority = EEventPriority::High;
        Event.Location = GetActorLocation();
        Event.Radius = GetZoneRadius();
        Event.Metadata.Add("DefendingFaction", ControllingFaction.ToString());
        Event.Metadata.Add("AttackingFaction", AttackingFaction.ToString());

        EventManager->BroadcastEvent(Event);
    }

    UE_LOG(LogEventSystem, Log, TEXT("Zone %s contested by faction: %s"), 
        *ZoneName, *AttackingFaction.ToString());
}

void ARadiantZone::SetResourceAvailability(FGameplayTag ResourceType, float Availability)
{
    ResourceAvailability.Add(ResourceType, FMath::Clamp(Availability, 0.0f, MaxResourceCapacity));
}

float ARadiantZone::GetResourceAvailability(FGameplayTag ResourceType) const
{
    if (const float* Availability = ResourceAvailability.Find(ResourceType))
    {
        return *Availability;
    }
    return 0.0f;
}

void ARadiantZone::ConsumeResource(FGameplayTag ResourceType, float Amount)
{
    if (float* Availability = ResourceAvailability.Find(ResourceType))
    {
        float OldAmount = *Availability;
        *Availability = FMath::Max(0.0f, *Availability - Amount);

        // Broadcast resource depletion if significant
        if (OldAmount > 0.0f && *Availability == 0.0f && EventManager)
        {
            FWorldEvent Event;
            Event.EventTag = FGameplayTag::RequestGameplayTag("Zone.Resource.Depleted");
            Event.Category = EEventCategory::Resource;
            Event.Scope = EEventScope::Zone;
            Event.Location = GetActorLocation();
            Event.Metadata.Add("ResourceType", ResourceType.ToString());

            EventManager->BroadcastEvent(Event);
        }
    }
}

void ARadiantZone::RegenerateResources()
{
    for (auto& Pair : ResourceAvailability)
    {
        Pair.Value = FMath::Min(MaxResourceCapacity, 
            Pair.Value + (ResourceRegenerationRate * MaxResourceCapacity));
    }
}

void ARadiantZone::OnPlayerDiscovered(AActor* Player)
{
    if (!bIsDiscovered && Player)
    {
        bIsDiscovered = true;

        // Play discovery sound
        if (DiscoverySound)
        {
            UGameplayStatics::PlaySoundAtLocation(this, DiscoverySound, GetActorLocation());
        }

        // Broadcast discovery event
        if (EventManager)
        {
            FWorldEvent Event;
            Event.EventTag = FGameplayTag::RequestGameplayTag("Zone.Discovered");
            Event.Category = EEventCategory::Discovery;
            Event.Scope = EEventScope::Zone;
            Event.Priority = EEventPriority::High;
            Event.Location = GetActorLocation();
            Event.Instigator = Player;
            Event.Metadata.Add("ZoneName", ZoneName);

            EventManager->BroadcastEvent(Event);
        }

        UE_LOG(LogEventSystem, Log, TEXT("Zone %s discovered by player"), *ZoneName);
    }
}

void ARadiantZone::SetSpawnRules(const TArray<FGameplayTag>& AllowedTypes)
{
    AllowedSpawnTypes = AllowedTypes;
}

void ARadiantZone::SpawnZoneNPC(TSubclassOf<AActor> NPCClass, FVector SpawnLocation)
{
    if (!NPCClass || SpawnedNPCs.Num() >= MaxNPCsInZone)
    {
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    if (AActor* NewNPC = GetWorld()->SpawnActor<AActor>(NPCClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams))
    {
        SpawnedNPCs.Add(NewNPC);
        ActorsInZone.Add(NewNPC);

        UE_LOG(LogEventSystem, Log, TEXT("Spawned NPC in zone %s"), *ZoneName);
    }
}

void ARadiantZone::DespawnAllNPCs()
{
    for (AActor* NPC : SpawnedNPCs)
    {
        if (IsValid(NPC))
        {
            NPC->Destroy();
        }
    }
    SpawnedNPCs.Empty();
}

void ARadiantZone::ActivateZone()
{
    bIsActive = true;
    SetActorTickEnabled(true);

    UE_LOG(LogEventSystem, Log, TEXT("Zone %s activated"), *ZoneName);
}

void ARadiantZone::DeactivateZone()
{
    bIsActive = false;
    SetActorTickEnabled(false);
    DespawnAllNPCs();

    UE_LOG(LogEventSystem, Log, TEXT("Zone %s deactivated"), *ZoneName);
}

void ARadiantZone::OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || !bIsActive)
    {
        return;
    }

    ActorsInZone.AddUnique(OtherActor);

    // Check if it's a player
    if (OtherActor->ActorHasTag("Player"))
    {
        PlayersInZone.AddUnique(OtherActor);
        
        if (!bIsDiscovered)
        {
            OnPlayerDiscovered(OtherActor);
        }
    }

    // Notify Blueprint
    OnZoneEntered(OtherActor);

    // Broadcast enter event
    if (EventManager)
    {
        FWorldEvent Event;
        Event.EventTag = FGameplayTag::RequestGameplayTag("Zone.ActorEntered");
        Event.Category = EEventCategory::System;
        Event.Scope = EEventScope::Local;
        Event.Location = OtherActor->GetActorLocation();
        Event.Instigator = OtherActor;
        Event.Metadata.Add("ZoneName", ZoneName);

        EventManager->BroadcastEvent(Event);
    }
}

void ARadiantZone::OnZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!OtherActor)
    {
        return;
    }

    ActorsInZone.Remove(OtherActor);
    PlayersInZone.Remove(OtherActor);

    // Notify Blueprint
    OnZoneExited(OtherActor);

    // Broadcast exit event
    if (EventManager)
    {
        FWorldEvent Event;
        Event.EventTag = FGameplayTag::RequestGameplayTag("Zone.ActorExited");
        Event.Category = EEventCategory::System;
        Event.Scope = EEventScope::Local;
        Event.Location = OtherActor->GetActorLocation();
        Event.Instigator = OtherActor;
        Event.Metadata.Add("ZoneName", ZoneName);

        EventManager->BroadcastEvent(Event);
    }
}

void ARadiantZone::UpdateWeather()
{
    if (!bIsActive || PossibleWeatherTypes.Num() == 0)
    {
        return;
    }

    // Random chance to change weather
    if (FMath::FRand() < WeatherChangeProbability)
    {
        int32 RandomIndex = FMath::RandRange(0, PossibleWeatherTypes.Num() - 1);
        SetWeather(PossibleWeatherTypes[RandomIndex]);
    }
}

void ARadiantZone::UpdateResources()
{
    if (!bIsActive)
    {
        return;
    }

    RegenerateResources();
}

void ARadiantZone::ProcessZoneEvents()
{
    if (!bIsActive)
    {
        return;
    }

    // Clean up expired events
    float CurrentTime = GetWorld()->GetTimeSeconds();
    ActiveZoneEvents.RemoveAll([CurrentTime](const FWorldEvent& Event)
    {
        return Event.Duration > 0.0f && 
               (CurrentTime - Event.Timestamp) > Event.Duration;
    });
}

void ARadiantZone::CheckFactionStatus()
{
    if (!ContestedByFaction.IsValid())
    {
        return;
    }

    // Simple faction control decay
    FactionControlStrength -= 0.001f;

    if (FactionControlStrength <= 0.0f)
    {
        // Zone captured
        SetControllingFaction(ContestedByFaction);
        ContestedByFaction = FGameplayTag();
    }
}

void ARadiantZone::UpdateDiscoverySound()
{
    // This could be expanded to play different sounds based on zone state
}