// Private/Core/RadiantPlayerState.cpp
// Player state implementation for RadiantRPG - manages skill-based character progression and player data

#include "Core/RadiantPlayerState.h"
#include "Core/RadiantGameState.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySet.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"

ARadiantPlayerState::ARadiantPlayerState()
{
    // Enable replication
    bReplicates = true;
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 1.0f; // Update every second for statistics

    // Create ability system component
    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

    // Create attribute set - will be set up in derived classes or blueprints
    AttributeSet = nullptr;

    // Initialize configuration
    TotalSkillCap = 700.0f; // UO-style total skill cap
    IndividualSkillCap = 100.0f; // Maximum value for any single skill
    ExperienceMultiplier = 1.0f;

    // Initialize internal state
    CachedEffectiveLevel = 1;
    bPlayerDataInitialized = false;
    SessionStartTime = 0.0;
    LastPosition = FVector::ZeroVector;

    // Initialize cached references
    RadiantGameState = nullptr;

    UE_LOG(LogTemp, Log, TEXT("RadiantPlayerState constructed"));
}

void ARadiantPlayerState::BeginPlay()
{
    Super::BeginPlay();

    // Cache game state reference
    RadiantGameState = GetWorld()->GetGameState<ARadiantGameState>();

    // Initialize session tracking
    SessionStartTime = FPlatformTime::Seconds();

    // Initialize player data if we're the authority
    if (HasAuthority())
    {
        InitializePlayerData();
    }

    // Build initial caches from replicated data
    RebuildSkillsCache();
    RebuildFactionReputationsCache();

    UE_LOG(LogTemp, Log, TEXT("RadiantPlayerState began play"));
}

void ARadiantPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Update final playtime statistics
    if (HasAuthority())
    {
        UpdatePlaytimeStatistics();
    }

    Super::EndPlay(EndPlayReason);
}

void ARadiantPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Replicate core player data using TArrays (UE 5.5 compatible)
    DOREPLIFETIME(ARadiantPlayerState, SkillEntries);
    DOREPLIFETIME(ARadiantPlayerState, FactionReputationEntries);
    DOREPLIFETIME(ARadiantPlayerState, Statistics);
    DOREPLIFETIME(ARadiantPlayerState, CharacterID);
    DOREPLIFETIME(ARadiantPlayerState, CharacterName);
}

void ARadiantPlayerState::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    // Initialize ability system component
    if (AbilitySystemComponent && HasAuthority())
    {
        // Grant default abilities
        GrantDefaultAbilities();
    }

    UE_LOG(LogTemp, Log, TEXT("RadiantPlayerState PostInitializeComponents complete"));
}

UAbilitySystemComponent* ARadiantPlayerState::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

// === SKILL SYSTEM IMPLEMENTATION ===

float ARadiantPlayerState::GetSkillValue(FGameplayTag SkillTag) const
{
    if (const FSkillData* SkillData = SkillsCache.Find(SkillTag))
    {
        return SkillData->CurrentValue;
    }
    return 0.0f;
}

FSkillData ARadiantPlayerState::GetSkillData(FGameplayTag SkillTag) const
{
    if (const FSkillData* SkillData = SkillsCache.Find(SkillTag))
    {
        return *SkillData;
    }
    return FSkillData();
}

void ARadiantPlayerState::AddSkillExperience(FGameplayTag SkillTag, float ExperienceAmount)
{
    if (!HasAuthority())
    {
        return;
    }

    // Apply experience multiplier
    float AdjustedExperience = ExperienceAmount * ExperienceMultiplier;
    
    ApplySkillExperience(SkillTag, AdjustedExperience);
}

void ARadiantPlayerState::SetSkillValue(FGameplayTag SkillTag, float NewValue)
{
    if (!HasAuthority())
    {
        return;
    }

    // Clamp to valid range
    NewValue = FMath::Clamp(NewValue, 0.0f, IndividualSkillCap);

    float OldValue = GetSkillValue(SkillTag);
    
    // Update cache
    FSkillData& SkillData = SkillsCache.FindOrAdd(SkillTag);
    SkillData.CurrentValue = NewValue;
    SkillData.CurrentExperience = CalculateSkillLevelFromExperience(NewValue);

    // Update replicated array
    UpdateReplicatedSkillEntries();

    // Update cached values
    UpdateCachedValues();

    // Broadcast change
    if (NewValue != OldValue)
    {
        OnSkillChanged.Broadcast(SkillTag, OldValue, NewValue);
        UE_LOG(LogTemp, Log, TEXT("Skill %s set to %.1f"), *SkillTag.ToString(), NewValue);
    }
}

float ARadiantPlayerState::GetTotalSkillPoints() const
{
    float Total = 0.0f;
    for (const auto& SkillPair : SkillsCache)
    {
        Total += SkillPair.Value.CurrentValue;
    }
    return Total;
}

int32 ARadiantPlayerState::GetEffectiveLevel() const
{
    return CachedEffectiveLevel;
}

float ARadiantPlayerState::GetSkillProgress(FGameplayTag SkillTag) const
{
    const FSkillData* SkillData = SkillsCache.Find(SkillTag);
    if (!SkillData)
    {
        return 0.0f;
    }

    float CurrentLevel = SkillData->CurrentValue;
    float CurrentExp = SkillData->CurrentExperience;
    float NextLevelExp = CalculateExperienceForNextLevel(SkillTag);
    float CurrentLevelExp = 0.0f; // Would need experience curve calculation

    if (NextLevelExp <= CurrentLevelExp)
    {
        return 100.0f; // Max level
    }

    return ((CurrentExp - CurrentLevelExp) / (NextLevelExp - CurrentLevelExp)) * 100.0f;
}

bool ARadiantPlayerState::CanLearnSkill(FGameplayTag SkillTag) const
{
    // Check if total skill cap would be exceeded
    float CurrentTotal = GetTotalSkillPoints();
    return (CurrentTotal < TotalSkillCap);
}

// === FACTION SYSTEM IMPLEMENTATION ===

float ARadiantPlayerState::GetFactionReputation(FGameplayTag FactionTag) const
{
    if (const float* Reputation = FactionReputationsCache.Find(FactionTag))
    {
        return *Reputation;
    }
    return 0.0f;
}

void ARadiantPlayerState::SetFactionReputation(FGameplayTag FactionTag, float NewReputation)
{
    if (!HasAuthority())
    {
        return;
    }

    float OldReputation = GetFactionReputation(FactionTag);
    
    // Update cache
    FactionReputationsCache.FindOrAdd(FactionTag) = NewReputation;

    // Update replicated array
    UpdateReplicatedFactionEntries();

    // Broadcast change
    if (NewReputation != OldReputation)
    {
        OnFactionReputationChangedBP(FactionTag, OldReputation, NewReputation);
        UE_LOG(LogTemp, Log, TEXT("Faction %s reputation set to %.1f"), *FactionTag.ToString(), NewReputation);
    }
}

void ARadiantPlayerState::AddFactionReputation(FGameplayTag FactionTag, float ReputationChange)
{
    if (!HasAuthority())
    {
        return;
    }

    float CurrentReputation = GetFactionReputation(FactionTag);
    SetFactionReputation(FactionTag, CurrentReputation + ReputationChange);
}

// === INTERNAL FUNCTIONS ===

void ARadiantPlayerState::InitializePlayerData()
{
    if (bPlayerDataInitialized)
    {
        return;
    }

    // Generate unique character ID if not set
    if (CharacterID.IsEmpty())
    {
        CharacterID = FGuid::NewGuid().ToString();
    }

    // Initialize statistics
    Statistics = FPlayerStatistics();
    Statistics.CreationTime = FPlatformTime::Seconds();

    // Initialize default skills if empty
    if (SkillsCache.Num() == 0)
    {
        // Add some basic starting skills
        FSkillData BasicSkill;
        BasicSkill.CurrentValue = 25.0f;
        BasicSkill.CurrentExperience = 0.0f;

        // These would come from GameplayTags in a real implementation
        // For now, we'll leave the skills empty until tags are properly set up
    }

    bPlayerDataInitialized = true;
    UpdateCachedValues();

    UE_LOG(LogTemp, Log, TEXT("Player data initialized for %s"), *CharacterID);
}

int32 ARadiantPlayerState::CalculateEffectiveLevel() const
{
    if (SkillsCache.Num() == 0)
    {
        return 1;
    }

    // Calculate level based on highest skills (UO-style)
    float TotalSkillValue = 0.0f;
    int32 SkillCount = 0;

    for (const auto& SkillPair : SkillsCache)
    {
        TotalSkillValue += SkillPair.Value.CurrentValue;
        SkillCount++;
    }

    if (SkillCount == 0)
    {
        return 1;
    }

    // Simple level calculation: average skill level
    float AverageSkill = TotalSkillValue / SkillCount;
    return FMath::Max(1, FMath::FloorToInt(AverageSkill / 10.0f) + 1);
}

void ARadiantPlayerState::UpdateCachedValues()
{
    int32 NewLevel = CalculateEffectiveLevel();
    
    if (NewLevel != CachedEffectiveLevel)
    {
        int32 OldLevel = CachedEffectiveLevel;
        CachedEffectiveLevel = NewLevel;
        OnPlayerLevelChanged.Broadcast(NewLevel);
        OnPlayerLevelChangedBP(NewLevel);
        
        UE_LOG(LogTemp, Log, TEXT("Player level changed: %d -> %d"), OldLevel, NewLevel);
    }
}

void ARadiantPlayerState::ApplySkillExperience(FGameplayTag SkillTag, float ExperienceAmount)
{
    FSkillData& SkillData = SkillsCache.FindOrAdd(SkillTag);
    
    float OldValue = SkillData.CurrentValue;
    SkillData.CurrentExperience += ExperienceAmount;
    
    // Calculate new skill value from experience
    float NewValue = CalculateSkillLevelFromExperience(SkillData.CurrentExperience);
    NewValue = FMath::Clamp(NewValue, 0.0f, IndividualSkillCap);
    
    SkillData.CurrentValue = NewValue;

    // Update replicated array
    UpdateReplicatedSkillEntries();

    // Update cached values
    UpdateCachedValues();

    // Broadcast change
    if (NewValue != OldValue)
    {
        OnSkillChanged.Broadcast(SkillTag, OldValue, NewValue);
        
        UE_LOG(LogTemp, Log, TEXT("Skill %s gained %.1f experience (%.1f -> %.1f)"),
               *SkillTag.ToString(), ExperienceAmount, OldValue, NewValue);
    }
}

void ARadiantPlayerState::GrantDefaultAbilities()
{
    if (!AbilitySystemComponent)
    {
        return;
    }

    // Grant default abilities
    for (TSubclassOf<UGameplayAbility> AbilityClass : DefaultAbilities)
    {
        if (AbilityClass)
        {
            AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Granted %d default abilities"), DefaultAbilities.Num());
}

void ARadiantPlayerState::UpdatePlaytimeStatistics()
{
    if (SessionStartTime > 0.0)
    {
        double CurrentTime = FPlatformTime::Seconds();
        float SessionTime = CurrentTime - SessionStartTime;
        Statistics.TotalPlayTime += SessionTime;
        SessionStartTime = CurrentTime; // Reset for next update
    }
}

float ARadiantPlayerState::CalculateExperienceForNextLevel(FGameplayTag SkillTag) const
{
    // Simple exponential curve for now
    float CurrentLevel = GetSkillValue(SkillTag);
    return FMath::Pow(CurrentLevel + 1, 2.0f) * 100.0f;
}

float ARadiantPlayerState::CalculateSkillLevelFromExperience(float Experience) const
{
    // Inverse of experience calculation
    return FMath::Sqrt(Experience / 100.0f);
}

void ARadiantPlayerState::RebuildSkillsCache()
{
    SkillsCache.Empty();
    for (const FSkillEntry& Entry : SkillEntries)
    {
        SkillsCache.Add(Entry.SkillTag, Entry.SkillData);
    }
}

void ARadiantPlayerState::RebuildFactionReputationsCache()
{
    FactionReputationsCache.Empty();
    for (const FFactionReputationEntry& Entry : FactionReputationEntries)
    {
        FactionReputationsCache.Add(Entry.FactionTag, Entry.ReputationValue);
    }
}

void ARadiantPlayerState::UpdateReplicatedSkillEntries()
{
    if (!HasAuthority())
    {
        return;
    }

    SkillEntries.Empty();
    for (const auto& SkillPair : SkillsCache)
    {
        SkillEntries.Add(FSkillEntry(SkillPair.Key, SkillPair.Value));
    }
}

void ARadiantPlayerState::UpdateReplicatedFactionEntries()
{
    if (!HasAuthority())
    {
        return;
    }

    FactionReputationEntries.Empty();
    for (const auto& FactionPair : FactionReputationsCache)
    {
        FactionReputationEntries.Add(FFactionReputationEntry(FactionPair.Key, FactionPair.Value));
    }
}

// === REPLICATION CALLBACKS ===

void ARadiantPlayerState::OnRep_SkillEntries()
{
    // Rebuild cache when skills replicate
    RebuildSkillsCache();
    UpdateCachedValues();
    
    UE_LOG(LogTemp, Log, TEXT("Skills replicated - %d skills received"), SkillEntries.Num());
}

void ARadiantPlayerState::OnRep_FactionReputationEntries()
{
    // Rebuild cache when faction reputations replicate
    RebuildFactionReputationsCache();
    
    UE_LOG(LogTemp, Log, TEXT("Faction reputations replicated - %d factions received"), FactionReputationEntries.Num());
}

void ARadiantPlayerState::OnRep_Statistics(const FPlayerStatistics& OldStatistics)
{
    // Broadcast any significant statistic changes
    OnStatisticUpdatedBP(TEXT("TotalPlayTime"), Statistics.TotalPlayTime);
    
    UE_LOG(LogTemp, Log, TEXT("Player statistics replicated"));
}

bool ARadiantPlayerState::MeetsSkillRequirements(const TMap<FGameplayTag, float>& Requirements) const
{
    for (const auto& Requirement : Requirements)
    {
        float CurrentSkillValue = GetSkillValue(Requirement.Key);
        if (CurrentSkillValue < Requirement.Value)
        {
            return false;
        }
    }
    return true;
}