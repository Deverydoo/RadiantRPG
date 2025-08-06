// Source/RadiantRPG/Public/World/EventListenerComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Types/EventTypes.h"
#include "Types/RadiantAITypes.h"
#include "EventListenerComponent.generated.h"

class UWorldEventManager;

/**
 * Component that allows actors to listen for and respond to world events
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RADIANTRPG_API UEventListenerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEventListenerComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // Event Reception
    UFUNCTION(BlueprintCallable, Category = "Event Listener")
    void OnEventReceived(const FWorldEvent& Event);

    // Configuration
    UFUNCTION(BlueprintCallable, Category = "Event Listener")
    void SubscribeToEvent(FGameplayTag EventTag, float MaxDistance = 0.0f);

    UFUNCTION(BlueprintCallable, Category = "Event Listener")
    void SubscribeToCategory(EEventCategory Category, float MaxDistance = 0.0f);

    UFUNCTION(BlueprintCallable, Category = "Event Listener")
    void UnsubscribeFromEvent(FGameplayTag EventTag);

    UFUNCTION(BlueprintCallable, Category = "Event Listener")
    void UnsubscribeFromCategory(EEventCategory Category);

    UFUNCTION(BlueprintCallable, Category = "Event Listener")
    void SetRequiresLineOfSight(bool bRequire);

    UFUNCTION(BlueprintCallable, Category = "Event Listener")
    void SetMinimumPriority(EEventPriority MinPriority);

    // Interest Queries
    UFUNCTION(BlueprintPure, Category = "Event Listener")
    bool IsInterestedInEvent(const FWorldEvent& Event) const;

    UFUNCTION(BlueprintPure, Category = "Event Listener")
    float GetCategoryInterestLevel(EEventCategory Category) const;

    UFUNCTION(BlueprintPure, Category = "Event Listener")
    bool RequiresLineOfSight() const { return bRequiresLineOfSight; }

    // Event Memory
    UFUNCTION(BlueprintCallable, Category = "Event Listener|Memory")
    void RememberEvent(const FWorldEvent& Event, float Importance = 1.0f);

    UFUNCTION(BlueprintPure, Category = "Event Listener|Memory")
    TArray<FEventMemory> GetRememberedEvents() const { return RememberedEvents; }

    UFUNCTION(BlueprintPure, Category = "Event Listener|Memory")
    TArray<FEventMemory> GetRecentEvents(float TimeWindow = 30.0f) const;

    UFUNCTION(BlueprintPure, Category = "Event Listener|Memory")
    FEventMemory GetMostRecentEventByTag(FGameplayTag Tag) const;

    UFUNCTION(BlueprintCallable, Category = "Event Listener|Memory")
    void ClearMemory();

    // Stimulus Response (for AI)
    UFUNCTION(BlueprintImplementableEvent, Category = "Event Listener|AI")
    void OnStimulusReceived(const FStimulus& Stimulus);

    // Blueprint Events
    UFUNCTION(BlueprintImplementableEvent, Category = "Event Listener")
    void OnEventReceivedBP(const FWorldEvent& Event);

    UFUNCTION(BlueprintImplementableEvent, Category = "Event Listener")
    void OnHighPriorityEvent(const FWorldEvent& Event);

    // Delegates
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnEventProcessed, const FWorldEvent&);
    FOnEventProcessed OnEventProcessed;

    UPROPERTY(BlueprintAssignable, Category = "Event Listener")
    FOnWorldEventBP OnEventProcessedBP;

protected:
    // Event Processing
    void ProcessReceivedEvent(const FWorldEvent& Event);
    void UpdateMemory();
    bool ShouldRememberEvent(const FWorldEvent& Event) const;
    float CalculateEventImportance(const FWorldEvent& Event) const;

    // Stimulus Conversion
    void ConvertEventToStimulus(const FWorldEvent& Event);

private:
    // Subscriptions
    UPROPERTY(EditAnywhere, Category = "Event Listener")
    TArray<FEventSubscription> Subscriptions;

    // Category Interest Levels (0-1)
    UPROPERTY(EditAnywhere, Category = "Event Listener", meta = (ClampMin = 0.0, ClampMax = 1.0))
    TMap<EEventCategory, float> CategoryInterestLevels;

    // Configuration
    UPROPERTY(EditAnywhere, Category = "Event Listener")
    bool bRequiresLineOfSight = false;

    UPROPERTY(EditAnywhere, Category = "Event Listener")
    EEventPriority MinimumPriority = EEventPriority::Low;

    UPROPERTY(EditAnywhere, Category = "Event Listener")
    bool bAutoSubscribeToAllCategories = false;

    UPROPERTY(EditAnywhere, Category = "Event Listener")
    float DefaultListenRadius = 3000.0f;

    // Memory
    UPROPERTY(VisibleAnywhere, Category = "Event Listener|Memory")
    TArray<FEventMemory> RememberedEvents;

    UPROPERTY(EditAnywhere, Category = "Event Listener|Memory")
    int32 MaxMemorySize = 50;

    UPROPERTY(EditAnywhere, Category = "Event Listener|Memory")
    float MemoryDuration = 300.0f; // 5 minutes

    UPROPERTY(EditAnywhere, Category = "Event Listener|Memory")
    float MinimumImportanceToRemember = 0.5f;

    // AI Configuration
    UPROPERTY(EditAnywhere, Category = "Event Listener|AI")
    bool bConvertEventsToStimuli = false;

    UPROPERTY(EditAnywhere, Category = "Event Listener|AI")
    TMap<EEventCategory, EAIStimulusType> CategoryToStimulusMapping;

    // Internal
    UPROPERTY()
    UWorldEventManager* EventManager = nullptr;

    FTimerHandle MemoryUpdateTimer;
};