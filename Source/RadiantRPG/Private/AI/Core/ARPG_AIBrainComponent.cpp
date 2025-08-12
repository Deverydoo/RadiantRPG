// Source/RadiantRPG/Private/AI/Core/ARPG_AIBrainComponent.cpp

#include "AI/Core/ARPG_AIBrainComponent.h"
#include "AI/Core/ARPG_AIMemoryComponent.h"
#include "AI/Core/ARPG_AIPerceptionComponent.h"
#include "AI/Core/ARPG_AINeedsComponent.h"
#include "AI/Core/ARPG_AIPersonalityComponent.h"
#include "AI/Core/ARPG_AIEventManager.h"
#include "Engine/World.h"
#include "RadiantRPG.h"
#include "AI/Core/ARPG_AIBehaviorExecutorComponent.h"
#include "Types/ARPG_AIDataTableTypes.h"
#include "Types/RadiantAITypes.h"

UARPG_AIBrainComponent::UARPG_AIBrainComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f; // Default 10Hz processing
    
    // Set default values
    TimeSinceLastUpdate = 0.0f;
    LastStimulusTime = 0.0f;
    
    // Initialize brain configuration with sensible defaults
    BrainConfig.ProcessingFrequency = 0.1f;
    BrainConfig.StimuliMemoryDuration = 30.0f;
    BrainConfig.CuriosityThreshold = 5.0f;
    BrainConfig.bEnableDebugLogging = false;
    
    // Initialize brain state
    CurrentBrainState.CurrentState = EARPG_BrainState::Inactive;
    CurrentBrainState.bIsEnabled = false;
    CurrentBrainState.TimeSinceLastStimulus = 0.0f;
    
    // Set default intents
    DefaultIdleIntent = FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Idle"));
    CuriosityIntentTags.Add(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Wander")));
    CuriosityIntentTags.Add(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Explore")));
    CuriosityIntentTags.Add(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Observe")));
}

void UARPG_AIBrainComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Load configuration first (data table takes priority)
    LoadBrainConfiguration();
    
    // Continue with existing initialization...
    InitializeComponentReferences();
    RegisterWithEventManager();

    BehaviorExecutor = GetOwner()->FindComponentByClass<UARPG_AIBehaviorExecutorComponent>();
    
    UE_LOG(LogARPG, Log, TEXT("Brain Component: Initialized with %s configuration"), 
           bUseDataTableConfig ? TEXT("DataTable") : TEXT("Blueprint"));
}

void UARPG_AIBrainComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnregisterFromEventManager();
    SetBrainState(EARPG_BrainState::Inactive);
    Super::EndPlay(EndPlayReason);
}

void UARPG_AIBrainComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    if (!CurrentBrainState.bIsEnabled)
    {
        return;
    }
    
    UpdateBrain(DeltaTime);
}

void UARPG_AIBrainComponent::InitializeBrain(const FARPG_AIBrainConfiguration& Config)
{
    BrainConfig = Config;
    PrimaryComponentTick.TickInterval = BrainConfig.ProcessingFrequency;
    
    // Clear current state
    ClearBrainState();
    
    if (BrainConfig.bEnableDebugLogging)
    {
        UE_LOG(LogARPG, Log, TEXT("Brain initialized with custom configuration"));
    }
}

void UARPG_AIBrainComponent::ProcessStimulus(const FARPG_AIStimulus& Stimulus)
{
    if (!CurrentBrainState.bIsEnabled)
    {
        return;
    }
    
    // Add to recent stimuli
    AddStimulus(Stimulus);
    
    // Notify that we received a stimulus
    OnStimulusReceived.Broadcast(GetOwner(), FStimulus()); // Convert to appropriate type
    BP_OnStimulusProcessed(Stimulus);
    
    // Update last stimulus time
    LastStimulusTime = GetWorld()->GetTimeSeconds();
    CurrentBrainState.TimeSinceLastStimulus = 0.0f;
    
    // If this is a high-intensity stimulus, force immediate processing
    if (Stimulus.Intensity >= 0.8f)
    {
        ForceIntentGeneration();
    }
    
    if (BrainConfig.bEnableDebugLogging)
    {
        UE_LOG(LogARPG, VeryVerbose, TEXT("Processed stimulus: %s (Intensity: %.2f)"), 
               *Stimulus.StimulusTag.ToString(), Stimulus.Intensity);
    }
}

FARPG_AIIntent UARPG_AIBrainComponent::GenerateIntent()
{
    if (!CurrentBrainState.bIsEnabled)
    {
        return CurrentBrainState.CurrentIntent;
    }
    
    SetBrainState(EARPG_BrainState::Deciding);
    
    // Build input vector from all sources
    FARPG_AIInputVector InputVector = BuildInputVector();
    
    // Try Blueprint implementation first
    FARPG_AIIntent CustomIntent = BP_GenerateCustomIntent(InputVector);
    if (CustomIntent.IntentTag.IsValid())
    {
        ApplyPersonalityToIntent(CustomIntent);
        if (ValidateIntent(CustomIntent))
        {
            CurrentBrainState.CurrentIntent = CustomIntent;
            OnIntentChanged.Broadcast(CustomIntent);
            SetBrainState(EARPG_BrainState::Executing);
            return CustomIntent;
        }
    }
    
    // Fall back to C++ processing
    FARPG_AIIntent NewIntent = ProcessInputVector(InputVector);
    
    // Apply personality modifiers
    ApplyPersonalityToIntent(NewIntent);
    
    // Validate the intent
    if (ValidateIntent(NewIntent))
    {
        CurrentBrainState.CurrentIntent = NewIntent;
        OnIntentChanged.Broadcast(NewIntent);
        SetBrainState(EARPG_BrainState::Executing);
        
        if (BrainConfig.bEnableDebugLogging)
        {
            UE_LOG(LogARPG, Log, TEXT("Generated intent: %s (Confidence: %.2f)"), 
                   *NewIntent.IntentTag.ToString(), NewIntent.Confidence);
        }
    }
    else
    {
        SetBrainState(EARPG_BrainState::Processing);
    }
    
    return CurrentBrainState.CurrentIntent;
}

void UARPG_AIBrainComponent::UpdateBrain(float DeltaTime)
{
    if (!CurrentBrainState.bIsEnabled)
    {
        return;
    }
    
    TimeSinceLastUpdate += DeltaTime;
    CurrentBrainState.TimeSinceLastStimulus = GetWorld()->GetTimeSeconds() - LastStimulusTime;
    
    // Update stimuli memory
    UpdateStimuliMemory(DeltaTime);
    
    // Check if we should activate curiosity
    if (ShouldActivateCuriosity())
    {
        FARPG_AIIntent CuriosityIntent = GenerateCuriosityIntent();
        if (ValidateIntent(CuriosityIntent))
        {
            CurrentBrainState.CurrentIntent = CuriosityIntent;
            OnIntentChanged.Broadcast(CuriosityIntent);
            SetBrainState(EARPG_BrainState::Executing);
            
            if (BrainConfig.bEnableDebugLogging)
            {
                UE_LOG(LogARPG, VeryVerbose, TEXT("Activated curiosity intent: %s"), 
                       *CuriosityIntent.IntentTag.ToString());
            }
        }
    }
    
    // Clean up old data
    CleanupOldStimuli();
}

bool UARPG_AIBrainComponent::ShouldActivateCuriosity() const
{
    if (!CurrentBrainState.bIsEnabled)
    {
        return false;
    }
    
    // Activate curiosity if no stimuli for a while
    return CurrentBrainState.TimeSinceLastStimulus > BrainConfig.CuriosityThreshold;
}

FARPG_AIBrainState UARPG_AIBrainComponent::GetBrainState() const
{
    return CurrentBrainState;
}

void UARPG_AIBrainComponent::SetBrainEnabled(bool bEnabled)
{
    if (CurrentBrainState.bIsEnabled != bEnabled)
    {
        CurrentBrainState.bIsEnabled = bEnabled;
        
        if (bEnabled)
        {
            SetBrainState(EARPG_BrainState::Processing);
        }
        else
        {
            SetBrainState(EARPG_BrainState::Inactive);
        }
        
        if (BrainConfig.bEnableDebugLogging)
        {
            UE_LOG(LogARPG, Log, TEXT("Brain %s"), bEnabled ? TEXT("enabled") : TEXT("disabled"));
        }
    }
}

float UARPG_AIBrainComponent::GetIntentConfidence() const
{
    return CurrentBrainState.CurrentIntent.Confidence;
}

void UARPG_AIBrainComponent::ForceIntentGeneration()
{
    GenerateIntent();
}

void UARPG_AIBrainComponent::ClearBrainState()
{
    RecentStimuli.Empty();
    CurrentBrainState.RecentStimuli.Empty();
    CurrentBrainState.CurrentIntent = FARPG_AIIntent();
    CurrentBrainState.TimeSinceLastStimulus = 0.0f;
    TimeSinceLastUpdate = 0.0f;
    LastStimulusTime = 0.0f;
    
    if (BrainConfig.bEnableDebugLogging)
    {
        UE_LOG(LogARPG, Log, TEXT("Brain state cleared"));
    }
}

TArray<FARPG_AIStimulus> UARPG_AIBrainComponent::GetRecentStimuli() const
{
    return RecentStimuli;
}

FARPG_AIInputVector UARPG_AIBrainComponent::GetCurrentInputVector() const
{
    return BuildInputVector();
}

void UARPG_AIBrainComponent::InitializeComponentReferences()
{
    if (AActor* Owner = GetOwner())
    {
        MemoryComponent = Owner->FindComponentByClass<UARPG_AIMemoryComponent>();
        PerceptionComponent = Owner->FindComponentByClass<UARPG_AIPerceptionComponent>();
        NeedsComponent = Owner->FindComponentByClass<UARPG_AINeedsComponent>();
        PersonalityComponent = Owner->FindComponentByClass<UARPG_AIPersonalityComponent>();
        
        if (BrainConfig.bEnableDebugLogging)
        {
            UE_LOG(LogARPG, Log, TEXT("Brain component references initialized - Memory: %s, Perception: %s, Needs: %s, Personality: %s"),
                   MemoryComponent.IsValid() ? TEXT("Found") : TEXT("Missing"),
                   PerceptionComponent.IsValid() ? TEXT("Found") : TEXT("Missing"),
                   NeedsComponent.IsValid() ? TEXT("Found") : TEXT("Missing"),
                   PersonalityComponent.IsValid() ? TEXT("Found") : TEXT("Missing"));
        }
    }
}

void UARPG_AIBrainComponent::RegisterWithEventManager()
{
    if (UWorld* World = GetWorld())
    {
        if (UARPG_AIEventManager* EventManager = World->GetSubsystem<UARPG_AIEventManager>())
        {
            EventManager->RegisterBrainComponent(this);
            
            if (BrainConfig.bEnableDebugLogging)
            {
                UE_LOG(LogARPG, Log, TEXT("Brain registered with event manager"));
            }
        }
    }
}

void UARPG_AIBrainComponent::UnregisterFromEventManager()
{
    if (UWorld* World = GetWorld())
    {
        if (UARPG_AIEventManager* EventManager = World->GetSubsystem<UARPG_AIEventManager>())
        {
            EventManager->UnregisterBrainComponent(this);
        }
    }
}

FARPG_AIInputVector UARPG_AIBrainComponent::BuildInputVector() const
{
    FARPG_AIInputVector InputVector;
    
    if (!GetWorld())
    {
        return InputVector;
    }
    
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    // Process recent stimuli
    for (const FARPG_AIStimulus& Stimulus : RecentStimuli)
    {
        float Age = CurrentTime - Stimulus.Timestamp;
        if (Age <= BrainConfig.StimuliMemoryDuration)
        {
            // Apply decay based on age
            float DecayFactor = FMath::Max(0.0f, 1.0f - (Age / BrainConfig.StimuliMemoryDuration));
            float EffectiveIntensity = Stimulus.Intensity * DecayFactor;
            
            // Accumulate by stimulus type
            if (InputVector.StimuliStrengths.Contains(Stimulus.StimulusType))
            {
                InputVector.StimuliStrengths[Stimulus.StimulusType] = FMath::Max(
                    InputVector.StimuliStrengths[Stimulus.StimulusType], EffectiveIntensity);
            }
            else
            {
                InputVector.StimuliStrengths.Add(Stimulus.StimulusType, EffectiveIntensity);
            }
        }
    }
    
    // Get input from needs component
    if (NeedsComponent.IsValid())
    {
        InputVector.NeedLevels = NeedsComponent->GetAllNeedsAsMap();
    }
    
    // Get personality traits if available
    if (PersonalityComponent.IsValid())
    {
        InputVector.PersonalityTraits = PersonalityComponent->GetPersonalityTraits();
    }
    
    // Set environmental context
    InputVector.EnvironmentalContext.Add(TEXT("CurrentTime"), FString::SanitizeFloat(CurrentTime));
    InputVector.EnvironmentalContext.Add(TEXT("TimeSinceLastStimulus"), 
                                        FString::SanitizeFloat(CurrentBrainState.TimeSinceLastStimulus));
    
    return InputVector;
}

FARPG_AIIntent UARPG_AIBrainComponent::ProcessInputVector(const FARPG_AIInputVector& InputVector)
{
    FARPG_AIIntent Intent;
    Intent.CreationTime = GetWorld()->GetTimeSeconds();
    
    // Find the strongest stimulus
    float MaxStimulusStrength = 0.0f;
    EARPG_StimulusType DominantStimulusType = EARPG_StimulusType::Visual;
    
    for (const auto& StimulusPair : InputVector.StimuliStrengths)
    {
        if (StimulusPair.Value > MaxStimulusStrength)
        {
            MaxStimulusStrength = StimulusPair.Value;
            DominantStimulusType = StimulusPair.Key;
        }
    }
    
    // Generate intent based on dominant stimulus
    if (MaxStimulusStrength > 0.7f)
    {
        // High-intensity stimulus - immediate response needed
        switch (DominantStimulusType)
        {
            case EARPG_StimulusType::WorldEvent:
                Intent.IntentTag = FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.React"));
                Intent.Priority = EARPG_AIIntentPriority::High;
                break;
            case EARPG_StimulusType::Audio:
                Intent.IntentTag = FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Investigate"));
                Intent.Priority = EARPG_AIIntentPriority::High;
                break;
            case EARPG_StimulusType::Visual:
                Intent.IntentTag = FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Observe"));
                Intent.Priority = EARPG_AIIntentPriority::Medium;
                break;
            default:
                Intent.IntentTag = FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Alert"));
                Intent.Priority = EARPG_AIIntentPriority::Medium;
                break;
        }
        Intent.Confidence = MaxStimulusStrength;
    }
    else if (MaxStimulusStrength > 0.3f)
    {
        // Medium-intensity stimulus - moderate response
        Intent.IntentTag = FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Observe"));
        Intent.Priority = EARPG_AIIntentPriority::Medium;
        Intent.Confidence = MaxStimulusStrength;
    }
    else
    {
        // Low or no stimulus - default behavior
        Intent.IntentTag = DefaultIdleIntent;
        Intent.Priority = EARPG_AIIntentPriority::Idle;
        Intent.Confidence = 0.8f; // High confidence in idle behavior
    }
    
    return Intent;
}

void UARPG_AIBrainComponent::ApplyPersonalityToIntent(FARPG_AIIntent& Intent)
{
    if (PersonalityComponent.IsValid())
    {
        // This would need to be implemented in the PersonalityComponent
        // PersonalityComponent->ModifyIntent(Intent);
        
        // For now, basic personality application
        const TMap<FGameplayTag, float>& Traits = PersonalityComponent->GetPersonalityTraits();
        
        // Adjust confidence based on personality
        if (const float* Confidence = Traits.Find(FGameplayTag::RequestGameplayTag(TEXT("Personality.Confidence"))))
        {
            Intent.Confidence = FMath::Clamp(Intent.Confidence * (*Confidence + 0.5f), 0.0f, 1.0f);
        }
        
        // Adjust priority based on personality
        if (const float* Aggression = Traits.Find(FGameplayTag::RequestGameplayTag(TEXT("Personality.Aggression"))))
        {
            if (*Aggression > 0.7f && Intent.Priority == EARPG_AIIntentPriority::Medium)
            {
                Intent.Priority = EARPG_AIIntentPriority::High;
            }
        }
    }
}

bool UARPG_AIBrainComponent::ValidateIntent(const FARPG_AIIntent& Intent) const
{
    // Basic validation
    if (!Intent.IntentTag.IsValid())
    {
        return false;
    }
    
    if (Intent.Confidence < 0.0f || Intent.Confidence > 1.0f)
    {
        return false;
    }
    
    // Additional validation can be added here
    return true;
}

void UARPG_AIBrainComponent::UpdateStimuliMemory(float DeltaTime)
{
    if (!GetWorld())
    {
        return;
    }
    
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    // Remove old stimuli
    RecentStimuli.RemoveAll([CurrentTime, this](const FARPG_AIStimulus& Stimulus) {
        return (CurrentTime - Stimulus.Timestamp) > BrainConfig.StimuliMemoryDuration;
    });
    
    CurrentBrainState.RecentStimuli.RemoveAll([CurrentTime, this](const FARPG_AIStimulus& Stimulus) {
        return (CurrentTime - Stimulus.Timestamp) > BrainConfig.StimuliMemoryDuration;
    });
}

FARPG_AIIntent UARPG_AIBrainComponent::GenerateCuriosityIntent()
{
    // Try Blueprint implementation first
    FARPG_AIIntent BlueprintIntent = BP_GenerateCuriosityIntent();
    if (BlueprintIntent.IntentTag.IsValid())
    {
        return BlueprintIntent;
    }
    
    // Default C++ curiosity intent
    FARPG_AIIntent CuriosityIntent;
    CuriosityIntent.CreationTime = GetWorld()->GetTimeSeconds();
    CuriosityIntent.Priority = EARPG_AIIntentPriority::Low;
    CuriosityIntent.Confidence = 0.6f;
    
    // Choose random curiosity behavior
    if (CuriosityIntentTags.Num() > 0)
    {
        int32 RandomIndex = FMath::RandRange(0, CuriosityIntentTags.Num() - 1);
        CuriosityIntent.IntentTag = CuriosityIntentTags[RandomIndex];
    }
    else
    {
        CuriosityIntent.IntentTag = FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Wander"));
    }
    
    return CuriosityIntent;
}

void UARPG_AIBrainComponent::SetBrainState(EARPG_BrainState NewState)
{
    if (CurrentBrainState.CurrentState != NewState)
    {
        CurrentBrainState.CurrentState = NewState;
        OnBrainStateChanged.Broadcast(NewState);
        
        if (BrainConfig.bEnableDebugLogging)
        {
            UE_LOG(LogARPG, VeryVerbose, TEXT("Brain state changed to: %d"), (int32)NewState);
        }
    }
}

void UARPG_AIBrainComponent::AddStimulus(const FARPG_AIStimulus& Stimulus)
{
    FARPG_AIStimulus TimestampedStimulus = Stimulus;
    TimestampedStimulus.Timestamp = GetWorld()->GetTimeSeconds();
    
    RecentStimuli.Add(TimestampedStimulus);
    CurrentBrainState.RecentStimuli.Add(TimestampedStimulus);
    
    // Limit the number of stored stimuli
    const int32 MaxStimuli = 50;
    if (RecentStimuli.Num() > MaxStimuli)
    {
        RecentStimuli.RemoveAt(0, RecentStimuli.Num() - MaxStimuli);
    }
    if (CurrentBrainState.RecentStimuli.Num() > MaxStimuli)
    {
        CurrentBrainState.RecentStimuli.RemoveAt(0, CurrentBrainState.RecentStimuli.Num() - MaxStimuli);
    }
}

void UARPG_AIBrainComponent::CleanupOldStimuli()
{
    if (!GetWorld())
    {
        return;
    }
    
    float CurrentTime = GetWorld()->GetTimeSeconds();
    const float CleanupThreshold = BrainConfig.StimuliMemoryDuration * 1.5f; // Clean up 50% beyond memory duration
    
    RecentStimuli.RemoveAll([CurrentTime, CleanupThreshold](const FARPG_AIStimulus& Stimulus) {
        return (CurrentTime - Stimulus.Timestamp) > CleanupThreshold;
    });
}

void UARPG_AIBrainComponent::OnAIEventReceived(const FARPG_AIEvent& AIEvent)
{
    if (!CurrentBrainState.bIsEnabled)
    {
        return;
    }
    
    // Convert AI event to stimulus
    FARPG_AIStimulus EventStimulus;
    EventStimulus.StimulusType = EARPG_StimulusType::WorldEvent;
    EventStimulus.StimulusTag = AIEvent.EventType;
    EventStimulus.Intensity = AIEvent.EventStrength;
    EventStimulus.Location = AIEvent.EventLocation;
    EventStimulus.SourceActor = AIEvent.EventInstigator;
    
    // Add event-specific data
    EventStimulus.StimulusData.Add(TEXT("Global"), AIEvent.bGlobal ? TEXT("1") : TEXT("0"));
    EventStimulus.StimulusData.Add(TEXT("Radius"), FString::SanitizeFloat(AIEvent.EventRadius));
    
    ProcessStimulus(EventStimulus);
}

FARPG_AIIntent UARPG_AIBrainComponent::GetCurrentIntent() const
{
    return CurrentBrainState.CurrentIntent;
}

void UARPG_AIBrainComponent::ExecuteIntent(const FARPG_AIIntent& Intent)
{
    if (BehaviorExecutor && BehaviorExecutor->CanExecuteIntent(Intent))
    {
        BehaviorExecutor->StartExecution(Intent);
        if (BrainConfig.bEnableDebugLogging)
        {
            UE_LOG(LogARPG, Log, TEXT("Delegated intent to behavior executor: %s"), *Intent.IntentTag.ToString());
        }
    }
    else
    {
        if (BrainConfig.bEnableDebugLogging)
        {
            UE_LOG(LogARPG, Warning, TEXT("No behavior executor available for intent: %s"), *Intent.IntentTag.ToString());
        }
        // Fall back to idle
        SetBrainState(EARPG_BrainState::Idle);
    }
}

void UARPG_AIBrainComponent::LoadBrainConfiguration()
{
    bool bConfigLoaded = false;
    
    // Try to load from data table first
    if (bUseDataTableConfig && LoadConfigurationFromDataTable(EffectiveConfig))
    {
        bConfigLoaded = true;
        UE_LOG(LogARPG, Log, TEXT("Brain: Loaded configuration from DataTable row '%s'"), 
               *BrainConfigRowName.ToString());
    }
    
    // Fallback to blueprint configuration
    if (!bConfigLoaded)
    {
        EffectiveConfig = BrainConfig; // Use blueprint-set config
        UE_LOG(LogARPG, Log, TEXT("Brain: Using Blueprint configuration (DataTable: %s)"), 
               bUseDataTableConfig ? TEXT("Failed") : TEXT("Disabled"));
    }
    
    // Apply the effective configuration
    BrainConfig = EffectiveConfig;
}

void UARPG_AIBrainComponent::SetDataTableConfig(UDataTable* DataTable, FName RowName)
{
    BrainConfigDataTable = DataTable;
    BrainConfigRowName = RowName;
    bUseDataTableConfig = (DataTable != nullptr && !RowName.IsNone());
    
    // If we're already initialized, reload configuration
    if (HasBegunPlay())
    {
        LoadBrainConfiguration();
    }
}

FARPG_AIBrainConfiguration UARPG_AIBrainComponent::GetEffectiveConfiguration() const
{
    return EffectiveConfig;
}

bool UARPG_AIBrainComponent::LoadConfigurationFromDataTable(FARPG_AIBrainConfiguration& OutConfig) const
{
    if (!BrainConfigDataTable || BrainConfigRowName.IsNone())
    {
        return false;
    }
    
    // Find the row in the data table
    FString ContextString = FString::Printf(TEXT("Loading brain config for %s"), 
                                          GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"));
    
    FARPG_AIBrainConfigRow* ConfigRow = BrainConfigDataTable->FindRow<FARPG_AIBrainConfigRow>(
        BrainConfigRowName, ContextString);
    
    if (!ConfigRow)
    {
        UE_LOG(LogARPG, Warning, TEXT("Brain: Failed to find row '%s' in brain config data table"), 
               *BrainConfigRowName.ToString());
        return false;
    }
    
    // Convert data table row to configuration struct
    OutConfig.BrainUpdateFrequency = ConfigRow->BrainUpdateFrequency;
    OutConfig.StimuliMemoryDuration = ConfigRow->StimuliMemoryDuration;
    OutConfig.CuriosityThreshold = ConfigRow->CuriosityThreshold;
    OutConfig.CuriosityStrength = ConfigRow->CuriosityStrength;
    OutConfig.bCanFormMemories = ConfigRow->bCanFormMemories;
    OutConfig.bCanLearn = ConfigRow->bCanLearn;
    OutConfig.bEnableDebugLogging = ConfigRow->bEnableDebugLogging;
    OutConfig.MaxTrackedStimuli = ConfigRow->MaxTrackedStimuli;
    OutConfig.DefaultIdleIntent = ConfigRow->DefaultIdleIntent;
    OutConfig.CuriosityIntents = ConfigRow->CuriosityIntents;
    
    return true;
}