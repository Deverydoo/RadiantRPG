// Source/RadiantRPG/Public/Types/ARPG_AITypes.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTags.h"
#include "UObject/NoExportTypes.h"
#include "ARPG_AITypes.generated.h"

class AActor;

/**
 * AI Intent priority levels
 */
UENUM(BlueprintType)
enum class EARPG_AIIntentPriority : uint8
{
    Idle = 0        UMETA(DisplayName = "Idle"),
    Low = 1         UMETA(DisplayName = "Low"),
    Medium = 2      UMETA(DisplayName = "Medium"),
    High = 3        UMETA(DisplayName = "High"),
    Critical = 4    UMETA(DisplayName = "Critical"),
    MAX             UMETA(Hidden)
};

/**
 * AI Brain processing states
 */
UENUM(BlueprintType)
enum class EARPG_BrainState : uint8
{
    Inactive        UMETA(DisplayName = "Inactive"),
    Idle            UMETA(DisplayName = "Idle"),
    Processing      UMETA(DisplayName = "Processing"),
    Deciding        UMETA(DisplayName = "Deciding"),
    Executing       UMETA(DisplayName = "Executing"),
    Curiosity       UMETA(DisplayName = "Curiosity"),
    Error           UMETA(DisplayName = "Error"),
    MAX             UMETA(Hidden)
};

/**
 * Types of AI stimuli
 */
UENUM(BlueprintType)
enum class EARPG_StimulusType : uint8
{
    Visual          UMETA(DisplayName = "Visual"),
    Audio           UMETA(DisplayName = "Audio"),
    Touch           UMETA(DisplayName = "Touch"),
    Memory          UMETA(DisplayName = "Memory"),
    WorldEvent      UMETA(DisplayName = "World Event"),
    Internal        UMETA(DisplayName = "Internal"),
    MAX             UMETA(Hidden)
};

/**
 * Basic needs tracked by AI
 */
UENUM(BlueprintType)
enum class EARPG_NeedType : uint8
{
    Hunger          UMETA(DisplayName = "Hunger"),
    Fatigue         UMETA(DisplayName = "Fatigue"),
    Safety          UMETA(DisplayName = "Safety"),
    Social          UMETA(DisplayName = "Social"),
    Curiosity       UMETA(DisplayName = "Curiosity"),
    Comfort         UMETA(DisplayName = "Comfort"),
    MAX             UMETA(Hidden)
};

/**
 * AI Personality traits
 */
UENUM(BlueprintType)
enum class EARPG_PersonalityTrait : uint8
{
    None            UMETA(DisplayName = "None"),
    Aggression      UMETA(DisplayName = "Aggression"),
    Caution         UMETA(DisplayName = "Caution"),
    Curiosity       UMETA(DisplayName = "Curiosity"),
    Sociability     UMETA(DisplayName = "Sociability"),
    Loyalty         UMETA(DisplayName = "Loyalty"),
    Greed           UMETA(DisplayName = "Greed"),
    Courage         UMETA(DisplayName = "Courage"),
    Intelligence    UMETA(DisplayName = "Intelligence"),
    Creativity      UMETA(DisplayName = "Creativity"),
    Patience        UMETA(DisplayName = "Patience"),
    Bravery         UMETA(DisplayName = "Bravery"),
    Impulsiveness   UMETA(DisplayName = "Impulsiveness"),
    MAX             UMETA(Hidden)
};

/**
 * Individual AI stimulus
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_AIStimulus
{
    GENERATED_BODY()

    /** Type of stimulus */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stimulus")
    EARPG_StimulusType StimulusType = EARPG_StimulusType::Visual;

    /** Gameplay tag identifying this stimulus */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stimulus")
    FGameplayTag StimulusTag;

    /** Strength/intensity of the stimulus */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stimulus", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Intensity = 0.5f;

    /** Location where stimulus occurred */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stimulus")
    FVector Location = FVector::ZeroVector;

    /** Actor that caused this stimulus */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stimulus")
    TWeakObjectPtr<AActor> SourceActor;

    /** Additional data about the stimulus */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stimulus")
    TMap<FString, FString> StimulusData;

    /** Timestamp when stimulus was created */
    UPROPERTY(BlueprintReadOnly, Category = "Stimulus")
    float Timestamp = 0.0f;

    FARPG_AIStimulus()
    {
        StimulusType = EARPG_StimulusType::Visual;
        Intensity = 0.5f;
        Location = FVector::ZeroVector;
        Timestamp = 0.0f;
    }
};

/**
 * AI Intent - high-level goal/action
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_AIIntent
{
    GENERATED_BODY()

    /** Type of intent */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intent")
    FGameplayTag IntentTag;

    /** Priority of this intent */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intent")
    EARPG_AIIntentPriority Priority = EARPG_AIIntentPriority::Medium;

    /** Confidence in this intent choice */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intent", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Confidence = 0.5f;

    /** Target actor for this intent */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intent")
    TWeakObjectPtr<AActor> TargetActor;

    /** Target location for this intent */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intent")
    FVector TargetLocation = FVector::ZeroVector;

    /** Additional parameters for intent execution */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intent")
    TMap<FString, FString> IntentParameters;

    /** When this intent was generated */
    UPROPERTY(BlueprintReadOnly, Category = "Intent")
    float CreationTime = 0.0f;

    FARPG_AIIntent()
    {
        Priority = EARPG_AIIntentPriority::Medium;
        Confidence = 0.5f;
        TargetLocation = FVector::ZeroVector;
        CreationTime = 0.0f;
    }
};

/**
 * Input vector for AI brain processing
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_AIInputVector
{
    GENERATED_BODY()

    /** Recent stimuli strength by type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    TMap<EARPG_StimulusType, float> StimuliStrengths;

    /** Current need levels */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    TMap<EARPG_NeedType, float> NeedLevels; // This field already exists

    /** Personality traits as gameplay tags */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    TMap<FGameplayTag, float> PersonalityTraits;

    /** Memory-contributed factors */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    TMap<FString, float> MemoryFactors;

    /** Environmental context factors */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    TMap<FString, FString> EnvironmentalContext;

    /** Health and status factors */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    TMap<FString, float> StatusFactors;

    /** Threat level assessment */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    int32 ThreatLevel;

    /** Social interaction need */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    float SocialNeed;
    
    FARPG_AIInputVector()
    {
        ThreatLevel = 0;
        SocialNeed = 0.5f;
        
        // Initialize with default values for all stimulus types
        for (int32 i = 0; i < (int32)EARPG_StimulusType::MAX; i++)
        {
            StimuliStrengths.Add((EARPG_StimulusType)i, 0.0f);
        }
        
        // Initialize with default values for all need types
        for (int32 i = 0; i < (int32)EARPG_NeedType::MAX; i++)
        {
            NeedLevels.Add((EARPG_NeedType)i, 0.5f); // This initialization already exists
        }
    }
};

/**
 * AI Need entry
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_AINeed
{
    GENERATED_BODY()

    /** Type of need */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need")
    EARPG_NeedType NeedType = EARPG_NeedType::Hunger;

    /** Current level of this need */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CurrentLevel = 0.5f;

    /** Rate at which this need changes over time */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need")
    float ChangeRate = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need")
    float CriticalThreshold = 0.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need")
    float DecayRate = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need")
    float UrgencyThreshold = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need")
    bool bIsActive = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need")
    bool bIsCritical = false;
    
    /** Minimum threshold before this need becomes urgent */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float UrgentThreshold = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need")
    float MaxLevel = 0.2f;

    /** Maximum threshold where this need is satisfied */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SatisfiedThreshold = 0.8f;

    /** Whether this need is currently urgent */
    UPROPERTY(BlueprintReadOnly, Category = "Need")
    bool bIsUrgent = false;

    FARPG_AINeed()
    {
        NeedType = EARPG_NeedType::Hunger;
        CurrentLevel = 0.5f;
        ChangeRate = 0.1f;
        UrgentThreshold = 0.2f;
        SatisfiedThreshold = 0.8f;
        bIsUrgent = false;
    }
};

/**
 * AI Personality entry
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_AIPersonalityTrait
{
    GENERATED_BODY()

    /** Type of personality trait */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
    EARPG_PersonalityTrait TraitType = EARPG_PersonalityTrait::Curiosity;

    /** Strength of this trait */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float TraitStrength = 0.5f;

    /** How much this trait can change over time */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Flexibility = 0.1f;

    FARPG_AIPersonalityTrait()
    {
        TraitType = EARPG_PersonalityTrait::Curiosity;
        TraitStrength = 0.5f;
        Flexibility = 0.1f;
    }
};

/**
 * Brain configuration
 */
/**
 * Complete AI Brain Configuration Structure
 * Matches the data table row structure
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_AIBrainConfiguration
{
    GENERATED_BODY()

    // === BASIC INFO ===
    
    /** Display name for this brain configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText DisplayName;

    /** Description of this brain configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText Description;

    /** NPC type this config applies to */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FGameplayTag NPCType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    float ProcessingFrequency = 1.0f;
    // === TIMING ===
    
    /** How often the brain updates (in seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float BrainUpdateFrequency = 1.0f;
    

    /** How long stimuli are remembered (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "1.0", ClampMax = "60.0"))
    float StimuliMemoryDuration = 10.0f;

    // === CURIOSITY ===
    
    /** Time without stimuli before curiosity activates (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curiosity", meta = (ClampMin = "5.0", ClampMax = "300.0"))
    float CuriosityThreshold = 30.0f;

    /** How strong curiosity is when activated */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curiosity", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float CuriosityStrength = 1.0f;

    // === FEATURES ===
    
    /** Whether this brain can form memories */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Features")
    bool bCanFormMemories = true;

    /** Whether this brain can learn and adapt */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Features")
    bool bCanLearn = false;

    /** Whether to log brain decisions for debugging */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bEnableDebugLogging = false;

    // === PERFORMANCE ===
    
    /** Maximum number of stimuli to track at once */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance", meta = (ClampMin = "5", ClampMax = "50"))
    int32 MaxTrackedStimuli = 20;

    // === INTENTS ===
    
    /** Default idle intent when no other intents are active */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intents")
    FGameplayTag DefaultIdleIntent;

    /** Possible curiosity behaviors */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intents")
    TArray<FGameplayTag> CuriosityIntents;

    FARPG_AIBrainConfiguration()
    {
        DisplayName = FText::FromString(TEXT("Default Brain"));
        Description = FText::FromString(TEXT("Basic AI brain configuration"));
        BrainUpdateFrequency = 1.0f;
        StimuliMemoryDuration = 10.0f;
        CuriosityThreshold = 30.0f;
        CuriosityStrength = 1.0f;
        bCanFormMemories = true;
        bCanLearn = false;
        bEnableDebugLogging = false;
        MaxTrackedStimuli = 20;
    }
};

/**
 * Current brain state
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_AIBrainState
{
    GENERATED_BODY()

    /** Current processing state */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    EARPG_BrainState CurrentState = EARPG_BrainState::Inactive;

    /** Current intent being processed */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    FARPG_AIIntent CurrentIntent;

    /** Recent stimuli being processed */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    TArray<FARPG_AIStimulus> RecentStimuli;

    /** Time since last stimulus received */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    float TimeSinceLastStimulus = 0.0f;

    /** Whether brain is enabled */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bIsEnabled = true;

    FARPG_AIBrainState()
    {
        CurrentState = EARPG_BrainState::Inactive;
        TimeSinceLastStimulus = 0.0f;
        bIsEnabled = true;
    }
};

