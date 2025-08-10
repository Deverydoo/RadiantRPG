// Source/RadiantRPG/Public/Types/EventTypes.h

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "EventTypes.generated.h"

/**
 * Event categories for world simulation
 */
UENUM(BlueprintType)
enum class EEventCategory : uint8
{
    System      UMETA(DisplayName = "System"),
    Combat      UMETA(DisplayName = "Combat"),
    Social      UMETA(DisplayName = "Social"),
    Economic    UMETA(DisplayName = "Economic"),
    Environmental UMETA(DisplayName = "Environmental"),
    Faction     UMETA(DisplayName = "Faction"),
    Discovery   UMETA(DisplayName = "Discovery"),
    Quest		UMETA(DisplayName = "Quest"),
    AI          UMETA(DisplayName = "AI"),
    Resource    UMETA(DisplayName = "Resource"),
    Weather     UMETA(DisplayName = "Weather"),
    Time        UMETA(DisplayName = "Time")
};

/**
 * Event scope - determines how far an event propagates
 */
UENUM(BlueprintType)
enum class EEventScope : uint8
{
    Local       UMETA(DisplayName = "Local"),        // Within immediate area
    Zone        UMETA(DisplayName = "Zone"),         // Entire zone/region
    Global      UMETA(DisplayName = "Global"),       // Entire world
    Faction     UMETA(DisplayName = "Faction"),      // All faction members
    Regional    UMETA(DisplayName = "Regional"),      // World region.
    Network     UMETA(DisplayName = "Network")       // Connected entities
};

/**
 * Event priority for processing order
 */
UENUM(BlueprintType)
enum class EEventPriority : uint8
{
    Low         UMETA(DisplayName = "Low"),
    Normal      UMETA(DisplayName = "Normal"),
    High        UMETA(DisplayName = "High"),
    Critical    UMETA(DisplayName = "Critical")
};

/**
 * Base event data structure
 */
USTRUCT(BlueprintType)
struct FWorldEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FGameplayTag EventTag;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    EEventCategory Category = EEventCategory::System;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    EEventScope Scope = EEventScope::Local;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    EEventPriority Priority = EEventPriority::Normal;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FVector Location = FVector::ZeroVector;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float Radius = 1000.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    AActor* Instigator = nullptr;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    AActor* Target = nullptr;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TMap<FString, FString> Metadata;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float Timestamp = 0.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float Duration = 0.0f;  // How long the event persists

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float Intensity = 1.0f;  // Event strength/importance

    FWorldEvent()
    {
        Timestamp = 0.0f;
    }
};


/**
 * Event subscription info
 */
USTRUCT(BlueprintType)
struct FEventSubscription
{
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag EventTag;

    UPROPERTY()
    EEventCategory Category = EEventCategory::System;

    UPROPERTY()
    float MaxDistance = 0.0f;  // 0 = unlimited

    UPROPERTY()
    bool bRequiresLineOfSight = false;

    UPROPERTY()
    EEventPriority MinPriority = EEventPriority::Low;
};

/**
 * Zone event configuration
 */
USTRUCT(BlueprintType)
struct FZoneEventConfig : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag ZoneTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FGameplayTag> AllowedEvents;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FGameplayTag> BlockedEvents;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float EventFrequencyMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<EEventCategory, float> CategoryWeights;
};

/**
 * Event history entry for memory systems
 */
USTRUCT(BlueprintType)
struct FEventMemory
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FWorldEvent Event;

    UPROPERTY(BlueprintReadWrite)
    float RecordedTime = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    float Importance = 1.0f;

    UPROPERTY(BlueprintReadWrite)
    bool bWasParticipant = false;

    UPROPERTY(BlueprintReadWrite)
    bool bWasWitness = false;
};

/**
 * Delegate signatures for event callbacks
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnWorldEvent, const FWorldEvent&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStimulusReceived, AActor*, const FStimulus&);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldEventBP, const FWorldEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStimulusReceivedBP, AActor*, Actor, const FStimulus&, Stimulus);