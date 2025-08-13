// Private/World/RadiantWorldManager.cpp
// World simulation manager implementation for RadiantRPG - simplified time system

#include "World/RadiantWorldManager.h"

#include "EngineUtils.h"
#include "World/RadiantZoneManager.h"
#include "World/WorldEventManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DateTime.h"
#include "Types/WorldManagerTypes.h"

URadiantWorldManager::URadiantWorldManager()
{
    // Initialize default settings
    SimulationSettings = FWorldSimulationSettings();
    TimeSettings = FSimpleTimeSettings();
    WeatherSettings = FWeatherSystemSettings();
    
    // Initialize runtime state
    CurrentWorldTime = FSimpleWorldTime();
    CurrentGlobalWeather = FWorldWeatherData();
    SimulationState = FWorldSimulationState();
    
    // Initialize internal state
    bSimulationActive = false;
    bIsInitialized = false;
    TimeSinceLastMajorUpdate = 0.0f;
    WorldEventManager = nullptr;
    
    // Initialize cached values for change detection
    PreviousTimeOfDay = CurrentWorldTime.GetTimeOfDay();
    PreviousSeason = CurrentWorldTime.GetSeason();
    
    UE_LOG(LogTemp, Log, TEXT("RadiantWorldManager constructed with simplified time system"));
}

// === SUBSYSTEM INTERFACE ===

void URadiantWorldManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    UE_LOG(LogTemp, Log, TEXT("RadiantWorldManager initializing..."));
    
    // Initialize core systems
    InitializeSimpleTimeSystem();
    InitializeWeatherSystem();
    InitializeEventSystem();
    InitializeDefaultWorldState();
    
    bIsInitialized = true;
    
    UE_LOG(LogTemp, Log, TEXT("RadiantWorldManager initialized successfully"));
}

void URadiantWorldManager::Deinitialize()
{
    UE_LOG(LogTemp, Log, TEXT("RadiantWorldManager shutting down..."));
    
    ShutdownWorldSimulation();
    
    // Clear all timer handles
    if (UWorld* World = GetWorld())
    {
        FTimerManager& TimerManager = World->GetTimerManager();
        TimerManager.ClearTimer(WeatherUpdateTimer);
        TimerManager.ClearTimer(WorldEventTimer);
        TimerManager.ClearTimer(SimulationSaveTimer);
    }
    
    // Clear references
    RegisteredZones.Empty();
    ActiveWorldEvents.Empty();
    WorldEventManager = nullptr;
    
    bIsInitialized = false;
    
    Super::Deinitialize();
    
    UE_LOG(LogTemp, Log, TEXT("RadiantWorldManager shutdown complete"));
}

bool URadiantWorldManager::ShouldCreateSubsystem(UObject* Outer) const
{
    // Only create in game instances
    return Outer && Outer->IsA<UGameInstance>();
}

// === TICKABLE GAME OBJECT INTERFACE ===

void URadiantWorldManager::Tick(float DeltaTime)
{
    if (!bIsInitialized || !bSimulationActive)
    {
        return;
    }
    
    // Update core world simulation
    UpdateWorldSimulation(DeltaTime);
    
    // Track time for major updates
    TimeSinceLastMajorUpdate += DeltaTime;
    
    // Perform major updates less frequently
    if (TimeSinceLastMajorUpdate >= SimulationSettings.MajorUpdateInterval)
    {
        ProcessScheduledEvents();
        ValidateZoneRegistrations();
        UpdateSimulationMetrics(DeltaTime);
        TimeSinceLastMajorUpdate = 0.0f;
    }
}

bool URadiantWorldManager::IsTickable() const
{
    return bIsInitialized && bSimulationActive && !IsTemplate();
}

TStatId URadiantWorldManager::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(URadiantWorldManager, STATGROUP_Tickables);
}

// === WORLD MANAGER INTERFACE IMPLEMENTATION ===

void URadiantWorldManager::InitializeWorldSimulation()
{
    if (bSimulationActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("World simulation already active"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Initializing world simulation..."));
    
    bSimulationActive = true;
    
    // Set up periodic timers
    if (UWorld* World = GetWorld())
    {
        FTimerManager& TimerManager = World->GetTimerManager();
        
        // Weather update timer
        TimerManager.SetTimer(WeatherUpdateTimer, this, &URadiantWorldManager::UpdateWeatherSystem,
                             WeatherSettings.UpdateInterval, true);
        
        // Auto-save timer (if enabled)
        if (SimulationSettings.AutoSaveInterval > 0.0f)
        {
            TimerManager.SetTimer(SimulationSaveTimer, [this]()
            {
                // TODO: Implement auto-save
                UE_LOG(LogTemp, Log, TEXT("Auto-save triggered"));
            }, SimulationSettings.AutoSaveInterval, true);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("World simulation initialized"));
}

void URadiantWorldManager::ShutdownWorldSimulation()
{
    if (!bSimulationActive)
    {
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Shutting down world simulation..."));
    
    bSimulationActive = false;
    
    UE_LOG(LogTemp, Log, TEXT("World simulation shutdown complete"));
}

void URadiantWorldManager::UpdateWorldSimulation(float DeltaTime)
{
    // Update time progression
    UpdateTimeProgression(DeltaTime);
    
    // Update weather transitions
    CalculateWeatherTransition(DeltaTime);
}

FWorldState URadiantWorldManager::GetWorldStateSnapshot() const
{
    FWorldState WorldState;
    
    // Store simple time data directly
    WorldState.CurrentTime = CurrentWorldTime;
    WorldState.bTimeProgressionEnabled = TimeSettings.bAutoTimeProgression;
    
    // Store weather data
    WorldState.GlobalWeather = CurrentGlobalWeather;
    
    // Store zone data and weather overrides
    for (const auto& ZonePair : RegisteredZones)
    {
        if (ZonePair.Value && ZonePair.Key.IsValid())
        {
            FString ZoneKey = ZonePair.Key.ToString();
            
            // Create zone data from the zone actor
            FZoneData ZoneData;
            ZoneData.ZoneID = *ZoneKey;
            ZoneData.DisplayName = FText::FromString(ZoneKey);
            ZoneData.ZoneType = ZonePair.Value->GetZoneType();
            ZoneData.ControllingFaction = ZonePair.Value->GetControllingFaction().GetTagName();
            
            WorldState.Zones.Add(ZoneKey, ZoneData);
            
            // Store zone population
            WorldState.ZonePopulations.Add(ZoneKey, ZonePair.Value->GetActorsInZone().Num());
            
            // Store zone weather if different from global
            EZoneWeather ZoneWeather = ZonePair.Value->GetCurrentWeather();
            if (ZoneWeather != static_cast<EZoneWeather>(CurrentGlobalWeather.CurrentWeather))
            {
                FWorldWeatherData ZoneWeatherData = CurrentGlobalWeather;
                ZoneWeatherData.CurrentWeather = static_cast<EWeatherType>(ZoneWeather);
                ZoneWeatherData.TargetWeather = static_cast<EWeatherType>(ZoneWeather);
                WorldState.ZoneWeatherOverrides.Add(ZoneKey, ZoneWeatherData);
            }
        }
    }
    
    // Store active events
    for (const FActiveWorldEvent& Event : ActiveWorldEvents)
    {
        WorldState.ActiveWorldEvents.AddTag(Event.EventTag);
        
        // Store event metadata if available
        if (Event.Instigator)
        {
            FString EventKey = Event.EventTag.ToString();
            FString InstigatorName = Event.Instigator->GetName();
            WorldState.EventMetadata.Add(EventKey + "_Instigator", InstigatorName);
        }
    }
    
    // Store simulation metadata
    WorldState.SimulationVersion = 1; // Current version
    WorldState.StateTimestamp = FDateTime::Now();
    WorldState.TotalSimulationTime = CurrentWorldTime.TotalGameSeconds;
    
    // Store simulation flags
    if (bSimulationActive)
    {
        WorldState.SimulationFlags.AddTag(FGameplayTag::RequestGameplayTag("Simulation.Active"));
    }
    if (WeatherSettings.bDynamicWeather)
    {
        WorldState.SimulationFlags.AddTag(FGameplayTag::RequestGameplayTag("Weather.Dynamic"));
    }
    
    UE_LOG(LogTemp, Log, TEXT("World state snapshot created: %s with %d zones, %d events"), 
           *WorldState.GetTimestampString(), WorldState.GetZoneCount(), WorldState.GetActiveEventCount());
    
    return WorldState;
}

void URadiantWorldManager::ApplyWorldStateSnapshot(const FWorldState& WorldState)
{
    UE_LOG(LogTemp, Log, TEXT("Applying world state snapshot from: %s"), *WorldState.GetTimestampString());
    
    // Check version compatibility
    if (!WorldState.IsCompatibleVersion(1))
    {
        UE_LOG(LogTemp, Warning, TEXT("World state version mismatch. Expected: 1, Got: %d"), 
               WorldState.SimulationVersion);
    }
    
    // Apply simple time data
    ETimeOfDay OldTimeOfDay = CurrentWorldTime.GetTimeOfDay();
    ESeason OldSeason = CurrentWorldTime.GetSeason();
    
    CurrentWorldTime = WorldState.CurrentTime;
    TimeSettings.bAutoTimeProgression = WorldState.bTimeProgressionEnabled;
    
    // Process time change events
    ProcessTimeChangeEvents(OldTimeOfDay, OldSeason);
    
    // Apply weather data
    CurrentGlobalWeather = WorldState.GlobalWeather;
    OnGlobalWeatherChanged(CurrentGlobalWeather);
    
    // Apply zone weather overrides
    for (const auto& WeatherPair : WorldState.ZoneWeatherOverrides)
    {
        FGameplayTag ZoneTag = FGameplayTag::RequestGameplayTag(*WeatherPair.Key);
        if (ARadiantZoneManager* Zone = GetZoneByTag(ZoneTag))
        {
            EZoneWeather ZoneWeather = static_cast<EZoneWeather>(WeatherPair.Value.CurrentWeather);
            Zone->SetWeather(ZoneWeather);
        }
    }
    
    // Apply zone data
    for (const auto& ZonePair : WorldState.Zones)
    {
        FGameplayTag ZoneTag = FGameplayTag::RequestGameplayTag(*ZonePair.Key);
        if (ARadiantZoneManager* Zone = GetZoneByTag(ZoneTag))
        {
            const FZoneData& ZoneData = ZonePair.Value;
            
            // Apply faction control
            if (!ZoneData.ControllingFaction.IsNone())
            {
                FString FactionString = ZoneData.ControllingFaction.ToString();
                FGameplayTag FactionTag = FGameplayTag::RequestGameplayTag(*FactionString);
                Zone->SetControllingFaction(FactionTag);
            }
            
            UE_LOG(LogTemp, Verbose, TEXT("Applied state to zone: %s"), *ZonePair.Key);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Zone not found for state application: %s"), *ZonePair.Key);
        }
    }
    
    // Clear and reapply world events
    ActiveWorldEvents.Empty();
    for (const FGameplayTag& EventTag : WorldState.ActiveWorldEvents.GetGameplayTagArray())
    {
        // Restore event with metadata if available
        FActiveWorldEvent NewEvent;
        NewEvent.EventTag = EventTag;
        NewEvent.StartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
        NewEvent.Duration = 0.0f; // Permanent events from save
        
        // Try to restore instigator from metadata
        FString EventKey = EventTag.ToString();
        if (const FString* InstigatorName = WorldState.EventMetadata.Find(EventKey + "_Instigator"))
        {
            // Try to find the instigator actor (this might not always work)
            if (UWorld* World = GetWorld())
            {
                for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr)
                {
                    if (ActorItr->GetName() == *InstigatorName)
                    {
                        NewEvent.Instigator = *ActorItr;
                        break;
                    }
                }
            }
        }
        
        ActiveWorldEvents.Add(NewEvent);
    }
    
    // Apply simulation flags
    if (WorldState.SimulationFlags.HasTag(FGameplayTag::RequestGameplayTag("Simulation.Active")))
    {
        if (!bSimulationActive)
        {
            InitializeWorldSimulation();
        }
    }
    
    if (WorldState.SimulationFlags.HasTag(FGameplayTag::RequestGameplayTag("Weather.Dynamic")))
    {
        WeatherSettings.bDynamicWeather = true;
    }
    
    // Update total simulation time
    // Note: We don't directly set this as it's calculated from CurrentWorldTime.TotalGameSeconds
    
    UE_LOG(LogTemp, Log, TEXT("World state snapshot applied successfully - Time: %s, Events: %d, Zones: %d"), 
           *GetFullTimeString(), ActiveWorldEvents.Num(), RegisteredZones.Num());
}

// === SIMPLE TIME MANAGER INTERFACE IMPLEMENTATION ===

void URadiantWorldManager::SetWorldTime(const FSimpleWorldTime& NewTime)
{
    ETimeOfDay OldTimeOfDay = CurrentWorldTime.GetTimeOfDay();
    ESeason OldSeason = CurrentWorldTime.GetSeason();
    
    CurrentWorldTime = NewTime;
    
    // Process time change events
    ProcessTimeChangeEvents(OldTimeOfDay, OldSeason);
    
    UE_LOG(LogTemp, Log, TEXT("World time set: %s"), *GetFullTimeString());
}

void URadiantWorldManager::AdvanceTime(float RealSecondsToAdvance)
{
    if (RealSecondsToAdvance <= 0.0f || CurrentWorldTime.bTimePaused)
    {
        return;
    }
    
    ETimeOfDay OldTimeOfDay = CurrentWorldTime.GetTimeOfDay();
    ESeason OldSeason = CurrentWorldTime.GetSeason();
    
    // Add scaled time
    float GameSecondsToAdvance = RealSecondsToAdvance * CurrentWorldTime.TimeScale;
    CurrentWorldTime.TotalGameSeconds += GameSecondsToAdvance;
    
    // Convert total seconds to time components
    int32 TotalMinutes = FMath::FloorToInt(CurrentWorldTime.TotalGameSeconds / 60.0f);
    CurrentWorldTime.SetFromTotalMinutes(TotalMinutes);
    
    // Process time change events
    ProcessTimeChangeEvents(OldTimeOfDay, OldSeason);
}

void URadiantWorldManager::AdvanceGameMinutes(int32 GameMinutesToAdvance)
{
    if (GameMinutesToAdvance <= 0 || CurrentWorldTime.bTimePaused)
    {
        return;
    }
    
    ETimeOfDay OldTimeOfDay = CurrentWorldTime.GetTimeOfDay();
    ESeason OldSeason = CurrentWorldTime.GetSeason();
    
    // Add game minutes directly
    int32 CurrentTotalMinutes = CurrentWorldTime.GetTotalMinutes();
    CurrentTotalMinutes += GameMinutesToAdvance;
    CurrentWorldTime.SetFromTotalMinutes(CurrentTotalMinutes);
    
    // Update total seconds to match
    CurrentWorldTime.TotalGameSeconds = CurrentTotalMinutes * 60.0f;
    
    // Process time change events
    ProcessTimeChangeEvents(OldTimeOfDay, OldSeason);
    
    UE_LOG(LogTemp, Log, TEXT("Advanced time by %d game minutes to: %s"), 
           GameMinutesToAdvance, *GetFullTimeString());
}

void URadiantWorldManager::SetTimeScale(float NewTimeScale)
{
    CurrentWorldTime.TimeScale = FMath::Clamp(NewTimeScale, 0.1f, 1000.0f);
    TimeSettings.DefaultTimeScale = CurrentWorldTime.TimeScale;
    
    UE_LOG(LogTemp, Log, TEXT("Time scale set to: %f"), CurrentWorldTime.TimeScale);
}

void URadiantWorldManager::SetTimePaused(bool bPaused)
{
    CurrentWorldTime.bTimePaused = bPaused;
    
    UE_LOG(LogTemp, Log, TEXT("Time %s"), bPaused ? TEXT("PAUSED") : TEXT("RESUMED"));
}

// === BLUEPRINT TIME INTERFACE ===

void URadiantWorldManager::SetSimpleWorldTime(const FSimpleWorldTime& NewTime)
{
    SetWorldTime(NewTime);
}

void URadiantWorldManager::AdvanceTimeBySeconds(float RealSecondsToAdvance)
{
    AdvanceTime(RealSecondsToAdvance);
}

void URadiantWorldManager::AdvanceTimeByGameMinutes(int32 GameMinutesToAdvance)
{
    AdvanceGameMinutes(GameMinutesToAdvance);
}

void URadiantWorldManager::SetGlobalTimeScale(float NewTimeScale)
{
    SetTimeScale(NewTimeScale);
}

void URadiantWorldManager::SetTimeProgression(bool bPaused)
{
    SetTimePaused(bPaused);
}

// === WEATHER SYSTEM INTERFACE ===

void URadiantWorldManager::SetGlobalWeather(EWeatherType NewWeather, float Intensity, float TransitionTime)
{
    CurrentGlobalWeather.TargetWeather = NewWeather;
    CurrentGlobalWeather.Intensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
    CurrentGlobalWeather.TransitionDuration = FMath::Max(TransitionTime, 0.1f);
    CurrentGlobalWeather.TransitionProgress = 0.0f;
    
    UE_LOG(LogTemp, Log, TEXT("Global weather changing to %s (Intensity: %f)"), 
           *UEnum::GetValueAsString(NewWeather), Intensity);
}

void URadiantWorldManager::SetZoneWeather(FGameplayTag ZoneTag, EWeatherType NewWeather, float Intensity)
{
    if (ARadiantZoneManager* Zone = GetZoneByTag(ZoneTag))
    {
        // Delegate to zone's weather system
        Zone->SetWeather(static_cast<EZoneWeather>(NewWeather));
        
        UE_LOG(LogTemp, Log, TEXT("Zone %s weather set to %s"), 
               *ZoneTag.ToString(), *UEnum::GetValueAsString(NewWeather));
    }
}

void URadiantWorldManager::UpdateWeatherSystem()
{
    // Update weather transition
    CalculateWeatherTransition(WeatherSettings.UpdateInterval);
    
    // Trigger weather events if needed
    if (CurrentGlobalWeather.TransitionProgress >= 1.0f && 
        CurrentGlobalWeather.CurrentWeather != CurrentGlobalWeather.TargetWeather)
    {
        EWeatherType OldWeather = CurrentGlobalWeather.CurrentWeather;
        CurrentGlobalWeather.CurrentWeather = CurrentGlobalWeather.TargetWeather;
        OnGlobalWeatherChanged(CurrentGlobalWeather);
        
        // Broadcast weather change event
        if (WorldEventManager)
        {
            WorldEventManager->BroadcastGlobalEvent(FGameplayTag::RequestGameplayTag("Event.Weather.Changed"));
        }
    }
}

bool URadiantWorldManager::DoesWeatherAffectVisibility() const
{
    switch (CurrentGlobalWeather.CurrentWeather)
    {
        case EWeatherType::Fog:
        case EWeatherType::HeavyRain:
        case EWeatherType::Blizzard:
        case EWeatherType::Sandstorm:
            return true;
        default:
            return false;
    }
}

// === ZONE MANAGEMENT INTERFACE ===

void URadiantWorldManager::RegisterZone(ARadiantZoneManager* Zone)
{
    if (!Zone)
    {
        UE_LOG(LogTemp, Warning, TEXT("Attempted to register null zone"));
        return;
    }
    
    FGameplayTag ZoneTag = Zone->GetZoneTag();
    if (!ZoneTag.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Attempted to register zone with invalid tag"));
        return;
    }
    
    if (RegisteredZones.Contains(ZoneTag))
    {
        UE_LOG(LogTemp, Warning, TEXT("Zone %s is already registered"), *ZoneTag.ToString());
        return;
    }
    
    RegisteredZones.Add(ZoneTag, Zone);
    
    UE_LOG(LogTemp, Log, TEXT("Registered zone: %s"), *ZoneTag.ToString());
}

void URadiantWorldManager::UnregisterZone(ARadiantZoneManager* Zone)
{
    if (!Zone)
    {
        return;
    }
    
    FGameplayTag ZoneTag = Zone->GetZoneTag();
    if (RegisteredZones.Contains(ZoneTag) && RegisteredZones[ZoneTag] == Zone)
    {
        RegisteredZones.Remove(ZoneTag);
        UE_LOG(LogTemp, Log, TEXT("Unregistered zone: %s"), *ZoneTag.ToString());
    }
}

ARadiantZoneManager* URadiantWorldManager::GetZoneByTag(FGameplayTag ZoneTag) const
{
    if (const TObjectPtr<ARadiantZoneManager>* FoundZone = RegisteredZones.Find(ZoneTag))
    {
        return *FoundZone;
    }
    return nullptr;
}

TArray<ARadiantZoneManager*> URadiantWorldManager::GetZonesByType(EZoneType ZoneType) const
{
    TArray<ARadiantZoneManager*> Zones;
    
    for (const auto& ZonePair : RegisteredZones)
    {
        if (ZonePair.Value && ZonePair.Value->GetZoneType() == ZoneType)
        {
            Zones.Add(ZonePair.Value);
        }
    }
    
    return Zones;
}

ARadiantZoneManager* URadiantWorldManager::GetZoneAtLocation(FVector WorldLocation) const
{
    for (const auto& ZonePair : RegisteredZones)
    {
        if (ZonePair.Value && ZonePair.Value->IsLocationInZone(WorldLocation))
        {
            return ZonePair.Value;
        }
    }
    return nullptr;
}

TArray<ARadiantZoneManager*> URadiantWorldManager::GetAllZones() const
{
    TArray<ARadiantZoneManager*> Zones;
    for (const auto& ZonePair : RegisteredZones)
    {
        if (ZonePair.Value)
        {
            Zones.Add(ZonePair.Value);
        }
    }
    return Zones;
}

// === WORLD EVENT INTERFACE ===

void URadiantWorldManager::TriggerGlobalEvent(FGameplayTag EventTag, float Duration, AActor* Instigator)
{
    FActiveWorldEvent NewEvent;
    NewEvent.EventTag = EventTag;
    NewEvent.StartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    NewEvent.Duration = Duration;
    NewEvent.Instigator = Instigator;
    
    ActiveWorldEvents.Add(NewEvent);
    
    if (WorldEventManager)
    {
        WorldEventManager->BroadcastGlobalEvent(EventTag, Instigator);
    }
    
    UE_LOG(LogTemp, Log, TEXT("Global event triggered: %s"), *EventTag.ToString());
}

bool URadiantWorldManager::StopWorldEvent(FGameplayTag EventTag)
{
    int32 RemovedEvents = ActiveWorldEvents.RemoveAll([EventTag](const FActiveWorldEvent& Event)
    {
        return Event.EventTag == EventTag;
    });
    
    if (RemovedEvents > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Stopped world event: %s"), *EventTag.ToString());
        return true;
    }
    
    return false;
}

// === DEBUG AND MONITORING ===

FString URadiantWorldManager::GetDebugString() const
{
    FString DebugString;
    DebugString += FString::Printf(TEXT("=== RadiantWorldManager Debug ===\n"));
    DebugString += FString::Printf(TEXT("Simulation Active: %s\n"), bSimulationActive ? TEXT("Yes") : TEXT("No"));
    DebugString += FString::Printf(TEXT("Time Paused: %s\n"), CurrentWorldTime.bTimePaused ? TEXT("Yes") : TEXT("No"));
    DebugString += FString::Printf(TEXT("Current Time: %s\n"), *GetFullTimeString());
    DebugString += FString::Printf(TEXT("Time Scale: %f\n"), CurrentWorldTime.TimeScale);
    DebugString += FString::Printf(TEXT("Current Weather: %s\n"), *UEnum::GetValueAsString(CurrentGlobalWeather.CurrentWeather));
    DebugString += FString::Printf(TEXT("Registered Zones: %d\n"), RegisteredZones.Num());
    DebugString += FString::Printf(TEXT("Active Events: %d\n"), ActiveWorldEvents.Num());
    
    return DebugString;
}

bool URadiantWorldManager::ValidateWorldState() const
{
    bool bIsValid = true;
    
    // Validate time data
    if (CurrentWorldTime.Season < 1)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid world season: %d"), CurrentWorldTime.Season);
        bIsValid = false;
    }
    
    if (CurrentWorldTime.Day < 1 || CurrentWorldTime.Day > 30)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid world day: %d"), CurrentWorldTime.Day);
        bIsValid = false;
    }
    
    if (CurrentWorldTime.Hour < 0 || CurrentWorldTime.Hour > 23)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid world hour: %d"), CurrentWorldTime.Hour);
        bIsValid = false;
    }
    
    if (CurrentWorldTime.Minute < 0 || CurrentWorldTime.Minute > 59)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid world minute: %d"), CurrentWorldTime.Minute);
        bIsValid = false;
    }
    
    // Validate weather data
    if (CurrentGlobalWeather.Intensity < 0.0f || CurrentGlobalWeather.Intensity > 1.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid weather intensity: %f"), CurrentGlobalWeather.Intensity);
        bIsValid = false;
    }
    
    // Validate zone references
    for (const auto& ZonePair : RegisteredZones)
    {
        if (!ZonePair.Value || ZonePair.Value->IsPendingKillPending())
        {
            UE_LOG(LogTemp, Error, TEXT("Invalid zone reference for tag: %s"), *ZonePair.Key.ToString());
            bIsValid = false;
        }
    }
    
    return bIsValid;
}

// === INTERNAL FUNCTIONS ===

void URadiantWorldManager::InitializeSimpleTimeSystem()
{
    // Apply time settings
    CurrentWorldTime.Season = TimeSettings.StartingSeason;
    CurrentWorldTime.Day = TimeSettings.StartingDay;
    CurrentWorldTime.Hour = TimeSettings.StartingHour;
    CurrentWorldTime.Minute = 0;
    CurrentWorldTime.TimeScale = TimeSettings.DefaultTimeScale;
    CurrentWorldTime.bTimePaused = !TimeSettings.bAutoTimeProgression;
    
    // Calculate initial total seconds
    int32 TotalMinutes = CurrentWorldTime.GetTotalMinutes();
    CurrentWorldTime.TotalGameSeconds = TotalMinutes * 60.0f;
    
    // Initialize cached values
    PreviousTimeOfDay = CurrentWorldTime.GetTimeOfDay();
    PreviousSeason = CurrentWorldTime.GetSeason();
    
    UE_LOG(LogTemp, Log, TEXT("Simple time system initialized - %s"), *GetFullTimeString());
}

void URadiantWorldManager::InitializeWeatherSystem()
{
    // Set default weather settings
    WeatherSettings.UpdateInterval = 30.0f; // Update every 30 seconds
    WeatherSettings.bDynamicWeather = true;
    WeatherSettings.WeatherChangeChance = 0.1f; // 10% chance per update
    
    // Initialize global weather to clear
    CurrentGlobalWeather.CurrentWeather = EWeatherType::Clear;
    CurrentGlobalWeather.TargetWeather = EWeatherType::Clear;
    CurrentGlobalWeather.Intensity = 0.5f;
    CurrentGlobalWeather.TransitionProgress = 1.0f;
    CurrentGlobalWeather.TransitionDuration = 60.0f;
    
    UE_LOG(LogTemp, Log, TEXT("Weather system initialized - Clear weather"));
}

void URadiantWorldManager::InitializeEventSystem()
{
    // Get reference to world event manager
    if (UWorld* World = GetWorld())
    {
        WorldEventManager = World->GetSubsystem<UWorldEventManager>();
        if (WorldEventManager)
        {
            UE_LOG(LogTemp, Log, TEXT("Connected to WorldEventManager"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("WorldEventManager not available"));
        }
    }
}

void URadiantWorldManager::InitializeDefaultWorldState()
{
    // Initialize simulation state
    SimulationState.bWeatherSystemActive = true;
    SimulationState.bEventSystemActive = true;
    SimulationState.bZoneSystemActive = true;
    
    UE_LOG(LogTemp, Log, TEXT("Default world state initialized"));
}

void URadiantWorldManager::UpdateTimeProgression(float DeltaTime)
{
    if (CurrentWorldTime.bTimePaused || !TimeSettings.bAutoTimeProgression)
    {
        return;
    }
    
    // Advance time automatically
    AdvanceTime(DeltaTime);
}

void URadiantWorldManager::ProcessTimeChangeEvents(ETimeOfDay OldTimeOfDay, ESeason OldSeason)
{
    ETimeOfDay NewTimeOfDay = CurrentWorldTime.GetTimeOfDay();
    ESeason NewSeason = CurrentWorldTime.GetSeason();
    
    // Check for time of day changes
    if (OldTimeOfDay != NewTimeOfDay)
    {
        PreviousTimeOfDay = NewTimeOfDay;
        OnTimeOfDayChanged.Broadcast(NewTimeOfDay);
        
        UE_LOG(LogTemp, Log, TEXT("Time of day changed to: %s"), 
               *UEnum::GetValueAsString(NewTimeOfDay));
    }
    
    // Check for season changes
    if (OldSeason != NewSeason)
    {
        PreviousSeason = NewSeason;
        OnSeasonChanged.Broadcast(NewSeason);
        
        UE_LOG(LogTemp, Log, TEXT("Season changed to: %s"), 
               *UEnum::GetValueAsString(NewSeason));
    }
    
    // Check for new day
    static int32 LastDay = CurrentWorldTime.Day;
    static int32 LastSeason = CurrentWorldTime.Season;
    if (LastDay != CurrentWorldTime.Day || LastSeason != CurrentWorldTime.Season)
    {
        LastDay = CurrentWorldTime.Day;
        LastSeason = CurrentWorldTime.Season;
        OnNewDay.Broadcast(CurrentWorldTime.Season, CurrentWorldTime.Day);
        
        UE_LOG(LogTemp, Log, TEXT("New day: Season %d, Day %d"), 
               CurrentWorldTime.Season, CurrentWorldTime.Day);
    }
    
    // Broadcast general time change
    OnTimeChanged.Broadcast(CurrentWorldTime);
    OnGlobalTimeChanged(CurrentWorldTime);
}

void URadiantWorldManager::CalculateWeatherTransition(float DeltaTime)
{
    if (CurrentGlobalWeather.CurrentWeather == CurrentGlobalWeather.TargetWeather)
    {
        return;
    }
    
    CurrentGlobalWeather.TransitionProgress += DeltaTime / CurrentGlobalWeather.TransitionDuration;
    CurrentGlobalWeather.TransitionProgress = FMath::Clamp(CurrentGlobalWeather.TransitionProgress, 0.0f, 1.0f);
}

void URadiantWorldManager::ProcessScheduledEvents()
{
    // Remove expired events
    if (UWorld* World = GetWorld())
    {
        float CurrentTime = World->GetTimeSeconds();
        
        ActiveWorldEvents.RemoveAll([CurrentTime](const FActiveWorldEvent& Event)
        {
            return Event.Duration > 0.0f && (CurrentTime - Event.StartTime) >= Event.Duration;
        });
    }
    
    // Limit maximum active events
    if (ActiveWorldEvents.Num() > SimulationSettings.MaxActiveEvents)
    {
        // Remove oldest events first
        ActiveWorldEvents.Sort([](const FActiveWorldEvent& A, const FActiveWorldEvent& B)
        {
            return A.StartTime < B.StartTime;
        });
        
        int32 EventsToRemove = ActiveWorldEvents.Num() - SimulationSettings.MaxActiveEvents;
        ActiveWorldEvents.RemoveAt(0, EventsToRemove);
    }
}

void URadiantWorldManager::ValidateZoneRegistrations()
{
    // Remove invalid zone references
    TArray<FGameplayTag> InvalidTags;
    
    for (const auto& ZonePair : RegisteredZones)
    {
        if (!ZonePair.Value || ZonePair.Value->IsPendingKillPending())
        {
            InvalidTags.Add(ZonePair.Key);
        }
    }
    
    for (const FGameplayTag& Tag : InvalidTags)
    {
        RegisteredZones.Remove(Tag);
        UE_LOG(LogTemp, Warning, TEXT("Removed invalid zone registration: %s"), *Tag.ToString());
    }
}

void URadiantWorldManager::UpdateSimulationMetrics(float DeltaTime)
{
    // Update performance metrics
    if (SimulationSettings.bEnablePerformanceMonitoring)
    {
        // Track simulation performance
        static float TotalUpdateTime = 0.0f;
        static int32 UpdateCount = 0;
        
        TotalUpdateTime += DeltaTime;
        UpdateCount++;
        
        // Log performance every 100 updates
        if (UpdateCount >= 100)
        {
            float AverageUpdateTime = TotalUpdateTime / UpdateCount;
            
            if (SimulationSettings.bEnableDebugLogging)
            {
                UE_LOG(LogTemp, Verbose, TEXT("WorldManager average update time: %f ms"), 
                       AverageUpdateTime * 1000.0f);
            }
            
            TotalUpdateTime = 0.0f;
            UpdateCount = 0;
        }
    }
}

// === EVENT HANDLERS ===

void URadiantWorldManager::OnGlobalWeatherChanged(const FWorldWeatherData& NewWeather)
{
    // Notify all registered zones of weather change
    for (const auto& ZonePair : RegisteredZones)
    {
        if (ZonePair.Value)
        {
            // Zones can choose to follow global weather or maintain their own
            // This is just a notification
        }
    }
}

void URadiantWorldManager::OnGlobalTimeChanged(const FSimpleWorldTime& NewTime)
{
    // Broadcast to other systems that might need time updates
    if (WorldEventManager)
    {
        // Trigger time-based events if needed
    }
}