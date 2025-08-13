#include "World/RadiantZoneManager.h"
#include "World/WorldEventManager.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

ARadiantZoneManager::ARadiantZoneManager()
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

    // Create audio components
    AmbientAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AmbientAudioComponent"));
    AmbientAudioComponent->SetupAttachment(RootComponent);
    AmbientAudioComponent->bAutoActivate = false;
    AmbientAudioComponent->bIsUISound = false;
    AmbientAudioComponent->SetVolumeMultiplier(AmbientSoundVolume);

    WeatherAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("WeatherAudioComponent"));
    WeatherAudioComponent->SetupAttachment(RootComponent);
    WeatherAudioComponent->bAutoActivate = false;
    WeatherAudioComponent->bIsUISound = false;

    // Default weather types
    PossibleWeatherTypes = { EZoneWeather::Clear, EZoneWeather::Cloudy, EZoneWeather::Rain };
}

void ARadiantZoneManager::BeginPlay()
{
    Super::BeginPlay();

    // Bind overlap events
    if (ZoneBounds)
    {
        ZoneBounds->OnComponentBeginOverlap.AddDynamic(this, &ARadiantZoneManager::OnZoneBeginOverlap);
        ZoneBounds->OnComponentEndOverlap.AddDynamic(this, &ARadiantZoneManager::OnZoneEndOverlap);
    }

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
                &ARadiantZoneManager::UpdateWeather,
                WeatherUpdateInterval,
                true
            );
        }

        World->GetTimerManager().SetTimer(
            ResourceUpdateTimer,
            this,
            &ARadiantZoneManager::UpdateResources,
            10.0f,
            true
        );

        World->GetTimerManager().SetTimer(
            EventProcessTimer,
            this,
            &ARadiantZoneManager::ProcessZoneEvents,
            5.0f,
            true
        );
    }

    // Auto-activate if configured
    if (bAutoActivateOnBeginPlay)
    {
        ActivateZone();
    }

    UE_LOG(LogTemp, Log, TEXT("Zone %s initialized"), *ZoneName);
}

void ARadiantZoneManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    DeactivateZone();

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

void ARadiantZoneManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsActive)
    {
        CheckFactionStatus();
    }
}

// Zone Management
void ARadiantZoneManager::ActivateZone()
{
    if (bIsActive)
    {
        return;
    }

    bIsActive = true;

    // Start ambient sound if available
    PlayAmbientSound();

    // Initialize weather
    if (CurrentWeather != EZoneWeather::Clear)
    {
        SetWeather(CurrentWeather);
    }

    // Broadcast activation event
    if (EventManager)
    {
        FWorldEvent Event;
        Event.EventTag = FGameplayTag::RequestGameplayTag("Zone.Activated");
        Event.Category = EEventCategory::System;
        Event.Scope = EEventScope::Zone;
        Event.Location = GetActorLocation();
        Event.Metadata.Add("ZoneName", ZoneName);

        EventManager->BroadcastEvent(Event);
    }

    UE_LOG(LogTemp, Log, TEXT("Zone %s activated"), *ZoneName);
}

void ARadiantZoneManager::DeactivateZone()
{
    if (!bIsActive)
    {
        return;
    }

    bIsActive = false;

    // Stop sounds
    StopAmbientSound();
    if (WeatherAudioComponent && WeatherAudioComponent->IsPlaying())
    {
        WeatherAudioComponent->FadeOut(1.0f, 0.0f);
    }

    // Clear active NPCs
    DespawnAllNPCs();

    UE_LOG(LogTemp, Log, TEXT("Zone %s deactivated"), *ZoneName);
}

// Audio Management
void ARadiantZoneManager::PlayAmbientSound()
{
    if (!AmbientAudioComponent || !AmbientSound)
    {
        return;
    }

    if (!AmbientAudioComponent->IsPlaying())
    {
        AmbientAudioComponent->SetSound(AmbientSound);
        AmbientAudioComponent->SetVolumeMultiplier(0.0f);
        AmbientAudioComponent->Play();
        AmbientAudioComponent->FadeIn(AmbientSoundFadeInTime, AmbientSoundVolume);
        
        UE_LOG(LogTemp, Log, TEXT("Zone %s: Started ambient sound"), *ZoneName);
    }
}

void ARadiantZoneManager::StopAmbientSound()
{
    if (AmbientAudioComponent && AmbientAudioComponent->IsPlaying())
    {
        AmbientAudioComponent->FadeOut(AmbientSoundFadeOutTime, 0.0f);
        UE_LOG(LogTemp, Log, TEXT("Zone %s: Stopping ambient sound"), *ZoneName);
    }
}

void ARadiantZoneManager::UpdateAmbientSound()
{
    if (!AmbientAudioComponent)
    {
        return;
    }

    // Adjust volume based on player proximity
    float ClosestDistance = FLT_MAX;
    for (AActor* Player : PlayersInZone)
    {
        if (Player)
        {
            float Distance = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
            ClosestDistance = FMath::Min(ClosestDistance, Distance);
        }
    }

    if (ClosestDistance < FLT_MAX)
    {
        float NormalizedDistance = FMath::Clamp(ClosestDistance / GetZoneRadius(), 0.0f, 1.0f);
        float TargetVolume = AmbientSoundVolume * (1.0f - NormalizedDistance * 0.5f);
        AmbientAudioComponent->SetVolumeMultiplier(TargetVolume);
    }
}

// Overlap Handlers
void ARadiantZoneManager::OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || !bIsActive)
    {
        return;
    }

    // Add to actors in zone
    ActorsInZone.AddUnique(OtherActor);

    // Check if it's a player
    if (ACharacter* Character = Cast<ACharacter>(OtherActor))
    {
        if (Character->IsPlayerControlled())
        {
            PlayersInZone.AddUnique(OtherActor);
            HandlePlayerEntry(OtherActor);
        }
    }

    // Notify Blueprint
    OnZoneEntered(OtherActor);

    // Broadcast entry event
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

    UE_LOG(LogTemp, Verbose, TEXT("Actor %s entered zone %s"), 
        *OtherActor->GetName(), *ZoneName);
}

void ARadiantZoneManager::OnZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
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

    UE_LOG(LogTemp, Verbose, TEXT("Actor %s exited zone %s"), 
        *OtherActor->GetName(), *ZoneName);
}

void ARadiantZoneManager::HandlePlayerEntry(AActor* Player)
{
    if (!Player)
    {
        return;
    }

    // Handle discovery
    if (!bIsDiscovered)
    {
        OnPlayerDiscovered(Player);
    }

    // Start playing ambient sound if this is the first player
    if (PlayersInZone.Num() == 1 && AmbientSound)
    {
        PlayAmbientSound();
    }
}

// Zone Information
float ARadiantZoneManager::GetZoneRadius() const
{
    if (ZoneBounds)
    {
        FVector BoxExtent = ZoneBounds->GetScaledBoxExtent();
        return FMath::Max3(BoxExtent.X, BoxExtent.Y, BoxExtent.Z);
    }
    return 5000.0f;
}

bool ARadiantZoneManager::IsLocationInZone(FVector Location) const
{
    if (!ZoneBounds)
    {
        return false;
    }

    FVector LocalLocation = GetActorTransform().InverseTransformPosition(Location);
    FVector BoxExtent = ZoneBounds->GetUnscaledBoxExtent();
    
    return FMath::Abs(LocalLocation.X) <= BoxExtent.X &&
           FMath::Abs(LocalLocation.Y) <= BoxExtent.Y &&
           FMath::Abs(LocalLocation.Z) <= BoxExtent.Z;
}

// Event System
void ARadiantZoneManager::OnEventOccurred(const FWorldEvent& Event)
{
    if (!bIsActive)
    {
        return;
    }

    // Check if event type is blocked
    for (const FGameplayTag& BlockedTag : BlockedEventTypes)
    {
        if (Event.EventTag.MatchesTag(BlockedTag))
        {
            return;
        }
    }

    // Check if event type is allowed (if list is not empty)
    if (AllowedEventTypes.Num() > 0)
    {
        bool bIsAllowed = false;
        for (const FGameplayTag& AllowedTag : AllowedEventTypes)
        {
            if (Event.EventTag.MatchesTag(AllowedTag))
            {
                bIsAllowed = true;
                break;
            }
        }
        
        if (!bIsAllowed)
        {
            return;
        }
    }

    // Apply frequency multiplier
    if (EventFrequencyMultiplier < 1.0f && FMath::FRand() > EventFrequencyMultiplier)
    {
        return;
    }

    // Add to active events
    ActiveZoneEvents.Add(Event);
}

void ARadiantZoneManager::TriggerZoneEvent(FGameplayTag EventTag, AActor* InInstigator)
{
    if (!EventManager || !bIsActive)
    {
        return;
    }

    EventManager->BroadcastZoneEvent(EventTag, ZoneTag, InInstigator);
}

void ARadiantZoneManager::BroadcastZoneAnnouncement(const FString& Message, EEventPriority Priority)
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

// Weather System
void ARadiantZoneManager::SetWeather(EZoneWeather NewWeather)
{
    if (CurrentWeather != NewWeather)
    {
        EZoneWeather OldWeather = CurrentWeather;
        CurrentWeather = NewWeather;

        // Update weather sound
        if (WeatherAudioComponent)
        {
            // Stop current weather sound
            if (WeatherAudioComponent->IsPlaying())
            {
                WeatherAudioComponent->FadeOut(1.0f, 0.0f);
            }

            // Play new weather sound if available
            if (USoundBase** SoundPtr = WeatherSounds.Find(NewWeather))
            {
                if (*SoundPtr)
                {
                    WeatherAudioComponent->SetSound(*SoundPtr);
                    WeatherAudioComponent->FadeIn(1.0f, 1.0f);
                }
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

        UE_LOG(LogTemp, Log, TEXT("Zone %s weather changed to %d"), 
            *ZoneName, (int32)NewWeather);
    }
}

void ARadiantZoneManager::StartWeatherCycle()
{
    if (GetWorld() && !WeatherUpdateTimer.IsValid())
    {
        GetWorld()->GetTimerManager().SetTimer(
            WeatherUpdateTimer,
            this,
            &ARadiantZoneManager::UpdateWeather,
            WeatherUpdateInterval,
            true
        );
    }
}

void ARadiantZoneManager::StopWeatherCycle()
{
    if (GetWorld() && WeatherUpdateTimer.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(WeatherUpdateTimer);
    }
}

// Faction System
void ARadiantZoneManager::SetControllingFaction(FGameplayTag FactionTag)
{
    if (ControllingFaction != FactionTag)
    {
        FGameplayTag OldFaction = ControllingFaction;
        ControllingFaction = FactionTag;
        FactionControlStrength = 1.0f;
        ContestedByFaction = FGameplayTag();

        // Notify Blueprint
        OnFactionControlChanged(FactionTag);

        // Broadcast faction change event
        if (EventManager)
        {
            FWorldEvent Event;
            Event.EventTag = FGameplayTag::RequestGameplayTag("Zone.Faction.ControlChanged");
            Event.Category = EEventCategory::Faction;
            Event.Scope = EEventScope::Zone;
            Event.Priority = EEventPriority::High;
            Event.Location = GetActorLocation();
            Event.Metadata.Add("OldFaction", OldFaction.ToString());
            Event.Metadata.Add("NewFaction", FactionTag.ToString());
            Event.Metadata.Add("ZoneName", ZoneName);

            EventManager->BroadcastEvent(Event);
        }

        UE_LOG(LogTemp, Log, TEXT("Zone %s control changed from %s to %s"),
            *ZoneName, *OldFaction.ToString(), *FactionTag.ToString());
    }
}

void ARadiantZoneManager::StartFactionConflict(FGameplayTag AttackingFaction)
{
    if (AttackingFaction != ControllingFaction && AttackingFaction.IsValid())
    {
        ContestedByFaction = AttackingFaction;
        
        if (EventManager)
        {
            FWorldEvent Event;
            Event.EventTag = FGameplayTag::RequestGameplayTag("Zone.Faction.ConflictStarted");
            Event.Category = EEventCategory::Faction;
            Event.Scope = EEventScope::Regional;
            Event.Priority = EEventPriority::High;
            Event.Location = GetActorLocation();
            Event.Metadata.Add("DefendingFaction", ControllingFaction.ToString());
            Event.Metadata.Add("AttackingFaction", AttackingFaction.ToString());
            Event.Metadata.Add("ZoneName", ZoneName);

            EventManager->BroadcastEvent(Event);
        }
    }
}

// Resource Management
void ARadiantZoneManager::SetResourceAvailability(FGameplayTag ResourceType, float Availability)
{
    ResourceAvailability.Add(ResourceType, FMath::Clamp(Availability, 0.0f, MaxResourceCapacity));
}

float ARadiantZoneManager::GetResourceAvailability(FGameplayTag ResourceType) const
{
    if (const float* Availability = ResourceAvailability.Find(ResourceType))
    {
        return *Availability;
    }
    return 0.0f;
}

void ARadiantZoneManager::ConsumeResource(FGameplayTag ResourceType, float Amount)
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

void ARadiantZoneManager::RegenerateResources()
{
    for (auto& Pair : ResourceAvailability)
    {
        Pair.Value = FMath::Min(MaxResourceCapacity, 
            Pair.Value + (ResourceRegenerationRate * MaxResourceCapacity));
    }
}

// Discovery System
void ARadiantZoneManager::OnPlayerDiscovered(AActor* Player)
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

        UE_LOG(LogTemp, Log, TEXT("Zone %s discovered by %s"), 
            *ZoneName, *Player->GetName());
    }
}

// Spawn Management
void ARadiantZoneManager::SetSpawnRules(const TArray<FGameplayTag>& InAllowedSpawnTypes)
{
    AllowedSpawnTypes = InAllowedSpawnTypes;
}

void ARadiantZoneManager::SpawnZoneNPC(TSubclassOf<AActor> NPCClass, FVector SpawnLocation)
{
    if (!NPCClass || SpawnedNPCs.Num() >= MaxNPCsInZone || !bIsActive)
    {
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    if (AActor* NewNPC = GetWorld()->SpawnActor<AActor>(NPCClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams))
    {
        SpawnedNPCs.Add(NewNPC);
        
        UE_LOG(LogTemp, Log, TEXT("Spawned NPC %s in zone %s"), 
            *NewNPC->GetName(), *ZoneName);
    }
}

void ARadiantZoneManager::DespawnAllNPCs()
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

// Internal Update Functions
void ARadiantZoneManager::UpdateWeather()
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

void ARadiantZoneManager::UpdateResources()
{
    if (!bIsActive)
    {
        return;
    }

    RegenerateResources();
}

void ARadiantZoneManager::ProcessZoneEvents()
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

    // Update ambient sound based on player proximity
    if (PlayersInZone.Num() > 0)
    {
        UpdateAmbientSound();
    }
}

void ARadiantZoneManager::CheckFactionStatus()
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