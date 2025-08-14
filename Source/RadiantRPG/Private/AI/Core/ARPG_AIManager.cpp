// Private/AI/Core/ARPG_AIManager.cpp - Missing Interface Method Implementations

#include "AI/Core/ARPG_AIManager.h"
#include "AI/Core/ARPG_AIEventManager.h"
#include "AI/Core/ARPG_AIBrainComponent.h"
#include "Characters/ARPG_BaseNPCCharacter.h"

// Constructor
UARPG_AIManager::UARPG_AIManager()
{
    // Initialize default values
    bAISystemEnabled = true;
    GlobalUpdateRate = 0.1f; // 10 updates per second default
    MaxActiveNPCs = 100;
    CurrentAILoad = 0.0f;
}

// Subsystem lifecycle
void UARPG_AIManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    OnSystemInitialized();
}

void UARPG_AIManager::Deinitialize()
{
    OnSystemShutdown();
    Super::Deinitialize();
}

bool UARPG_AIManager::ShouldCreateSubsystem(UObject* Outer) const
{
    return true;
}

void UARPG_AIManager::OnWorldBeginPlay(UWorld& InWorld)
{
    StartUpdateTimer();
}

void UARPG_AIManager::OnSystemInitialized()
{
    UE_LOG(LogTemp, Log, TEXT("ARPG_AIManager: System initialized - broadcasting event"));
    
    // Broadcast initialization to SystemEventCoordinator
    FSystemEventCoordinator::Get().BroadcastSystemInitialized(
        ESystemType::AIManager,
        true,
        TEXT("ARPG_AIManager")
    );
}

void UARPG_AIManager::OnSystemShutdown()
{
    StopUpdateTimer();
    RegisteredAIs.Empty();
    RegisteredNPCs.Empty();
    RegisteredBrains.Empty();
}

// AI Registration
void UARPG_AIManager::RegisterAI(AActor* AIActor)
{
    if (AIActor && !RegisteredAIs.Contains(AIActor))
    {
        RegisteredAIs.Add(AIActor);
    }
}

void UARPG_AIManager::UnregisterAI(AActor* AIActor)
{
    RegisteredAIs.Remove(AIActor);
}

// Event Broadcasting
void UARPG_AIManager::BroadcastGlobalEvent(const FARPG_AIEvent& Event)
{
    // Broadcast to all registered AI actors
    for (AActor* AI : RegisteredAIs)
    {
        if (IsValid(AI))
        {
            // Handle event broadcasting logic
        }
    }
}

int32 UARPG_AIManager::GetActiveAICount() const
{
    return RegisteredAIs.Num();
}

// NPC Management
void UARPG_AIManager::RegisterNPC(AARPG_BaseNPCCharacter* NPC)
{
    if (NPC && !RegisteredNPCs.Contains(NPC))
    {
        RegisteredNPCs.Add(NPC);
    }
}

void UARPG_AIManager::UnregisterNPC(AARPG_BaseNPCCharacter* NPC)
{
    RegisteredNPCs.Remove(NPC);
}

TArray<AARPG_BaseNPCCharacter*> UARPG_AIManager::GetAllNPCs() const
{
    return RegisteredNPCs;
}

TArray<AARPG_BaseNPCCharacter*> UARPG_AIManager::GetNPCsInRadius(FVector Location, float Radius) const
{
    TArray<AARPG_BaseNPCCharacter*> Result;
    const float RadiusSq = Radius * Radius;
    
    for (AARPG_BaseNPCCharacter* NPC : RegisteredNPCs)
    {
        if (IsValid(NPC))
        {
            float DistSq = FVector::DistSquared(NPC->GetActorLocation(), Location);
            if (DistSq <= RadiusSq)
            {
                Result.Add(NPC);
            }
        }
    }
    
    return Result;
}

TArray<AARPG_BaseNPCCharacter*> UARPG_AIManager::GetNPCsByFaction(FGameplayTag FactionTag) const
{
    TArray<AARPG_BaseNPCCharacter*> Result;
    
    for (AARPG_BaseNPCCharacter* NPC : RegisteredNPCs)
    {
        if (IsValid(NPC))
        {
            // Check if NPC has faction tag
            // This assumes NPCs have a method to check faction
            // Adjust based on your actual implementation
            Result.Add(NPC);
        }
    }
    
    return Result;
}

TArray<AARPG_BaseNPCCharacter*> UARPG_AIManager::GetNPCsByType(FGameplayTag TypeTag) const
{
    TArray<AARPG_BaseNPCCharacter*> Result;
    
    for (AARPG_BaseNPCCharacter* NPC : RegisteredNPCs)
    {
        if (IsValid(NPC))
        {
            // Check if NPC has type tag
            // Adjust based on your actual implementation
            Result.Add(NPC);
        }
    }
    
    return Result;
}

// Brain Component Management
void UARPG_AIManager::RegisterBrainComponent(UARPG_AIBrainComponent* Brain)
{
    if (Brain && !RegisteredBrains.Contains(Brain))
    {
        RegisteredBrains.Add(Brain);
    }
}

void UARPG_AIManager::UnregisterBrainComponent(UARPG_AIBrainComponent* Brain)
{
    RegisteredBrains.Remove(Brain);
}

TArray<UARPG_AIBrainComponent*> UARPG_AIManager::GetActiveBrains() const
{
    return RegisteredBrains;
}

// System Control
void UARPG_AIManager::SetAISystemEnabled(bool bEnabled)
{
    bAISystemEnabled = bEnabled;
    
    if (!bAISystemEnabled)
    {
        StopUpdateTimer();
    }
    else
    {
        StartUpdateTimer();
    }
}

bool UARPG_AIManager::IsAISystemEnabled() const
{
    return bAISystemEnabled;
}

void UARPG_AIManager::SetGlobalUpdateRate(float Rate)
{
    GlobalUpdateRate = FMath::Clamp(Rate, 0.01f, 1.0f);
    
    // Restart timer with new rate
    if (bAISystemEnabled)
    {
        StopUpdateTimer();
        StartUpdateTimer();
    }
}

float UARPG_AIManager::GetGlobalUpdateRate() const
{
    return GlobalUpdateRate;
}

// Stimulus Broadcasting
void UARPG_AIManager::BroadcastStimulus(const FARPG_AIStimulus& Stimulus, FVector Location, float Radius)
{
    const float RadiusSq = Radius * Radius;
    
    for (UARPG_AIBrainComponent* Brain : RegisteredBrains)
    {
        if (IsValid(Brain) && Brain->GetOwner())
        {
            float DistSq = FVector::DistSquared(Brain->GetOwner()->GetActorLocation(), Location);
            if (DistSq <= RadiusSq)
            {
                // Send stimulus to brain
                // Brain->ReceiveStimulus(Stimulus);
            }
        }
    }
}

// Configuration
void UARPG_AIManager::UpdateConfiguration(const FARPG_AIManagerConfig& NewConfig)
{
    // Update configuration settings
    SetGlobalUpdateRate(NewConfig.GlobalUpdateRate);
    MaxActiveNPCs = NewConfig.MaxActiveNPCs;
    // Apply other config settings as needed
}

// Protected Methods
void UARPG_AIManager::PeriodicUpdate()
{
    if (!bAISystemEnabled)
        return;
    
    // Process AI updates
    ProcessAIUpdates(GlobalUpdateRate);
    
    // Update metrics
    UpdateAILoadMetrics();
}

void UARPG_AIManager::StartUpdateTimer()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            UpdateTimerHandle,
            this,
            &UARPG_AIManager::UpdateAISystems,
            GlobalUpdateRate,
            true
        );
        
        UE_LOG(LogTemp, Log, TEXT("ARPG_AIManager: Update timer started"));
    }
}

void UARPG_AIManager::UpdateAISystems()
{
    if (!bAISystemEnabled)
    {
        return;
    }
    
    float DeltaTime = GetWorld() ? GetWorld()->GetDeltaSeconds() : GlobalUpdateRate;
    
    // Process AI updates
    ProcessAIUpdates(DeltaTime);
    
    // Update AI load metrics
    UpdateAILoadMetrics();
    
    // Clean up invalid references
    CleanupInvalidReferences();
    
    if (bDebugVisualizationEnabled)
    {
        DebugLogAIStats();
    }
}

void UARPG_AIManager::CleanupInvalidReferences()
{
    // Clean up invalid AI actors
    for (int32 i = RegisteredAIs.Num() - 1; i >= 0; --i)
    {
        if (!IsValid(RegisteredAIs[i]))
        {
            RegisteredAIs.RemoveAtSwap(i);
        }
    }
    
    // Clean up invalid NPCs
    for (int32 i = RegisteredNPCs.Num() - 1; i >= 0; --i)
    {
        if (!IsValid(RegisteredNPCs[i]))
        {
            RegisteredNPCs.RemoveAtSwap(i);
        }
    }
    
    // Clean up invalid brain components
    for (int32 i = RegisteredBrains.Num() - 1; i >= 0; --i)
    {
        if (!IsValid(RegisteredBrains[i]))
        {
            RegisteredBrains.RemoveAtSwap(i);
        }
    }
}

void UARPG_AIManager::DebugLogAIStats()
{
    if (GetWorld())
    {
        static float LastDebugTime = 0.0f;
        float CurrentTime = GetWorld()->GetTimeSeconds();
        
        // Only log stats every 5 seconds to avoid spam
        if (CurrentTime - LastDebugTime >= 5.0f)
        {
            UE_LOG(LogTemp, Log, TEXT("AIManager Stats - Active AIs: %d, NPCs: %d, Brains: %d, Load: %.2f"),
                RegisteredAIs.Num(),
                RegisteredNPCs.Num(), 
                RegisteredBrains.Num(),
                CurrentAILoad);
            
            LastDebugTime = CurrentTime;
        }
    }
}

void UARPG_AIManager::StopUpdateTimer()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(UpdateTimerHandle);
        UE_LOG(LogTemp, Log, TEXT("ARPG_AIManager: Update timer stopped"));
    }
}

void UARPG_AIManager::ProcessAIUpdates(float DeltaTime)
{
    // Update all registered AI brains
    for (UARPG_AIBrainComponent* Brain : RegisteredBrains)
    {
        if (IsValid(Brain))
        {
            // Process brain update
            // Brain->UpdateBrain(DeltaTime);
        }
    }
}

void UARPG_AIManager::UpdateAILoadMetrics()
{
    // Calculate current AI system load
    int32 ActiveCount = 0;
    
    for (AARPG_BaseNPCCharacter* NPC : RegisteredNPCs)
    {
        if (IsValid(NPC))
        {
            ActiveCount++;
        }
    }
    
    CurrentAILoad = (float)ActiveCount / (float)FMath::Max(1, MaxActiveNPCs);
}

// Interface implementation for IARPG_AIManagerInterface
void IARPG_AIManagerInterface::PeriodicUpdate()
{
    // Default empty implementation for interface
}

void UARPG_AIManager::SetGlobalAIUpdateRate(float UpdateRate)
{
    Configuration.GlobalUpdateRate = FMath::Max(0.01f, UpdateRate);
    
    // Restart the update timer with new rate
    StopUpdateTimer();
    if (bIsAISystemEnabled && Configuration.GlobalUpdateRate > 0.0f)
    {
        StartUpdateTimer();
    }
    
    UE_LOG(LogTemp, Log, TEXT("AIManager: Global update rate set to %.2f"), UpdateRate);
}

float UARPG_AIManager::GetGlobalAIUpdateRate() const
{
    return Configuration.GlobalUpdateRate;
}

void UARPG_AIManager::TriggerGlobalCuriosity(float Intensity)
{
    if (!bIsAISystemEnabled || Intensity <= 0.0f)
    {
        return;
    }
    
    // Create curiosity event
    FARPG_AIEvent CuriosityEvent;
    CuriosityEvent.EventID = FGuid::NewGuid();
    CuriosityEvent.EventType = FGameplayTag::RequestGameplayTag(TEXT("AI.Event.GlobalCuriosity"));
    CuriosityEvent.EventStrength = FMath::Clamp(Intensity * Configuration.GlobalCuriosityMultiplier, 0.0f, 1.0f);
    CuriosityEvent.bGlobal = true;
    CuriosityEvent.Timestamp = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    
    // Broadcast to all NPCs
    BroadcastAIEvent(CuriosityEvent);
    
    UE_LOG(LogTemp, Log, TEXT("AIManager: Triggered global curiosity with intensity %.2f"), Intensity);
}

void UARPG_AIManager::SetGlobalCuriosityMultiplier(float Multiplier)
{
    Configuration.GlobalCuriosityMultiplier = FMath::Max(0.0f, Multiplier);
    
    UE_LOG(LogTemp, Verbose, TEXT("AIManager: Global curiosity multiplier set to %.2f"), Multiplier);
}

float UARPG_AIManager::GetAISystemLoad() const
{
    return CurrentAILoad;
}

void UARPG_AIManager::SetMaxActiveNPCs(int32 MaxNPCs)
{
    Configuration.MaxActiveNPCs = FMath::Max(1, MaxNPCs);
    
    // Update load metrics with new maximum
    UpdateAILoadMetrics();
    
    UE_LOG(LogTemp, Log, TEXT("AIManager: Max active NPCs set to %d"), MaxNPCs);
}

int32 UARPG_AIManager::GetMaxActiveNPCs() const
{
    return Configuration.MaxActiveNPCs;
}

void UARPG_AIManager::BroadcastAIEvent(const FARPG_AIEvent& Event)
{
    // Forward to the event manager
    if (IsValid(EventManager))
    {
        EventManager->BroadcastAIEvent(Event);
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("AIManager: Broadcasted AI event %s"), 
            *Event.EventType.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot broadcast AI event - EventManager not available"));
    }
}

void UARPG_AIManager::OnZoneTransition(FGameplayTag FromZone, FGameplayTag ToZone)
{
    UE_LOG(LogTemp, Log, TEXT("AIManager: Zone transition from %s to %s"), 
        *FromZone.ToString(), *ToZone.ToString());
    
    // Broadcast the delegate
    OnZoneTransitionDelegate.Broadcast(FromZone, ToZone);
    
    // Create zone transition event
    FARPG_AIEvent ZoneEvent;
    ZoneEvent.EventID = FGuid::NewGuid();
    ZoneEvent.EventType = FGameplayTag::RequestGameplayTag(TEXT("AI.Event.ZoneTransition"));
    ZoneEvent.bGlobal = true;
    ZoneEvent.EventStrength = 1.0f;
    ZoneEvent.Timestamp = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    ZoneEvent.EventData.Add(TEXT("FromZone"), FromZone.ToString());
    ZoneEvent.EventData.Add(TEXT("ToZone"), ToZone.ToString());
    
    // Broadcast to all NPCs
    BroadcastAIEvent(ZoneEvent);
    
    // Update NPCs for the new zone
    UpdateNPCsForZone(ToZone);
}

void UARPG_AIManager::UpdateNPCsForZone(FGameplayTag ZoneTag)
{
    // Update NPC priorities based on zone
    for (AARPG_BaseNPCCharacter* NPC : RegisteredNPCs)
    {
        if (!IsValid(NPC))
        {
            continue;
        }
        
        // Check if NPC is in or relevant to this zone
        FVector NPCLocation = NPC->GetActorLocation();
        
        // TODO: Implement zone bounds checking when zone system is available
        // For now, update all NPCs with zone information
        
        // Notify NPC of zone update
        if (UARPG_AIBrainComponent* Brain = NPC->FindComponentByClass<UARPG_AIBrainComponent>())
        {
            // Create zone update stimulus
            FARPG_AIStimulus ZoneStimulus;
            ZoneStimulus.StimulusType = EARPG_StimulusType::WorldEvent;
            ZoneStimulus.StimulusTag = FGameplayTag::RequestGameplayTag(TEXT("AI.Stimulus.ZoneUpdate"));
            ZoneStimulus.Intensity = 0.5f;
            ZoneStimulus.Location = NPCLocation;
            ZoneStimulus.Timestamp = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
            ZoneStimulus.StimulusData.Add(TEXT("Zone"), ZoneTag.ToString());
            
            NPC->ProcessStimulus(ZoneStimulus);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("AIManager: Updated NPCs for zone %s"), *ZoneTag.ToString());
}

void UARPG_AIManager::SetAIDebugEnabled(bool bEnabled)
{
    bDebugVisualizationEnabled = bEnabled;
    Configuration.bEnableDebugVisualization = bEnabled;
    
    UE_LOG(LogTemp, Log, TEXT("AIManager: Debug visualization %s"), 
        bEnabled ? TEXT("enabled") : TEXT("disabled"));
}

bool UARPG_AIManager::IsAIDebugEnabled() const
{
    return bDebugVisualizationEnabled;
}

// === Additional Required Methods ===

void UARPG_AIManager::UpdateAIProcessing(float DeltaTime)
{
    if (!bIsAISystemEnabled)
    {
        return;
    }
    
    // Process AI updates
    ProcessAIUpdates(DeltaTime);
}

void UARPG_AIManager::ForceUpdateAllAI()
{
    if (!bIsAISystemEnabled)
    {
        return;
    }
    
    float DeltaTime = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;
    
    // Force update all NPCs regardless of priority
    for (AARPG_BaseNPCCharacter* NPC : RegisteredNPCs)
    {
        if (IsValid(NPC))
        {
            NPC->UpdateBrain(DeltaTime);
        }
    }
    
    // Force update all brain components
    for (UARPG_AIBrainComponent* Brain : RegisteredBrains)
    {
        if (IsValid(Brain))
        {
            Brain->UpdateBrain(DeltaTime);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("AIManager: Forced update of all %d NPCs and %d brains"), 
        RegisteredNPCs.Num(), RegisteredBrains.Num());
}

void UARPG_AIManager::EnableDebugVisualization(bool bEnabled)
{
    SetAIDebugEnabled(bEnabled);
}

bool UARPG_AIManager::IsDebugVisualizationEnabled() const
{
    return IsAIDebugEnabled();
}