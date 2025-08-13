// Public/Types/EconomyTypes.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "WorldTypes.h"
#include "EconomyTypes.generated.h"

/**
 * Economic goods categories
 */
UENUM(BlueprintType)
enum class EGoodsCategory : uint8
{
    None            UMETA(DisplayName = "None"),
    Food            UMETA(DisplayName = "Food"),
    Materials       UMETA(DisplayName = "Raw Materials"),
    Tools           UMETA(DisplayName = "Tools"),
    Weapons         UMETA(DisplayName = "Weapons"),
    Armor           UMETA(DisplayName = "Armor"),
    Luxury          UMETA(DisplayName = "Luxury Goods"),
    Medicine        UMETA(DisplayName = "Medicine"),
    Fuel            UMETA(DisplayName = "Fuel"),
    
    MAX             UMETA(Hidden)
};

/**
 * Market demand levels
 */
UENUM(BlueprintType)
enum class EMarketDemand : uint8
{
    VeryLow         UMETA(DisplayName = "Very Low"),
    Low             UMETA(DisplayName = "Low"),
    Normal          UMETA(DisplayName = "Normal"),
    High            UMETA(DisplayName = "High"),
    VeryHigh        UMETA(DisplayName = "Very High"),
    Critical        UMETA(DisplayName = "Critical"),
    
    MAX             UMETA(Hidden)
};

/**
 * Economic zone types
 */
UENUM(BlueprintType)
enum class EEconomicZoneType : uint8
{
    Rural           UMETA(DisplayName = "Rural"),
    Town            UMETA(DisplayName = "Town"),
    City            UMETA(DisplayName = "City"),
    Industrial      UMETA(DisplayName = "Industrial"),
    Mining          UMETA(DisplayName = "Mining"),
    Trade           UMETA(DisplayName = "Trade Hub"),
    Military        UMETA(DisplayName = "Military"),
    Frontier        UMETA(DisplayName = "Frontier"),
    
    MAX             UMETA(Hidden)
};

/**
 * Goods data for trading
 */
USTRUCT(BlueprintType)
struct FGoodsData : public FTableRowBase
{
    GENERATED_BODY()

    /** Unique goods identifier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FName GoodsID;

    /** Display name */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText DisplayName;

    /** Description */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText Description;

    /** Goods category */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    EGoodsCategory Category = EGoodsCategory::None;

    /** Base value in copper */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economics", meta = (ClampMin = "1"))
    int32 BaseValue = 100;

    /** Weight per unit */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties", meta = (ClampMin = "0.01"))
    float Weight = 1.0f;

    /** Volume per unit */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties", meta = (ClampMin = "0.01"))
    float Volume = 1.0f;

    /** Can this good spoil over time */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
    bool bCanSpoil = false;

    /** Days until spoilage */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties", meta = (EditCondition = "bCanSpoil", ClampMin = "0.1"))
    float SpoilageTime = 30.0f;

    /** Zones that produce this good */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economics")
    TArray<EEconomicZoneType> ProducingZones;

    /** Zones that consume this good */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economics")
    TArray<EEconomicZoneType> ConsumingZones;

    /** Tags for gameplay integration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
    FGameplayTagContainer GoodsTags;

    FGoodsData()
    {
        GoodsID = NAME_None;
        DisplayName = FText::FromString(TEXT("Unknown Good"));
        Description = FText::FromString(TEXT("A trade good"));
    }
};

/**
 * Market supply and demand data
 */
USTRUCT(BlueprintType)
struct FMarketData
{
    GENERATED_BODY()

    /** Current supply amount */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
    int32 Supply = 0;

    /** Current demand amount */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
    int32 Demand = 0;

    /** Current market price */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
    int32 CurrentPrice = 100;

    /** Historical price average */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
    int32 AveragePrice = 100;

    /** Last transaction time */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
    float LastTransactionTime = 0.0f;

    /** Market demand level */
    UPROPERTY(BlueprintReadOnly, Category = "Market")
    EMarketDemand DemandLevel = EMarketDemand::Normal;

    FMarketData()
    {
    }

    /** Calculate demand level based on supply/demand ratio */
    EMarketDemand CalculateDemandLevel() const;

    /** Get price modifier based on supply/demand */
    float GetPriceModifier() const;
};

/**
 * Zone economic configuration
 */
USTRUCT(BlueprintType)
struct FZoneEconomicData : public FTableRowBase
{
    GENERATED_BODY()

    /** Zone identifier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FGameplayTag ZoneTag;

    /** Zone type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    EEconomicZoneType ZoneType = EEconomicZoneType::Rural;

    /** Population size affecting market */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economics", meta = (ClampMin = "1"))
    int32 Population = 100;

    /** Economic prosperity modifier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economics", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float ProsperityModifier = 1.0f;

    /** Trade route connections */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade")
    TArray<FGameplayTag> ConnectedZones;

    /** Goods this zone produces */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
    TMap<FName, int32> ProducedGoods;

    /** Goods this zone consumes */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumption")
    TMap<FName, int32> ConsumedGoods;

    /** Current market data for all goods */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
    TMap<FName, FMarketData> Markets;

    FZoneEconomicData()
    {
    }
};