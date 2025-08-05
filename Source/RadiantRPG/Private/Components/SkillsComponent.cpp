// Private/Components/SkillsComponent.cpp

#include "Components/SkillsComponent.h"
#include "Characters/BaseCharacter.h"
#include "Types/RadiantTypes.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"

USkillsComponent::USkillsComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bWantsInitializeComponent = true;
    
    // Set defaults
    TotalSkillCap = 700.0f;
    ExperienceGainMultiplier = 1.0f;
    bEnforceSkillCaps = true;
    CurrentTotalSkillPoints = 0.0f;
    
    SetIsReplicatedByDefault(true);
}

void USkillsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(USkillsComponent, Skills);
    DOREPLIFETIME(USkillsComponent, CurrentTotalSkillPoints);
}

void USkillsComponent::BeginPlay()
{
    Super::BeginPlay();
    
    OwnerCharacter = Cast<ABaseCharacter>(GetOwner());
    InitializeDefaultSkills();
}

void USkillsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void USkillsComponent::InitializeDefaultSkills()
{
    if (Skills.Num() > 0)
        return; // Already initialized
    
    // Initialize basic skills with starting values
    TArray<ESkillType> AllSkills = {
        ESkillType::OneHanded,
        ESkillType::TwoHanded,
        ESkillType::Marksman,
        ESkillType::Block,
        ESkillType::Smithing,
        ESkillType::HeavyArmor,
        ESkillType::LightArmor,
        ESkillType::Pickpocket,
        ESkillType::Lockpicking,
        ESkillType::Sneak,
        ESkillType::Alchemy,
        ESkillType::Speech,
        ESkillType::Alteration,
        ESkillType::Conjuration,
        ESkillType::Destruction,
        ESkillType::Illusion,
        ESkillType::Restoration,
        ESkillType::Enchanting
    };
    
    for (ESkillType SkillType : AllSkills)
    {
        FLegacySkillData SkillData;
        SkillData.CurrentValue = 5.0f;
        SkillData.MaxValue = 100.0f;
        SkillData.Experience = 0.0f;
        SkillData.TotalExperience = 0.0f;
        SkillData.TemporaryModifier = 0.0f;
        SkillData.Modifier = 0.0f;
        SkillData.bIsLocked = false;
        
        Skills.Add(SkillType, SkillData);
    }
    
    UpdateTotalSkillPoints();
    
    UE_LOG(LogTemp, Log, TEXT("%s initialized %d skills"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"), 
           Skills.Num());
}

void USkillsComponent::GainSkillExperience(ESkillType SkillType, float ExperienceAmount)
{
    FLegacySkillData* SkillData = Skills.Find(SkillType);
    if (!SkillData || SkillData->bIsLocked)
        return;
    
    float AdjustedExperience = ExperienceAmount * ExperienceGainMultiplier;
    SkillData->Experience += AdjustedExperience;
    SkillData->TotalExperience += AdjustedExperience; // Sync legacy field
    
    float OldValue = SkillData->CurrentValue;
    CalculateLevelFromExperience(*SkillData);
    
    if (SkillData->CurrentValue != OldValue)
    {
        UpdateTotalSkillPoints();
        
        // Handle skill cap if needed
        if (bEnforceSkillCaps)
        {
            HandleSkillCap();
        }
        
        // Broadcast skill level up
        OnSkillLevelUp.Broadcast(SkillType, SkillData->CurrentValue);
        
        UE_LOG(LogTemp, Log, TEXT("%s skill %s leveled up to %.1f"), 
               OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
               *UEnum::GetValueAsString(SkillType), SkillData->CurrentValue);
    }
    
    // Always broadcast skill changed with experience
    BroadcastSkillChanged(SkillType);
}

void USkillsComponent::CalculateLevelFromExperience(FLegacySkillData& SkillData)
{
    float RequiredExperience = CalculateExperienceForLevel(SkillData.CurrentValue + 1.0f);
    
    // Level up while we have enough experience and haven't hit the cap
    while (SkillData.Experience >= RequiredExperience && SkillData.CurrentValue < SkillData.MaxValue)
    {
        SkillData.CurrentValue += 1.0f;
        
        UE_LOG(LogTemp, Verbose, TEXT("%s skill leveled up to %.1f"), 
               OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
               SkillData.CurrentValue);
        
        // Check if we need to calculate next level requirement
        if (SkillData.CurrentValue < SkillData.MaxValue)
        {
            RequiredExperience = CalculateExperienceForLevel(SkillData.CurrentValue + 1.0f);
        }
        else
        {
            break; // Reached skill cap
        }
    }
}

float USkillsComponent::CalculateExperienceForLevel(float Level) const
{
    // Exponential curve: Each level requires more experience
    // Formula: BaseExp * (Level^1.5)
    float BaseExperience = 100.0f;
    return BaseExperience * FMath::Pow(Level, 1.5f);
}

void USkillsComponent::HandleSkillCap()
{
    if (CurrentTotalSkillPoints <= TotalSkillCap)
        return;
        
    // Fixed: Add the required parameter (CurrentTotalSkillPoints)
    OnSkillCapReached.Broadcast(CurrentTotalSkillPoints);
    
    // Find the lowest skill that isn't locked and decay it
    float LowestSkillValue = 100.0f;
    ESkillType LowestSkillType = ESkillType::OneHanded;
    
    for (const auto& SkillPair : Skills)
    {
        if (!SkillPair.Value.bIsLocked && SkillPair.Value.CurrentValue < LowestSkillValue)
        {
            LowestSkillValue = SkillPair.Value.CurrentValue;
            LowestSkillType = SkillPair.Key;
        }
    }
    
    // Decay the lowest skill
    FLegacySkillData* LowestSkill = Skills.Find(LowestSkillType);
    if (LowestSkill && LowestSkill->CurrentValue > 0.0f)
    {
        LowestSkill->CurrentValue = FMath::Max(0.0f, LowestSkill->CurrentValue - 0.1f);
        LowestSkill->Experience = CalculateExperienceForLevel(LowestSkill->CurrentValue);
        LowestSkill->TotalExperience = LowestSkill->Experience; // Sync legacy field
        
        UE_LOG(LogTemp, Log, TEXT("%s skill %s decayed to %.1f due to skill cap"), 
               OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
               *UEnum::GetValueAsString(LowestSkillType), LowestSkill->CurrentValue);
               
        BroadcastSkillChanged(LowestSkillType);
    }
}

void USkillsComponent::UpdateTotalSkillPoints()
{
    CurrentTotalSkillPoints = 0.0f;
    
    for (const auto& SkillPair : Skills)
    {
        CurrentTotalSkillPoints += SkillPair.Value.CurrentValue;
    }
}

void USkillsComponent::BroadcastSkillChanged(ESkillType SkillType)
{
    if (const FLegacySkillData* SkillData = Skills.Find(SkillType))
    {
        // Fixed: Add the required third parameter (Experience)
        OnSkillChanged.Broadcast(SkillType, SkillData->CurrentValue, SkillData->Experience);
    }
}

void USkillsComponent::SetSkillValue(ESkillType SkillType, float NewValue)
{
    FLegacySkillData* SkillData = Skills.Find(SkillType);
    if (!SkillData)
        return;
        
    NewValue = FMath::Clamp(NewValue, 0.0f, SkillData->MaxValue);
    SkillData->CurrentValue = NewValue;
    
    // Update experience to match the new level
    SkillData->Experience = CalculateExperienceForLevel(NewValue);
    SkillData->TotalExperience = SkillData->Experience; // Sync legacy field
    
    UpdateTotalSkillPoints();
    BroadcastSkillChanged(SkillType);
}

void USkillsComponent::AddSkillModifier(ESkillType SkillType, float ModifierAmount)
{
    FLegacySkillData* SkillData = Skills.Find(SkillType);
    if (!SkillData)
        return;
        
    SkillData->TemporaryModifier += ModifierAmount;
    SkillData->Modifier += ModifierAmount; // Legacy field
    BroadcastSkillChanged(SkillType);
}

void USkillsComponent::RemoveSkillModifier(ESkillType SkillType, float ModifierAmount)
{
    FLegacySkillData* SkillData = Skills.Find(SkillType);
    if (!SkillData)
        return;
        
    SkillData->TemporaryModifier -= ModifierAmount;
    SkillData->Modifier -= ModifierAmount; // Legacy field
    BroadcastSkillChanged(SkillType);
}

void USkillsComponent::LockSkill(ESkillType SkillType, bool bLocked)
{
    FLegacySkillData* SkillData = Skills.Find(SkillType);
    if (!SkillData)
        return;
        
    SkillData->bIsLocked = bLocked;
    
    UE_LOG(LogTemp, Log, TEXT("%s %s skill %s"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
           bLocked ? TEXT("locked") : TEXT("unlocked"),
           *UEnum::GetValueAsString(SkillType));
}

float USkillsComponent::GetSkillValue(ESkillType SkillType) const
{
    const FLegacySkillData* SkillData = Skills.Find(SkillType);
    return SkillData ? SkillData->CurrentValue : 0.0f;
}

float USkillsComponent::GetEffectiveSkillValue(ESkillType SkillType) const
{
    const FLegacySkillData* SkillData = Skills.Find(SkillType);
    return SkillData ? SkillData->CurrentValue + SkillData->TemporaryModifier : 0.0f;
}

float USkillsComponent::GetSkillExperience(ESkillType SkillType) const
{
    const FLegacySkillData* SkillData = Skills.Find(SkillType);
    return SkillData ? SkillData->Experience : 0.0f;
}

float USkillsComponent::GetSkillProgress(ESkillType SkillType) const
{
    const FLegacySkillData* SkillData = Skills.Find(SkillType);
    if (!SkillData || SkillData->CurrentValue >= SkillData->MaxValue)
        return 1.0f;
    
    float CurrentLevelExp = CalculateExperienceForLevel(SkillData->CurrentValue);
    float NextLevelExp = CalculateExperienceForLevel(SkillData->CurrentValue + 1.0f);
    
    return FMath::Clamp((SkillData->Experience - CurrentLevelExp) / (NextLevelExp - CurrentLevelExp), 0.0f, 1.0f);
}

bool USkillsComponent::IsSkillLocked(ESkillType SkillType) const
{
    const FLegacySkillData* SkillData = Skills.Find(SkillType);
    return SkillData ? SkillData->bIsLocked : false;
}

float USkillsComponent::GetRemainingSkillPoints() const
{
    return FMath::Max(0.0f, TotalSkillCap - CurrentTotalSkillPoints);
}

void USkillsComponent::ResetSkill(ESkillType SkillType)
{
    FLegacySkillData* SkillData = Skills.Find(SkillType);
    if (!SkillData)
        return;
    
    SkillData->CurrentValue = 5.0f; // Default starting value
    SkillData->Experience = 0.0f;
    SkillData->TotalExperience = 0.0f;
    SkillData->TemporaryModifier = 0.0f;
    SkillData->Modifier = 0.0f;
    SkillData->bIsLocked = false;
    
    UpdateTotalSkillPoints();
    BroadcastSkillChanged(SkillType);
    
    UE_LOG(LogTemp, Log, TEXT("%s skill %s has been reset"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
           *UEnum::GetValueAsString(SkillType));
}

FGameplayTag USkillsComponent::SkillTypeToGameplayTag(ESkillType SkillType)
{
    // Convert legacy skill types to gameplay tags
    switch (SkillType)
    {
        case ESkillType::OneHanded: return FGameplayTag::RequestGameplayTag(FName("Skill.Combat.OneHanded"));
        case ESkillType::TwoHanded: return FGameplayTag::RequestGameplayTag(FName("Skill.Combat.TwoHanded"));
        case ESkillType::Marksman: return FGameplayTag::RequestGameplayTag(FName("Skill.Combat.Marksman"));
        case ESkillType::Block: return FGameplayTag::RequestGameplayTag(FName("Skill.Combat.Block"));
        case ESkillType::Smithing: return FGameplayTag::RequestGameplayTag(FName("Skill.Crafting.Smithing"));
        case ESkillType::HeavyArmor: return FGameplayTag::RequestGameplayTag(FName("Skill.Combat.HeavyArmor"));
        case ESkillType::LightArmor: return FGameplayTag::RequestGameplayTag(FName("Skill.Combat.LightArmor"));
        case ESkillType::Pickpocket: return FGameplayTag::RequestGameplayTag(FName("Skill.Stealth.Pickpocket"));
        case ESkillType::Lockpicking: return FGameplayTag::RequestGameplayTag(FName("Skill.Stealth.Lockpicking"));
        case ESkillType::Sneak: return FGameplayTag::RequestGameplayTag(FName("Skill.Stealth.Sneak"));
        case ESkillType::Alchemy: return FGameplayTag::RequestGameplayTag(FName("Skill.Magic.Alchemy"));
        case ESkillType::Speech: return FGameplayTag::RequestGameplayTag(FName("Skill.Social.Speech"));
        case ESkillType::Alteration: return FGameplayTag::RequestGameplayTag(FName("Skill.Magic.Alteration"));
        case ESkillType::Conjuration: return FGameplayTag::RequestGameplayTag(FName("Skill.Magic.Conjuration"));
        case ESkillType::Destruction: return FGameplayTag::RequestGameplayTag(FName("Skill.Magic.Destruction"));
        case ESkillType::Illusion: return FGameplayTag::RequestGameplayTag(FName("Skill.Magic.Illusion"));
        case ESkillType::Restoration: return FGameplayTag::RequestGameplayTag(FName("Skill.Magic.Restoration"));
        case ESkillType::Enchanting: return FGameplayTag::RequestGameplayTag(FName("Skill.Magic.Enchanting"));
        default: return FGameplayTag();
    }
}

ESkillType USkillsComponent::GameplayTagToSkillType(FGameplayTag SkillTag)
{
    // Convert gameplay tags back to legacy skill types
    FString TagString = SkillTag.ToString();
    
    if (TagString.Contains("OneHanded")) return ESkillType::OneHanded;
    if (TagString.Contains("TwoHanded")) return ESkillType::TwoHanded;
    if (TagString.Contains("Marksman")) return ESkillType::Marksman;
    if (TagString.Contains("Block")) return ESkillType::Block;
    if (TagString.Contains("Smithing")) return ESkillType::Smithing;
    if (TagString.Contains("HeavyArmor")) return ESkillType::HeavyArmor;
    if (TagString.Contains("LightArmor")) return ESkillType::LightArmor;
    if (TagString.Contains("Pickpocket")) return ESkillType::Pickpocket;
    if (TagString.Contains("Lockpicking")) return ESkillType::Lockpicking;
    if (TagString.Contains("Sneak")) return ESkillType::Sneak;
    if (TagString.Contains("Alchemy")) return ESkillType::Alchemy;
    if (TagString.Contains("Speech")) return ESkillType::Speech;
    if (TagString.Contains("Alteration")) return ESkillType::Alteration;
    if (TagString.Contains("Conjuration")) return ESkillType::Conjuration;
    if (TagString.Contains("Destruction")) return ESkillType::Destruction;
    if (TagString.Contains("Illusion")) return ESkillType::Illusion;
    if (TagString.Contains("Restoration")) return ESkillType::Restoration;
    if (TagString.Contains("Enchanting")) return ESkillType::Enchanting;
    
    return ESkillType::OneHanded; // Default fallback
}

void USkillsComponent::OnRep_Skills()
{
    UpdateTotalSkillPoints();
    
    UE_LOG(LogTemp, Verbose, TEXT("%s skills replicated (%d skills, %.1f total points)"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
           Skills.Num(), CurrentTotalSkillPoints);
}

bool USkillsComponent::IsAtSkillCap() const
{
    return CurrentTotalSkillPoints >= TotalSkillCap;
}

TArray<ESkillType> USkillsComponent::GetHighestSkills(int32 Count) const
{
    TArray<TPair<ESkillType, float>> SkillValuePairs;
    
    // Create array of skill type/value pairs
    for (const auto& SkillPair : Skills)
    {
        SkillValuePairs.Add(TPair<ESkillType, float>(SkillPair.Key, SkillPair.Value.CurrentValue));
    }
    
    // Sort by skill value in descending order
    SkillValuePairs.Sort([](const TPair<ESkillType, float>& A, const TPair<ESkillType, float>& B)
    {
        return A.Value > B.Value;
    });
    
    // Extract the top N skill types
    TArray<ESkillType> HighestSkills;
    for (int32 i = 0; i < FMath::Min(Count, SkillValuePairs.Num()); ++i)
    {
        HighestSkills.Add(SkillValuePairs[i].Key);
    }
    
    return HighestSkills;
}

FLegacySkillData USkillsComponent::GetSkillData(ESkillType SkillType) const
{
    const FLegacySkillData* SkillData = Skills.Find(SkillType);
    if (SkillData)
    {
        return *SkillData;
    }
    
    // Return default skill data if not found
    FLegacySkillData DefaultSkillData;
    DefaultSkillData.CurrentValue = 0.0f;
    DefaultSkillData.MaxValue = 100.0f;
    DefaultSkillData.Experience = 0.0f;
    DefaultSkillData.TotalExperience = 0.0f;
    DefaultSkillData.TemporaryModifier = 0.0f;
    DefaultSkillData.Modifier = 0.0f;
    DefaultSkillData.bIsLocked = false;
    
    return DefaultSkillData;
}

void USkillsComponent::InitializeSkillsFromDataTable()
{
    if (!SkillDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("SkillsComponent: No skill data table assigned, using default initialization"));
        InitializeDefaultSkills();
        return;
    }
    
    // Clear existing skills
    Skills.Empty();
    
    // Get all rows from the data table
    TArray<FName> RowNames = SkillDataTable->GetRowNames();
    
    for (const FName& RowName : RowNames)
    {
        // Try to find the row data
        FString ContextString = FString::Printf(TEXT("Loading skill data for %s"), *RowName.ToString());
        FSkillTableRow* SkillRow = SkillDataTable->FindRow<FSkillTableRow>(RowName, ContextString);
        
        if (SkillRow)
        {
            // Convert the table row to our legacy skill data format
            FLegacySkillData SkillData;
            SkillData.CurrentValue = SkillRow->StartingValue;
            SkillData.MaxValue = SkillRow->MaxValue;
            SkillData.Experience = 0.0f;
            SkillData.TotalExperience = 0.0f;
            SkillData.TemporaryModifier = 0.0f;
            SkillData.Modifier = 0.0f;
            
            // Handle the bStartsLocked property safely
            if (SkillRow->bStartsLocked)
            {
                SkillData.bIsLocked = true;
            }
            else
            {
                SkillData.bIsLocked = false;
            }
            
            // Add to skills map using the skill type from the table
            Skills.Add(SkillRow->SkillType, SkillData);
            
            UE_LOG(LogTemp, Log, TEXT("Loaded skill %s from data table"), *UEnum::GetValueAsString(SkillRow->SkillType));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to load skill data for row %s"), *RowName.ToString());
        }
    }
    
    // If no skills were loaded from table, fall back to default initialization
    if (Skills.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No skills loaded from data table, falling back to default initialization"));
        InitializeDefaultSkills();
        return;
    }
    
    UpdateTotalSkillPoints();
    
    UE_LOG(LogTemp, Log, TEXT("%s initialized %d skills from data table"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"), 
           Skills.Num());
}

void USkillsComponent::ResetAllSkills()
{
    for (auto& SkillPair : Skills)
    {
        FLegacySkillData& SkillData = SkillPair.Value;
        
        // Reset to default starting values
        SkillData.CurrentValue = 5.0f; // Default starting value
        SkillData.Experience = 0.0f;
        SkillData.TotalExperience = 0.0f;
        SkillData.TemporaryModifier = 0.0f;
        SkillData.Modifier = 0.0f;
        SkillData.bIsLocked = false;
        
        // Broadcast change for each skill
        BroadcastSkillChanged(SkillPair.Key);
    }
    
    UpdateTotalSkillPoints();
    
    UE_LOG(LogTemp, Log, TEXT("%s reset all skills to default values"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"));
}