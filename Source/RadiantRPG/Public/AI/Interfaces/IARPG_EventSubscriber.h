// Source/RadiantRPG/Public/AI/Interfaces/IARPG_EventSubscriber.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTags.h"
#include "IARPG_EventSubscriber.generated.h"

struct FARPG_AIEvent;

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType)
class UARPG_EventSubscriber : public UInterface
{
    GENERATED_BODY()
};

/**
 * Interface for objects that can subscribe to and receive AI events
 * Allows decoupled event-driven communication between AI systems
 */
class RADIANTRPG_API IARPG_EventSubscriber
{
    GENERATED_BODY()

public:
    /** Called when a subscribed event occurs */
    UFUNCTION(BlueprintImplementableEvent, Category = "AI Events")
    void OnEventReceived(FGameplayTag EventType, const FARPG_AIEvent& EventData);

    /** 
     * Get the gameplay tags this subscriber wants to listen to
     * Return empty array to listen to all events
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "AI Events")
    TArray<FGameplayTag> GetSubscribedEventTypes() const;

    /**
     * Get the maximum distance this subscriber cares about events
     * Return -1.0f for no distance limit
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "AI Events")
    float GetEventListeningRadius() const;

    /**
     * Check if this subscriber should receive a specific event
     * Allows for custom filtering logic beyond simple type/distance checks
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "AI Events")
    bool ShouldReceiveEvent(const FARPG_AIEvent& Event) const;

    // C++ versions for direct implementation
    
    /** C++ version of OnEventReceived */
    virtual void ReceiveEvent(FGameplayTag EventType, const FARPG_AIEvent& EventData) {}

    /** C++ version of GetSubscribedEventTypes */
    virtual TArray<FGameplayTag> GetSubscribedEvents() const { return TArray<FGameplayTag>(); }

    /** C++ version of GetEventListeningRadius */
    virtual float GetListeningRadius() const { return -1.0f; }

    /** C++ version of ShouldReceiveEvent */
    virtual bool WantsEvent(const FARPG_AIEvent& Event) const { return true; }

    /** Get the UObject for this interface (used for Blueprint callbacks) */
    virtual UObject* GetSubscriberObject() = 0;
};