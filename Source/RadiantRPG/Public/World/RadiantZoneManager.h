// Source/RadiantRPG/Public/World/RadiantZone.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Types/EventTypes.h"
#include "Types/WorldTypes.h"
#include "RadiantZoneManager.generated.h"

class UBoxComponent;
class USphereComponent;
class UAudioComponent;
class UWorldEventManager;

/**
 * Represents a zone in the world with its own rules and events
 */
UCLASS()
class RADIANTRPG_API ARadiantZoneManager : public AActor
{
    GENERATED_BODY()

public:
    ARadiantZoneManager();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

public:
    // Zone Information
    UFUNCTION(BlueprintPure, Category = "Zone")
    FGameplayTag GetZoneTag() const { return ZoneTag; }

    UFUNCTION(BlueprintPure, Category = "Zone")
    EZoneType GetZoneType() const { return ZoneType; }

    UFUNCTION(BlueprintPure, Category = "Zone")
    float GetZoneRadius() const;

    UFUNCTION(BlueprintPure, Category = "Zone")
    bool IsLocationInZone(FVector Location) const;

    UFUNCTION(BlueprintPure, Category = "Zone")
    TArray<AActor*> GetActorsInZone() const { return ActorsInZone; }

    // Zone Events
    UFUNCTION(BlueprintCallable, Category = "Zone|Events")
    void OnEventOccurred(const FWorldEvent& Event);

    UFUNCTION(BlueprintCallable, Category = "Zone|Events")
    void TriggerZoneEvent(FGameplayTag EventTag, AActor* InInstigator = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Zone|Events")
    void BroadcastZoneAnnouncement(const FString& Message, EEventPriority Priority = EEventPriority::Normal);

    // Weather System
    UFUNCTION(BlueprintCallable, Category = "Zone|Weather")
    void SetWeather(EZoneWeather NewWeather);

    UFUNCTION(BlueprintPure, Category = "Zone|Weather")
    EZoneWeather GetCurrentWeather() const { return CurrentWeather; }

    UFUNCTION(BlueprintCallable, Category = "Zone|Weather")
    void StartWeatherCycle();

    UFUNCTION(BlueprintCallable, Category = "Zone|Weather")
    void StopWeatherCycle();

    // Faction Control
    UFUNCTION(BlueprintCallable, Category = "Zone|Faction")
    void SetControllingFaction(FGameplayTag FactionTag);

    UFUNCTION(BlueprintPure, Category = "Zone|Faction")
    FGameplayTag GetControllingFaction() const { return ControllingFaction; }

    UFUNCTION(BlueprintCallable, Category = "Zone|Faction")
    void StartFactionConflict(FGameplayTag AttackingFaction);

    // Resource Management
    UFUNCTION(BlueprintCallable, Category = "Zone|Resources")
    void SetResourceAvailability(FGameplayTag ResourceType, float Availability);

    UFUNCTION(BlueprintPure, Category = "Zone|Resources")
    float GetResourceAvailability(FGameplayTag ResourceType) const;

    UFUNCTION(BlueprintCallable, Category = "Zone|Resources")
    void ConsumeResource(FGameplayTag ResourceType, float Amount);

    UFUNCTION(BlueprintCallable, Category = "Zone|Resources")
    void RegenerateResources();

    // Discovery System
    UFUNCTION(BlueprintCallable, Category = "Zone|Discovery")
    void OnPlayerDiscovered(AActor* Player);

    UFUNCTION(BlueprintPure, Category = "Zone|Discovery")
    bool IsDiscovered() const { return bIsDiscovered; }

    // Spawn Management
    UFUNCTION(BlueprintCallable, Category = "Zone|Spawning")
    void SetSpawnRules(const TArray<FGameplayTag>& AllowedSpawnTypes);

    UFUNCTION(BlueprintCallable, Category = "Zone|Spawning")
    void SpawnZoneNPC(TSubclassOf<AActor> NPCClass, FVector SpawnLocation);

    UFUNCTION(BlueprintCallable, Category = "Zone|Spawning")
    void DespawnAllNPCs();

    // Zone State
    UFUNCTION(BlueprintCallable, Category = "Zone")
    void ActivateZone();

    UFUNCTION(BlueprintCallable, Category = "Zone")
    void DeactivateZone();

    UFUNCTION(BlueprintPure, Category = "Zone")
    bool IsZoneActive() const { return bIsActive; }

    // Audio Management
    UFUNCTION(BlueprintCallable, Category = "Zone|Audio")
    void PlayAmbientSound();

    UFUNCTION(BlueprintCallable, Category = "Zone|Audio")
    void StopAmbientSound();

    UFUNCTION(BlueprintCallable, Category = "Zone|Audio")
    void UpdateAmbientSound();

    // Blueprint Events
    UFUNCTION(BlueprintImplementableEvent, Category = "Zone")
    void OnZoneEntered(AActor* EnteredActor);

    UFUNCTION(BlueprintImplementableEvent, Category = "Zone")
    void OnZoneExited(AActor* ExitedActor);

    UFUNCTION(BlueprintImplementableEvent, Category = "Zone")
    void OnWeatherChanged(EZoneWeather NewWeather);

    UFUNCTION(BlueprintImplementableEvent, Category = "Zone")
    void OnFactionControlChanged(FGameplayTag NewFaction);

protected:
    // Overlap handlers
    UFUNCTION()
    void OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    // Internal update functions
    void UpdateWeather();
    void UpdateResources();
    void ProcessZoneEvents();
    void CheckFactionStatus();
    void HandlePlayerEntry(AActor* Player);

private:
    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    UBoxComponent* ZoneBounds;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    USphereComponent* ZoneInfluence;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    UAudioComponent* AmbientAudioComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    UAudioComponent* WeatherAudioComponent;

    // Zone Configuration
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Config", meta = (AllowPrivateAccess = "true"))
    FString ZoneName = "Unnamed Zone";

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Config", meta = (AllowPrivateAccess = "true"))
    FGameplayTag ZoneTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Config", meta = (AllowPrivateAccess = "true"))
    EZoneType ZoneType = EZoneType::Wilderness;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Config", meta = (AllowPrivateAccess = "true"))
    EZoneDanger DangerLevel = EZoneDanger::Low;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Config", meta = (AllowPrivateAccess = "true"))
    bool bIsActive = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Config", meta = (AllowPrivateAccess = "true"))
    bool bAutoActivateOnBeginPlay = true;

    // Zone State
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone State", meta = (AllowPrivateAccess = "true"))
    bool bIsDiscovered = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone State", meta = (AllowPrivateAccess = "true"))
    TArray<AActor*> ActorsInZone;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone State", meta = (AllowPrivateAccess = "true"))
    TArray<AActor*> PlayersInZone;

    // Weather
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Weather", meta = (AllowPrivateAccess = "true"))
    EZoneWeather CurrentWeather = EZoneWeather::Clear;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Weather", meta = (AllowPrivateAccess = "true"))
    TArray<EZoneWeather> PossibleWeatherTypes;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Weather", meta = (AllowPrivateAccess = "true"))
    float WeatherUpdateInterval = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Weather", meta = (AllowPrivateAccess = "true"))
    float WeatherChangeProbability = 0.3f;

    // Faction
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Faction", meta = (AllowPrivateAccess = "true"))
    FGameplayTag ControllingFaction;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone Faction", meta = (AllowPrivateAccess = "true"))
    FGameplayTag ContestedByFaction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Faction", meta = (AllowPrivateAccess = "true"))
    float FactionControlStrength = 1.0f;

    // Resources
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone Resources", meta = (AllowPrivateAccess = "true"))
    TMap<FGameplayTag, float> ResourceAvailability;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Resources", meta = (AllowPrivateAccess = "true"))
    float ResourceRegenerationRate = 0.01f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Resources", meta = (AllowPrivateAccess = "true"))
    float MaxResourceCapacity = 100.0f;

    // Events
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Events", meta = (AllowPrivateAccess = "true"))
    TArray<FGameplayTag> AllowedEventTypes;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Events", meta = (AllowPrivateAccess = "true"))
    TArray<FGameplayTag> BlockedEventTypes;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Events", meta = (AllowPrivateAccess = "true"))
    float EventFrequencyMultiplier = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone Events", meta = (AllowPrivateAccess = "true"))
    TArray<FWorldEvent> ActiveZoneEvents;

    // Spawning
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Spawning", meta = (AllowPrivateAccess = "true"))
    TArray<FGameplayTag> AllowedSpawnTypes;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Spawning", meta = (AllowPrivateAccess = "true"))
    int32 MaxNPCsInZone = 20;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone Spawning", meta = (AllowPrivateAccess = "true"))
    TArray<AActor*> SpawnedNPCs;

    // Audio
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Audio", meta = (AllowPrivateAccess = "true"))
    USoundBase* DiscoverySound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Audio", meta = (AllowPrivateAccess = "true"))
    USoundBase* AmbientSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Audio", meta = (AllowPrivateAccess = "true"))
    float AmbientSoundVolume = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Audio", meta = (AllowPrivateAccess = "true"))
    float AmbientSoundFadeInTime = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Audio", meta = (AllowPrivateAccess = "true"))
    float AmbientSoundFadeOutTime = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Audio", meta = (AllowPrivateAccess = "true"))
    TMap<EZoneWeather, USoundBase*> WeatherSounds;

    // System References
    UPROPERTY()
    UWorldEventManager* EventManager = nullptr;

    // Timers
    FTimerHandle WeatherUpdateTimer;
    FTimerHandle ResourceUpdateTimer;
    FTimerHandle EventProcessTimer;
};