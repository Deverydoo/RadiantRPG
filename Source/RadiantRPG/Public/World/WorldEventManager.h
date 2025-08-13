// Source/RadiantRPG/Public/World/WorldEventManager.h

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "Types/EventTypes.h"
#include "WorldEventManager.generated.h"

class UEventListenerComponent;
class ARadiantZoneManager;

/**
 * Core event manager subsystem - the heartbeat of the world
 */
UCLASS()
class RADIANTRPG_API UWorldEventManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // Subsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    // Event Broadcasting
    UFUNCTION(BlueprintCallable, Category = "Event System")
    void BroadcastEvent(const FWorldEvent& Event);

    UFUNCTION(BlueprintCallable, Category = "Event System")
    void BroadcastEventAtLocation(FGameplayTag EventTag, FVector Location, float Radius, 
        AActor* Instigator = nullptr, EEventPriority Priority = EEventPriority::Normal);

    UFUNCTION(BlueprintCallable, Category = "Event System")
    void BroadcastGlobalEvent(FGameplayTag EventTag, AActor* Instigator = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Event System")
    void BroadcastZoneEvent(FGameplayTag EventTag, FGameplayTag ZoneTag, AActor* Instigator = nullptr);

    // Stimulus Broadcasting (for AI)
    UFUNCTION(BlueprintCallable, Category = "Event System|AI")
    void BroadcastStimulus(const FStimulus& Stimulus);

    UFUNCTION(BlueprintCallable, Category = "Event System|AI")
    void CreateVisualStimulus(AActor* Source, float Intensity = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "Event System|AI")
    void CreateAuditoryStimulus(AActor* Source, float Intensity = 1.0f, float Radius = 2000.0f);

    // Listener Registration
    void RegisterListener(UEventListenerComponent* Listener);
    void UnregisterListener(UEventListenerComponent* Listener);

    // Zone Management
    UFUNCTION(BlueprintCallable, Category = "Event System|Zones")
    void RegisterZone(ARadiantZoneManager* Zone);

    UFUNCTION(BlueprintCallable, Category = "Event System|Zones")
    void UnregisterZone(ARadiantZoneManager* Zone);

    UFUNCTION(BlueprintCallable, Category = "Event System|Zones")
    ARadiantZoneManager* GetZoneAtLocation(FVector Location) const;

    // Event Queries
    UFUNCTION(BlueprintCallable, Category = "Event System|Query")
    TArray<FWorldEvent> GetActiveEventsInRadius(FVector Location, float Radius) const;

    UFUNCTION(BlueprintCallable, Category = "Event System|Query")
    TArray<FWorldEvent> GetActiveEventsByCategory(EEventCategory Category) const;

    UFUNCTION(BlueprintCallable, Category = "Event System|Query")
    TArray<FWorldEvent> GetActiveEventsByTag(FGameplayTag Tag) const;

    UFUNCTION(BlueprintCallable, Category = "Event System|Query")
    bool IsEventActiveAtLocation(FGameplayTag EventTag, FVector Location, float CheckRadius = 500.0f) const;

    // Event History
    UFUNCTION(BlueprintCallable, Category = "Event System|History")
    TArray<FEventMemory> GetEventHistory(float TimeWindow = 60.0f) const;

    UFUNCTION(BlueprintCallable, Category = "Event System|History")
    TArray<FEventMemory> GetEventHistoryForActor(AActor* Actor, float TimeWindow = 60.0f) const;

    // Delegates
    FOnWorldEvent OnEventBroadcast;
    FOnStimulusReceived OnStimulusCreated;

    UPROPERTY(BlueprintAssignable, Category = "Event System")
    FOnWorldEventBP OnEventBroadcastBP;

    UPROPERTY(BlueprintAssignable, Category = "Event System")
    FOnStimulusReceivedBP OnStimulusCreatedBP;

protected:
    // Event Processing
    void ProcessEvent(const FWorldEvent& Event);
    void NotifyListenersInRange(const FWorldEvent& Event);
    void NotifyZones(const FWorldEvent& Event);
    void UpdateActiveEvents();
    void CleanupExpiredEvents();

    // Helper functions
    bool ShouldListenerReceiveEvent(UEventListenerComponent* Listener, const FWorldEvent& Event) const;
    float CalculateEventRelevance(UEventListenerComponent* Listener, const FWorldEvent& Event) const;
    bool HasLineOfSight(FVector From, FVector To) const;

private:
    // Active listeners
    UPROPERTY()
    TArray<TWeakObjectPtr<UEventListenerComponent>> RegisteredListeners;

    // Active zones
    UPROPERTY()
    TArray<TWeakObjectPtr<ARadiantZoneManager>> RegisteredZones;

    // Currently active events
    UPROPERTY()
    TArray<FWorldEvent> ActiveEvents;

    // Event history for memory systems
    UPROPERTY()
    TArray<FEventMemory> EventHistory;

    // Configuration
    UPROPERTY()
    float EventHistoryDuration = 300.0f; // 5 minutes

    UPROPERTY()
    int32 MaxActiveEvents = 100;

    UPROPERTY()
    int32 MaxHistoryEntries = 500;

    // Update timer
    FTimerHandle UpdateTimerHandle;

    UPROPERTY()
    float UpdateInterval = 0.5f;
};