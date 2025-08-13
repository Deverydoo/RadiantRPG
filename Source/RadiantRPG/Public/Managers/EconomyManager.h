// Public/Economy/EconomyManager.h

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "Interfaces/IEconomyInterface.h"
#include "Types/EconomyTypes.h"
#include "GameplayTagContainer.h"
#include "EconomyManager.generated.h"

/**
 * Economy simulation configuration
 */
USTRUCT(BlueprintType)
struct FEconomySimulationConfig
{
    GENERATED_BODY()

    /** Enable economy simulation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
    bool bEnableSimulation = true;

    /** Price update frequency in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation", meta = (ClampMin = "1.0"))
    float PriceUpdateInterval = 60.0f;

    /** Supply/demand update frequency in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation", meta = (ClampMin = "5.0"))
    float SupplyDemandUpdateInterval = 30.0f;

    /** Maximum price change per update (percentage) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation", meta = (ClampMin = "0.01", ClampMax = "0.5"))
    float MaxPriceChangeRate = 0.1f;

    /** Enable debug logging */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bEnableDebugLogging = false;

    FEconomySimulationConfig()
    {
    }
};

/**
 * Economy Manager Subsystem
 * Handles supply/demand simulation and goods trading
 */
UCLASS(BlueprintType, Blueprintable)
class RADIANTRPG_API UEconomyManager : public UGameInstanceSubsystem, public IEconomyInterface
{
    GENERATED_BODY()

public:
    // === SUBSYSTEM LIFECYCLE ===
    
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // === CONFIGURATION ===
    
    /** Initialize economy system with data tables */
    UFUNCTION(BlueprintCallable, Category = "Economy")
    void InitializeEconomy(UDataTable* GoodsDataTable, UDataTable* ZoneEconomicDataTable);

    /** Start economy simulation */
    UFUNCTION(BlueprintCallable, Category = "Economy")
    void StartEconomicSimulation();

    /** Stop economy simulation */
    UFUNCTION(BlueprintCallable, Category = "Economy")
    void StopEconomicSimulation();

    /** Configure simulation parameters */
    UFUNCTION(BlueprintCallable, Category = "Economy")
    void ConfigureSimulation(const FEconomySimulationConfig& Config);

    // === IECONOMYINTERFACE IMPLEMENTATION ===
    
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Economy")
    int32 GetGoodsPrice(const FName& GoodsID, const FGameplayTag& ZoneTag) const;
    virtual int32 GetGoodsPrice_Implementation(const FName& GoodsID, const FGameplayTag& ZoneTag) const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Economy")
    bool AreGoodsAvailable(const FName& GoodsID, const FGameplayTag& ZoneTag, int32 Quantity = 1) const;
    virtual bool AreGoodsAvailable_Implementation(const FName& GoodsID, const FGameplayTag& ZoneTag, int32 Quantity = 1) const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Economy")
    EMarketDemand GetGoodsDemand(const FName& GoodsID, const FGameplayTag& ZoneTag) const;
    virtual EMarketDemand GetGoodsDemand_Implementation(const FName& GoodsID, const FGameplayTag& ZoneTag) const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Economy")
    bool BuyGoods(const FName& GoodsID, int32 Quantity, const FGameplayTag& ZoneTag, int32& TotalCost);
    virtual bool BuyGoods_Implementation(const FName& GoodsID, int32 Quantity, const FGameplayTag& ZoneTag, int32& TotalCost);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Economy")
    bool SellGoods(const FName& GoodsID, int32 Quantity, const FGameplayTag& ZoneTag, int32& TotalRevenue);
    virtual bool SellGoods_Implementation(const FName& GoodsID, int32 Quantity, const FGameplayTag& ZoneTag, int32& TotalRevenue);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Economy")
    TArray<FName> GetAvailableGoods(const FGameplayTag& ZoneTag) const;
    virtual TArray<FName> GetAvailableGoods_Implementation(const FGameplayTag& ZoneTag) const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Economy")
    FMarketData GetMarketData(const FName& GoodsID, const FGameplayTag& ZoneTag) const;
    virtual FMarketData GetMarketData_Implementation(const FName& GoodsID, const FGameplayTag& ZoneTag) const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Economy")
    FZoneEconomicData GetZoneEconomicData(const FGameplayTag& ZoneTag) const;
    virtual FZoneEconomicData GetZoneEconomicData_Implementation(const FGameplayTag& ZoneTag) const;

    // === MARKET MANIPULATION ===
    
    /** Add supply to zone market */
    UFUNCTION(BlueprintCallable, Category = "Economy")
    void AddSupply(const FName& GoodsID, const FGameplayTag& ZoneTag, int32 Amount);

    /** Add demand to zone market */
    UFUNCTION(BlueprintCallable, Category = "Economy")
    void AddDemand(const FName& GoodsID, const FGameplayTag& ZoneTag, int32 Amount);

    /** Force price update for specific goods */
    UFUNCTION(BlueprintCallable, Category = "Economy")
    void UpdateGoodsPrice(const FName& GoodsID, const FGameplayTag& ZoneTag);

    // === ZONE MANAGEMENT ===
    
    /** Register new economic zone */
    UFUNCTION(BlueprintCallable, Category = "Economy")
    bool RegisterZone(const FZoneEconomicData& ZoneData);

    /** Remove economic zone */
    UFUNCTION(BlueprintCallable, Category = "Economy")
    void UnregisterZone(const FGameplayTag& ZoneTag);

    /** Update zone production rates */
    UFUNCTION(BlueprintCallable, Category = "Economy")
    void UpdateZoneProduction(const FGameplayTag& ZoneTag, const TMap<FName, int32>& ProductionRates);

    // === EVENTS ===
    
    /** Economy events delegate */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnMarketEvent, FName, GoodsID, FGameplayTag, ZoneTag, int32, OldValue, int32, NewValue);

    /** Called when prices change */
    UPROPERTY(BlueprintAssignable, Category = "Economy Events")
    FOnMarketEvent OnPriceChanged;

    /** Called when shortage occurs */
    UPROPERTY(BlueprintAssignable, Category = "Economy Events")
    FOnMarketEvent OnShortage;

    /** Called when surplus occurs */
    UPROPERTY(BlueprintAssignable, Category = "Economy Events")
    FOnMarketEvent OnSurplus;

protected:
    // === INTERNAL SIMULATION ===
    
    /** Update all market prices */
    UFUNCTION()
    void UpdateMarketPrices();

    /** Update supply and demand */
    UFUNCTION()
    void UpdateSupplyDemand();

    /** Process zone production */
    void ProcessZoneProduction();

    /** Process zone consumption */
    void ProcessZoneConsumption();

    /** Calculate new price based on supply/demand */
    int32 CalculatePrice(const FName& GoodsID, const FMarketData& InMarketData, const FGoodsData& InGoodsData) const;
    void InitializeZoneMarkets(const FGameplayTag& ZoneTag);

    /** Find zone economic data */
    FZoneEconomicData* FindZoneData(const FGameplayTag& ZoneTag);
    const FZoneEconomicData* FindZoneData(const FGameplayTag& ZoneTag) const;

    /** Find goods data */
    const FGoodsData* FindGoodsData(const FName& GoodsID) const;

    /** Log economy debug information */
    void LogEconomyDebug(const FString& Message) const;

private:
    // === DATA ===
    
    /** Simulation configuration */
    UPROPERTY(EditAnywhere, Category = "Configuration")
    FEconomySimulationConfig SimulationConfig;

    /** Zone economic data */
    UPROPERTY()
    TMap<FGameplayTag, FZoneEconomicData> ZoneEconomicData;

    /** Goods data lookup */
    UPROPERTY()
    TMap<FName, FGoodsData> GoodsData;

    /** Simulation timers */
    FTimerHandle PriceUpdateTimer;
    FTimerHandle SupplyDemandTimer;

    /** Simulation state */
    bool bIsSimulationRunning = false;
    bool bIsInitialized = false;
};