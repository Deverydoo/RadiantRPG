// Public/Economy/IEconomyInterface.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Types/EconomyTypes.h"
#include "GameplayTagContainer.h"
#include "IEconomyInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UEconomyInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * Interface for economy system interactions
 */
class RADIANTRPG_API IEconomyInterface
{
    GENERATED_BODY()

public:
    // === PRICE QUERIES ===
    
    /** Get current market price for goods in a zone */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Economy")
    int32 GetGoodsPrice(const FName& GoodsID, const FGameplayTag& ZoneTag) const;

    /** Check if goods are available in zone */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Economy")
    bool AreGoodsAvailable(const FName& GoodsID, const FGameplayTag& ZoneTag, int32 Quantity = 1) const;

    /** Get market demand level for goods */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Economy")
    EMarketDemand GetGoodsDemand(const FName& GoodsID, const FGameplayTag& ZoneTag) const;

    // === TRANSACTIONS ===
    
    /** Execute buy transaction */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Economy")
    bool BuyGoods(const FName& GoodsID, int32 Quantity, const FGameplayTag& ZoneTag, int32& TotalCost);

    /** Execute sell transaction */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Economy")
    bool SellGoods(const FName& GoodsID, int32 Quantity, const FGameplayTag& ZoneTag, int32& TotalRevenue);

    // === MARKET DATA ===
    
    /** Get all available goods in zone */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Economy")
    TArray<FName> GetAvailableGoods(const FGameplayTag& ZoneTag) const;

    /** Get market data for specific goods */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Economy")
    FMarketData GetMarketData(const FName& GoodsID, const FGameplayTag& ZoneTag) const;

    /** Get zone economic data */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Economy")
    FZoneEconomicData GetZoneEconomicData(const FGameplayTag& ZoneTag) const;

    // === EVENTS ===
    
    /** Called when market prices change significantly */
    UFUNCTION(BlueprintImplementableEvent, Category = "Economy Events")
    void OnMarketPriceChanged(const FName& GoodsID, const FGameplayTag& ZoneTag, int32 OldPrice, int32 NewPrice);

    /** Called when goods shortage occurs */
    UFUNCTION(BlueprintImplementableEvent, Category = "Economy Events")
    void OnGoodsShortage(const FName& GoodsID, const FGameplayTag& ZoneTag);

    /** Called when goods surplus occurs */
    UFUNCTION(BlueprintImplementableEvent, Category = "Economy Events")
    void OnGoodsSurplus(const FName& GoodsID, const FGameplayTag& ZoneTag);
};