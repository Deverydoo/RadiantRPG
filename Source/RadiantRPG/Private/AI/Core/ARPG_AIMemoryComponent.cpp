// Source/RadiantRPG/Private/AI/Core/ARPG_AIMemoryComponent.cpp

#include "AI/Core/ARPG_AIMemoryComponent.h"
#include "AI/Core/ARPG_AIBrainComponent.h"
#include "AI/Core/ARPG_AIEventManager.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "RadiantRPG.h"

// Memory Entry Implementation
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

// Component Implementation
UARPG_AIMemoryComponent::UARPG_AIMemoryComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 1.0f;
    
    MemoryConfig = FARPG_MemoryConfiguration();
    
    UE_LOG(LogARPG, Log, TEXT("AIMemoryComponent: Component created"));
}

void UARPG_AIMemoryComponent::BeginPlay()
{
    Super::BeginPlay();
    
    InitializeMemoryStorage();
    RegisterWithEventManager();
    
    if (AActor* Owner = GetOwner())
    {
        BrainComponent = Owner->FindComponentByClass<UARPG_AIBrainComponent>();
        if (!BrainComponent.IsValid())
        {
            UE_LOG(LogARPG, Warning, TEXT("AIMemoryComponent: No brain component found on %s"), *Owner->GetName());
        }
    }
    
    UE_LOG(LogARPG, Log, TEXT("AIMemoryComponent: Initialized for %s"), 
           GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"));
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
    
    if (CurrentTime - LastDecayUpdate >= MemoryConfig.DecayUpdateFrequency)
    {
        UpdateMemoryDecay();
        ProcessMemoryTransfer();
        CleanupForgottenMemories();
        LastDecayUpdate = CurrentTime;
    }
}

void UARPG_AIMemoryComponent::FormMemory(const FARPG_MemoryEntry& MemoryEntry)
{
    FARPG_MemoryEntry NewMemory = MemoryEntry;
    NewMemory.CreationTime = GetWorld()->GetTimeSeconds();
    NewMemory.LastAccessTime = NewMemory.CreationTime;
    
    bool bIsLongTerm = (NewMemory.Strength >= MemoryConfig.LongTermThreshold) || 
                       NewMemory.bIsPermanent ||
                       (NewMemory.Relevance >= EARPG_MemoryRelevance::High);
    
    AddMemoryToStorage(NewMemory, bIsLongTerm);
    
    OnMemoryFormed.Broadcast(NewMemory, NewMemory.MemoryType);
    BP_OnMemoryFormed(NewMemory);
    
    if (MemoryConfig.bEnableDebugLogging)
    {
        UE_LOG(LogARPG, Log, TEXT("Memory formed: %s (%s) - %s"), 
               *NewMemory.MemoryTag.ToString(), 
               bIsLongTerm ? TEXT("Long-term") : TEXT("Short-term"),
               *NewMemory.Description);
    }
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
    
    FormMemory(Memory);
}

void UARPG_AIMemoryComponent::FormLocationMemory(FVector Location, FGameplayTag LocationTag, const FString& Description, EARPG_MemoryRelevance Relevance)
{
    FARPG_MemoryEntry Memory;
    Memory.MemoryType = EARPG_MemoryType::Location;
    Memory.MemoryTag = LocationTag;
    Memory.Relevance = Relevance;
    Memory.Location = Location;
    Memory.Strength = 0.7f;
    Memory.DecayRate = 0.05f;
    Memory.Description = Description.IsEmpty() ? FString::Printf(TEXT("Location: %s"), *LocationTag.ToString()) : Description;
    
    Memory.MemoryData.Add(TEXT("LocationX"), FString::SanitizeFloat(Location.X));
    Memory.MemoryData.Add(TEXT("LocationY"), FString::SanitizeFloat(Location.Y));
    Memory.MemoryData.Add(TEXT("LocationZ"), FString::SanitizeFloat(Location.Z));
    
    FormMemory(Memory);
}

void UARPG_AIMemoryComponent::FormEntityMemory(AActor* Entity, FGameplayTag EntityTag, const FString& Description, EARPG_MemoryRelevance Relevance, float EmotionalWeight)
{
    if (!IsValid(Entity))
    {
        return;
    }
    
    FARPG_MemoryEntry Memory;
    Memory.MemoryType = EARPG_MemoryType::Entity;
    Memory.MemoryTag = EntityTag;
    Memory.Relevance = Relevance;
    Memory.Location = Entity->GetActorLocation();
    Memory.AssociatedActor = Entity;
    Memory.EmotionalWeight = EmotionalWeight;
    Memory.Strength = 0.8f;
    Memory.DecayRate = 0.03f;
    Memory.Description = Description.IsEmpty() ? 
        FString::Printf(TEXT("Entity: %s (%s)"), *Entity->GetName(), *EntityTag.ToString()) : Description;
    
    Memory.MemoryData.Add(TEXT("EntityClass"), Entity->GetClass()->GetName());
    Memory.MemoryData.Add(TEXT("FirstMet"), FString::SanitizeFloat(GetWorld()->GetTimeSeconds()));
    
    if (FMath::Abs(EmotionalWeight) > 0.5f)
    {
        Memory.bIsVivid = true;
    }
    
    FormMemory(Memory);
}

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
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    TArray<const TMap<EARPG_MemoryType, TArray<FARPG_MemoryEntry>>*> Storages = {&ShortTermMemories, &LongTermMemories};
    
    for (const auto* Storage : Storages)
    {
        for (const auto& TypePair : *Storage)
        {
            for (const FARPG_MemoryEntry& Memory : TypePair.Value)
            {
                if (FMath::Abs(Memory.EmotionalWeight) >= MinEmotionalWeight)
                {
                    float CurrentStrength = Memory.GetCurrentStrength(CurrentTime);
                    if (CurrentStrength > MemoryConfig.ForgetThreshold)
                    {
                        Results.Add(Memory);
                    }
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
        if (MemoryType == EARPG_MemoryType::MAX)
        {
            for (const auto& TypePair : *Storage)
            {
                for (const FARPG_MemoryEntry& Memory : TypePair.Value)
                {
                    float CurrentStrength = Memory.GetCurrentStrength(CurrentTime);
                    if (CurrentStrength > MemoryConfig.ForgetThreshold)
                    {
                        Results.Add(Memory);
                    }
                }
            }
        }
        else if (Storage->Contains(MemoryType))
        {
            for (const FARPG_MemoryEntry& Memory : (*Storage)[MemoryType])
            {
                float CurrentStrength = Memory.GetCurrentStrength(CurrentTime);
                if (CurrentStrength > MemoryConfig.ForgetThreshold)
                {
                    Results.Add(Memory);
                }
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

void UARPG_AIMemoryComponent::ForgetMemory(int32 MemoryIndex)
{
    UE_LOG(LogARPG, Warning, TEXT("ForgetMemory by index not fully implemented - needs better indexing system"));
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
    
    if (MemoryConfig.bEnableDebugLogging && ForgottenCount > 0)
    {
        UE_LOG(LogARPG, Log, TEXT("Forgot %d memories about %s"), ForgottenCount, *Actor->GetName());
    }
}

void UARPG_AIMemoryComponent::ForgetMemoriesOfType(EARPG_MemoryType MemoryType)
{
    int32 ForgottenCount = 0;
    
    if (ShortTermMemories.Contains(MemoryType))
    {
        ForgottenCount += ShortTermMemories[MemoryType].Num();
        for (const FARPG_MemoryEntry& Memory : ShortTermMemories[MemoryType])
        {
            OnMemoryForgotten.Broadcast(Memory, Memory.MemoryType);
            BP_OnMemoryForgotten(Memory);
        }
        ShortTermMemories[MemoryType].Empty();
    }
    
    if (LongTermMemories.Contains(MemoryType))
    {
        ForgottenCount += LongTermMemories[MemoryType].Num();
        for (const FARPG_MemoryEntry& Memory : LongTermMemories[MemoryType])
        {
            OnMemoryForgotten.Broadcast(Memory, Memory.MemoryType);
            BP_OnMemoryForgotten(Memory);
        }
        LongTermMemories[MemoryType].Empty();
    }
    
    if (MemoryConfig.bEnableDebugLogging && ForgottenCount > 0)
    {
        UE_LOG(LogARPG, Log, TEXT("Forgot %d memories of type %d"), ForgottenCount, static_cast<int32>(MemoryType));
    }
}

void UARPG_AIMemoryComponent::ClearAllMemories()
{
    int32 TotalForgotten = 0;
    
    for (auto& TypePair : ShortTermMemories)
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
    
    for (auto& TypePair : LongTermMemories)
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
    
    if (MemoryConfig.bEnableDebugLogging)
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

void UARPG_AIMemoryComponent::SetMemoryConfiguration(const FARPG_MemoryConfiguration& NewConfig)
{
    MemoryConfig = NewConfig;
    
    CleanupForgottenMemories();
    
    if (MemoryConfig.bEnableDebugLogging)
    {
        UE_LOG(LogARPG, Log, TEXT("Memory configuration updated"));
    }
}

void UARPG_AIMemoryComponent::ContributeToInputs(FARPG_AIInputVector& Inputs) const
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    TArray<FARPG_MemoryEntry> RecentThreats = GetRecentMemories(EARPG_MemoryType::Event, 300.0f, 10);
    float ThreatMemoryStrength = 0.0f;
    
    for (const FARPG_MemoryEntry& Memory : RecentThreats)
    {
        if (Memory.MemoryTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Event.Combat"))) ||
            Memory.MemoryTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Event.Danger"))))
        {
            ThreatMemoryStrength += Memory.GetCurrentStrength(CurrentTime) * Memory.GetRelevanceFloat();
        }
    }
    
    TArray<FARPG_MemoryEntry> RecentSocial = GetRecentMemories(EARPG_MemoryType::Event, 600.0f, 5);
    float SocialMemoryStrength = 0.0f;
    
    for (const FARPG_MemoryEntry& Memory : RecentSocial)
    {
        if (Memory.MemoryTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Event.Social"))))
        {
            SocialMemoryStrength += Memory.GetCurrentStrength(CurrentTime) * Memory.EmotionalWeight;
        }
    }
    
    Inputs.ThreatLevel = FMath::Max(Inputs.ThreatLevel, ThreatMemoryStrength);
    Inputs.SocialNeed = FMath::Clamp(Inputs.SocialNeed + (SocialMemoryStrength * 0.1f), 0.0f, 1.0f);
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

void UARPG_AIMemoryComponent::InitializeMemoryStorage()
{
    for (int32 i = 0; i < static_cast<int32>(EARPG_MemoryType::MAX); i++)
    {
        EARPG_MemoryType Type = static_cast<EARPG_MemoryType>(i);
        ShortTermMemories.Add(Type, TArray<FARPG_MemoryEntry>());
        LongTermMemories.Add(Type, TArray<FARPG_MemoryEntry>());
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
            
            if (Memory.Strength >= MemoryConfig.LongTermThreshold && 
                MemoryAge >= MemoryConfig.ShortTermDuration * 0.5f)
            {
                ShouldTransfer = true;
            }
            
            if (Memory.Relevance >= EARPG_MemoryRelevance::High)
            {
                ShouldTransfer = true;
            }
            
            if (Memory.bIsVivid && MemoryAge >= MemoryConfig.ShortTermDuration * 0.3f)
            {
                ShouldTransfer = true;
            }
            
            if (ShouldTransfer)
            {
                LongTermMemories[Memory.MemoryType].Add(Memory);
                TypePair.Value.RemoveAt(i);
                
                if (MemoryConfig.bEnableDebugLogging)
                {
                    UE_LOG(LogARPG, VeryVerbose, TEXT("Transferred memory to long-term: %s"), *Memory.Description);
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
                if (Memory.ShouldForget(CurrentTime, MemoryConfig.ForgetThreshold))
                {
                    OnMemoryForgotten.Broadcast(Memory, Memory.MemoryType);
                    BP_OnMemoryForgotten(Memory);
                    TypePair.Value.RemoveAt(i);
                    ForgottenCount++;
                }
            }
        }
    }
    
    for (auto& TypePair : ShortTermMemories)
    {
        if (TypePair.Value.Num() > MemoryConfig.MaxShortTermMemories)
        {
            TypePair.Value.Sort([CurrentTime](const FARPG_MemoryEntry& A, const FARPG_MemoryEntry& B) {
                float StrengthA = A.GetCurrentStrength(CurrentTime);
                float StrengthB = B.GetCurrentStrength(CurrentTime);
                if (FMath::IsNearlyEqual(StrengthA, StrengthB))
                {
                    return A.CreationTime < B.CreationTime;
                }
                return StrengthA < StrengthB;
            });
            
            int32 ToRemove = TypePair.Value.Num() - MemoryConfig.MaxShortTermMemories;
            for (int32 i = 0; i < ToRemove; i++)
            {
                OnMemoryForgotten.Broadcast(TypePair.Value[i], TypePair.Value[i].MemoryType);
                BP_OnMemoryForgotten(TypePair.Value[i]);
                ForgottenCount++;
            }
            TypePair.Value.RemoveAt(0, FMath::Min(ToRemove, TypePair.Value.Num()));
        }
    }
    
    if (MemoryConfig.bEnableDebugLogging && ForgottenCount > 0)
    {
        UE_LOG(LogARPG, VeryVerbose, TEXT("Cleanup: Forgot %d memories"), ForgottenCount);
    }
}

void UARPG_AIMemoryComponent::RegisterWithEventManager()
{
    if (UWorld* World = GetWorld())
    {
        if (UARPG_AIEventManager* EventManager = World->GetSubsystem<UARPG_AIEventManager>())
        {
            EventManager->OnAnyEvent.AddDynamic(this, &UARPG_AIMemoryComponent::OnAIEventReceived);
            
            if (MemoryConfig.bEnableDebugLogging)
            {
                UE_LOG(LogARPG, Log, TEXT("Memory component registered with event manager"));
            }
        }
    }
}

void UARPG_AIMemoryComponent::UnregisterFromEventManager()
{
    if (UWorld* World = GetWorld())
    {
        if (UARPG_AIEventManager* EventManager = World->GetSubsystem<UARPG_AIEventManager>())
        {
            EventManager->OnAnyEvent.RemoveDynamic(this, &UARPG_AIMemoryComponent::OnAIEventReceived);
        }
    }
}

void UARPG_AIMemoryComponent::AddMemoryToStorage(const FARPG_MemoryEntry& Memory, bool bIsLongTerm)
{
    if (bIsLongTerm)
    {
        LongTermMemories[Memory.MemoryType].Add(Memory);
    }
    else
    {
        ShortTermMemories[Memory.MemoryType].Add(Memory);
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
    else if (Event.EventStrength > 0.5f)
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
    float BlueprintResult = BP_CalculateEmotionalWeight(Event);
    if (!FMath::IsNearlyZero(BlueprintResult))
    {
        return BlueprintResult;
    }
    
    FGameplayTag EventType = Event.EventType;
    
    if (EventType.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Event.Combat"))))
    {
        return Event.EventTarget == GetOwner() ? -0.8f : -0.3f;
    }
    
    if (EventType.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Event.Death"))))
    {
        return -1.0f;
    }
    
    if (EventType.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Event.Social.Positive"))))
    {
        return 0.5f;
    }
    
    if (EventType.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Event.Social.Negative"))))
    {
        return -0.5f;
    }
    
    return 0.0f;
}

bool UARPG_AIMemoryComponent::DoesMemoryMatchQuery(const FARPG_MemoryEntry& Memory, const FARPG_MemoryQuery& Query) const
{
    if (Query.RequiredTag.IsValid() && !Memory.MemoryTag.MatchesTag(Query.RequiredTag))
    {
        return false;
    }
    
    if (Memory.Relevance < Query.MinRelevance)
    {
        return false;
    }
    
    if (Query.TimeWindow > 0.0f)
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();
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
    
    if (Query.bRequireActor && Memory.AssociatedActor != Query.RequiredActor && Memory.SecondaryActor != Query.RequiredActor)
    {
        return false;
    }
    
    if (Query.MinStrength > 0.0f)
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();
        if (Memory.GetCurrentStrength(CurrentTime) < Query.MinStrength)
        {
            return false;
        }
    }
    
    return true;
}

void UARPG_AIMemoryComponent::SortMemoriesByRelevance(TArray<FARPG_MemoryEntry>& Memories) const
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    Memories.Sort([CurrentTime](const FARPG_MemoryEntry& A, const FARPG_MemoryEntry& B) {
        float ScoreA = A.GetRelevanceFloat() * A.GetCurrentStrength(CurrentTime);
        float ScoreB = B.GetRelevanceFloat() * B.GetCurrentStrength(CurrentTime);
        return ScoreA > ScoreB;
    });
}

void UARPG_AIMemoryComponent::SortMemoriesByTime(TArray<FARPG_MemoryEntry>& Memories, bool bNewestFirst) const
{
    if (bNewestFirst)
    {
        Memories.Sort([](const FARPG_MemoryEntry& A, const FARPG_MemoryEntry& B) {
            return A.CreationTime > B.CreationTime;
        });
    }
    else
    {
        Memories.Sort([](const FARPG_MemoryEntry& A, const FARPG_MemoryEntry& B) {
            return A.CreationTime < B.CreationTime;
        });
    }
}