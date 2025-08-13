// Private/Core/RadiantGameState.cpp
// Game state implementation for RadiantRPG - manages replicated world state (updated for simplified time)

#include "Core/RadiantGameState.h"
#include "Core/RadiantGameManager.h"
#include "World/RadiantWorldManager.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

ARadiantGameState::ARadiantGameState()
{
    // Enable replication
    bReplicates = true;
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 1.0f; // Update every second for world state

    // Initialize simplified world time
    WorldTime = FSimpleWorldTime();
    bTimeProgressionEnabled = true;

    // Initialize event system
    MaxConcurrentEvents = 10;

    // Initialize references
    GameManager = nullptr;
    WorldManager = nullptr;

    // Initialize internal state
    LastEventUpdateTime = 0.0f;
    NextEventID = 1;
    bGameStateInitialized = false;

    UE_LOG(LogTemp, Log, TEXT("RadiantGameState constructed with simplified time system"));
}

void ARadiantGameState::BeginPlay()
{
    Super::BeginPlay();

    // Cache manager references using UE 5.5 method
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        GameManager = GameInstance->GetSubsystem<URadiantGameManager>();
        WorldManager = GameInstance->GetSubsystem<URadiantWorldManager>();
    }

    // Initialize world state if we're the authority
    if (HasAuthority())
    {
        InitializeWorldState();
    }

    bGameStateInitialized = true;
    UE_LOG(LogTemp, Log, TEXT("RadiantGameState BeginPlay completed - %s"), 
           HasAuthority() ? TEXT("Authority") : TEXT("Client"));
}

void ARadiantGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UE_LOG(LogTemp, Log, TEXT("RadiantGameState EndPlay"));
    
    // Clear references
    GameManager = nullptr;
    WorldManager = nullptr;
    
    Super::EndPlay(EndPlayReason);
}

void ARadiantGameState::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bGameStateInitialized)
    {
        return;
    }

    // Only authority updates world state
    if (HasAuthority())
    {
        UpdateWorldState(DeltaTime);
    }
}

void ARadiantGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Replicate world time
    DOREPLIFETIME(ARadiantGameState, WorldTime);
    DOREPLIFETIME(ARadiantGameState, bTimeProgressionEnabled);

    // Replicate world events
    DOREPLIFETIME(ARadiantGameState, ActiveWorldEvents);

    // Replicate global state
    DOREPLIFETIME(ARadiantGameState, GlobalFlags);
    DOREPLIFETIME(ARadiantGameState, GlobalVariables);
    DOREPLIFETIME(ARadiantGameState, GlobalStrings);

    // Replicate faction data
    DOREPLIFETIME(ARadiantGameState, ActiveWars);
}

// === WORLD TIME INTERFACE (DELEGATED TO WORLDMANAGER) ===

void ARadiantGameState::SetWorldTime(const FSimpleWorldTime& NewTimeData)
{
    if (!HasAuthority())
        return;

    // Delegate to WorldManager if available
    if (WorldManager)
    {
        WorldManager->SetWorldTime(NewTimeData);
        // WorldManager will update our replicated state
    }
    else
    {
        // Fallback: update replicated state directly
        WorldTime = NewTimeData;
        OnRep_WorldTime();
    }
}

void ARadiantGameState::AddWorldTime(float SecondsToAdd)
{
    if (!HasAuthority())
        return;

    // Delegate to WorldManager if available
    if (WorldManager)
    {
        WorldManager->AdvanceTime(SecondsToAdd);
        // WorldManager will update our replicated state
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AddWorldTime called but WorldManager not available"));
    }
}

void ARadiantGameState::SetTimeScale(float NewTimeScale)
{
    if (!HasAuthority())
        return;

    // Delegate to WorldManager if available
    if (WorldManager)
    {
        WorldManager->SetTimeScale(NewTimeScale);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SetTimeScale called but WorldManager not available"));
    }
}

void ARadiantGameState::SetTimePaused(bool bPaused)
{
    if (!HasAuthority())
        return;

    // Delegate to WorldManager if available
    if (WorldManager)
    {
        WorldManager->SetTimePaused(bPaused);
        bTimeProgressionEnabled = !bPaused;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SetTimePaused called but WorldManager not available"));
    }
}

void ARadiantGameState::SyncTimeFromWorldManager()
{
    if (!HasAuthority() || !WorldManager)
        return;

    // Sync our replicated time with WorldManager's authoritative time
    FSimpleWorldTime NewTime = WorldManager->GetWorldTime();
    if (WorldTime.Season != NewTime.Season || 
        WorldTime.Day != NewTime.Day || 
        WorldTime.Hour != NewTime.Hour || 
        WorldTime.Minute != NewTime.Minute)
    {
        WorldTime = NewTime;
        bTimeProgressionEnabled = !WorldManager->IsTimePaused();
        
        // Force replication update
        OnRep_WorldTime();
    }
}

// === WORLD EVENTS INTERFACE ===

void ARadiantGameState::AddWorldEvent(const FWorldEventData& EventData)
{
    if (!HasAuthority())
        return;

    if (!ValidateWorldEventData(EventData))
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid world event data provided"));
        return;
    }

    // Check max concurrent events
    if (ActiveWorldEvents.Num() >= MaxConcurrentEvents)
    {
        UE_LOG(LogTemp, Warning, TEXT("Maximum concurrent events (%d) reached"), MaxConcurrentEvents);
        return;
    }

    FWorldEventData NewEvent = EventData;
    NewEvent.EventID = NextEventID++;
    NewEvent.StartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

    ActiveWorldEvents.Add(NewEvent);
    
    OnWorldEventStarted.Broadcast(NewEvent);
    OnWorldEventStartedBP(NewEvent);

    UE_LOG(LogTemp, Log, TEXT("World event added: ID %d"), NewEvent.EventID);
}

bool ARadiantGameState::RemoveWorldEvent(int32 EventID)
{
    if (!HasAuthority())
        return false;

    int32 RemovedCount = ActiveWorldEvents.RemoveAll([EventID](const FWorldEventData& Event)
    {
        return Event.EventID == EventID;
    });

    if (RemovedCount > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("World event removed: ID %d"), EventID);
        return true;
    }

    return false;
}

bool ARadiantGameState::IsEventActive(int32 EventID) const
{
    return ActiveWorldEvents.ContainsByPredicate([EventID](const FWorldEventData& Event)
    {
        return Event.EventID == EventID;
    });
}

// === FACTION INTERFACE ===

void ARadiantGameState::StartWar(FGameplayTag FactionA, FGameplayTag FactionB)
{
    if (!HasAuthority())
        return;

    if (!FactionA.IsValid() || !FactionB.IsValid() || FactionA == FactionB)
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid faction tags for war"));
        return;
    }

    // Generate war tag
    FString WarString = FString::Printf(TEXT("%s_vs_%s"), *FactionA.ToString(), *FactionB.ToString());
    FGameplayTag WarTag = FGameplayTag::RequestGameplayTag(*WarString);

    if (!ActiveWars.Contains(WarTag))
    {
        ActiveWars.Add(WarTag);
        OnFactionRelationshipChanged.Broadcast(FactionA, FactionB);
        OnFactionRelationshipChangedBP(FactionA, FactionB, -100.0f); // War = -100 relationship

        UE_LOG(LogTemp, Log, TEXT("War started: %s vs %s"), *FactionA.ToString(), *FactionB.ToString());
    }
}

void ARadiantGameState::EndWar(FGameplayTag FactionA, FGameplayTag FactionB)
{
    if (!HasAuthority())
        return;

    // Generate both possible war tags
    FString WarStringAB = FString::Printf(TEXT("%s_vs_%s"), *FactionA.ToString(), *FactionB.ToString());
    FString WarStringBA = FString::Printf(TEXT("%s_vs_%s"), *FactionB.ToString(), *FactionA.ToString());
    
    FGameplayTag WarTagAB = FGameplayTag::RequestGameplayTag(*WarStringAB);
    FGameplayTag WarTagBA = FGameplayTag::RequestGameplayTag(*WarStringBA);

    bool bWarEnded = false;
    bWarEnded |= ActiveWars.Remove(WarTagAB) > 0;
    bWarEnded |= ActiveWars.Remove(WarTagBA) > 0;

    if (bWarEnded)
    {
        OnFactionRelationshipChanged.Broadcast(FactionA, FactionB);
        OnFactionRelationshipChangedBP(FactionA, FactionB, 0.0f); // Peace = neutral relationship

        UE_LOG(LogTemp, Log, TEXT("War ended: %s vs %s"), *FactionA.ToString(), *FactionB.ToString());
    }
}

bool ARadiantGameState::AreFactionsAtWar(FGameplayTag FactionA, FGameplayTag FactionB) const
{
    if (!FactionA.IsValid() || !FactionB.IsValid() || FactionA == FactionB)
    {
        return false;
    }

    // Generate both possible war tags
    FString WarStringAB = FString::Printf(TEXT("%s_vs_%s"), *FactionA.ToString(), *FactionB.ToString());
    FString WarStringBA = FString::Printf(TEXT("%s_vs_%s"), *FactionB.ToString(), *FactionA.ToString());
    
    FGameplayTag WarTagAB = FGameplayTag::RequestGameplayTag(*WarStringAB);
    FGameplayTag WarTagBA = FGameplayTag::RequestGameplayTag(*WarStringBA);

    return ActiveWars.Contains(WarTagAB) || ActiveWars.Contains(WarTagBA);
}

// === GLOBAL STATE INTERFACE ===

void ARadiantGameState::SetGlobalFlag(FGameplayTag Flag, bool bValue)
{
    if (!HasAuthority())
        return;

    if (!Flag.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid flag provided"));
        return;
    }

    bool bPreviousValue = GlobalFlags.HasTag(Flag);

    if (bValue && !bPreviousValue)
    {
        GlobalFlags.AddTag(Flag);
        OnGlobalFlagChanged.Broadcast(Flag);
        OnGlobalFlagChangedBP(Flag, bValue);
        
        UE_LOG(LogTemp, Log, TEXT("Global flag set: %s"), *Flag.ToString());
    }
    else if (!bValue && bPreviousValue)
    {
        GlobalFlags.RemoveTag(Flag);
        OnGlobalFlagChanged.Broadcast(Flag);
        OnGlobalFlagChangedBP(Flag, bValue);
        
        UE_LOG(LogTemp, Log, TEXT("Global flag removed: %s"), *Flag.ToString());
    }
}

void ARadiantGameState::SetGlobalVariable(const FString& VariableName, float Value)
{
    if (!HasAuthority())
    {
        ServerSetGlobalVariable(VariableName, Value);
        return;
    }

    GlobalVariables.SetVariable(VariableName, Value);
    UE_LOG(LogTemp, Verbose, TEXT("Global variable %s set to %f"), *VariableName, Value);
}

float ARadiantGameState::GetGlobalVariable(const FString& VariableName, float DefaultValue) const
{
    return GlobalVariables.GetVariable(VariableName, DefaultValue);
}

void ARadiantGameState::SetGlobalString(const FString& VariableName, const FString& Value)
{
    if (!HasAuthority())
    {
        ServerSetGlobalString(VariableName, Value);
        return;
    }

    GlobalStrings.SetVariable(VariableName, Value);
    UE_LOG(LogTemp, Verbose, TEXT("Global string %s set to %s"), *VariableName, *Value);
}

FString ARadiantGameState::GetGlobalString(const FString& VariableName, const FString& DefaultValue) const
{
    return GlobalStrings.GetVariable(VariableName);
}

// === INTERNAL FUNCTIONS ===

void ARadiantGameState::InitializeWorldState()
{
    UE_LOG(LogTemp, Log, TEXT("RadiantGameState: Initializing world state (Authority)"));

    // Sync time from WorldManager if available
    if (WorldManager)
    {
        SyncTimeFromWorldManager();
    }
    else
    {
        // Fallback: use default time
        WorldTime = FSimpleWorldTime();
        UE_LOG(LogTemp, Warning, TEXT("WorldManager not available during GameState init"));
    }

    // Initialize collections
    ActiveWorldEvents.Empty();
    GlobalFlags = FGameplayTagContainer();
    GlobalVariables.Empty();
    GlobalStrings.Empty();
    ActiveWars.Empty();

    UE_LOG(LogTemp, Log, TEXT("World state initialized - Time: %s"), *WorldTime.GetFullTimeString());
}

void ARadiantGameState::UpdateWorldState(float DeltaTime)
{
    // Sync time from WorldManager periodically
    static float TimeSyncAccumulator = 0.0f;
    TimeSyncAccumulator += DeltaTime;
    
    if (TimeSyncAccumulator >= 1.0f) // Sync every second
    {
        if (WorldManager)
        {
            SyncTimeFromWorldManager();
        }
        TimeSyncAccumulator = 0.0f;
    }

    // Update world events
    UpdateWorldEvents(DeltaTime);
}

void ARadiantGameState::UpdateWorldEvents(float DeltaTime)
{
    if (!HasAuthority())
        return;

    float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

    // Remove expired events
    TArray<FWorldEventData> ExpiredEvents;
    ActiveWorldEvents.RemoveAll([CurrentTime, &ExpiredEvents](const FWorldEventData& Event)
    {
        bool bExpired = Event.Duration > 0.0f && (CurrentTime - Event.StartTime) >= Event.Duration;
        if (bExpired)
        {
            ExpiredEvents.Add(Event);
        }
        return bExpired;
    });

    // Broadcast expired events
    for (const FWorldEventData& ExpiredEvent : ExpiredEvents)
    {
        OnWorldEventEnded.Broadcast(ExpiredEvent);
        OnWorldEventEndedBP(ExpiredEvent);
        UE_LOG(LogTemp, Log, TEXT("World event expired: ID %d"), ExpiredEvent.EventID);
    }
}

bool ARadiantGameState::ValidateWorldEventData(const FWorldEventData& EventData) const
{
    // Basic validation
    if (EventData.EventType == FGameplayTag())
    {
        return false;
    }

    if (EventData.Duration < 0.0f)
    {
        return false;
    }

    return true;
}

float ARadiantGameState::GetFactionRelationship(FGameplayTag FactionA, FGameplayTag FactionB) const
{
    if (!FactionA.IsValid() || !FactionB.IsValid())
    {
        return 0.0f; // Neutral for invalid factions
    }

    // Same faction relationship
    if (FactionA == FactionB)
    {
        return 1.0f; // Same faction is always allied
    }

    // Check if factions are at war
    if (AreFactionsAtWar(FactionA, FactionB))
    {
        return -1.0f; // War means hostile
    }

    // Check for hierarchical faction relationships (parent/child tags)
    if (FactionA.MatchesTag(FactionB) || FactionB.MatchesTag(FactionA))
    {
        return 1.0f; // Related factions are allied
    }

    // Check for specific faction alliances using includes from RadiantGameplayTags.h
    
    // Kingdom subfactions are allied
    if (FactionA.ToString().Contains(TEXT("Faction.Kingdom")) && FactionB.ToString().Contains(TEXT("Faction.Kingdom")))
    {
        return 0.75f; // Strong alliance
    }

    // Merchant subfactions are allied
    if (FactionA.ToString().Contains(TEXT("Faction.Merchants")) && FactionB.ToString().Contains(TEXT("Faction.Merchants")))
    {
        return 0.75f; // Strong alliance
    }

    // Bandit subfactions are allied
    if (FactionA.ToString().Contains(TEXT("Faction.Bandits")) && FactionB.ToString().Contains(TEXT("Faction.Bandits")))
    {
        return 0.75f; // Strong alliance
    }

    // Villager subfactions are allied
    if (FactionA.ToString().Contains(TEXT("Faction.Villagers")) && FactionB.ToString().Contains(TEXT("Faction.Villagers")))
    {
        return 0.75f; // Strong alliance
    }

    // Bandits are hostile to most other factions
    if (FactionA.ToString().Contains(TEXT("Faction.Bandits")) && !FactionB.ToString().Contains(TEXT("Faction.Bandits")))
    {
        if (!FactionB.ToString().Contains(TEXT("Faction.Neutral")) && !FactionB.ToString().Contains(TEXT("Faction.Wildlife")))
        {
            return -0.5f; // Generally hostile
        }
    }

    if (FactionB.ToString().Contains(TEXT("Faction.Bandits")) && !FactionA.ToString().Contains(TEXT("Faction.Bandits")))
    {
        if (!FactionA.ToString().Contains(TEXT("Faction.Neutral")) && !FactionA.ToString().Contains(TEXT("Faction.Wildlife")))
        {
            return -0.5f; // Generally hostile
        }
    }

    // Wildlife and neutral factions are neutral to most
    if (FactionA.ToString().Contains(TEXT("Faction.Wildlife")) || FactionB.ToString().Contains(TEXT("Faction.Wildlife")) ||
        FactionA.ToString().Contains(TEXT("Faction.Neutral")) || FactionB.ToString().Contains(TEXT("Faction.Neutral")))
    {
        return 0.0f; // Neutral
    }

    // Default neutral relationship
    return 0.0f;
}

FString ARadiantGameState::GetFactionRelationshipKey(FGameplayTag FactionA, FGameplayTag FactionB) const
{
    // Ensure consistent ordering for relationship keys
    if (FactionA.ToString() < FactionB.ToString())
    {
        return FString::Printf(TEXT("%s_%s"), *FactionA.ToString(), *FactionB.ToString());
    }
    else
    {
        return FString::Printf(TEXT("%s_%s"), *FactionB.ToString(), *FactionA.ToString());
    }
}

// === REPLICATION CALLBACKS ===

void ARadiantGameState::OnRep_WorldTime()
{
    OnWorldTimeChanged.Broadcast(WorldTime);
    OnWorldTimeChangedBP(WorldTime);
    
    UE_LOG(LogTemp, Verbose, TEXT("World time replicated: %s"), *WorldTime.GetFullTimeString());
}

void ARadiantGameState::OnRep_ActiveWorldEvents()
{
    UE_LOG(LogTemp, Verbose, TEXT("World events replicated: %d active events"), ActiveWorldEvents.Num());
}

void ARadiantGameState::OnRep_GlobalFlags()
{
    UE_LOG(LogTemp, Verbose, TEXT("Global flags replicated: %d flags"), GlobalFlags.Num());
}

void ARadiantGameState::OnRep_ActiveWars()
{
    UE_LOG(LogTemp, Verbose, TEXT("Active wars replicated: %d wars"), ActiveWars.Num());
}

// === SERVER RPC IMPLEMENTATIONS ===

void ARadiantGameState::ServerSetGlobalVariable_Implementation(const FString& VariableName, float Value)
{
    SetGlobalVariable(VariableName, Value);
}

void ARadiantGameState::ServerSetGlobalString_Implementation(const FString& VariableName, const FString& Value)
{
    SetGlobalString(VariableName, Value);
}