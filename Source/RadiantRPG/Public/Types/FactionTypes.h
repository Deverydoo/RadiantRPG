// Source/RadiantRPG/Public/Types/FactionTypes.h

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RadiantAITypes.h"
#include "FactionTypes.generated.h"

/**
 * Faction types/categories
 */
UENUM(BlueprintType)
enum class EFactionType : uint8
{
    None            UMETA(DisplayName = "None"),
    Settlement      UMETA(DisplayName = "Settlement"),
    Military        UMETA(DisplayName = "Military"),
    Bandits         UMETA(DisplayName = "Bandits"),
    Merchants       UMETA(DisplayName = "Merchants"),
    Wildlife        UMETA(DisplayName = "Wildlife"),
    Mutants         UMETA(DisplayName = "Mutants"),
    Cultists        UMETA(DisplayName = "Cultists"),
    Nomads          UMETA(DisplayName = "Nomads"),
    Undead          UMETA(DisplayName = "Undead"),
    Elemental       UMETA(DisplayName = "Elemental"),
    
    MAX             UMETA(Hidden)
};

/**
 * Faction goals that drive AI behavior
 */
UENUM(BlueprintType)
enum class EFactionGoal : uint8
{
    None            UMETA(DisplayName = "None"),
    Survive         UMETA(DisplayName = "Survive"),
    Expand          UMETA(DisplayName = "Expand Territory"),
    Trade           UMETA(DisplayName = "Trade"),
    Conquer         UMETA(DisplayName = "Conquer"),
    Defend          UMETA(DisplayName = "Defend"),
    Raid            UMETA(DisplayName = "Raid"),
    Explore         UMETA(DisplayName = "Explore"),
    Research        UMETA(DisplayName = "Research"),
    Convert         UMETA(DisplayName = "Convert"),
    Purge           UMETA(DisplayName = "Purge"),
    
    MAX             UMETA(Hidden)
};

/**
 * Faction activity states
 */
UENUM(BlueprintType)
enum class EFactionState : uint8
{
    Peaceful        UMETA(DisplayName = "Peaceful"),
    Alert           UMETA(DisplayName = "Alert"),
    Mobilizing      UMETA(DisplayName = "Mobilizing"),
    AtWar           UMETA(DisplayName = "At War"),
    Retreating      UMETA(DisplayName = "Retreating"),
    Rebuilding      UMETA(DisplayName = "Rebuilding"),
    Expanding       UMETA(DisplayName = "Expanding"),
    Migrating       UMETA(DisplayName = "Migrating"),
    
    MAX             UMETA(Hidden)
};

/**
 * Territory control types
 */
UENUM(BlueprintType)
enum class ETerritoryControl : uint8
{
    Uncontrolled    UMETA(DisplayName = "Uncontrolled"),
    Contested       UMETA(DisplayName = "Contested"),
    Controlled      UMETA(DisplayName = "Controlled"),
    Fortified       UMETA(DisplayName = "Fortified"),
    
    MAX             UMETA(Hidden)
};

/**
 * Faction resource types
 */
UENUM(BlueprintType)
enum class EFactionResource : uint8
{
    Population      UMETA(DisplayName = "Population"),
    Food            UMETA(DisplayName = "Food"),
    Materials       UMETA(DisplayName = "Materials"),
    Weapons         UMETA(DisplayName = "Weapons"),
    Gold            UMETA(DisplayName = "Gold"),
    Influence       UMETA(DisplayName = "Influence"),
    Knowledge       UMETA(DisplayName = "Knowledge"),
    MagicPower      UMETA(DisplayName = "Magic Power"),
    
    MAX             UMETA(Hidden)
};

/**
 * Faction reputation levels
 */
UENUM(BlueprintType)
enum class EReputationLevel : uint8
{
    Hated           UMETA(DisplayName = "Hated"),          // -1000 to -600
    Hostile         UMETA(DisplayName = "Hostile"),        // -600 to -300
    Disliked        UMETA(DisplayName = "Disliked"),       // -300 to -100
    Neutral         UMETA(DisplayName = "Neutral"),        // -100 to 100
    Liked           UMETA(DisplayName = "Liked"),          // 100 to 300
    Honored         UMETA(DisplayName = "Honored"),        // 300 to 600
    Revered         UMETA(DisplayName = "Revered"),        // 600 to 1000
    
    MAX             UMETA(Hidden)
};

/**
 * Faction relationship configuration
 */
USTRUCT(BlueprintType)
struct FFactionRelationship
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName FactionID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RelationshipValue = 0.0f; // -100 to 100

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERelationshipStanding Standing = ERelationshipStanding::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsAtWar = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasTradingRights = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasAlliance = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float LastConflictTime = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FName> SharedEnemies;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FName> TradeGoods;

    FFactionRelationship()
    {
    }
};

/**
 * Resource stockpile entry
 */
USTRUCT(BlueprintType)
struct FFactionResourceEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EFactionResource ResourceType = EFactionResource::Food;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CurrentAmount = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxCapacity = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ProductionRate = 0.0f; // Per hour

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ConsumptionRate = 0.0f; // Per hour

    FFactionResourceEntry()
    {
    }

    float GetNetRate() const { return ProductionRate - ConsumptionRate; }
    bool IsDeficit() const { return GetNetRate() < 0.0f; }
};

/**
 * Territory control data
 */
USTRUCT(BlueprintType)
struct FTerritoryData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ZoneID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ControllingFaction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETerritoryControl ControlLevel = ETerritoryControl::Uncontrolled;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ControlStrength = 0.0f; // 0-100

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FName> ContestingFactions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector ControlPoint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DefenseRating = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FFactionResourceEntry> LocalResources;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer TerritoryTags;

    FTerritoryData()
    {
    }
};

/**
 * Faction member role
 */
UENUM(BlueprintType)
enum class EFactionRole : uint8
{
    Civilian        UMETA(DisplayName = "Civilian"),
    Soldier         UMETA(DisplayName = "Soldier"),
    Officer         UMETA(DisplayName = "Officer"),
    Leader          UMETA(DisplayName = "Leader"),
    Merchant        UMETA(DisplayName = "Merchant"),
    Crafter         UMETA(DisplayName = "Crafter"),
    Scout           UMETA(DisplayName = "Scout"),
    Healer          UMETA(DisplayName = "Healer"),
    Mage            UMETA(DisplayName = "Mage"),
    
    MAX             UMETA(Hidden)
};

/**
 * Faction member data
 */
USTRUCT(BlueprintType)
struct FFactionMember
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TWeakObjectPtr<AActor> MemberActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName MemberID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EFactionRole Role = EFactionRole::Civilian;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Loyalty = 50.0f; // 0-100

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Contribution = 0.0f; // Faction contribution score

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Rank = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer MemberTags;

    FFactionMember()
    {
    }
};

/**
 * Faction event for broadcasting
 */
USTRUCT(BlueprintType)
struct FFactionEvent
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName EventID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName FactionID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText EventDescription;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag EventType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Timestamp = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector EventLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FName> InvolvedFactions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FName, FString> EventData;

    FFactionEvent()
    {
    }
};

/**
 * Complete faction data
 */
USTRUCT(BlueprintType)
struct FFactionData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName FactionID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EFactionType FactionType = EFactionType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EFactionState CurrentState = EFactionState::Peaceful;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<EFactionGoal> ActiveGoals;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FFactionRelationship> Relationships;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FFactionResourceEntry> Resources;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FTerritoryData> ControlledTerritories;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FFactionMember> Members;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Power = 100.0f; // Overall faction strength

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Morale = 50.0f; // 0-100

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector HomeLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer FactionTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UTexture2D> FactionIcon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor FactionColor = FLinearColor::White;

    FFactionData()
    {
    }
};

/**
 * Player reputation with faction
 */
USTRUCT(BlueprintType)
struct FPlayerFactionReputation
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName FactionID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ReputationValue = 0.0f; // -1000 to 1000

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EReputationLevel Level = EReputationLevel::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsKnown = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 QuestsCompleted = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MembersKilled = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float LastInteractionTime = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FName> CompletedQuests;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer ReputationTags;

    FPlayerFactionReputation()
    {
    }

    void UpdateLevel()
    {
        if (ReputationValue < -600.0f)
            Level = EReputationLevel::Hated;
        else if (ReputationValue < -300.0f)
            Level = EReputationLevel::Hostile;
        else if (ReputationValue < -100.0f)
            Level = EReputationLevel::Disliked;
        else if (ReputationValue < 100.0f)
            Level = EReputationLevel::Neutral;
        else if (ReputationValue < 300.0f)
            Level = EReputationLevel::Liked;
        else if (ReputationValue < 600.0f)
            Level = EReputationLevel::Honored;
        else
            Level = EReputationLevel::Revered;
    }
};