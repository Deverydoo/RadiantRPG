// Public/Types/SystemTypes.h
// System initialization event types

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "SystemTypes.generated.h"

UENUM(BlueprintType)
enum class ESystemType : uint8
{
    GameManager UMETA(DisplayName = "Game Manager"),
    WorldManager UMETA(DisplayName = "World Manager"),
    ZoneManager UMETA(DisplayName = "Zone Manager"),
    AIManager UMETA(DisplayName = "AI Manager"),
    EconomyManager UMETA(DisplayName = "Economy Manager"),
    FactionManager UMETA(DisplayName = "Faction Manager"),
    EventManager UMETA(DisplayName = "Event Manager")
};

USTRUCT(BlueprintType)
struct FSystemInitializationEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "System Event")
    ESystemType SystemType;

    UPROPERTY(BlueprintReadOnly, Category = "System Event")
    bool bInitializationSuccessful;

    UPROPERTY(BlueprintReadOnly, Category = "System Event")
    FString SystemName;

    UPROPERTY(BlueprintReadOnly, Category = "System Event")
    double InitializationTime;

    FSystemInitializationEvent()
        : SystemType(ESystemType::GameManager)
        , bInitializationSuccessful(false)
        , InitializationTime(0.0)
    {}

    FSystemInitializationEvent(ESystemType InSystemType, bool bSuccess, const FString& InName)
        : SystemType(InSystemType)
        , bInitializationSuccessful(bSuccess)
        , SystemName(InName)
        , InitializationTime(FPlatformTime::Seconds())
    {}
};

// Global system initialization events
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSystemInitialized, const FSystemInitializationEvent&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSystemShutdown, const FSystemInitializationEvent&);

/**
 * Global system coordinator - handles system initialization events
 */
class RADIANTRPG_API FSystemEventCoordinator
{
public:
    static FSystemEventCoordinator& Get()
    {
        static FSystemEventCoordinator Instance;
        return Instance;
    }

    // Event broadcasting
    FOnSystemInitialized OnSystemInitialized;
    FOnSystemShutdown OnSystemShutdown;

    // Track initialized systems
    TSet<ESystemType> InitializedSystems;
    TMap<ESystemType, FString> SystemNames;

    void BroadcastSystemInitialized(ESystemType SystemType, bool bSuccess, const FString& SystemName)
    {
        FSystemInitializationEvent Event(SystemType, bSuccess, SystemName);
        
        if (bSuccess)
        {
            InitializedSystems.Add(SystemType);
            SystemNames.Add(SystemType, SystemName);
        }

        OnSystemInitialized.Broadcast(Event);
        
        UE_LOG(LogTemp, Log, TEXT("System Event: %s %s"), 
               *SystemName, 
               bSuccess ? TEXT("initialized successfully") : TEXT("failed to initialize"));
    }

    void BroadcastSystemShutdown(ESystemType SystemType, const FString& SystemName)
    {
        FSystemInitializationEvent Event(SystemType, true, SystemName);
        InitializedSystems.Remove(SystemType);
        SystemNames.Remove(SystemType);
        
        OnSystemShutdown.Broadcast(Event);
        
        UE_LOG(LogTemp, Log, TEXT("System Event: %s shutdown"), *SystemName);
    }

    bool IsSystemInitialized(ESystemType SystemType) const
    {
        return InitializedSystems.Contains(SystemType);
    }

    TArray<ESystemType> GetInitializedSystems() const
    {
        return InitializedSystems.Array();
    }

private:
    FSystemEventCoordinator() = default;
};