// Private/Economy/EconomyManager.cpp

#include "Managers/EconomyManager.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Engine/Engine.h"

// === SUBSYSTEM LIFECYCLE ===

void UEconomyManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    UE_LOG(LogTemp, Log, TEXT("EconomyManager: Initializing economy subsystem"));
    
    // Set default configuration
    SimulationConfig = FEconomySimulationConfig();
    bIsInitialized = false;
    bIsSimulationRunning = false;
}

void UEconomyManager::Deinitialize()
{
    StopEconomicSimulation();
    
    UE_LOG(LogTemp, Log, TEXT("EconomyManager: Deinitializing economy subsystem"));
    
    Super::Deinitialize();
}

// === CONFIGURATION ===

void UEconomyManager::InitializeEconomy(UDataTable* GoodsDataTable, UDataTable* ZoneEconomicDataTable)
{
    if (!GoodsDataTable || !ZoneEconomicDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("EconomyManager: Invalid data tables provided"));
        return;
    }

    // Load goods data
    TArray<FGoodsData*> GoodsRows;
    GoodsDataTable->GetAllRows<FGoodsData>(TEXT("LoadingGoodsData"), GoodsRows);
    
    GoodsData.Empty();
    for (const FGoodsData* Row : GoodsRows)
    {
        if (Row && Row->GoodsID != NAME_None)
        {
            GoodsData.Add(Row->GoodsID, *Row);
        }
    }

    // Load zone economic data
    TArray<FZoneEconomicData*> ZoneRows;
    ZoneEconomicDataTable->GetAllRows<FZoneEconomicData>(TEXT("LoadingZoneData"), ZoneRows);
    
    ZoneEconomicData.Empty();
    for (const FZoneEconomicData* Row : ZoneRows)
    {
        if (Row && Row->ZoneTag.IsValid())
        {
            ZoneEconomicData.Add(Row->ZoneTag, *Row);
            InitializeZoneMarkets(Row->ZoneTag);
        }
    }

    bIsInitialized = true;
    LogEconomyDebug(FString::Printf(TEXT("Economy initialized with %d goods and %d zones"), 
                                  GoodsData.Num(), ZoneEconomicData.Num()));
}

void UEconomyManager::StartEconomicSimulation()
{
    if (!bIsInitialized)
    {
        UE_LOG(LogTemp, Warning, TEXT("EconomyManager: Cannot start simulation - not initialized"));
        return;
    }

    if (bIsSimulationRunning)
    {
        UE_LOG(LogTemp, Warning, TEXT("EconomyManager: Simulation already running"));
        return;
    }

    if (!SimulationConfig.bEnableSimulation)
    {
        LogEconomyDebug(TEXT("Simulation disabled in configuration"));
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("EconomyManager: No valid world for simulation"));
        return;
    }

    // Start simulation timers
    World->GetTimerManager().SetTimer(
        PriceUpdateTimer,
        this,
        &UEconomyManager::UpdateMarketPrices,
        SimulationConfig.PriceUpdateInterval,
        true
    );

    World->GetTimerManager().SetTimer(
        SupplyDemandTimer,
        this,
        &UEconomyManager::UpdateSupplyDemand,
        SimulationConfig.SupplyDemandUpdateInterval,
        true
    );

    bIsSimulationRunning = true;
    LogEconomyDebug(TEXT("Economic simulation started"));
}

void UEconomyManager::StopEconomicSimulation()
{
    if (!bIsSimulationRunning)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(PriceUpdateTimer);
        World->GetTimerManager().ClearTimer(SupplyDemandTimer);
    }

    bIsSimulationRunning = false;
    LogEconomyDebug(TEXT("Economic simulation stopped"));
}

void UEconomyManager::ConfigureSimulation(const FEconomySimulationConfig& Config)
{
    bool bWasRunning = bIsSimulationRunning;
    
    if (bWasRunning)
    {
        StopEconomicSimulation();
    }

    SimulationConfig = Config;

    if (bWasRunning)
    {
        StartEconomicSimulation();
    }

    LogEconomyDebug(TEXT("Simulation configuration updated"));
}

// === IECONOMYINTERFACE IMPLEMENTATION ===

int32 UEconomyManager::GetGoodsPrice_Implementation(const FName& GoodsID, const FGameplayTag& ZoneTag) const
{
    const FZoneEconomicData* ZoneData = FindZoneData(ZoneTag);
    if (!ZoneData)
    {
        return 0;
    }

    const FMarketData* MarketData = ZoneData->Markets.Find(GoodsID);
    if (!MarketData)
    {
        // Return base price if no market data
        const FGoodsData* Goods = FindGoodsData(GoodsID);
        return Goods ? Goods->BaseValue : 0;
    }

    return MarketData->CurrentPrice;
}

bool UEconomyManager::AreGoodsAvailable_Implementation(const FName& GoodsID, const FGameplayTag& ZoneTag, int32 Quantity) const
{
    const FZoneEconomicData* ZoneData = FindZoneData(ZoneTag);
    if (!ZoneData)
    {
        return false;
    }

    const FMarketData* MarketData = ZoneData->Markets.Find(GoodsID);
    if (!MarketData)
    {
        return false;
    }

    return MarketData->Supply >= Quantity;
}

EMarketDemand UEconomyManager::GetGoodsDemand_Implementation(const FName& GoodsID, const FGameplayTag& ZoneTag) const
{
    const FZoneEconomicData* ZoneData = FindZoneData(ZoneTag);
    if (!ZoneData)
    {
        return EMarketDemand::Normal;
    }

    const FMarketData* MarketData = ZoneData->Markets.Find(GoodsID);
    if (!MarketData)
    {
        return EMarketDemand::Normal;
    }

    return MarketData->CalculateDemandLevel();
}

bool UEconomyManager::BuyGoods_Implementation(const FName& GoodsID, int32 Quantity, const FGameplayTag& ZoneTag, int32& TotalCost)
{
    TotalCost = 0;
    
    FZoneEconomicData* ZoneData = FindZoneData(ZoneTag);
    if (!ZoneData)
    {
        return false;
    }

    FMarketData* MarketData = ZoneData->Markets.Find(GoodsID);
    if (!MarketData || MarketData->Supply < Quantity)
    {
        return false;
    }

    TotalCost = MarketData->CurrentPrice * Quantity;
    MarketData->Supply -= Quantity;
    MarketData->LastTransactionTime = GetWorld()->GetTimeSeconds();

    // Increase demand slightly after purchase
    MarketData->Demand += FMath::Max(1, Quantity / 4);

    LogEconomyDebug(FString::Printf(TEXT("Sold %d %s for %d copper in %s"), 
                                  Quantity, *GoodsID.ToString(), TotalCost, *ZoneTag.ToString()));

    return true;
}

bool UEconomyManager::SellGoods_Implementation(const FName& GoodsID, int32 Quantity, const FGameplayTag& ZoneTag, int32& TotalRevenue)
{
    TotalRevenue = 0;
    
    FZoneEconomicData* ZoneData = FindZoneData(ZoneTag);
    if (!ZoneData)
    {
        return false;
    }

    FMarketData* MarketData = ZoneData->Markets.Find(GoodsID);
    if (!MarketData)
    {
        // Create market data if it doesn't exist
        const FGoodsData* Goods = FindGoodsData(GoodsID);
        if (!Goods)
        {
            return false;
        }

        FMarketData NewMarket;
        NewMarket.CurrentPrice = Goods->BaseValue;
        NewMarket.AveragePrice = Goods->BaseValue;
        ZoneData->Markets.Add(GoodsID, NewMarket);
        MarketData = ZoneData->Markets.Find(GoodsID);
    }

    TotalRevenue = MarketData->CurrentPrice * Quantity;
    MarketData->Supply += Quantity;
    MarketData->LastTransactionTime = GetWorld()->GetTimeSeconds();

    // Decrease demand slightly after sale
    MarketData->Demand = FMath::Max(0, MarketData->Demand - FMath::Max(1, Quantity / 4));

    LogEconomyDebug(FString::Printf(TEXT("Bought %d %s for %d copper from %s"), 
                                  Quantity, *GoodsID.ToString(), TotalRevenue, *ZoneTag.ToString()));

    return true;
}

TArray<FName> UEconomyManager::GetAvailableGoods_Implementation(const FGameplayTag& ZoneTag) const
{
    TArray<FName> AvailableGoods;
    
    const FZoneEconomicData* ZoneData = FindZoneData(ZoneTag);
    if (!ZoneData)
    {
        return AvailableGoods;
    }

    for (const auto& MarketPair : ZoneData->Markets)
    {
        if (MarketPair.Value.Supply > 0)
        {
            AvailableGoods.Add(MarketPair.Key);
        }
    }

    return AvailableGoods;
}

FMarketData UEconomyManager::GetMarketData_Implementation(const FName& GoodsID, const FGameplayTag& ZoneTag) const
{
    const FZoneEconomicData* ZoneData = FindZoneData(ZoneTag);
    if (!ZoneData)
    {
        return FMarketData();
    }

    const FMarketData* MarketData = ZoneData->Markets.Find(GoodsID);
    return MarketData ? *MarketData : FMarketData();
}

FZoneEconomicData UEconomyManager::GetZoneEconomicData_Implementation(const FGameplayTag& ZoneTag) const
{
    const FZoneEconomicData* ZoneData = FindZoneData(ZoneTag);
    return ZoneData ? *ZoneData : FZoneEconomicData();
}

// === MARKET MANIPULATION ===

void UEconomyManager::AddSupply(const FName& GoodsID, const FGameplayTag& ZoneTag, int32 Amount)
{
    FZoneEconomicData* ZoneData = FindZoneData(ZoneTag);
    if (!ZoneData)
    {
        return;
    }

    FMarketData* MarketData = ZoneData->Markets.Find(GoodsID);
    if (MarketData)
    {
        MarketData->Supply += Amount;
        LogEconomyDebug(FString::Printf(TEXT("Added %d supply of %s to %s"), 
                                      Amount, *GoodsID.ToString(), *ZoneTag.ToString()));
    }
}

void UEconomyManager::AddDemand(const FName& GoodsID, const FGameplayTag& ZoneTag, int32 Amount)
{
    FZoneEconomicData* ZoneData = FindZoneData(ZoneTag);
    if (!ZoneData)
    {
        return;
    }

    FMarketData* MarketData = ZoneData->Markets.Find(GoodsID);
    if (MarketData)
    {
        MarketData->Demand += Amount;
        LogEconomyDebug(FString::Printf(TEXT("Added %d demand of %s to %s"), 
                                      Amount, *GoodsID.ToString(), *ZoneTag.ToString()));
    }
}

void UEconomyManager::UpdateGoodsPrice(const FName& GoodsID, const FGameplayTag& ZoneTag)
{
    FZoneEconomicData* ZoneData = FindZoneData(ZoneTag);
    if (!ZoneData)
    {
        return;
    }

    FMarketData* MarketData = ZoneData->Markets.Find(GoodsID);
    const FGoodsData* Goods = FindGoodsData(GoodsID);
    
    if (!MarketData || !Goods)
    {
        return;
    }

    int32 OldPrice = MarketData->CurrentPrice;
    MarketData->CurrentPrice = CalculatePrice(GoodsID, *MarketData, *Goods);

    if (FMath::Abs(OldPrice - MarketData->CurrentPrice) > 0)
    {
        OnPriceChanged.Broadcast(GoodsID, ZoneTag, OldPrice, MarketData->CurrentPrice);
    }
}

// === ZONE MANAGEMENT ===

bool UEconomyManager::RegisterZone(const FZoneEconomicData& ZoneData)
{
    if (!ZoneData.ZoneTag.IsValid())
    {
        return false;
    }

    ZoneEconomicData.Add(ZoneData.ZoneTag, ZoneData);
    InitializeZoneMarkets(ZoneData.ZoneTag);

    LogEconomyDebug(FString::Printf(TEXT("Registered economic zone: %s"), *ZoneData.ZoneTag.ToString()));
    return true;
}

void UEconomyManager::UnregisterZone(const FGameplayTag& ZoneTag)
{
    if (ZoneEconomicData.Remove(ZoneTag) > 0)
    {
        LogEconomyDebug(FString::Printf(TEXT("Unregistered economic zone: %s"), *ZoneTag.ToString()));
    }
}

void UEconomyManager::UpdateZoneProduction(const FGameplayTag& ZoneTag, const TMap<FName, int32>& ProductionRates)
{
    FZoneEconomicData* ZoneData = FindZoneData(ZoneTag);
    if (!ZoneData)
    {
        return;
    }

    ZoneData->ProducedGoods = ProductionRates;
    LogEconomyDebug(FString::Printf(TEXT("Updated production rates for zone: %s"), *ZoneTag.ToString()));
}

// === INTERNAL SIMULATION ===

void UEconomyManager::UpdateMarketPrices()
{
    for (auto& ZonePair : ZoneEconomicData)
    {
        for (auto& MarketPair : ZonePair.Value.Markets)
        {
            UpdateGoodsPrice(MarketPair.Key, ZonePair.Key);
        }
    }
}

void UEconomyManager::UpdateSupplyDemand()
{
    ProcessZoneProduction();
    ProcessZoneConsumption();
}

void UEconomyManager::ProcessZoneProduction()
{
    for (auto& ZonePair : ZoneEconomicData)
    {
        FZoneEconomicData& ZoneData = ZonePair.Value;
        
        for (const auto& ProductionPair : ZoneData.ProducedGoods)
        {
            int32 ProducedAmount = FMath::RoundToInt(ProductionPair.Value * SimulationConfig.SupplyDemandUpdateInterval / 3600.0f);
            AddSupply(ProductionPair.Key, ZonePair.Key, ProducedAmount);
        }
    }
}

void UEconomyManager::ProcessZoneConsumption()
{
    for (auto& ZonePair : ZoneEconomicData)
    {
        FZoneEconomicData& ZoneData = ZonePair.Value;
        
        for (const auto& ConsumptionPair : ZoneData.ConsumedGoods)
        {
            int32 ConsumedAmount = FMath::RoundToInt(ConsumptionPair.Value * SimulationConfig.SupplyDemandUpdateInterval / 3600.0f);
            AddDemand(ConsumptionPair.Key, ZonePair.Key, ConsumedAmount);
        }
    }
}

int32 UEconomyManager::CalculatePrice(const FName& GoodsID, const FMarketData& InMarketData, const FGoodsData& InGoodsData) const
{
    float PriceModifier = InMarketData.GetPriceModifier();
    int32 NewPrice = FMath::RoundToInt(InGoodsData.BaseValue * PriceModifier);
    
    // Limit price change rate
    float MaxChange = InMarketData.CurrentPrice * SimulationConfig.MaxPriceChangeRate;
    int32 PriceDifference = NewPrice - InMarketData.CurrentPrice;
    
    if (FMath::Abs(PriceDifference) > MaxChange)
    {
        NewPrice = InMarketData.CurrentPrice + FMath::Sign(PriceDifference) * MaxChange;
    }

    return FMath::Max(1, NewPrice);
}

void UEconomyManager::InitializeZoneMarkets(const FGameplayTag& ZoneTag)
{
    FZoneEconomicData* ZoneData = FindZoneData(ZoneTag);
    if (!ZoneData)
    {
        return;
    }

    // Initialize markets for all known goods
    for (const auto& GoodsPair : GoodsData)
    {
        if (!ZoneData->Markets.Contains(GoodsPair.Key))
        {
            FMarketData NewMarket;
            NewMarket.CurrentPrice = GoodsPair.Value.BaseValue;
            NewMarket.AveragePrice = GoodsPair.Value.BaseValue;
            NewMarket.Supply = 0;
            NewMarket.Demand = 0;
            
            ZoneData->Markets.Add(GoodsPair.Key, NewMarket);
        }
    }
}

// === UTILITY FUNCTIONS ===

FZoneEconomicData* UEconomyManager::FindZoneData(const FGameplayTag& ZoneTag)
{
    return ZoneEconomicData.Find(ZoneTag);
}

const FZoneEconomicData* UEconomyManager::FindZoneData(const FGameplayTag& ZoneTag) const
{
    return ZoneEconomicData.Find(ZoneTag);
}

const FGoodsData* UEconomyManager::FindGoodsData(const FName& GoodsID) const
{
    return GoodsData.Find(GoodsID);
}

void UEconomyManager::LogEconomyDebug(const FString& Message) const
{
    if (SimulationConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("EconomyManager: %s"), *Message);
    }
}
