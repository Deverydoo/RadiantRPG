// Private/AI/Core/ARPG_AIMemoryComponent.cpp

#include "AI/Core/ARPG_AIMemoryComponent.h"

#include "EngineUtils.h"
#include "AI/Core/ARPG_AIBrainComponent.h"
#include "AI/Core/ARPG_AIEventManager.h"
#include "Engine/World.h"
#include "RadiantRPG.h"
#include "AI/Components/ARPG_RelationshipComponent.h"
#include "AI/Interfaces/ARPG_FactionInterface.h"
#include "Types/ARPG_AIDataTableTypes.h"
#include "World/RadiantZone.h"

UARPG_AIMemoryComponent::UARPG_AIMemoryComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 1.0f;
    
    // Initialize configuration
    MemoryConfig = FARPG_MemoryConfiguration();
    EffectiveConfig = MemoryConfig;
    MemoryDataTable = nullptr;
    DataTableRowName = NAME_None;
    bUseDataTableConfig = false;
    LastDecayUpdate = 0.0f;
    
    UE_LOG(LogARPG, Log, TEXT("AIMemoryComponent: Component created"));
}

void UARPG_AIMemoryComponent::BeginPlay()
{
    Super::BeginPlay();
    
    LoadMemoryConfiguration();
    InitializeMemoryStorage();
    InitializeComponentReferences();
    RegisterWithEventManager();
    
    UE_LOG(LogARPG, Log, TEXT("AIMemoryComponent: Initialized for %s with %s configuration"), 
           GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"),
           bUseDataTableConfig ? TEXT("data table") : TEXT("blueprint"));
}

void UARPG_AIMemoryComponent::LoadMemoryConfiguration()
{
    FARPG_MemoryConfiguration NewConfig;
    
    if (LoadConfigurationFromDataTable(NewConfig))
    {
        EffectiveConfig = NewConfig;
        bUseDataTableConfig = true;
        
        if (EffectiveConfig.bEnableDebugLogging)
        {
            UE_LOG(LogARPG, Log, TEXT("Memory Component: Loaded configuration from data table '%s' row '%s'"), 
                MemoryDataTable ? *MemoryDataTable->GetName() : TEXT("None"),
                *DataTableRowName.ToString());
        }
    }
    else
    {
        EffectiveConfig = MemoryConfig;
        bUseDataTableConfig = false;
        
        if (MemoryConfig.bEnableDebugLogging)
        {
            UE_LOG(LogARPG, Log, TEXT("Memory Component: Using blueprint configuration"));
        }
    }
}

void UARPG_AIMemoryComponent::SetDataTableConfig(UDataTable* DataTable, FName RowName)
{
    MemoryDataTable = DataTable;
    DataTableRowName = RowName;
    
    // If we're already initialized, reload configuration
    if (HasBegunPlay())
    {
        LoadMemoryConfiguration();
    }
}

FARPG_MemoryConfiguration UARPG_AIMemoryComponent::GetEffectiveConfiguration() const
{
    return EffectiveConfig;
}

bool UARPG_AIMemoryComponent::LoadConfigurationFromDataTable(FARPG_MemoryConfiguration& OutConfig) const
{
    if (!MemoryDataTable || DataTableRowName == NAME_None)
    {
        return false;
    }
    
    if (FARPG_MemoryConfigRow* ConfigRow = MemoryDataTable->FindRow<FARPG_MemoryConfigRow>(DataTableRowName, TEXT("Memory Config Lookup")))
    {
        // Convert data table row to configuration struct
        OutConfig.MaxShortTermMemories = ConfigRow->MaxShortTermMemories;
        OutConfig.MaxLongTermMemories = ConfigRow->MaxLongTermMemories;
        OutConfig.ShortTermDuration = ConfigRow->ShortTermDuration;
        OutConfig.LongTermThreshold = ConfigRow->LongTermThreshold;
        OutConfig.ForgetThreshold = ConfigRow->ForgetThreshold;
        OutConfig.DecayUpdateFrequency = ConfigRow->DecayUpdateFrequency;
        OutConfig.bEnableDebugLogging = ConfigRow->bEnableDebugLogging;
        
        return true;
    }
    
    UE_LOG(LogARPG, Warning, TEXT("Memory Component: Failed to find data table row '%s' in table '%s'"),
           *DataTableRowName.ToString(),
           MemoryDataTable ? *MemoryDataTable->GetName() : TEXT("None"));
    
    return false;
}

void UARPG_AIMemoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnregisterFromEventManager();
    Super::EndPlay(EndPlayReason);
}

void UARPG_AIMemoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    if (CurrentTime - LastDecayUpdate >= EffectiveConfig.DecayUpdateFrequency)
    {
        UpdateMemoryDecay();
        ProcessMemoryTransfer();
        CleanupForgottenMemories();
        LastDecayUpdate = CurrentTime;
    }
}

void UARPG_AIMemoryComponent::InitializeMemoryStorage()
{
    for (int32 i = 0; i < static_cast<int32>(EARPG_MemoryType::MAX); i++)
    {
        EARPG_MemoryType Type = static_cast<EARPG_MemoryType>(i);
        ShortTermMemories.Add(Type, TArray<FARPG_MemoryEntry>());
        LongTermMemories.Add(Type, TArray<FARPG_MemoryEntry>());
    }
}

void UARPG_AIMemoryComponent::InitializeComponentReferences()
{
    if (AActor* Owner = GetOwner())
    {
        BrainComponent = Owner->FindComponentByClass<UARPG_AIBrainComponent>();
        if (!BrainComponent.IsValid())
        {
            UE_LOG(LogARPG, Warning, TEXT("AIMemoryComponent: No brain component found on %s"), *Owner->GetName());
        }
    }
}

void UARPG_AIMemoryComponent::RegisterWithEventManager()
{
    if (UWorld* World = GetWorld())
    {
        if (UARPG_AIEventManager* EventManager = World->GetSubsystem<UARPG_AIEventManager>())
        {
            EventManager->RegisterSubscriber(this);
        }
    }
}

void UARPG_AIMemoryComponent::UnregisterFromEventManager()
{
    if (UWorld* World = GetWorld())
    {
        if (UARPG_AIEventManager* EventManager = World->GetSubsystem<UARPG_AIEventManager>())
        {
            EventManager->UnregisterSubscriber(this);
        }
    }
}

void UARPG_AIMemoryComponent::FormMemory(const FARPG_MemoryEntry& MemoryEntry)
{
    FARPG_MemoryEntry NewMemory = MemoryEntry;
    NewMemory.CreationTime = GetWorld()->GetTimeSeconds();
    NewMemory.LastAccessTime = NewMemory.CreationTime;
    
    bool bIsLongTerm = (NewMemory.Strength >= EffectiveConfig.LongTermThreshold) || 
                       NewMemory.bIsPermanent ||
                       (NewMemory.Relevance >= EARPG_MemoryRelevance::High);
    
    AddMemoryToStorage(NewMemory, bIsLongTerm);
    
    OnMemoryFormed.Broadcast(NewMemory, NewMemory.MemoryType);
    BP_OnMemoryFormed(NewMemory);
    
    if (EffectiveConfig.bEnableDebugLogging)
    {
        UE_LOG(LogARPG, Log, TEXT("Memory formed: %s (%s) - %s"), 
               *NewMemory.MemoryTag.ToString(), 
               bIsLongTerm ? TEXT("Long-term") : TEXT("Short-term"),
               *NewMemory.Description);
    }
}

void UARPG_AIMemoryComponent::AddMemoryToStorage(const FARPG_MemoryEntry& Memory, bool bIsLongTerm)
{
    if (bIsLongTerm)
    {
        TArray<FARPG_MemoryEntry>& MemoriesOfType = LongTermMemories[Memory.MemoryType];
        MemoriesOfType.Add(Memory);
        
        if (MemoriesOfType.Num() > EffectiveConfig.MaxLongTermMemories)
        {
            MemoriesOfType.Sort([](const FARPG_MemoryEntry& A, const FARPG_MemoryEntry& B) {
                if (A.bIsPermanent != B.bIsPermanent)
                {
                    return B.bIsPermanent;
                }
                return A.Strength < B.Strength;
            });
            
            if (!MemoriesOfType[0].bIsPermanent)
            {
                MemoriesOfType.RemoveAt(0);
            }
        }
    }
    else
    {
        TArray<FARPG_MemoryEntry>& MemoriesOfType = ShortTermMemories[Memory.MemoryType];
        MemoriesOfType.Add(Memory);
        
        if (MemoriesOfType.Num() > EffectiveConfig.MaxShortTermMemories)
        {
            MemoriesOfType.Sort([](const FARPG_MemoryEntry& A, const FARPG_MemoryEntry& B) {
                return A.Strength < B.Strength;
            });
            MemoriesOfType.RemoveAt(0);
        }
    }
}

void UARPG_AIMemoryComponent::UpdateMemoryDecay()
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    TArray<TMap<EARPG_MemoryType, TArray<FARPG_MemoryEntry>>*> Storages = {&ShortTermMemories, &LongTermMemories};
    
    for (auto* Storage : Storages)
    {
        for (auto& TypePair : *Storage)
        {
            for (FARPG_MemoryEntry& Memory : TypePair.Value)
            {
                Memory.Strength = Memory.GetCurrentStrength(CurrentTime);
            }
        }
    }
}

void UARPG_AIMemoryComponent::ProcessMemoryTransfer()
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    for (auto& TypePair : ShortTermMemories)
    {
        for (int32 i = TypePair.Value.Num() - 1; i >= 0; i--)
        {
            FARPG_MemoryEntry& Memory = TypePair.Value[i];
            float MemoryAge = CurrentTime - Memory.CreationTime;
            
            bool ShouldTransfer = false;
            
            if (Memory.Strength >= EffectiveConfig.LongTermThreshold && 
                MemoryAge >= EffectiveConfig.ShortTermDuration * 0.5f)
            {
                ShouldTransfer = true;
            }
            else if (Memory.bIsVivid && MemoryAge >= EffectiveConfig.ShortTermDuration * 0.3f)
            {
                ShouldTransfer = true;
            }
            else if (MemoryAge >= EffectiveConfig.ShortTermDuration)
            {
                ShouldTransfer = Memory.Strength >= EffectiveConfig.LongTermThreshold * 0.7f;
            }
            
            if (ShouldTransfer)
            {
                FARPG_MemoryEntry TransferredMemory = Memory;
                TypePair.Value.RemoveAt(i);
                AddMemoryToStorage(TransferredMemory, true);
                
                if (EffectiveConfig.bEnableDebugLogging)
                {
                    UE_LOG(LogARPG, Log, TEXT("Memory transferred to long-term: %s"), 
                           *TransferredMemory.MemoryTag.ToString());
                }
            }
        }
    }
}

void UARPG_AIMemoryComponent::CleanupForgottenMemories()
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    int32 ForgottenCount = 0;
    
    TArray<TMap<EARPG_MemoryType, TArray<FARPG_MemoryEntry>>*> Storages = {&ShortTermMemories, &LongTermMemories};
    
    for (auto* Storage : Storages)
    {
        for (auto& TypePair : *Storage)
        {
            for (int32 i = TypePair.Value.Num() - 1; i >= 0; i--)
            {
                const FARPG_MemoryEntry& Memory = TypePair.Value[i];
                if (!Memory.bIsPermanent && Memory.ShouldForget(CurrentTime, EffectiveConfig.ForgetThreshold))
                {
                    OnMemoryForgotten.Broadcast(Memory, Memory.MemoryType);
                    BP_OnMemoryForgotten(Memory);
                    TypePair.Value.RemoveAt(i);
                    ForgottenCount++;
                }
            }
        }
    }
    
    if (ForgottenCount > 0 && EffectiveConfig.bEnableDebugLogging)
    {
        UE_LOG(LogARPG, Log, TEXT("Forgot %d memories during cleanup"), ForgottenCount);
    }
}

void UARPG_AIMemoryComponent::SetMemoryConfiguration(const FARPG_MemoryConfiguration& NewConfig)
{
    MemoryConfig = NewConfig;
    EffectiveConfig = NewConfig;
    bUseDataTableConfig = false;
    
    CleanupForgottenMemories();
    
    if (EffectiveConfig.bEnableDebugLogging)
    {
        UE_LOG(LogARPG, Log, TEXT("Memory configuration updated at runtime"));
    }
}

void UARPG_AIMemoryComponent::OnAIEventReceived(FGameplayTag EventType, const FARPG_AIEvent& EventData)
{
    if (ShouldFormMemoryFromEvent(EventData))
    {
        EARPG_MemoryRelevance Relevance = DetermineEventRelevance(EventData);
        float EmotionalWeight = CalculateEmotionalWeight(EventData);
        
        FormEventMemory(EventData, Relevance, EmotionalWeight);
    }
}

bool UARPG_AIMemoryComponent::ShouldFormMemoryFromEvent(const FARPG_AIEvent& Event) const
{
    if (Event.EventStrength < 0.1f)
    {
        return false;
    }
    
    if (Event.bGlobal)
    {
        return true;
    }
    
    if (Event.EventRadius > 0.0f)
    {
        float Distance = FVector::Dist(Event.EventLocation, GetOwner()->GetActorLocation());
        return Distance <= Event.EventRadius;
    }
    
    return true;
}

EARPG_MemoryRelevance UARPG_AIMemoryComponent::DetermineEventRelevance(const FARPG_AIEvent& Event) const
{
    EARPG_MemoryRelevance BlueprintResult = BP_DetermineEventRelevance(Event);
    if (BlueprintResult != EARPG_MemoryRelevance::MAX)
    {
        return BlueprintResult;
    }
    
    FGameplayTag EventType = Event.EventType;
    
    if (EventType.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Event.Combat"))))
    {
        return EARPG_MemoryRelevance::High;
    }
    
    if (EventType.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Event.Death"))))
    {
        return EARPG_MemoryRelevance::Critical;
    }
    
    if (EventType.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Event.Social"))))
    {
        return EARPG_MemoryRelevance::Medium;
    }
    
    if (EventType.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Event.Time"))))
    {
        return EARPG_MemoryRelevance::Low;
    }
    
    if (Event.EventTarget == GetOwner() || Event.EventInstigator == GetOwner())
    {
        return EARPG_MemoryRelevance::High;
    }
    
    if (Event.EventStrength > 0.7f)
    {
        return EARPG_MemoryRelevance::High;
    }
    else if (Event.EventStrength > 0.4f)
    {
        return EARPG_MemoryRelevance::Medium;
    }
    else
    {
        return EARPG_MemoryRelevance::Low;
    }
}

float UARPG_AIMemoryComponent::CalculateEmotionalWeight(const FARPG_AIEvent& Event) const
{
    float BlueprintWeight = BP_CalculateEmotionalWeight(Event);
    if (BlueprintWeight != 0.0f)
    {
        return BlueprintWeight;
    }
    
    if (Event.EventType.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Event.Positive"))))
    {
        return 0.7f;
    }
    else if (Event.EventType.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Event.Negative"))))
    {
        return -0.7f;
    }
    
    return 0.0f;
}

void UARPG_AIMemoryComponent::FormEventMemory(const FARPG_AIEvent& Event, EARPG_MemoryRelevance Relevance, float EmotionalWeight)
{
    if (!ShouldFormMemoryFromEvent(Event))
    {
        return;
    }
    
    FARPG_MemoryEntry Memory;
    Memory.MemoryType = EARPG_MemoryType::Event;
    Memory.MemoryTag = Event.EventType;
    Memory.Relevance = Relevance;
    Memory.Location = Event.EventLocation;
    Memory.AssociatedActor = Event.EventInstigator;
    Memory.SecondaryActor = Event.EventTarget;
    Memory.EmotionalWeight = EmotionalWeight;
    Memory.Strength = FMath::Clamp(Event.EventStrength, 0.1f, 1.0f);
    Memory.DecayRate = 0.1f;
    
    FString ActorName = Event.EventInstigator.IsValid() ? Event.EventInstigator->GetName() : TEXT("Unknown");
    Memory.Description = FString::Printf(TEXT("Event: %s by %s"), *Event.EventType.ToString(), *ActorName);
    
    Memory.MemoryData.Add(TEXT("EventStrength"), FString::SanitizeFloat(Event.EventStrength));
    Memory.MemoryData.Add(TEXT("EventRadius"), FString::SanitizeFloat(Event.EventRadius));
    if (Event.bGlobal)
    {
        Memory.MemoryData.Add(TEXT("Global"), TEXT("true"));
    }
    
    if (FMath::Abs(EmotionalWeight) > 0.5f)
    {
        Memory.bIsVivid = true;
    }
    
    FormMemory(Memory);
}

float UARPG_AIMemoryComponent::GetRelevanceStrength(EARPG_MemoryRelevance Relevance) const
{
    // Convert relevance enum to memory strength values
    // Higher relevance = stronger initial memory strength
    switch (Relevance)
    {
    case EARPG_MemoryRelevance::Trivial:
        return 0.2f; // Very weak memories, likely to be forgotten quickly
            
    case EARPG_MemoryRelevance::Low:
        return 0.4f; // Weak memories, fade unless reinforced
            
    case EARPG_MemoryRelevance::Medium:
        return 0.6f; // Moderate strength, standard memory strength
            
    case EARPG_MemoryRelevance::High:
        return 0.8f; // Strong memories, likely to persist
            
    case EARPG_MemoryRelevance::Critical:
        return 1.0f; // Maximum strength, very persistent
            
    default:
        // Fallback for any undefined relevance values
            UE_LOG(LogARPG, Warning, TEXT("GetRelevanceStrength: Unknown relevance type %d, defaulting to medium"), 
                   static_cast<int32>(Relevance));
        return 0.6f;
    }
}

void UARPG_AIMemoryComponent::FormLocationMemory(FVector Location, FGameplayTag LocationTag, const FString& Description, EARPG_MemoryRelevance Relevance)
{
    FARPG_MemoryEntry Memory;
    Memory.MemoryType = EARPG_MemoryType::Location;
    Memory.MemoryTag = LocationTag;
    Memory.Relevance = Relevance;
    Memory.Location = Location;
    Memory.Strength = GetRelevanceStrength(Relevance);
    Memory.DecayRate = CalculateLocationDecayRate(LocationTag, Relevance);
    Memory.Description = Description.IsEmpty() ? 
        FString::Printf(TEXT("Location: %s"), *LocationTag.ToString()) : Description;
    
    // Store location-specific data for spatial queries
    Memory.MemoryData.Add(TEXT("LocationX"), FString::SanitizeFloat(Location.X));
    Memory.MemoryData.Add(TEXT("LocationY"), FString::SanitizeFloat(Location.Y));
    Memory.MemoryData.Add(TEXT("LocationZ"), FString::SanitizeFloat(Location.Z));
    
    // Add zone information if available
    if (UWorld* World = GetWorld())
    {
        if (ARadiantZone* Zone = FindZoneAtLocation(Location))
        {
            FGameplayTag ZoneTag = Zone->GetZoneTag();
            if (ZoneTag.IsValid())
            {
                Memory.MemoryData.Add(TEXT("Zone"), ZoneTag.ToString());
                Memory.MemoryData.Add(TEXT("ZoneType"), FString::FromInt(static_cast<int32>(Zone->GetZoneType())));
            }
        }
    }
    
    // Check if we already have a memory of this location
    TArray<FARPG_MemoryEntry> ExistingLocationMemories = GetMemoriesNearLocation(Location, 500.0f, 5);
    for (const FARPG_MemoryEntry& ExistingMemory : ExistingLocationMemories)
    {
        if (ExistingMemory.MemoryTag == LocationTag)
        {
            // Reinforce existing memory instead of creating duplicate
            ReinforceLocationMemory(ExistingMemory, Memory.Strength);
            return;
        }
    }
    
    FormMemory(Memory);
    
    if (EffectiveConfig.bEnableDebugLogging)
    {
        UE_LOG(LogARPG, VeryVerbose, TEXT("Location memory formed: %s at %s"), 
               *LocationTag.ToString(), *Location.ToString());
    }
}

ARadiantZone* UARPG_AIMemoryComponent::FindZoneAtLocation(FVector Location) const
{
    if (UWorld* World = GetWorld())
    {
        // Find all RadiantZone actors in the world
        for (TActorIterator<ARadiantZone> ActorIterator(World); ActorIterator; ++ActorIterator)
        {
            ARadiantZone* Zone = *ActorIterator;
            if (Zone && Zone->IsLocationInZone(Location))
            {
                return Zone;
            }
        }
    }
    return nullptr;
}

float UARPG_AIMemoryComponent::CalculateLocationDecayRate(FGameplayTag LocationTag, EARPG_MemoryRelevance Relevance) const
{
    float BaseDecayRate = 0.05f; // Locations decay slowly
    
    // Adjust based on relevance
    switch (Relevance)
    {
        case EARPG_MemoryRelevance::Critical:
            return BaseDecayRate * 0.1f; // Very slow decay
        case EARPG_MemoryRelevance::High:
            return BaseDecayRate * 0.5f;
        case EARPG_MemoryRelevance::Medium:
            return BaseDecayRate;
        case EARPG_MemoryRelevance::Low:
            return BaseDecayRate * 2.0f;
        default:
            return BaseDecayRate;
    }
}

void UARPG_AIMemoryComponent::ReinforceLocationMemory(const FARPG_MemoryEntry& ExistingMemory, float StrengthBoost)
{
    // Find and reinforce the existing memory
    TArray<TMap<EARPG_MemoryType, TArray<FARPG_MemoryEntry>>*> Storages = {&ShortTermMemories, &LongTermMemories};
    
    for (auto* Storage : Storages)
    {
        if (Storage->Contains(EARPG_MemoryType::Location))
        {
            TArray<FARPG_MemoryEntry>& LocationMemories = (*Storage)[EARPG_MemoryType::Location];
            for (FARPG_MemoryEntry& Memory : LocationMemories)
            {
                if (Memory.MemoryTag == ExistingMemory.MemoryTag && 
                    FVector::Dist(Memory.Location, ExistingMemory.Location) < 100.0f)
                {
                    Memory.Strength = FMath::Clamp(Memory.Strength + StrengthBoost * 0.3f, 0.0f, 1.0f);
                    Memory.LastAccessTime = GetWorld()->GetTimeSeconds();
                    
                    if (EffectiveConfig.bEnableDebugLogging)
                    {
                        UE_LOG(LogARPG, VeryVerbose, TEXT("Reinforced location memory: %s (new strength: %.2f)"), 
                               *Memory.MemoryTag.ToString(), Memory.Strength);
                    }
                    return;
                }
            }
        }
    }
}

void UARPG_AIMemoryComponent::FormEntityMemory(AActor* Entity, FGameplayTag EntityTag, const FString& Description, float EmotionalWeight)
{
    if (!IsValid(Entity))
    {
        UE_LOG(LogARPG, Warning, TEXT("FormEntityMemory called with invalid entity"));
        return;
    }
    
    FARPG_MemoryEntry Memory;
    Memory.MemoryType = EARPG_MemoryType::Entity;
    Memory.MemoryTag = EntityTag;
    Memory.Relevance = DetermineEntityRelevance(Entity, EmotionalWeight);
    Memory.Location = Entity->GetActorLocation();
    Memory.AssociatedActor = Entity;
    Memory.EmotionalWeight = EmotionalWeight;
    Memory.Strength = CalculateEntityMemoryStrength(Entity, EmotionalWeight);
    Memory.DecayRate = CalculateEntityDecayRate(Entity, EmotionalWeight);
    Memory.Description = Description.IsEmpty() ? 
        FString::Printf(TEXT("Entity: %s (%s)"), *Entity->GetName(), *EntityTag.ToString()) : Description;
    
    // Store entity-specific data
    Memory.MemoryData.Add(TEXT("EntityClass"), Entity->GetClass()->GetName());
    Memory.MemoryData.Add(TEXT("FirstMet"), FString::SanitizeFloat(GetWorld()->GetTimeSeconds()));
    
    // Store faction information if available
    if (IARPG_FactionInterface* FactionInterface = Cast<IARPG_FactionInterface>(Entity))
    {
        FGameplayTag FactionTag = FactionInterface->GetFactionTag();
        if (FactionTag.IsValid())
        {
            Memory.MemoryData.Add(TEXT("Faction"), FactionTag.ToString());
        }
    }
    
    // Store relationship information
    if (BrainComponent.IsValid())
    {
        if (UARPG_RelationshipComponent* RelationshipComp = 
            BrainComponent->GetOwner()->FindComponentByClass<UARPG_RelationshipComponent>())
        {
            float Relationship = RelationshipComp->GetRelationshipValue(Entity);
            Memory.MemoryData.Add(TEXT("Relationship"), FString::SanitizeFloat(Relationship));
        }
    }
    
    // Check for existing entity memory to avoid duplicates
    if (HasMemoryAboutActor(Entity))
    {
        ReinforceEntityMemory(Entity, Memory.Strength, EmotionalWeight);
        return;
    }
    
    // Mark as vivid if emotionally significant
    if (FMath::Abs(EmotionalWeight) > 0.5f)
    {
        Memory.bIsVivid = true;
    }
    
    // Mark as permanent for critically important entities
    if (Memory.Relevance == EARPG_MemoryRelevance::Critical || 
        EntityTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Entity.Player"))))
    {
        Memory.bIsPermanent = true;
    }
    
    FormMemory(Memory);
    
    if (EffectiveConfig.bEnableDebugLogging)
    {
        UE_LOG(LogARPG, VeryVerbose, TEXT("Entity memory formed: %s (%s) - Emotional Weight: %.2f"), 
               *Entity->GetName(), *EntityTag.ToString(), EmotionalWeight);
    }
}

EARPG_MemoryRelevance UARPG_AIMemoryComponent::DetermineEntityRelevance(AActor* Entity, float EmotionalWeight) const
{
    if (!IsValid(Entity))
    {
        return EARPG_MemoryRelevance::Low;
    }
    
    // Players are always critical
    if (Entity->IsA<APawn>() && Cast<APawn>(Entity)->IsPlayerControlled())
    {
        return EARPG_MemoryRelevance::Critical;
    }
    
    // High emotional weight indicates high relevance
    if (FMath::Abs(EmotionalWeight) > 0.7f)
    {
        return EARPG_MemoryRelevance::High;
    }
    
    // Check faction relationships
    if (BrainComponent.IsValid())
    {
        if (UARPG_RelationshipComponent* RelationshipComp = 
            BrainComponent->GetOwner()->FindComponentByClass<UARPG_RelationshipComponent>())
        {
            float Relationship = RelationshipComp->GetRelationshipValue(Entity);
            if (FMath::Abs(Relationship) > 0.5f)
            {
                return EARPG_MemoryRelevance::High;
            }
        }
    }
    
    // Moderate emotional weight or unknown entities
    if (FMath::Abs(EmotionalWeight) > 0.3f)
    {
        return EARPG_MemoryRelevance::Medium;
    }
    
    return EARPG_MemoryRelevance::Low;
}

float UARPG_AIMemoryComponent::CalculateEntityMemoryStrength(AActor* Entity, float EmotionalWeight) const
{
    float BaseStrength = 0.8f; // Entities are inherently memorable
    
    // Boost for emotional encounters
    float EmotionalBoost = FMath::Abs(EmotionalWeight) * 0.3f;
    
    // Players get a strength boost
    if (Entity->IsA<APawn>() && Cast<APawn>(Entity)->IsPlayerControlled())
    {
        BaseStrength = 1.0f;
    }
    
    return FMath::Clamp(BaseStrength + EmotionalBoost, 0.1f, 1.0f);
}

float UARPG_AIMemoryComponent::CalculateEntityDecayRate(AActor* Entity, float EmotionalWeight) const
{
    float BaseDecayRate = 0.03f; // Entities decay slowly - we remember people
    
    // Emotional memories decay slower
    if (FMath::Abs(EmotionalWeight) > 0.5f)
    {
        BaseDecayRate *= 0.5f;
    }
    
    // Player memories decay extremely slowly
    if (Entity->IsA<APawn>() && Cast<APawn>(Entity)->IsPlayerControlled())
    {
        BaseDecayRate *= 0.1f;
    }
    
    return BaseDecayRate;
}

void UARPG_AIMemoryComponent::ReinforceEntityMemory(AActor* Entity, float StrengthBoost, float NewEmotionalWeight)
{
    if (!IsValid(Entity))
    {
        return;
    }
    
    TArray<TMap<EARPG_MemoryType, TArray<FARPG_MemoryEntry>>*> Storages = {&ShortTermMemories, &LongTermMemories};
    
    for (auto* Storage : Storages)
    {
        if (Storage->Contains(EARPG_MemoryType::Entity))
        {
            TArray<FARPG_MemoryEntry>& EntityMemories = (*Storage)[EARPG_MemoryType::Entity];
            for (FARPG_MemoryEntry& Memory : EntityMemories)
            {
                if (Memory.AssociatedActor == Entity)
                {
                    // Reinforce strength
                    Memory.Strength = FMath::Clamp(Memory.Strength + StrengthBoost * 0.5f, 0.0f, 1.0f);
                    Memory.LastAccessTime = GetWorld()->GetTimeSeconds();
                    
                    // Update emotional weight (weighted average)
                    float CurrentWeight = Memory.EmotionalWeight;
                    Memory.EmotionalWeight = (CurrentWeight + NewEmotionalWeight) * 0.5f;
                    
                    // Update location
                    Memory.Location = Entity->GetActorLocation();
                    
                    // Update relationship if available
                    if (BrainComponent.IsValid())
                    {
                        if (UARPG_RelationshipComponent* RelationshipComp = 
                            BrainComponent->GetOwner()->FindComponentByClass<UARPG_RelationshipComponent>())
                        {
                            float Relationship = RelationshipComp->GetRelationshipValue(Entity);
                            Memory.MemoryData.Add(TEXT("Relationship"), FString::SanitizeFloat(Relationship));
                        }
                    }
                    
                    // Make vivid if emotionally significant
                    if (FMath::Abs(Memory.EmotionalWeight) > 0.5f)
                    {
                        Memory.bIsVivid = true;
                    }
                    
                    if (EffectiveConfig.bEnableDebugLogging)
                    {
                        UE_LOG(LogARPG, VeryVerbose, TEXT("Reinforced entity memory: %s (new strength: %.2f, emotional weight: %.2f)"), 
                               *Entity->GetName(), Memory.Strength, Memory.EmotionalWeight);
                    }
                    return;
                }
            }
        }
    }
}

float FARPG_MemoryEntry::GetCurrentStrength(float CurrentTime) const
{
    if (bIsPermanent)
    {
        return Strength;
    }

    float TimeSinceCreation = CurrentTime - CreationTime;
    float EffectiveDecayRate = DecayRate;
    
    if (bIsVivid)
    {
        EffectiveDecayRate *= 0.5f;
    }
    
    float EmotionalBoost = FMath::Abs(EmotionalWeight) * 2.0f;
    EffectiveDecayRate = FMath::Max(0.01f, EffectiveDecayRate - EmotionalBoost);
    
    float DecayMultiplier = FMath::Exp(-EffectiveDecayRate * TimeSinceCreation / 3600.0f);
    
    return FMath::Max(0.0f, Strength * DecayMultiplier);
}

bool FARPG_MemoryEntry::ShouldForget(float CurrentTime, float ForgetThreshold) const
{
    if (bIsPermanent)
    {
        return false;
    }
    
    return GetCurrentStrength(CurrentTime) < ForgetThreshold;
}

void FARPG_MemoryEntry::AccessMemory(float CurrentTime)
{
    LastAccessTime = CurrentTime;
    AccessCount++;
    
    float ReinforcementBoost = 0.02f * FMath::Min(5, AccessCount);
    Strength = FMath::Min(1.0f, Strength + ReinforcementBoost);
}

float FARPG_MemoryEntry::GetRelevanceFloat() const
{
    switch (Relevance)
    {
    case EARPG_MemoryRelevance::Trivial:  return 0.1f;
    case EARPG_MemoryRelevance::Low:      return 0.3f;
    case EARPG_MemoryRelevance::Medium:   return 0.5f;
    case EARPG_MemoryRelevance::High:     return 0.7f;
    case EARPG_MemoryRelevance::Critical: return 0.9f;
    default: return 0.5f;
    }
}

// === Query Methods Implementation ===

TArray<FARPG_MemoryEntry> UARPG_AIMemoryComponent::QueryMemories(const FARPG_MemoryQuery& Query) const
{
    TArray<FARPG_MemoryEntry> Results;
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    TArray<TMap<EARPG_MemoryType, TArray<FARPG_MemoryEntry>>*> MemoryStorages = {
        const_cast<TMap<EARPG_MemoryType, TArray<FARPG_MemoryEntry>>*>(&ShortTermMemories),
        const_cast<TMap<EARPG_MemoryType, TArray<FARPG_MemoryEntry>>*>(&LongTermMemories)
    };
    
    for (auto* Storage : MemoryStorages)
    {
        if (Query.MemoryType != EARPG_MemoryType::MAX && Storage->Contains(Query.MemoryType))
        {
            const TArray<FARPG_MemoryEntry>& MemoriesOfType = (*Storage)[Query.MemoryType];
            for (const FARPG_MemoryEntry& Memory : MemoriesOfType)
            {
                if (DoesMemoryMatchQuery(Memory, Query))
                {
                    FARPG_MemoryEntry MemoryCopy = Memory;
                    if (Query.bAccessMemories)
                    {
                        const_cast<FARPG_MemoryEntry&>(Memory).AccessMemory(CurrentTime);
                    }
                    Results.Add(MemoryCopy);
                }
            }
        }
        else if (Query.MemoryType == EARPG_MemoryType::MAX)
        {
            for (const auto& TypePair : *Storage)
            {
                for (const FARPG_MemoryEntry& Memory : TypePair.Value)
                {
                    if (DoesMemoryMatchQuery(Memory, Query))
                    {
                        FARPG_MemoryEntry MemoryCopy = Memory;
                        if (Query.bAccessMemories)
                        {
                            const_cast<FARPG_MemoryEntry&>(Memory).AccessMemory(CurrentTime);
                        }
                        Results.Add(MemoryCopy);
                    }
                }
            }
        }
    }
    
    if (Query.bSortByRelevance)
    {
        SortMemoriesByRelevance(Results);
    }
    else if (Query.bSortByTime)
    {
        SortMemoriesByTime(Results, true);
    }
    
    if (Query.MaxResults > 0 && Results.Num() > Query.MaxResults)
    {
        Results.SetNum(Query.MaxResults);
    }
    
    return Results;
}

TArray<FARPG_MemoryEntry> UARPG_AIMemoryComponent::GetRecentMemories(EARPG_MemoryType MemoryType, float TimeWindow, int32 MaxResults) const
{
    FARPG_MemoryQuery Query;
    Query.MemoryType = MemoryType;
    Query.TimeWindow = TimeWindow;
    Query.MaxResults = MaxResults;
    Query.bSortByTime = true;
    Query.bAccessMemories = false;
    
    return QueryMemories(Query);
}

TArray<FARPG_MemoryEntry> UARPG_AIMemoryComponent::GetMemoriesAboutActor(AActor* Actor, int32 MaxResults) const
{
    if (!IsValid(Actor))
    {
        return TArray<FARPG_MemoryEntry>();
    }
    
    FARPG_MemoryQuery Query;
    Query.MemoryType = EARPG_MemoryType::MAX;
    Query.bRequireActor = true;
    Query.RequiredActor = Actor;
    Query.MaxResults = MaxResults;
    Query.bSortByRelevance = true;
    
    return QueryMemories(Query);
}

TArray<FARPG_MemoryEntry> UARPG_AIMemoryComponent::GetMemoriesNearLocation(FVector Location, float Radius, int32 MaxResults) const
{
    FARPG_MemoryQuery Query;
    Query.MemoryType = EARPG_MemoryType::MAX;
    Query.SearchLocation = Location;
    Query.SearchRadius = Radius;
    Query.MaxResults = MaxResults;
    Query.bSortByRelevance = true;
    
    return QueryMemories(Query);
}

TArray<FARPG_MemoryEntry> UARPG_AIMemoryComponent::GetEmotionalMemories(float MinEmotionalWeight, int32 MaxResults) const
{
    TArray<FARPG_MemoryEntry> Results;
    
    TArray<const TMap<EARPG_MemoryType, TArray<FARPG_MemoryEntry>>*> Storages = {&ShortTermMemories, &LongTermMemories};
    
    for (const auto* Storage : Storages)
    {
        for (const auto& TypePair : *Storage)
        {
            for (const FARPG_MemoryEntry& Memory : TypePair.Value)
            {
                if (FMath::Abs(Memory.EmotionalWeight) >= MinEmotionalWeight)
                {
                    Results.Add(Memory);
                }
            }
        }
    }
    
    Results.Sort([](const FARPG_MemoryEntry& A, const FARPG_MemoryEntry& B) {
        return FMath::Abs(A.EmotionalWeight) > FMath::Abs(B.EmotionalWeight);
    });
    
    if (MaxResults > 0 && Results.Num() > MaxResults)
    {
        Results.SetNum(MaxResults);
    }
    
    return Results;
}

TArray<FARPG_MemoryEntry> UARPG_AIMemoryComponent::GetStrongestMemories(EARPG_MemoryType MemoryType, int32 MaxResults) const
{
    TArray<FARPG_MemoryEntry> Results;
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    TArray<const TMap<EARPG_MemoryType, TArray<FARPG_MemoryEntry>>*> Storages = {&ShortTermMemories, &LongTermMemories};
    
    for (const auto* Storage : Storages)
    {
        if (MemoryType != EARPG_MemoryType::MAX && Storage->Contains(MemoryType))
        {
            const TArray<FARPG_MemoryEntry>& MemoriesOfType = (*Storage)[MemoryType];
            Results.Append(MemoriesOfType);
        }
        else if (MemoryType == EARPG_MemoryType::MAX)
        {
            for (const auto& TypePair : *Storage)
            {
                Results.Append(TypePair.Value);
            }
        }
    }
    
    Results.Sort([CurrentTime](const FARPG_MemoryEntry& A, const FARPG_MemoryEntry& B) {
        return A.GetCurrentStrength(CurrentTime) > B.GetCurrentStrength(CurrentTime);
    });
    
    if (MaxResults > 0 && Results.Num() > MaxResults)
    {
        Results.SetNum(MaxResults);
    }
    
    return Results;
}

float UARPG_AIMemoryComponent::GetMemoryStrengthFor(FGameplayTag MemoryTag) const
{
    float TotalStrength = 0.0f;
    int32 MatchingMemories = 0;
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    TArray<const TMap<EARPG_MemoryType, TArray<FARPG_MemoryEntry>>*> Storages = {&ShortTermMemories, &LongTermMemories};
    
    for (const auto* Storage : Storages)
    {
        for (const auto& TypePair : *Storage)
        {
            for (const FARPG_MemoryEntry& Memory : TypePair.Value)
            {
                if (Memory.MemoryTag.MatchesTag(MemoryTag))
                {
                    TotalStrength += Memory.GetCurrentStrength(CurrentTime);
                    MatchingMemories++;
                }
            }
        }
    }
    
    return MatchingMemories > 0 ? TotalStrength / MatchingMemories : 0.0f;
}

    // === Memory Management Implementation ===

void UARPG_AIMemoryComponent::ForgetMemory(int32 MemoryIndex)
{
    UE_LOG(LogARPG, Warning, TEXT("ForgetMemory by index not fully implemented"));
}

void UARPG_AIMemoryComponent::ForgetMemoriesAboutActor(AActor* Actor)
{
    if (!IsValid(Actor))
    {
        return;
    }
    
    int32 ForgottenCount = 0;
    TArray<TMap<EARPG_MemoryType, TArray<FARPG_MemoryEntry>>*> Storages = {&ShortTermMemories, &LongTermMemories};
    
    for (auto* Storage : Storages)
    {
        for (auto& TypePair : *Storage)
        {
            for (int32 i = TypePair.Value.Num() - 1; i >= 0; i--)
            {
                const FARPG_MemoryEntry& Memory = TypePair.Value[i];
                if (Memory.AssociatedActor == Actor || Memory.SecondaryActor == Actor)
                {
                    OnMemoryForgotten.Broadcast(Memory, Memory.MemoryType);
                    BP_OnMemoryForgotten(Memory);
                    TypePair.Value.RemoveAt(i);
                    ForgottenCount++;
                }
            }
        }
    }
    
    if (EffectiveConfig.bEnableDebugLogging && ForgottenCount > 0)
    {
        UE_LOG(LogARPG, Log, TEXT("Forgot %d memories about %s"), ForgottenCount, *Actor->GetName());
    }
}

void UARPG_AIMemoryComponent::ForgetMemoriesOfType(EARPG_MemoryType MemoryType)
{
    int32 ForgottenCount = 0;
    
    if (ShortTermMemories.Contains(MemoryType))
    {
        TArray<FARPG_MemoryEntry>& Memories = ShortTermMemories[MemoryType];
        for (const FARPG_MemoryEntry& Memory : Memories)
        {
            OnMemoryForgotten.Broadcast(Memory, Memory.MemoryType);
            BP_OnMemoryForgotten(Memory);
        }
        ForgottenCount += Memories.Num();
        Memories.Empty();
    }
    
    if (LongTermMemories.Contains(MemoryType))
    {
        TArray<FARPG_MemoryEntry>& Memories = LongTermMemories[MemoryType];
        for (const FARPG_MemoryEntry& Memory : Memories)
        {
            OnMemoryForgotten.Broadcast(Memory, Memory.MemoryType);
            BP_OnMemoryForgotten(Memory);
        }
        ForgottenCount += Memories.Num();
        Memories.Empty();
    }
    
    if (EffectiveConfig.bEnableDebugLogging)
    {
        UE_LOG(LogARPG, Log, TEXT("Forgot %d memories of type %d"), ForgottenCount, (int32)MemoryType);
    }
}

void UARPG_AIMemoryComponent::ClearAllMemories()
{
    int32 TotalForgotten = 0;
    
    TArray<TMap<EARPG_MemoryType, TArray<FARPG_MemoryEntry>>*> Storages = {&ShortTermMemories, &LongTermMemories};
    
    for (auto* Storage : Storages)
    {
        for (auto& TypePair : *Storage)
        {
            for (const FARPG_MemoryEntry& Memory : TypePair.Value)
            {
                if (!Memory.bIsPermanent)
                {
                    OnMemoryForgotten.Broadcast(Memory, Memory.MemoryType);
                    BP_OnMemoryForgotten(Memory);
                    TotalForgotten++;
                }
            }
            
            TypePair.Value.RemoveAll([](const FARPG_MemoryEntry& Memory) {
                return !Memory.bIsPermanent;
            });
        }
    }
    
    if (EffectiveConfig.bEnableDebugLogging)
    {
        UE_LOG(LogARPG, Log, TEXT("Cleared all memories - forgot %d total"), TotalForgotten);
    }
}

int32 UARPG_AIMemoryComponent::GetMemoryCount(EARPG_MemoryType MemoryType) const
{
    if (MemoryType == EARPG_MemoryType::MAX)
    {
        int32 Total = 0;
        for (const auto& TypePair : ShortTermMemories)
        {
            Total += TypePair.Value.Num();
        }
        for (const auto& TypePair : LongTermMemories)
        {
            Total += TypePair.Value.Num();
        }
        return Total;
    }
    else
    {
        int32 Count = 0;
        if (ShortTermMemories.Contains(MemoryType))
        {
            Count += ShortTermMemories[MemoryType].Num();
        }
        if (LongTermMemories.Contains(MemoryType))
        {
            Count += LongTermMemories[MemoryType].Num();
        }
        return Count;
    }
}

int32 UARPG_AIMemoryComponent::GetShortTermMemoryCount() const
{
    int32 Total = 0;
    for (const auto& TypePair : ShortTermMemories)
    {
        Total += TypePair.Value.Num();
    }
    return Total;
}

int32 UARPG_AIMemoryComponent::GetLongTermMemoryCount() const
{
    int32 Total = 0;
    for (const auto& TypePair : LongTermMemories)
    {
        Total += TypePair.Value.Num();
    }
    return Total;
}

bool UARPG_AIMemoryComponent::HasMemoryAboutActor(AActor* Actor) const
{
    if (!IsValid(Actor))
    {
        return false;
    }
    
    TArray<const TMap<EARPG_MemoryType, TArray<FARPG_MemoryEntry>>*> Storages = {&ShortTermMemories, &LongTermMemories};
    
    for (const auto* Storage : Storages)
    {
        for (const auto& TypePair : *Storage)
        {
            for (const FARPG_MemoryEntry& Memory : TypePair.Value)
            {
                if (Memory.AssociatedActor == Actor || Memory.SecondaryActor == Actor)
                {
                    return true;
                }
            }
        }
    }
    
    return false;
}

bool UARPG_AIMemoryComponent::HasMemoryOfType(EARPG_MemoryType MemoryType, FGameplayTag SpecificTag) const
{
    TArray<const TMap<EARPG_MemoryType, TArray<FARPG_MemoryEntry>>*> Storages = {&ShortTermMemories, &LongTermMemories};
    
    for (const auto* Storage : Storages)
    {
        if (Storage->Contains(MemoryType))
        {
            const TArray<FARPG_MemoryEntry>& MemoriesOfType = (*Storage)[MemoryType];
            for (const FARPG_MemoryEntry& Memory : MemoriesOfType)
            {
                if (SpecificTag.IsValid())
                {
                    if (Memory.MemoryTag.MatchesTag(SpecificTag))
                    {
                        return true;
                    }
                }
                else
                {
                    return true;
                }
            }
        }
    }
    
    return false;
}

void UARPG_AIMemoryComponent::ReinforceMemory(int32 MemoryIndex, float StrengthBoost)
{
    UE_LOG(LogARPG, Warning, TEXT("ReinforceMemory by index not fully implemented"));
}

void UARPG_AIMemoryComponent::MakeMemoryVivid(int32 MemoryIndex)
{
    UE_LOG(LogARPG, Warning, TEXT("MakeMemoryVivid by index not fully implemented"));
}

void UARPG_AIMemoryComponent::MakeMemoryPermanent(int32 MemoryIndex)
{
    UE_LOG(LogARPG, Warning, TEXT("MakeMemoryPermanent by index not fully implemented"));
}

void UARPG_AIMemoryComponent::ContributeToInputs(FARPG_AIInputVector& Inputs) const
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    TArray<FARPG_MemoryEntry> RecentThreats = GetRecentMemories(EARPG_MemoryType::Event, 300.0f, 10);
    float ThreatMemoryStrength = 0.0f;
    
    for (const FARPG_MemoryEntry& Memory : RecentThreats)
    {
        if (Memory.MemoryTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Event.Threat"))))
        {
            ThreatMemoryStrength += Memory.GetCurrentStrength(CurrentTime);
        }
    }
    
    // Add threat memory input (this would need to be defined in FARPG_AIInputVector)
    // Inputs.ThreatMemory = ThreatMemoryStrength;
    
    if (EffectiveConfig.bEnableDebugLogging)
    {
        UE_LOG(LogARPG, Verbose, TEXT("Memory contributed threat strength: %f"), ThreatMemoryStrength);
    }
}

    // === Helper Functions ===

bool UARPG_AIMemoryComponent::DoesMemoryMatchQuery(const FARPG_MemoryEntry& Memory, const FARPG_MemoryQuery& Query) const
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    if (Query.TimeWindow > 0.0f)
    {
        float MemoryAge = CurrentTime - Memory.CreationTime;
        if (MemoryAge > Query.TimeWindow)
        {
            return false;
        }
    }
    
    if (Query.SearchRadius > 0.0f)
    {
        float Distance = FVector::Dist(Memory.Location, Query.SearchLocation);
        if (Distance > Query.SearchRadius)
        {
            return false;
        }
    }
    
    if (Query.bRequireActor && Query.RequiredActor.IsValid())
    {
        if (Memory.AssociatedActor != Query.RequiredActor && Memory.SecondaryActor != Query.RequiredActor)
        {
            return false;
        }
    }
    
    if (Memory.GetCurrentStrength(CurrentTime) < 0.05f)
    {
        return false;
    }
    
    return true;
}

void UARPG_AIMemoryComponent::SortMemoriesByRelevance(TArray<FARPG_MemoryEntry>& Memories) const
{
    Memories.Sort([](const FARPG_MemoryEntry& A, const FARPG_MemoryEntry& B) {
        return A.GetRelevanceFloat() > B.GetRelevanceFloat();
    });
}

void UARPG_AIMemoryComponent::SortMemoriesByTime(TArray<FARPG_MemoryEntry>& Memories, bool bMostRecentFirst) const
{
    Memories.Sort([bMostRecentFirst](const FARPG_MemoryEntry& A, const FARPG_MemoryEntry& B) {
        return bMostRecentFirst ? (A.CreationTime > B.CreationTime) : (A.CreationTime < B.CreationTime);
    });
}