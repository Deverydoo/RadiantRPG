// Source/RadiantRPG/Public/Types/ARPG_AIEventTypes.h

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "ARPG_AIEventTypes.generated.h"

class AActor;

/**
 * Memory relevance/importance levels
 */
UENUM(BlueprintType)
enum class EARPG_MemoryRelevance : uint8
{
    Trivial         UMETA(DisplayName = "Trivial"),
    Low             UMETA(DisplayName = "Low"),
    Medium          UMETA(DisplayName = "Medium"),
    High            UMETA(DisplayName = "High"),
    Critical        UMETA(DisplayName = "Critical"),
    MAX             UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EARPG_MemoryType : uint8
{
    // Existing types...
    Any              UMETA(DisplayName = "Any"),
    Event            UMETA(DisplayName = "Event"),
    Entity           UMETA(DisplayName = "Entity"),
    Location         UMETA(DisplayName = "Location"),
    
    // === EXPANDED MEMORY CATEGORIES ===
    
    // Combat & Conflict
    Combat           UMETA(DisplayName = "Combat Memory"),
    Threat           UMETA(DisplayName = "Threat Memory"),
    Victory          UMETA(DisplayName = "Victory Memory"),
    Defeat           UMETA(DisplayName = "Defeat Memory"),
    Death            UMETA(DisplayName = "Death"),
    
    // Social & Emotional
    Social           UMETA(DisplayName = "Social Memory"),
    Emotion          UMETA(DisplayName = "Emotional Memory"),
    Trust            UMETA(DisplayName = "Trust Memory"),
    Betrayal         UMETA(DisplayName = "Betrayal Memory"),
    Honor            UMETA(DisplayName = "Honor Memory"),
    
    // Knowledge & Learning
    Academic         UMETA(DisplayName = "Academic Memory"),
    Magic            UMETA(DisplayName = "Magical Memory"),
    Craft            UMETA(DisplayName = "Crafting Memory"),
    Knowledge        UMETA(DisplayName = "Knowledge Memory"),
    Research         UMETA(DisplayName = "Research Memory"),
    Technology       UMETA(DisplayName = "Technology Memory"),
    
    // Survival & Nature
    Survival         UMETA(DisplayName = "Survival Memory"),
    Food             UMETA(DisplayName = "Food Memory"),
    Territory        UMETA(DisplayName = "Territory Memory"),
    Hunt             UMETA(DisplayName = "Hunting Memory"),
    Nature           UMETA(DisplayName = "Nature Memory"),
    Weather          UMETA(DisplayName = "Weather Memory"),
    
    // Cultural & Historical
    Ancient          UMETA(DisplayName = "Ancient Memory"),
    Clan             UMETA(DisplayName = "Clan Memory"),
    Tribe            UMETA(DisplayName = "Tribal Memory"),
    Culture          UMETA(DisplayName = "Cultural Memory"),
    Tradition        UMETA(DisplayName = "Tradition Memory"),
    
    // Material & Possessions
    Treasure         UMETA(DisplayName = "Treasure Memory"),
    Possession       UMETA(DisplayName = "Possession Memory"),
    Trade            UMETA(DisplayName = "Trade Memory"),
    
    // Supernatural & Divine
    Divine           UMETA(DisplayName = "Divine Memory"),
    Corruption       UMETA(DisplayName = "Corruption Memory"),
    Pure             UMETA(DisplayName = "Pure Memory"),
    Aberrant         UMETA(DisplayName = "Aberrant Memory"),
    Psychic          UMETA(DisplayName = "Psychic Memory"),
    
    // Behavioral Patterns
    Command          UMETA(DisplayName = "Command Memory"),
    Task             UMETA(DisplayName = "Task Memory"),
    Guardian         UMETA(DisplayName = "Guardian Memory"),
    Deception        UMETA(DisplayName = "Deception Memory"),
    Adaptation       UMETA(DisplayName = "Adaptation Memory"),
    
    // Environmental
    Water            UMETA(DisplayName = "Water Memory"),
    Fire             UMETA(DisplayName = "Fire Memory"),
    Element          UMETA(DisplayName = "Elemental Memory"),
    Season           UMETA(DisplayName = "Seasonal Memory"),
    
    // Social Groups
    Pack             UMETA(DisplayName = "Pack Memory"),
    Herd             UMETA(DisplayName = "Herd Memory"),
    Family           UMETA(DisplayName = "Family Memory"),
    
    // Abstract Concepts
    Dominance        UMETA(DisplayName = "Dominance Memory"),
    Submission       UMETA(DisplayName = "Submission Memory"),
    Justice          UMETA(DisplayName = "Justice Memory"),
    Temptation       UMETA(DisplayName = "Temptation Memory"),
    Obsession        UMETA(DisplayName = "Obsession Memory"),
    
    // Biological & Physical
    Physical         UMETA(DisplayName = "Physical Memory"),
    Blood            UMETA(DisplayName = "Blood Memory"),
    Growth           UMETA(DisplayName = "Growth Memory"),
    
    // Special Categories
    Trivial          UMETA(DisplayName = "Trivial Memory"),
    Mundane          UMETA(DisplayName = "Mundane Memory"),
    Normal           UMETA(DisplayName = "Normal Memory"),
    Cosmic           UMETA(DisplayName = "Cosmic Memory"),
    Arcane           UMETA(DisplayName = "Arcane Memory"),
    
    // Web/Predator specific
    Web              UMETA(DisplayName = "Web Memory"),
    Prey             UMETA(DisplayName = "Prey Memory"),
    
    // Environmental hazards
    Sunlight         UMETA(DisplayName = "Sunlight Memory"),
    Desert           UMETA(DisplayName = "Desert Memory"),
    
    // Tactics
    Tactics          UMETA(DisplayName = "Tactical Memory"),
    
    MAX              UMETA(Hidden)
};

/**
 * AI Event - specialized event for AI brain processing
 * Extends basic world events with AI-specific data
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_AIEvent
{
    GENERATED_BODY()

    /** Unique identifier for this event */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    FGuid EventID;

    /** Type/tag of this event */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    FGameplayTag EventType;

    /** Location where event occurred */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    FVector EventLocation = FVector::ZeroVector;

    /** Actor that initiated/caused this event */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    TWeakObjectPtr<AActor> EventInstigator;

    /** Target actor (if applicable) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    TWeakObjectPtr<AActor> EventTarget;

    /** Strength/importance of this event (0.0 - 1.0) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EventStrength = 0.5f;

    /** Radius of event influence */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (ClampMin = "0.0"))
    float EventRadius = 1000.0f;

    /** Whether this is a global event (affects all AI regardless of distance) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    bool bGlobal = false;

    /** When this event occurred */
    UPROPERTY(BlueprintReadOnly, Category = "Event")
    float Timestamp = 0.0f;

    /** How long this event persists (0 = instant) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (ClampMin = "0.0"))
    float Duration = 0.0f;

    /** Additional event-specific data */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    TMap<FString, FString> EventData;
    double EventTime;

    FARPG_AIEvent()
    {
        EventID = FGuid::NewGuid();
        EventType = FGameplayTag::EmptyTag;
        EventLocation = FVector::ZeroVector;
        EventStrength = 0.5f;
        EventRadius = 1000.0f;
        bGlobal = false;
        Timestamp = 0.0f;
        Duration = 0.0f;
    }

    /** Check if event is still active based on duration */
    bool IsActive(float CurrentTime) const
    {
        if (Duration <= 0.0f)
        {
            return true; // Instant events are always "active" for queries
        }
        return (CurrentTime - Timestamp) <= Duration;
    }

    /** Get event age in seconds */
    float GetAge(float CurrentTime) const
    {
        return CurrentTime - Timestamp;
    }

    /** Check if event affects a specific location */
    bool AffectsLocation(FVector Location, float MaxDistance = -1.0f) const
    {
        if (bGlobal)
        {
            return true;
        }
        
        float CheckRadius = MaxDistance > 0.0f ? FMath::Min(EventRadius, MaxDistance) : EventRadius;
        if (CheckRadius <= 0.0f)
        {
            return true; // No radius limit
        }
        
        return FVector::Dist(EventLocation, Location) <= CheckRadius;
    }
};

/**
 * Memory entry for AI memory system
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_MemoryEntry
{
    GENERATED_BODY()

    /** Type of memory */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
    EARPG_MemoryType MemoryType = EARPG_MemoryType::Event;

    /** Gameplay tag describing this memory */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
    FGameplayTag MemoryTag;

    /** How important/relevant this memory is */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
    EARPG_MemoryRelevance Relevance = EARPG_MemoryRelevance::Medium;

    /** Current memory strength (0.0 - 1.0) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Strength = 1.0f;

    /** How quickly this memory decays over time */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory", meta = (ClampMin = "0.0"))
    float DecayRate = 0.1f;

    /** Emotional weight associated with this memory (-1.0 to 1.0) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float EmotionalWeight = 0.0f;

    /** Location associated with this memory */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
    FVector Location = FVector::ZeroVector;

    /** Primary actor associated with this memory */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
    TWeakObjectPtr<AActor> AssociatedActor;

    /** Secondary actor (e.g., target of an event) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
    TWeakObjectPtr<AActor> SecondaryActor;

    /** Human-readable description of this memory */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
    FString Description;

    /** Additional memory-specific data */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
    TMap<FString, FString> MemoryData;

    /** When this memory was created */
    UPROPERTY(BlueprintReadOnly, Category = "Memory")
    float CreationTime = 0.0f;

    /** When this memory was last accessed/recalled */
    UPROPERTY(BlueprintReadOnly, Category = "Memory")
    float LastAccessTime = 0.0f;

    /** How many times this memory has been accessed */
    UPROPERTY(BlueprintReadOnly, Category = "Memory")
    int32 AccessCount = 0;

    /** Whether this memory is vivid (decays slower) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
    bool bIsVivid = false;

    /** Whether this memory is permanent (never decays) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
    bool bIsPermanent = false;

    FARPG_MemoryEntry()
    {
        MemoryType = EARPG_MemoryType::Event;
        MemoryTag = FGameplayTag::EmptyTag;
        Relevance = EARPG_MemoryRelevance::Medium;
        Strength = 1.0f;
        DecayRate = 0.1f;
        EmotionalWeight = 0.0f;
        Location = FVector::ZeroVector;
        CreationTime = 0.0f;
        LastAccessTime = 0.0f;
        AccessCount = 0;
        bIsVivid = false;
        bIsPermanent = false;
    }

    /** Get current memory strength accounting for decay */
    float GetCurrentStrength(float CurrentTime) const;

    /** Check if memory should be forgotten */
    bool ShouldForget(float CurrentTime, float ForgetThreshold = 0.1f) const;

    /** Access this memory (updates access time and count, slightly reinforces) */
    void AccessMemory(float CurrentTime);

    /** Get relevance as a float value for sorting */
    float GetRelevanceFloat() const;
};

/**
 * Memory query parameters
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_MemoryQuery
{
    GENERATED_BODY()

    /** Type of memories to search for (MAX = all types) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
    EARPG_MemoryType MemoryType = EARPG_MemoryType::MAX;

    /** Specific tag to match */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
    FGameplayTag RequiredTag;

    /** Minimum relevance level */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
    EARPG_MemoryRelevance MinRelevance = EARPG_MemoryRelevance::Trivial;

    /** Search within this time window (0 = no time limit) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query", meta = (ClampMin = "0.0"))
    float TimeWindow = 0.0f;

    /** Search near this location */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
    FVector SearchLocation = FVector::ZeroVector;

    /** Search radius around location (0 = no location filtering) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query", meta = (ClampMin = "0.0"))
    float SearchRadius = 0.0f;

    /** Require memories involving this actor */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
    bool bRequireActor = false;

    /** The required actor */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
    TWeakObjectPtr<AActor> RequiredActor;

    /** Maximum number of results to return (0 = unlimited) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query", meta = (ClampMin = "0"))
    int32 MaxResults = 0;

    /** Sort results by relevance (highest first) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
    bool bSortByRelevance = true;

    /** Sort results by time (most recent first) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
    bool bSortByTime = false;

    /** Whether accessing memories affects their strength */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
    bool bAccessMemories = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
    float MinStrength;

    FARPG_MemoryQuery()
    {
        MemoryType = EARPG_MemoryType::MAX;
        RequiredTag = FGameplayTag::EmptyTag;
        MinRelevance = EARPG_MemoryRelevance::Trivial;
        TimeWindow = 0.0f;
        SearchLocation = FVector::ZeroVector;
        SearchRadius = 0.0f;
        bRequireActor = false;
        MaxResults = 0;
        bSortByRelevance = true;
        bSortByTime = false;
        bAccessMemories = true;
    }
};