// Private/Core/RadiantGameState.cpp
// Game state implementation for RadiantRPG - manages replicated world state

#include "Core/RadiantGameState.h"
#include "Core/RadiantGameManager.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

ARadiantGameState::ARadiantGameState()
{
    // Enable replication
    bReplicates = true;
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 1.0f; // Update every second for world state

    // Initialize world time
    WorldTime = FWorldTimeData();
    SecondsPerGameDay = 1440.0f; // 24 minutes = 1 game day (60x speed)
    bAutoUpdateTimeOfDay = true;

    // TODO: Initialize weather system when implemented
    // WorldWeather = FWorldWeatherData();
    // bDynamicWeather = true;
    // WeatherChangeChance = 0.05f; // 5% chance per minute

    // Initialize event system
    MaxConcurrentEvents = 10;

    // Initialize references
    GameManager = nullptr;

    // Initialize internal state
    // TODO: Weather placeholders
    // LastWeatherUpdateTime = 0.0f;
    LastEventUpdateTime = 0.0f;
    NextEventID = 1;
    bGameStateInitialized = false;

    UE_LOG(LogTemp, Log, TEXT("RadiantGameState constructed"));
}

void ARadiantGameState::BeginPlay()
{
    Super::BeginPlay();

    // Cache game manager reference using UE 5.5 method
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        GameManager = GameInstance->GetSubsystem<URadiantGameManager>();
    }

    // Initialize world state if we're the authority
    if (HasAuthority())
    {
        InitializeWorldState();
    }

    bGameStateInitialized = true;
    UE_LOG(LogTemp, Log, TEXT("RadiantGameState BeginPlay completed"));
}

void ARadiantGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bGameStateInitialized = false;
    UE_LOG(LogTemp, Log, TEXT("RadiantGameState EndPlay"));
    Super::EndPlay(EndPlayReason);
}

void ARadiantGameState::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    UE_LOG(LogTemp, Log, TEXT("RadiantGameState PostInitializeComponents"));
}

void ARadiantGameState::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bGameStateInitialized)
        return;

    // Only update on authority
    if (HasAuthority())
    {
        UpdateWorldTime(DeltaTime);
        UpdateWorldEvents(DeltaTime);
        
        // TODO: Weather updates when system is implemented
        // UpdateWorldWeather(DeltaTime);
    }
}

void ARadiantGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARadiantGameState, WorldTime);
    // TODO: Weather replication when implemented
    // DOREPLIFETIME(ARadiantGameState, WorldWeather);
    DOREPLIFETIME(ARadiantGameState, KnownFactions);
    DOREPLIFETIME(ARadiantGameState, ActiveWorldEvents);
    DOREPLIFETIME(ARadiantGameState, GlobalFlags);
    DOREPLIFETIME(ARadiantGameState, ActiveWars);
}

// === INTERNAL INITIALIZATION ===

void ARadiantGameState::InitializeWorldState()
{
    if (!HasAuthority())
        return;

    // Initialize time system
    WorldTime.GameTimeSeconds = 0.0f;
    WorldTime.GameDay = 1;
    WorldTime.TimeOfDay = ETimeOfDay::Dawn;
    WorldTime.TimeScale = 60.0f; // 60x speed

    // Initialize faction system
    InitializeKnownFactions();

    // Clear any existing world events
    ActiveWorldEvents.Empty();
    ActiveWars.Empty();

    UE_LOG(LogTemp, Log, TEXT("World state initialized"));
}

// === WORLD TIME IMPLEMENTATION ===

void ARadiantGameState::SetWorldTime(const FWorldTimeData& NewTimeData)
{
    if (!HasAuthority())
        return;

    WorldTime = NewTimeData;
    UpdateTimeOfDay();
    
    OnWorldTimeChanged.Broadcast(WorldTime);
    OnWorldTimeChangedBP(WorldTime);
}

void ARadiantGameState::AddWorldTime(float SecondsToAdd)
{
    if (!HasAuthority())
        return;

    WorldTime.GameTimeSeconds += SecondsToAdd;
    
    // Check for day transitions
    float SecondsInDay = SecondsPerGameDay;
    if (SecondsInDay > 0.0f)
    {
        int32 NewDay = FMath::FloorToInt(WorldTime.GameTimeSeconds / SecondsInDay) + 1;
        if (NewDay != WorldTime.GameDay)
        {
            WorldTime.GameDay = NewDay;
            UE_LOG(LogTemp, Log, TEXT("New game day: %d"), WorldTime.GameDay);
        }
    }

    UpdateTimeOfDay();
}

void ARadiantGameState::SetTimeScale(float NewTimeScale)
{
    if (!HasAuthority())
        return;

    WorldTime.TimeScale = FMath::Max(0.1f, NewTimeScale);
    UE_LOG(LogTemp, Log, TEXT("Time scale set to: %.2f"), WorldTime.TimeScale);
}

void ARadiantGameState::SetTimePaused(bool bPaused)
{
    if (!HasAuthority())
        return;

    // Store the pause state in time scale (0 = paused)
    if (bPaused && WorldTime.TimeScale > 0.0f)
    {
        // Store previous scale and pause
        // For now, we'll just set to very small value instead of 0
        WorldTime.TimeScale = 0.001f;
    }
    else if (!bPaused && WorldTime.TimeScale <= 0.001f)
    {
        // Resume with default scale
        WorldTime.TimeScale = 60.0f;
    }

    UE_LOG(LogTemp, Log, TEXT("Time %s"), bPaused ? TEXT("paused") : TEXT("resumed"));
}

float ARadiantGameState::GetTimeOfDayFloat() const
{
    if (SecondsPerGameDay <= 0.0f)
        return 0.0f;

    return FMath::Fmod(WorldTime.GameTimeSeconds, SecondsPerGameDay) / SecondsPerGameDay;
}

bool ARadiantGameState::IsDaytime() const
{
    switch (WorldTime.TimeOfDay)
    {
        case ETimeOfDay::Dawn:
        case ETimeOfDay::Morning:
        case ETimeOfDay::Midday:
        case ETimeOfDay::Noon:
            return true;
        default:
            return false;
    }
}

bool ARadiantGameState::IsNighttime() const
{
    return !IsDaytime();
}

void ARadiantGameState::UpdateWorldTime(float DeltaTime)
{
    if (!bAutoUpdateTimeOfDay || WorldTime.TimeScale <= 0.0f)
        return;

    // Update game time
    float PreviousTime = WorldTime.GameTimeSeconds;
    WorldTime.GameTimeSeconds += DeltaTime * WorldTime.TimeScale;

    // Check for day transitions
    if (SecondsPerGameDay > 0.0f)
    {
        int32 PreviousDay = FMath::FloorToInt(PreviousTime / SecondsPerGameDay) + 1;
        int32 CurrentDay = FMath::FloorToInt(WorldTime.GameTimeSeconds / SecondsPerGameDay) + 1;
        
        if (CurrentDay != PreviousDay)
        {
            WorldTime.GameDay = CurrentDay;
            UE_LOG(LogTemp, Log, TEXT("Day changed to: %d"), WorldTime.GameDay);
        }
    }

    // Update time of day
    ETimeOfDay PreviousTimeOfDay = WorldTime.TimeOfDay;
    UpdateTimeOfDay();

    // Broadcast time changes
    if (PreviousTimeOfDay != WorldTime.TimeOfDay)
    {
        OnWorldTimeChanged.Broadcast(WorldTime);
        OnWorldTimeChangedBP(WorldTime);
    }
}

void ARadiantGameState::UpdateTimeOfDay()
{
    ETimeOfDay NewTimeOfDay = CalculateTimeOfDay(WorldTime.GameTimeSeconds);
    if (NewTimeOfDay != WorldTime.TimeOfDay)
    {
        WorldTime.TimeOfDay = NewTimeOfDay;
    }
}

ETimeOfDay ARadiantGameState::CalculateTimeOfDay(float GameTimeSeconds) const
{
    if (SecondsPerGameDay <= 0.0f)
        return ETimeOfDay::Dawn;

    float TimeOfDayFloat = FMath::Fmod(GameTimeSeconds, SecondsPerGameDay) / SecondsPerGameDay;
    
    if (TimeOfDayFloat < 0.125f)        // 0-3 hours (midnight-3am)
        return ETimeOfDay::Midnight;
    else if (TimeOfDayFloat < 0.25f)    // 3-6 hours (3am-6am)
        return ETimeOfDay::Night;
    else if (TimeOfDayFloat < 0.375f)   // 6-9 hours (6am-9am)
        return ETimeOfDay::Dawn;
    else if (TimeOfDayFloat < 0.5f)     // 9-12 hours (9am-noon)
        return ETimeOfDay::Morning;
    else if (TimeOfDayFloat < 0.625f)   // 12-15 hours (noon-3pm)
        return ETimeOfDay::Noon;
    else if (TimeOfDayFloat < 0.75f)    // 15-18 hours (3pm-6pm)
        return ETimeOfDay::Midday;
    else if (TimeOfDayFloat < 0.875f)   // 18-21 hours (6pm-9pm)
        return ETimeOfDay::Dusk;
    else                                // 21-24 hours (9pm-midnight)
        return ETimeOfDay::Evening;
}

// === FACTION SYSTEM IMPLEMENTATION ===

float ARadiantGameState::GetFactionRelationship(FGameplayTag FactionA, FGameplayTag FactionB) const
{
    const FFactionRelationshipData& Relationship = FindFactionRelationship(FactionA, FactionB);
    return Relationship.ReputationValue;
}

void ARadiantGameState::SetFactionRelationship(FGameplayTag FactionA, FGameplayTag FactionB, float RelationshipValue)
{
    if (!HasAuthority())
        return;

    FString Key = GetFactionRelationshipKey(FactionA, FactionB);
    
    FFactionRelationshipData* Relationship = FactionRelationships.Find(Key);
    if (!Relationship)
    {
        // Create new relationship
        FFactionRelationshipData NewRelationship;
        NewRelationship.ReputationValue = RelationshipValue;
        FactionRelationships.Add(Key, NewRelationship);
        Relationship = &FactionRelationships[Key];
    }
    else
    {
        Relationship->ReputationValue = RelationshipValue;
    }

    // Update relationship enum based on value
    if (RelationshipValue <= -75.0f)
        Relationship->Relationship = EFactionRelationship::Enemy;
    else if (RelationshipValue <= -25.0f)
        Relationship->Relationship = EFactionRelationship::Hostile;
    else if (RelationshipValue <= -5.0f)
        Relationship->Relationship = EFactionRelationship::Unfriendly;
    else if (RelationshipValue <= 5.0f)
        Relationship->Relationship = EFactionRelationship::Neutral;
    else if (RelationshipValue <= 25.0f)
        Relationship->Relationship = EFactionRelationship::Friendly;
    else
        Relationship->Relationship = EFactionRelationship::Allied;

    // Broadcast relationship change
    OnFactionRelationshipChanged.Broadcast(FactionA, FactionB);
    OnFactionRelationshipChangedBP(FactionA, FactionB, RelationshipValue);

    UE_LOG(LogTemp, Log, TEXT("Faction relationship updated: %s <-> %s = %.2f"), 
           *FactionA.ToString(), *FactionB.ToString(), RelationshipValue);
}

const FFactionRelationshipData& ARadiantGameState::FindFactionRelationship(FGameplayTag FactionA, FGameplayTag FactionB) const
{
    FString Key = GetFactionRelationshipKey(FactionA, FactionB);
    
    // Try to find existing relationship
    if (const FFactionRelationshipData* FoundRelationship = FactionRelationships.Find(Key))
    {
        return *FoundRelationship;
    }
    
    // Return static default neutral relationship if not found
    static const FFactionRelationshipData DefaultNeutralRelationship;
    return DefaultNeutralRelationship;
}

void ARadiantGameState::DeclareFactionWar(FGameplayTag FactionA, FGameplayTag FactionB)
{
    if (!HasAuthority())
        return;

    // Add to active wars list
    FString WarTag = FString::Printf(TEXT("%s_vs_%s"), *FactionA.ToString(), *FactionB.ToString());
    FGameplayTag WarGameplayTag = FGameplayTag::RequestGameplayTag(FName(*WarTag));
    ActiveWars.AddUnique(WarGameplayTag);

    UE_LOG(LogTemp, Warning, TEXT("War declared between %s and %s"), 
           *FactionA.ToString(), *FactionB.ToString());
}

void ARadiantGameState::EndFactionWar(FGameplayTag FactionA, FGameplayTag FactionB)
{
    if (!HasAuthority())
        return;

    // Mark as no longer at war
    FFactionRelationshipData* Relationship = FactionRelationships.Find(GetFactionRelationshipKey(FactionA, FactionB));
    if (Relationship)
    {
        // Improve relationship slightly (but still hostile)
        Relationship->ReputationValue = FMath::Max(Relationship->ReputationValue, -50.0f);
    }

    // Remove from active wars
    FString WarTag = FString::Printf(TEXT("%s_vs_%s"), *FactionA.ToString(), *FactionB.ToString());
    FGameplayTag WarGameplayTag = FGameplayTag::RequestGameplayTag(FName(*WarTag));
    ActiveWars.Remove(WarGameplayTag);

    UE_LOG(LogTemp, Log, TEXT("War ended between %s and %s"), 
           *FactionA.ToString(), *FactionB.ToString());
}

TArray<FGameplayTag> ARadiantGameState::GetFactionsAtWar() const
{
    return ActiveWars;
}

void ARadiantGameState::InitializeKnownFactions()
{
    // Initialize default factions - this would typically be loaded from data tables
    KnownFactions.Empty();
    
    // Add common faction tags
    KnownFactions.Add(FGameplayTag::RequestGameplayTag(FName("Faction.Empire")).ToString());
    KnownFactions.Add(FGameplayTag::RequestGameplayTag(FName("Faction.Rebels")).ToString());
    KnownFactions.Add(FGameplayTag::RequestGameplayTag(FName("Faction.Merchants")).ToString());
    KnownFactions.Add(FGameplayTag::RequestGameplayTag(FName("Faction.Bandits")).ToString());
    KnownFactions.Add(FGameplayTag::RequestGameplayTag(FName("Faction.Wildlife")).ToString());

    UE_LOG(LogTemp, Log, TEXT("Initialized %d known factions"), KnownFactions.Num());
}

FString ARadiantGameState::GetFactionRelationshipKey(FGameplayTag FactionA, FGameplayTag FactionB) const
{
    // Ensure consistent key ordering
    FString NameA = FactionA.ToString();
    FString NameB = FactionB.ToString();
    
    if (NameA.Compare(NameB) < 0)
        return FString::Printf(TEXT("%s|%s"), *NameA, *NameB);
    else
        return FString::Printf(TEXT("%s|%s"), *NameB, *NameA);
}

// === WORLD EVENTS IMPLEMENTATION ===

bool ARadiantGameState::StartWorldEvent(const FWorldEventData& EventData)
{
    if (!HasAuthority())
        return false;

    // Validate event data
    if (!ValidateWorldEventData(EventData))
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid world event data provided"));
        return false;
    }

    // Check if we're at max concurrent events
    if (ActiveWorldEvents.Num() >= MaxConcurrentEvents)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot start world event - at maximum concurrent events (%d)"), 
               MaxConcurrentEvents);
        return false;
    }

    // Check if event ID already exists
    for (const FWorldEventData& ExistingEvent : ActiveWorldEvents)
    {
        if (ExistingEvent.EventID == EventData.EventID)
        {
            UE_LOG(LogTemp, Warning, TEXT("World event ID already exists: %s"), *EventData.EventID);
            return false;
        }
    }

    // Create new event with current timestamp
    FWorldEventData NewEvent = EventData;
    NewEvent.EventStartTime = GetWorld()->GetTimeSeconds();
    NewEvent.bIsActive = true;

    // Generate ID if not provided
    if (NewEvent.EventID.IsEmpty())
    {
        NewEvent.EventID = GenerateEventID();
    }

    // Add to active events
    ActiveWorldEvents.Add(NewEvent);

    // Broadcast event started
    OnWorldEventStarted.Broadcast(NewEvent);
    OnWorldEventStartedBP(NewEvent);

    UE_LOG(LogTemp, Log, TEXT("World event started: %s at %s"), 
           *NewEvent.EventID, *NewEvent.EventLocation.ToString());

    return true;
}

bool ARadiantGameState::EndWorldEvent(const FString& EventID)
{
    if (!HasAuthority())
        return false;

    for (int32 i = ActiveWorldEvents.Num() - 1; i >= 0; i--)
    {
        if (ActiveWorldEvents[i].EventID == EventID)
        {
            FWorldEventData EndedEvent = ActiveWorldEvents[i];
            ActiveWorldEvents.RemoveAt(i);

            // Broadcast event ended
            OnWorldEventEnded.Broadcast(EndedEvent);
            OnWorldEventEndedBP(EndedEvent);

            UE_LOG(LogTemp, Log, TEXT("World event ended: %s"), *EventID);
            return true;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("World event not found for ending: %s"), *EventID);
    return false;
}

const FWorldEventData& ARadiantGameState::FindWorldEvent(const FString& EventID) const
{
    for (const FWorldEventData& Event : ActiveWorldEvents)
    {
        if (Event.EventID == EventID)
        {
            return Event;
        }
    }
    static const FWorldEventData DefaultEmptyEvent;
    return DefaultEmptyEvent;
}

bool ARadiantGameState::IsWorldEventActive(const FString& EventID) const
{
    const FWorldEventData& FoundEvent = FindWorldEvent(EventID);
    return !FoundEvent.EventID.IsEmpty() && FoundEvent.EventID == EventID;
}

void ARadiantGameState::UpdateWorldEvents(float DeltaTime)
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    // Update events every 10 seconds
    if (CurrentTime - LastEventUpdateTime < 10.0f)
        return;
        
    LastEventUpdateTime = CurrentTime;

    // Check for expired events
    for (int32 i = ActiveWorldEvents.Num() - 1; i >= 0; i--)
    {
        FWorldEventData& Event = ActiveWorldEvents[i];
        
        // Skip indefinite events
        if (Event.EventDuration < 0.0f)
            continue;
            
        // Check if event has expired
        float EventAge = CurrentTime - Event.EventStartTime;
        if (EventAge >= Event.EventDuration)
        {
            EndWorldEvent(Event.EventID);
        }
    }
}

bool ARadiantGameState::ValidateWorldEventData(const FWorldEventData& EventData) const
{
    // Basic validation
    if (!EventData.EventType.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("World event has invalid event type"));
        return false;
    }
    
    if (EventData.EventRadius <= 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("World event has invalid radius: %.2f"), EventData.EventRadius);
        return false;
    }
    
    return true;
}

FString ARadiantGameState::GenerateEventID()
{
    return FString::Printf(TEXT("Event_%d"), NextEventID++);
}

// === GLOBAL FLAGS IMPLEMENTATION ===

void ARadiantGameState::SetGlobalFlag(FGameplayTag Flag, bool bValue)
{
    if (!HasAuthority())
        return;

    bool bPreviousValue = GlobalFlags.HasTag(Flag);
    
    if (bValue)
    {
        GlobalFlags.AddTag(Flag);
    }
    else
    {
        GlobalFlags.RemoveTag(Flag);
    }

    // Only broadcast if value actually changed
    if (bPreviousValue != bValue)
    {
        OnGlobalFlagChanged.Broadcast(Flag);
        OnGlobalFlagChangedBP(Flag, bValue);
        
        UE_LOG(LogTemp, Log, TEXT("Global flag %s set to %s"), 
               *Flag.ToString(), bValue ? TEXT("true") : TEXT("false"));
    }
}

bool ARadiantGameState::GetGlobalFlag(FGameplayTag Flag) const
{
    return GlobalFlags.HasTag(Flag);
}

void ARadiantGameState::ToggleGlobalFlag(FGameplayTag Flag)
{
    SetGlobalFlag(Flag, !GetGlobalFlag(Flag));
}

// === GLOBAL VARIABLES IMPLEMENTATION ===

void ARadiantGameState::ServerSetGlobalVariable_Implementation(const FString& VariableName, float Value)
{
    GlobalVariables.Add(VariableName, Value);
    UE_LOG(LogTemp, VeryVerbose, TEXT("Global variable %s set to %.2f"), *VariableName, Value);
}

float ARadiantGameState::GetGlobalVariable(const FString& VariableName, float DefaultValue) const
{
    const float* Value = GlobalVariables.Find(VariableName);
    return Value ? *Value : DefaultValue;
}

void ARadiantGameState::ServerSetGlobalString_Implementation(const FString& VariableName, const FString& Value)
{
    GlobalStrings.Add(VariableName, Value);
    UE_LOG(LogTemp, VeryVerbose, TEXT("Global string %s set to %s"), *VariableName, *Value);
}

FString ARadiantGameState::GetGlobalString(const FString& VariableName, const FString& DefaultValue) const
{
    const FString* Value = GlobalStrings.Find(VariableName);
    return Value ? *Value : DefaultValue;
}

// === UTILITY FUNCTIONS ===

FString ARadiantGameState::GetFormattedTimeString() const
{
    int32 Hours = FMath::FloorToInt(WorldTime.GetHours());
    int32 Minutes = FMath::FloorToInt(WorldTime.GetMinutes());
    
    return FString::Printf(TEXT("%02d:%02d"), Hours, Minutes);
}

FString ARadiantGameState::GetFormattedDateString() const
{
    return FString::Printf(TEXT("Day %d"), WorldTime.GameDay);
}

// === REPLICATION CALLBACKS ===

void ARadiantGameState::OnRep_WorldTime()
{
    OnWorldTimeChanged.Broadcast(WorldTime);
    OnWorldTimeChangedBP(WorldTime);
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