// Private/Components/NeedsComponent.cpp

#include "Components/NeedsComponent.h"
#include "Characters/BaseCharacter.h"
#include "RadiantRPG.h"
#include "Engine/World.h"
#include "Types/ARPG_AIDataTableTypes.h"

UNeedsComponent::UNeedsComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 1.0f;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    
    bNeedsActive = true;
    bNeedsUpdatePaused = false;
    bAutoPopulateDefaults = true;
    GlobalDecayMultiplier = 1.0f;
    CreatureArchetype = EARPG_CreatureArchetype::Human;
    
    TimeSinceLastUpdate = 0.0f;
    LastLogTime = 0.0f;
}

void UNeedsComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCharacter = Cast<ABaseCharacter>(GetOwner());
    
    // Only auto-populate if the Current Needs map is empty
    if (CurrentNeeds.Num() == 0)
    {
        if (bAutoPopulateDefaults)
        {
            InitializeNeedsFromDataTable();
        }
        else
        {
            InitializeDefaultNeeds();
        }
    }

    // Update tick interval from config
    PrimaryComponentTick.TickInterval = NeedsConfig.UpdateFrequency;

    // Initialize critical states
    for (auto& NeedPair : CurrentNeeds)
    {
        UpdateNeedCriticalStatus(NeedPair.Key);
    }

    UE_LOG(LogRadiantRPG, Log, TEXT("NeedsComponent initialized for %s with %d needs"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"), CurrentNeeds.Num());
}

void UNeedsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    if (!bNeedsActive || bNeedsUpdatePaused)
    {
        return;
    }
    
    TimeSinceLastUpdate += DeltaTime;
    
    if (TimeSinceLastUpdate >= NeedsConfig.UpdateFrequency)
    {
        UpdateNeedsOverTime(TimeSinceLastUpdate);
        TimeSinceLastUpdate = 0.0f;
    }
}

void UNeedsComponent::AutoPopulateNeedsFromArchetype()
{
    // Try to load from data table first
    if (CreatureNeedsDataTable)
    {
        FName RowName = DataTableRowName.IsNone() ? 
            FName(*UEnum::GetValueAsString(CreatureArchetype).Replace(TEXT("EARPG_CreatureArchetype::"), TEXT(""))) : 
            DataTableRowName;
            
        if (LoadNeedsFromDataTable(RowName, CreatureNeedsDataTable))
        {
            return;
        }
    }
    
    // Fallback to hardcoded archetype configs
    TArray<FARPG_CreatureNeedConfig> ArchetypeConfigs = GetArchetypeNeedConfigs(CreatureArchetype);
    
    for (const FARPG_CreatureNeedConfig& Config : ArchetypeConfigs)
    {
        FARPG_AINeed Need;
        Need.NeedType = Config.NeedType;
        Need.CurrentLevel = Config.StartingLevel;
        Need.DecayRate = Config.DecayRate;
        Need.UrgencyThreshold = Config.UrgencyThreshold;
        Need.CriticalThreshold = Config.CriticalThreshold;
        Need.MaxLevel = Config.MaxLevel;
        Need.bIsActive = Config.bIsActive;
        Need.bIsCritical = false;
        
        CurrentNeeds.Add(Config.NeedType, Need);
    }
    
    if (NeedsConfig.bEnableDebugLogging)
    {
        UE_LOG(LogRadiantRPG, Log, TEXT("Auto-populated %d needs for archetype %s"), 
               CurrentNeeds.Num(), 
               *UEnum::GetValueAsString(CreatureArchetype));
    }
}

TArray<FARPG_CreatureNeedConfig> UNeedsComponent::GetArchetypeNeedConfigs(EARPG_CreatureArchetype Archetype) const
{
    TArray<FARPG_CreatureNeedConfig> Configs;
    
    // Define needs based on creature archetype
    switch (Archetype)
    {
        case EARPG_CreatureArchetype::Human:
        case EARPG_CreatureArchetype::Villager:
        case EARPG_CreatureArchetype::Guard:
        case EARPG_CreatureArchetype::Merchant:
        case EARPG_CreatureArchetype::Soldier:
        {
            // Humans have all basic needs
            FARPG_CreatureNeedConfig HungerConfig;
            HungerConfig.NeedType = EARPG_NeedType::Hunger;
            HungerConfig.StartingLevel = 0.3f;
            HungerConfig.DecayRate = 0.002f;
            HungerConfig.UrgencyThreshold = 0.7f;
            HungerConfig.CriticalThreshold = 0.9f;
            HungerConfig.MaxLevel = 1.0f;
            HungerConfig.bIsActive = true;
            HungerConfig.Priority = 0.8f;
            Configs.Add(HungerConfig);
            
            FARPG_CreatureNeedConfig FatigueConfig;
            FatigueConfig.NeedType = EARPG_NeedType::Fatigue;
            FatigueConfig.StartingLevel = 0.2f;
            FatigueConfig.DecayRate = 0.001f;
            FatigueConfig.UrgencyThreshold = 0.7f;
            FatigueConfig.CriticalThreshold = 0.9f;
            FatigueConfig.MaxLevel = 1.0f;
            FatigueConfig.bIsActive = true;
            FatigueConfig.Priority = 0.6f;
            Configs.Add(FatigueConfig);
            
            FARPG_CreatureNeedConfig SafetyConfig;
            SafetyConfig.NeedType = EARPG_NeedType::Safety;
            SafetyConfig.StartingLevel = 0.1f;
            SafetyConfig.DecayRate = 0.0f;
            SafetyConfig.UrgencyThreshold = 0.5f;
            SafetyConfig.CriticalThreshold = 0.8f;
            SafetyConfig.MaxLevel = 1.0f;
            SafetyConfig.bIsActive = true;
            SafetyConfig.Priority = 1.0f;
            Configs.Add(SafetyConfig);
            
            FARPG_CreatureNeedConfig SocialConfig;
            SocialConfig.NeedType = EARPG_NeedType::Social;
            SocialConfig.StartingLevel = 0.4f;
            SocialConfig.DecayRate = 0.0005f;
            SocialConfig.UrgencyThreshold = 0.6f;
            SocialConfig.CriticalThreshold = 0.8f;
            SocialConfig.MaxLevel = 1.0f;
            SocialConfig.bIsActive = true;
            SocialConfig.Priority = 0.4f;
            Configs.Add(SocialConfig);
            
            FARPG_CreatureNeedConfig ComfortConfig;
            ComfortConfig.NeedType = EARPG_NeedType::Comfort;
            ComfortConfig.StartingLevel = 0.5f;
            ComfortConfig.DecayRate = 0.0003f;
            ComfortConfig.UrgencyThreshold = 0.7f;
            ComfortConfig.CriticalThreshold = 0.9f;
            ComfortConfig.MaxLevel = 1.0f;
            ComfortConfig.bIsActive = true;
            ComfortConfig.Priority = 0.3f;
            Configs.Add(ComfortConfig);
            
            FARPG_CreatureNeedConfig CuriosityConfig;
            CuriosityConfig.NeedType = EARPG_NeedType::Curiosity;
            CuriosityConfig.StartingLevel = 0.3f;
            CuriosityConfig.DecayRate = 0.0008f;
            CuriosityConfig.UrgencyThreshold = 0.6f;
            CuriosityConfig.CriticalThreshold = 0.8f;
            CuriosityConfig.MaxLevel = 1.0f;
            CuriosityConfig.bIsActive = true;
            CuriosityConfig.Priority = 0.2f;
            Configs.Add(CuriosityConfig);
            break;
        }
        
        case EARPG_CreatureArchetype::Bandit:
        {
            // Bandits have reduced social needs, higher safety needs
            FARPG_CreatureNeedConfig HungerConfig;
            HungerConfig.NeedType = EARPG_NeedType::Hunger;
            HungerConfig.StartingLevel = 0.3f;
            HungerConfig.DecayRate = 0.0025f;
            HungerConfig.UrgencyThreshold = 0.7f;
            HungerConfig.CriticalThreshold = 0.9f;
            HungerConfig.MaxLevel = 1.0f;
            HungerConfig.bIsActive = true;
            HungerConfig.Priority = 0.8f;
            Configs.Add(HungerConfig);
            
            FARPG_CreatureNeedConfig FatigueConfig;
            FatigueConfig.NeedType = EARPG_NeedType::Fatigue;
            FatigueConfig.StartingLevel = 0.2f;
            FatigueConfig.DecayRate = 0.0008f;
            FatigueConfig.UrgencyThreshold = 0.8f;
            FatigueConfig.CriticalThreshold = 0.95f;
            FatigueConfig.MaxLevel = 1.0f;
            FatigueConfig.bIsActive = true;
            FatigueConfig.Priority = 0.6f;
            Configs.Add(FatigueConfig);
            
            FARPG_CreatureNeedConfig SafetyConfig;
            SafetyConfig.NeedType = EARPG_NeedType::Safety;
            SafetyConfig.StartingLevel = 0.4f;
            SafetyConfig.DecayRate = 0.001f;
            SafetyConfig.UrgencyThreshold = 0.4f;
            SafetyConfig.CriticalThreshold = 0.7f;
            SafetyConfig.MaxLevel = 1.0f;
            SafetyConfig.bIsActive = true;
            SafetyConfig.Priority = 1.0f;
            Configs.Add(SafetyConfig);
            
            FARPG_CreatureNeedConfig SocialConfig;
            SocialConfig.NeedType = EARPG_NeedType::Social;
            SocialConfig.StartingLevel = 0.6f;
            SocialConfig.DecayRate = 0.0002f;
            SocialConfig.UrgencyThreshold = 0.8f;
            SocialConfig.CriticalThreshold = 0.95f;
            SocialConfig.MaxLevel = 1.0f;
            SocialConfig.bIsActive = true;
            SocialConfig.Priority = 0.2f;
            Configs.Add(SocialConfig);
            
            FARPG_CreatureNeedConfig ComfortConfig;
            ComfortConfig.NeedType = EARPG_NeedType::Comfort;
            ComfortConfig.StartingLevel = 0.7f;
            ComfortConfig.DecayRate = 0.0001f;
            ComfortConfig.UrgencyThreshold = 0.9f;
            ComfortConfig.CriticalThreshold = 0.98f;
            ComfortConfig.MaxLevel = 1.0f;
            ComfortConfig.bIsActive = true;
            ComfortConfig.Priority = 0.1f;
            Configs.Add(ComfortConfig);
            break;
        }
        
        case EARPG_CreatureArchetype::Wolf:
        case EARPG_CreatureArchetype::Bear:
        case EARPG_CreatureArchetype::Deer:
        {
            // Animals have basic survival needs
            FARPG_CreatureNeedConfig HungerConfig;
            HungerConfig.NeedType = EARPG_NeedType::Hunger;
            HungerConfig.StartingLevel = 0.4f;
            HungerConfig.DecayRate = 0.003f;
            HungerConfig.UrgencyThreshold = 0.6f;
            HungerConfig.CriticalThreshold = 0.8f;
            HungerConfig.MaxLevel = 1.0f;
            HungerConfig.bIsActive = true;
            HungerConfig.Priority = 1.0f;
            Configs.Add(HungerConfig);
            
            FARPG_CreatureNeedConfig FatigueConfig;
            FatigueConfig.NeedType = EARPG_NeedType::Fatigue;
            FatigueConfig.StartingLevel = 0.3f;
            FatigueConfig.DecayRate = 0.0015f;
            FatigueConfig.UrgencyThreshold = 0.7f;
            FatigueConfig.CriticalThreshold = 0.9f;
            FatigueConfig.MaxLevel = 1.0f;
            FatigueConfig.bIsActive = true;
            FatigueConfig.Priority = 0.7f;
            Configs.Add(FatigueConfig);
            
            FARPG_CreatureNeedConfig SafetyConfig;
            SafetyConfig.NeedType = EARPG_NeedType::Safety;
            SafetyConfig.StartingLevel = 0.2f;
            SafetyConfig.DecayRate = 0.0f;
            SafetyConfig.UrgencyThreshold = 0.3f;
            SafetyConfig.CriticalThreshold = 0.6f;
            SafetyConfig.MaxLevel = 1.0f;
            SafetyConfig.bIsActive = true;
            SafetyConfig.Priority = 0.9f;
            Configs.Add(SafetyConfig);
            
            FARPG_CreatureNeedConfig CuriosityConfig;
            CuriosityConfig.NeedType = EARPG_NeedType::Curiosity;
            CuriosityConfig.StartingLevel = 0.2f;
            CuriosityConfig.DecayRate = 0.0012f;
            CuriosityConfig.UrgencyThreshold = 0.7f;
            CuriosityConfig.CriticalThreshold = 0.9f;
            CuriosityConfig.MaxLevel = 1.0f;
            CuriosityConfig.bIsActive = true;
            CuriosityConfig.Priority = 0.3f;
            Configs.Add(CuriosityConfig);
            break;
        }
        
        case EARPG_CreatureArchetype::Undead:
        case EARPG_CreatureArchetype::Skeleton:
        case EARPG_CreatureArchetype::Zombie:
        {
            // Undead don't have hunger or fatigue, but have other drives
            FARPG_CreatureNeedConfig HungerConfig;
            HungerConfig.NeedType = EARPG_NeedType::Hunger;
            HungerConfig.StartingLevel = 0.0f;
            HungerConfig.DecayRate = 0.0f;
            HungerConfig.UrgencyThreshold = 1.0f;
            HungerConfig.CriticalThreshold = 1.0f;
            HungerConfig.MaxLevel = 1.0f;
            HungerConfig.bIsActive = false;
            HungerConfig.Priority = 0.0f;
            Configs.Add(HungerConfig);
            
            FARPG_CreatureNeedConfig FatigueConfig;
            FatigueConfig.NeedType = EARPG_NeedType::Fatigue;
            FatigueConfig.StartingLevel = 0.0f;
            FatigueConfig.DecayRate = 0.0f;
            FatigueConfig.UrgencyThreshold = 1.0f;
            FatigueConfig.CriticalThreshold = 1.0f;
            FatigueConfig.MaxLevel = 1.0f;
            FatigueConfig.bIsActive = false;
            FatigueConfig.Priority = 0.0f;
            Configs.Add(FatigueConfig);
            
            FARPG_CreatureNeedConfig SafetyConfig;
            SafetyConfig.NeedType = EARPG_NeedType::Safety;
            SafetyConfig.StartingLevel = 0.1f;
            SafetyConfig.DecayRate = 0.0f;
            SafetyConfig.UrgencyThreshold = 0.8f;
            SafetyConfig.CriticalThreshold = 0.95f;
            SafetyConfig.MaxLevel = 1.0f;
            SafetyConfig.bIsActive = true;
            SafetyConfig.Priority = 0.5f;
            Configs.Add(SafetyConfig);
            
            FARPG_CreatureNeedConfig CuriosityConfig;
            CuriosityConfig.NeedType = EARPG_NeedType::Curiosity;
            CuriosityConfig.StartingLevel = 0.1f;
            CuriosityConfig.DecayRate = 0.0005f;
            CuriosityConfig.UrgencyThreshold = 0.8f;
            CuriosityConfig.CriticalThreshold = 0.95f;
            CuriosityConfig.MaxLevel = 1.0f;
            CuriosityConfig.bIsActive = true;
            CuriosityConfig.Priority = 0.2f;
            Configs.Add(CuriosityConfig);
            break;
        }
        
        case EARPG_CreatureArchetype::Spirit:
        {
            // Spirits have unique needs
            FARPG_CreatureNeedConfig SafetyConfig;
            SafetyConfig.NeedType = EARPG_NeedType::Safety;
            SafetyConfig.StartingLevel = 0.2f;
            SafetyConfig.DecayRate = 0.0f;
            SafetyConfig.UrgencyThreshold = 0.6f;
            SafetyConfig.CriticalThreshold = 0.8f;
            SafetyConfig.MaxLevel = 1.0f;
            SafetyConfig.bIsActive = true;
            SafetyConfig.Priority = 0.8f;
            Configs.Add(SafetyConfig);
            
            FARPG_CreatureNeedConfig CuriosityConfig;
            CuriosityConfig.NeedType = EARPG_NeedType::Curiosity;
            CuriosityConfig.StartingLevel = 0.5f;
            CuriosityConfig.DecayRate = 0.002f;
            CuriosityConfig.UrgencyThreshold = 0.5f;
            CuriosityConfig.CriticalThreshold = 0.7f;
            CuriosityConfig.MaxLevel = 1.0f;
            CuriosityConfig.bIsActive = true;
            CuriosityConfig.Priority = 0.9f;
            Configs.Add(CuriosityConfig);
            break;
        }
        
        case EARPG_CreatureArchetype::Construct:
        case EARPG_CreatureArchetype::Golem:
        {
            // Constructs have minimal needs
            FARPG_CreatureNeedConfig SafetyConfig;
            SafetyConfig.NeedType = EARPG_NeedType::Safety;
            SafetyConfig.StartingLevel = 0.1f;
            SafetyConfig.DecayRate = 0.0f;
            SafetyConfig.UrgencyThreshold = 0.9f;
            SafetyConfig.CriticalThreshold = 0.98f;
            SafetyConfig.MaxLevel = 1.0f;
            SafetyConfig.bIsActive = true;
            SafetyConfig.Priority = 1.0f;
            Configs.Add(SafetyConfig);
            break;
        }
        
        default:
        {
            // Generic fallback
            FARPG_CreatureNeedConfig HungerConfig;
            HungerConfig.NeedType = EARPG_NeedType::Hunger;
            HungerConfig.StartingLevel = 0.3f;
            HungerConfig.DecayRate = 0.002f;
            HungerConfig.UrgencyThreshold = 0.7f;
            HungerConfig.CriticalThreshold = 0.9f;
            HungerConfig.MaxLevel = 1.0f;
            HungerConfig.bIsActive = true;
            HungerConfig.Priority = 0.8f;
            Configs.Add(HungerConfig);
            
            FARPG_CreatureNeedConfig FatigueConfig;
            FatigueConfig.NeedType = EARPG_NeedType::Fatigue;
            FatigueConfig.StartingLevel = 0.2f;
            FatigueConfig.DecayRate = 0.001f;
            FatigueConfig.UrgencyThreshold = 0.7f;
            FatigueConfig.CriticalThreshold = 0.9f;
            FatigueConfig.MaxLevel = 1.0f;
            FatigueConfig.bIsActive = true;
            FatigueConfig.Priority = 0.6f;
            Configs.Add(FatigueConfig);
            
            FARPG_CreatureNeedConfig SafetyConfig;
            SafetyConfig.NeedType = EARPG_NeedType::Safety;
            SafetyConfig.StartingLevel = 0.1f;
            SafetyConfig.DecayRate = 0.0f;
            SafetyConfig.UrgencyThreshold = 0.5f;
            SafetyConfig.CriticalThreshold = 0.8f;
            SafetyConfig.MaxLevel = 1.0f;
            SafetyConfig.bIsActive = true;
            SafetyConfig.Priority = 1.0f;
            Configs.Add(SafetyConfig);
            break;
        }
    }
    
    return Configs;
}

bool UNeedsComponent::LoadNeedsFromDataTable(FName RowName, UDataTable* DataTable)
{
    UDataTable* TableToUse = DataTable ? DataTable : CreatureNeedsDataTable;
    if (!TableToUse)
    {
        return false;
    }
    
    FARPG_CreatureNeedsProfile* ProfileRow = TableToUse->FindRow<FARPG_CreatureNeedsProfile>(RowName, TEXT("LoadNeedsFromDataTable"));
    if (!ProfileRow)
    {
        if (NeedsConfig.bEnableDebugLogging)
        {
            UE_LOG(LogRadiantRPG, Warning, TEXT("Could not find needs profile row '%s' in data table"), *RowName.ToString());
        }
        return false;
    }
    
    // Clear current needs
    CurrentNeeds.Empty();
    
    // Apply configuration from data table
    NeedsConfig.UpdateFrequency = ProfileRow->UpdateFrequency;
    NeedsConfig.bEnableDebugLogging = ProfileRow->bEnableDebugLogging;
    GlobalDecayMultiplier = ProfileRow->GlobalDecayMultiplier;
    
    // Create needs from profile
    for (const FARPG_CreatureNeedConfig& Config : ProfileRow->NeedConfigs)
    {
        FARPG_AINeed Need;
        Need.NeedType = Config.NeedType;
        Need.CurrentLevel = Config.StartingLevel;
        Need.DecayRate = Config.DecayRate;
        Need.UrgencyThreshold = Config.UrgencyThreshold;
        Need.CriticalThreshold = Config.CriticalThreshold;
        Need.MaxLevel = Config.MaxLevel;
        Need.bIsActive = Config.bIsActive;
        Need.bIsCritical = false;
        
        CurrentNeeds.Add(Config.NeedType, Need);
    }
    
    // Update tick interval
    PrimaryComponentTick.TickInterval = NeedsConfig.UpdateFrequency;
    
    if (NeedsConfig.bEnableDebugLogging)
    {
        UE_LOG(LogRadiantRPG, Log, TEXT("Loaded %d needs from data table row '%s'"), 
               CurrentNeeds.Num(), *RowName.ToString());
    }
    
    return true;
}

void UNeedsComponent::InitializeNeedsFromArchetype(EARPG_CreatureArchetype Archetype, UDataTable* DataTable)
{
    CreatureArchetype = Archetype;
    if (DataTable)
    {
        CreatureNeedsDataTable = DataTable;
    }
    
    AutoPopulateNeedsFromArchetype();
    
    // Initialize critical states
    for (auto& NeedPair : CurrentNeeds)
    {
        UpdateNeedCriticalStatus(NeedPair.Key);
    }
}

void UNeedsComponent::SetCreatureArchetype(EARPG_CreatureArchetype  NewArchetype, bool bReloadNeeds)
{
    CreatureArchetype = NewArchetype;
    
    if (bReloadNeeds && bAutoPopulateDefaults)
    {
        InitializeNeedsFromDataTable();
    }
}

FARPG_AINeed UNeedsComponent::CreateDefaultNeedConfig(EARPG_NeedType NeedType) const
{
    FARPG_AINeed Need;
    Need.NeedType = NeedType;
    Need.CurrentLevel = 0.3f;
    Need.DecayRate = GetDefaultDecayRateForNeed(NeedType);
    Need.UrgencyThreshold = 0.7f;
    Need.CriticalThreshold = 0.9f;
    Need.MaxLevel = 1.0f;
    Need.bIsActive = true;
    Need.bIsCritical = false;
    
    return Need;
}

float UNeedsComponent::GetNeedLevel(EARPG_NeedType NeedType) const
{
    if (const FARPG_AINeed* Need = CurrentNeeds.Find(NeedType))
    {
        return Need->CurrentLevel;
    }
    return 0.0f;
}

void UNeedsComponent::SetNeedLevel(EARPG_NeedType NeedType, float NewLevel)
{
    if (FARPG_AINeed* Need = CurrentNeeds.Find(NeedType))
    {
        float OldLevel = Need->CurrentLevel;
        Need->CurrentLevel = FMath::Clamp(NewLevel, 0.0f, Need->MaxLevel);
        
        if (OldLevel != Need->CurrentLevel)
        {
            OnNeedValueChanged(NeedType, OldLevel, Need->CurrentLevel);
            UpdateNeedCriticalStatus(NeedType);
        }
    }
}

void UNeedsComponent::ModifyNeedLevel(EARPG_NeedType NeedType, float DeltaAmount)
{
    if (FARPG_AINeed* Need = CurrentNeeds.Find(NeedType))
    {
        float OldLevel = Need->CurrentLevel;
        Need->CurrentLevel = FMath::Clamp(Need->CurrentLevel + DeltaAmount, 0.0f, Need->MaxLevel);
        
        if (OldLevel != Need->CurrentLevel)
        {
            OnNeedValueChanged(NeedType, OldLevel, Need->CurrentLevel);
            UpdateNeedCriticalStatus(NeedType);
        }
    }
}

bool UNeedsComponent::IsNeedCritical(EARPG_NeedType NeedType) const
{
    if (const FARPG_AINeed* Need = CurrentNeeds.Find(NeedType))
    {
        return Need->CurrentLevel >= Need->CriticalThreshold;
    }
    return false;
}

TArray<EARPG_NeedType> UNeedsComponent::GetCriticalNeeds() const
{
    TArray<EARPG_NeedType> CriticalNeeds;
    
    for (const auto& NeedPair : CurrentNeeds)
    {
        if (IsNeedCritical(NeedPair.Key))
        {
            CriticalNeeds.Add(NeedPair.Key);
        }
    }
    
    return CriticalNeeds;
}

TArray<EARPG_NeedType> UNeedsComponent::GetUrgentNeeds() const
{
    TArray<EARPG_NeedType> UrgentNeeds;
    
    for (const auto& NeedPair : CurrentNeeds)
    {
        const FARPG_AINeed& Need = NeedPair.Value;
        if (Need.CurrentLevel >= Need.UrgencyThreshold)
        {
            UrgentNeeds.Add(NeedPair.Key);
        }
    }
    
    // Sort by priority (highest first)
    UrgentNeeds.Sort([this](const EARPG_NeedType& A, const EARPG_NeedType& B) {
        return GetNeedPriority(A) > GetNeedPriority(B);
    });
    
    return UrgentNeeds;
}

void UNeedsComponent::InitializeNeeds(const FARPG_NeedsConfiguration& Config, const TArray<FARPG_AINeed>& InitialNeeds)
{
    NeedsConfig = Config;
    PrimaryComponentTick.TickInterval = NeedsConfig.UpdateFrequency;
    
    // Clear current needs and set new ones
    CurrentNeeds.Empty();
    for (const FARPG_AINeed& Need : InitialNeeds)
    {
        CurrentNeeds.Add(Need.NeedType, Need);
    }
    
    // Initialize critical states
    for (auto& NeedPair : CurrentNeeds)
    {
        UpdateNeedCriticalStatus(NeedPair.Key);
    }
    
    if (NeedsConfig.bEnableDebugLogging)
    {
        UE_LOG(LogRadiantRPG, Log, TEXT("Needs initialized with %d custom needs"), InitialNeeds.Num());
    }
}

void UNeedsComponent::UpdateNeedsConfiguration(const FARPG_NeedsConfiguration& NewConfig)
{
    NeedsConfig = NewConfig;
    PrimaryComponentTick.TickInterval = NeedsConfig.UpdateFrequency;
    
    if (NeedsConfig.bEnableDebugLogging)
    {
        UE_LOG(LogRadiantRPG, Log, TEXT("Needs configuration updated"));
    }
}

void UNeedsComponent::InitializeDefaultNeeds()
{
    // Create basic needs that all characters should have
    TArray<EARPG_NeedType> BasicNeedTypes = {
        EARPG_NeedType::Hunger,
        EARPG_NeedType::Fatigue,
        EARPG_NeedType::Safety,
        EARPG_NeedType::Social,
        EARPG_NeedType::Comfort,
        EARPG_NeedType::Curiosity
    };
    
    for (EARPG_NeedType NeedType : BasicNeedTypes)
    {
        CurrentNeeds.Add(NeedType, CreateDefaultNeedConfig(NeedType));
    }
    
    if (NeedsConfig.bEnableDebugLogging)
    {
        UE_LOG(LogRadiantRPG, Log, TEXT("Initialized %d default needs"), BasicNeedTypes.Num());
    }
}

void UNeedsComponent::UpdateNeedsOverTime(float DeltaTime)
{
    for (auto& NeedPair : CurrentNeeds)
    {
        FARPG_AINeed& Need = NeedPair.Value;
        
        if (!Need.bIsActive)
        {
            continue;
        }
        
        float OldLevel = Need.CurrentLevel;
        
        // Apply decay over time
        float DecayAmount = Need.DecayRate * DeltaTime * GlobalDecayMultiplier;
        Need.CurrentLevel = FMath::Clamp(Need.CurrentLevel + DecayAmount, 0.0f, Need.MaxLevel);
        
        if (OldLevel != Need.CurrentLevel)
        {
            OnNeedValueChanged(NeedPair.Key, OldLevel, Need.CurrentLevel);
            UpdateNeedCriticalStatus(NeedPair.Key);
        }
    }
}

void UNeedsComponent::UpdateNeedCriticalStatus(EARPG_NeedType NeedType)
{
    const FARPG_AINeed* Need = CurrentNeeds.Find(NeedType);
    if (!Need)
    {
        return;
    }
    
    bool bWasCritical = Need->bIsCritical;
    bool bIsNowCritical = Need->CurrentLevel >= Need->CriticalThreshold;
    
    if (bWasCritical != bIsNowCritical)
    {
        // Update the critical state
        CurrentNeeds[NeedType].bIsCritical = bIsNowCritical;
        
        if (bIsNowCritical)
        {
            OnNeedBecomeCritical(NeedType);
            OnNeedCritical.Broadcast(NeedType);
            BP_OnNeedCritical(NeedType);
        }
        else
        {
            OnNeedNoLongerCritical(NeedType);
            OnNeedSatisfied.Broadcast(NeedType);
            BP_OnNeedSatisfied(NeedType);
        }
    }
}

float UNeedsComponent::GetNeedPriority(EARPG_NeedType NeedType) const
{
    // Default priority ordering (higher = more important)
    switch (NeedType)
    {
        case EARPG_NeedType::Safety: return 1.0f;
        case EARPG_NeedType::Hunger: return 0.8f;
        case EARPG_NeedType::Fatigue: return 0.6f;
        case EARPG_NeedType::Social: return 0.4f;
        case EARPG_NeedType::Comfort: return 0.3f;
        case EARPG_NeedType::Curiosity: return 0.2f;
        default: return 0.5f;
    }
}

void UNeedsComponent::OnNeedValueChanged(EARPG_NeedType NeedType, float OldValue, float NewValue)
{
    OnNeedChanged.Broadcast(NeedType, NewValue);
    BP_OnNeedChanged(NeedType, OldValue, NewValue);
    
    // Throttled logging
    if (NeedsConfig.bEnableDebugLogging)
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();
        if (CurrentTime - LastLogTime >= 5.0f) // Log at most every 5 seconds
        {
            UE_LOG(LogRadiantRPG, Log, TEXT("%s: Need %s changed from %.2f to %.2f"), 
                   OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
                   *UEnum::GetValueAsString(NeedType), OldValue, NewValue);
            LastLogTime = CurrentTime;
        }
    }
}

void UNeedsComponent::OnNeedBecomeCritical(EARPG_NeedType NeedType)
{
    if (NeedsConfig.bEnableDebugLogging)
    {
        UE_LOG(LogRadiantRPG, Warning, TEXT("%s: Need %s became critical!"), 
               OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
               *UEnum::GetValueAsString(NeedType));
    }
}

void UNeedsComponent::OnNeedNoLongerCritical(EARPG_NeedType NeedType)
{
    if (NeedsConfig.bEnableDebugLogging)
    {
        UE_LOG(LogRadiantRPG, Log, TEXT("%s: Need %s is no longer critical"), 
               OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
               *UEnum::GetValueAsString(NeedType));
    }
}

float UNeedsComponent::GetDefaultDecayRateForNeed(EARPG_NeedType NeedType) const
{
    // Default decay rates per second (positive = need increases over time)
    switch (NeedType)
    {
        case EARPG_NeedType::Hunger: return 0.002f; // ~8 minutes to go from 0 to 1
        case EARPG_NeedType::Fatigue: return 0.001f; // ~16 minutes to go from 0 to 1
        case EARPG_NeedType::Safety: return 0.0f; // Only changes based on events
        case EARPG_NeedType::Social: return 0.0005f; // ~33 minutes to go from 0 to 1
        case EARPG_NeedType::Comfort: return 0.0003f; // ~55 minutes to go from 0 to 1
        case EARPG_NeedType::Curiosity: return 0.0008f; // ~20 minutes to go from 0 to 1
        default: return 0.001f;
    }
}

void UNeedsComponent::InitializeNeedsFromDataTable()
{
    if (!CreatureNeedsDataTable)
    {
        UE_LOG(LogRadiantRPG, Warning, TEXT("NeedsComponent: No creature needs data table assigned, falling back to defaults"));
        InitializeDefaultNeeds();
        return;
    }

    // Use the DataTableRowName if specified, otherwise try to infer from creature archetype
    FName RowName = DataTableRowName;
    if (RowName.IsNone() && CreatureArchetype != EARPG_CreatureArchetype::Unknown)
    {
        // Convert archetype enum to string for row lookup
        RowName = FName(*UEnum::GetValueAsString(CreatureArchetype));
    }

    if (RowName.IsNone())
    {
        UE_LOG(LogRadiantRPG, Warning, TEXT("NeedsComponent: No valid row name for data table lookup, using defaults"));
        InitializeDefaultNeeds();
        return;
    }

    // Find the configuration row
    FString ContextString = FString::Printf(TEXT("Loading needs config for %s"), 
                                          OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"));
    FARPG_AINeedsConfigRow* ConfigRow = CreatureNeedsDataTable->FindRow<FARPG_AINeedsConfigRow>(RowName, ContextString);

    if (!ConfigRow)
    {
        UE_LOG(LogRadiantRPG, Warning, TEXT("NeedsComponent: Failed to find row '%s' in needs data table, using defaults"), 
               *RowName.ToString());
        InitializeDefaultNeeds();
        return;
    }

    // Clear current needs and populate from data table
    CurrentNeeds.Empty();

    // Create needs based on the configuration
    TArray<TPair<EARPG_NeedType, float>> NeedConfigs = {
        {EARPG_NeedType::Hunger, ConfigRow->StartingHunger / 100.0f},
        {EARPG_NeedType::Fatigue, ConfigRow->StartingFatigue / 100.0f},
        {EARPG_NeedType::Safety, ConfigRow->StartingSafety / 100.0f},
        {EARPG_NeedType::Social, ConfigRow->StartingSocial / 100.0f}
    };

    for (const auto& NeedConfig : NeedConfigs)
    {
        EARPG_NeedType NeedType = NeedConfig.Key;
        float StartingLevel = NeedConfig.Value;

        // Skip needs that are disabled for this creature type
        if ((NeedType == EARPG_NeedType::Hunger && !ConfigRow->bNeedsFood) ||
            (NeedType == EARPG_NeedType::Fatigue && !ConfigRow->bNeedsSleep) ||
            (NeedType == EARPG_NeedType::Social && !ConfigRow->bNeedsSocial))
        {
            continue;
        }

        FARPG_AINeed NewNeed;
        NewNeed.NeedType = NeedType;
        NewNeed.CurrentLevel = StartingLevel;
        NewNeed.DecayRate = GetDecayRateFromConfig(NeedType, *ConfigRow);
        NewNeed.UrgencyThreshold = 0.7f;
        NewNeed.CriticalThreshold = 0.9f;
        NewNeed.bIsActive = true;

        CurrentNeeds.Add(NeedType, NewNeed);
    }

    // Update configuration from data table
    NeedsConfig.UpdateFrequency = ConfigRow->UpdateFrequency;

    UE_LOG(LogRadiantRPG, Log, TEXT("NeedsComponent: Initialized %d needs from data table row '%s'"), 
           CurrentNeeds.Num(), *RowName.ToString());
}

float UNeedsComponent::GetDecayRateFromConfig(EARPG_NeedType NeedType, const FARPG_AINeedsConfigRow& ConfigRow) const
{
    switch (NeedType)
    {
    case EARPG_NeedType::Hunger:
        return ConfigRow.HungerDecayRate / 60.0f; // Convert per-minute to per-second
    case EARPG_NeedType::Fatigue:
        return ConfigRow.FatigueDecayRate / 60.0f;
    case EARPG_NeedType::Safety:
        return ConfigRow.SafetyDecayRate / 60.0f;
    case EARPG_NeedType::Social:
        return ConfigRow.SocialDecayRate / 60.0f;
    default:
        return GetDefaultDecayRateForNeed(NeedType);
    }
}