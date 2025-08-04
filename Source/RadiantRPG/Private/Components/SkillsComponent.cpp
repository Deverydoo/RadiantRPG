// Private/Components/SkillsComponent.cpp

#include "Components/SkillsComponent.h"
#include "Characters/BaseCharacter.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"

USkillsComponent::USkillsComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    // Ultima Online inspired settings
    TotalSkillCap = 700.0f; // Total skill points allowed
    IndividualSkillCap = 100.0f; // Max for any single skill
    ExperienceGainMultiplier = 1.0f;
    bAllowSkillLoss = true; // Skills can decay when at cap
    SkillLossRate = 0.1f; // How much skills decay per point gained elsewhere
    
    CurrentTotalSkillPoints = 0.0f;
    OwnerCharacter = nullptr;
    SkillDataTable = nullptr;
}

void USkillsComponent::BeginPlay()
{
    Super::BeginPlay();

    // Cache owner reference
    OwnerCharacter = Cast<ABaseCharacter>(GetOwner());
    
    // Initialize skills from data table if available
    if (SkillDataTable)
    {
        InitializeSkillsFromDataTable();
    }
    else
    {
        // Initialize with default starting skills
        InitializeDefaultSkills();
    }
    
    UpdateTotalSkillPoints();
    
    UE_LOG(LogTemp, Log, TEXT("SkillsComponent initialized for %s with %f total skill points"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"), 
           CurrentTotalSkillPoints);
}

void USkillsComponent::InitializeDefaultSkills()
{
    // Initialize all skill types with starting values
    TArray<ESkillType> AllSkillTypes = {
        ESkillType::OneHanded, ESkillType::TwoHanded, ESkillType::Archery, ESkillType::Block,
        ESkillType::HeavyArmor, ESkillType::LightArmor, ESkillType::Destruction, ESkillType::Restoration,
        ESkillType::Illusion, ESkillType::Conjuration, ESkillType::Enchanting, ESkillType::Alchemy,
        ESkillType::Sneak, ESkillType::Lockpicking, ESkillType::Pickpocket, ESkillType::Smithing,
        ESkillType::Tailoring, ESkillType::Cooking, ESkillType::Athletics, ESkillType::Acrobatics,
        ESkillType::Speechcraft, ESkillType::Mercantile, ESkillType::Lore, ESkillType::Survival
    };
    
    for (ESkillType SkillType : AllSkillTypes)
    {
        FSkillData NewSkill;
        NewSkill.SkillType = SkillType;
        NewSkill.CurrentValue = 5.0f; // Starting skill level
        NewSkill.Experience = 0.0f;
        NewSkill.bIsLocked = false;
        NewSkill.Modifier = 0.0f;
        
        Skills.Add(SkillType, NewSkill);
    }
}

void USkillsComponent::InitializeSkillsFromDataTable()
{
    if (!SkillDataTable)
        return;
        
    // Clear existing skills
    Skills.Empty();
    
    // Get all skill types from enum
    TArray<ESkillType> AllSkillTypes = {
        ESkillType::OneHanded, ESkillType::TwoHanded, ESkillType::Archery, ESkillType::Block,
        ESkillType::HeavyArmor, ESkillType::LightArmor, ESkillType::Destruction, ESkillType::Restoration,
        ESkillType::Illusion, ESkillType::Conjuration, ESkillType::Enchanting, ESkillType::Alchemy,
        ESkillType::Sneak, ESkillType::Lockpicking, ESkillType::Pickpocket, ESkillType::Smithing,
        ESkillType::Tailoring, ESkillType::Cooking, ESkillType::Athletics, ESkillType::Acrobatics,
        ESkillType::Speechcraft, ESkillType::Mercantile, ESkillType::Lore, ESkillType::Survival
    };
    
    for (ESkillType SkillType : AllSkillTypes)
    {
        FString SkillName = UEnum::GetValueAsString(SkillType);
        FName RowName = FName(*SkillName);
        
        FSkillTableRow* SkillRow = SkillDataTable->FindRow<FSkillTableRow>(RowName, TEXT(""));
        
        FSkillData NewSkill;
        NewSkill.SkillType = SkillType;
        
        if (SkillRow)
        {
            NewSkill.CurrentValue = SkillRow->StartingValue;
        }
        else
        {
            NewSkill.CurrentValue = 5.0f; // Default starting value
        }
        
        NewSkill.Experience = 0.0f;
        NewSkill.bIsLocked = false;
        NewSkill.Modifier = 0.0f;
        
        Skills.Add(SkillType, NewSkill);
    }
    
    UE_LOG(LogTemp, Log, TEXT("Skills initialized from data table with %d skills"), Skills.Num());
}

void USkillsComponent::GainSkillExperience(ESkillType SkillType, float ExperienceAmount)
{
    if (ExperienceAmount <= 0.0f)
        return;
        
    FSkillData* SkillData = Skills.Find(SkillType);
    if (!SkillData || SkillData->bIsLocked)
        return;
        
    // Apply global experience multiplier
    float AdjustedExperience = ExperienceAmount * ExperienceGainMultiplier;
    
    // Apply skill-specific multiplier from data table
    if (SkillDataTable)
    {
        FString SkillName = UEnum::GetValueAsString(SkillType);
        FName RowName = FName(*SkillName);
        FSkillTableRow* SkillRow = SkillDataTable->FindRow<FSkillTableRow>(RowName, TEXT(""));
        
        if (SkillRow)
        {
            AdjustedExperience *= SkillRow->ExperienceMultiplier;
        }
    }
    
    // Add experience
    SkillData->Experience += AdjustedExperience;
    
    // Check for skill level up
    TryLevelUpSkill(SkillType);
    
    // Update total and handle skill cap
    UpdateTotalSkillPoints();
    
    BroadcastSkillChanged(SkillType);
    
    UE_LOG(LogTemp, Verbose, TEXT("%s gained %f experience in %s (Total: %f)"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
           AdjustedExperience, *UEnum::GetValueAsString(SkillType), SkillData->Experience);
}

void USkillsComponent::TryLevelUpSkill(ESkillType SkillType)
{
    FSkillData* SkillData = Skills.Find(SkillType);
    if (!SkillData)
        return;
        
    float RequiredExperience = CalculateExperienceForLevel(SkillData->CurrentValue + 1.0f);
    
    while (SkillData->Experience >= RequiredExperience && SkillData->CurrentValue < IndividualSkillCap)
    {
        SkillData->CurrentValue += 1.0f;
        
        // Broadcast level up
        OnSkillLevelUp.Broadcast(SkillType, SkillData->CurrentValue);
        OnSkillLevelUpBP(SkillType, SkillData->CurrentValue);
        
        UE_LOG(LogTemp, Log, TEXT("%s leveled up %s to %.1f"), 
               OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
               *UEnum::GetValueAsString(SkillType), SkillData->CurrentValue);
        
        // Check if we need to calculate next level requirement
        if (SkillData->CurrentValue < IndividualSkillCap)
        {
            RequiredExperience = CalculateExperienceForLevel(SkillData->CurrentValue + 1.0f);
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

float USkillsComponent::CalculateLevelFromExperience(float Experience) const
{
    // Reverse of the experience formula
    float BaseExperience = 100.0f;
    return FMath::Pow(Experience / BaseExperience, 1.0f / 1.5f);
}

void USkillsComponent::SetSkillValue(ESkillType SkillType, float NewValue)
{
    FSkillData* SkillData = Skills.Find(SkillType);
    if (!SkillData)
        return;
        
    NewValue = FMath::Clamp(NewValue, 0.0f, IndividualSkillCap);
    SkillData->CurrentValue = NewValue;
    
    // Update experience to match the new level
    SkillData->Experience = CalculateExperienceForLevel(NewValue);
    
    UpdateTotalSkillPoints();
    BroadcastSkillChanged(SkillType);
}

void USkillsComponent::AddSkillModifier(ESkillType SkillType, float ModifierAmount)
{
    FSkillData* SkillData = Skills.Find(SkillType);
    if (!SkillData)
        return;
        
    SkillData->Modifier += ModifierAmount;
    BroadcastSkillChanged(SkillType);
}

void USkillsComponent::RemoveSkillModifier(ESkillType SkillType, float ModifierAmount)
{
    FSkillData* SkillData = Skills.Find(SkillType);
    if (!SkillData)
        return;
        
    SkillData->Modifier -= ModifierAmount;
    BroadcastSkillChanged(SkillType);
}

void USkillsComponent::LockSkill(ESkillType SkillType, bool bLocked)
{
    FSkillData* SkillData = Skills.Find(SkillType);
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
    const FSkillData* SkillData = Skills.Find(SkillType);
    return SkillData ? SkillData->CurrentValue : 0.0f;
}

float USkillsComponent::GetEffectiveSkillValue(ESkillType SkillType) const
{
    const FSkillData* SkillData = Skills.Find(SkillType);
    if (!SkillData)
        return 0.0f;
        
    float EffectiveValue = SkillData->CurrentValue + SkillData->Modifier;
    return FMath::Clamp(EffectiveValue, 0.0f, IndividualSkillCap);
}

float USkillsComponent::GetSkillExperience(ESkillType SkillType) const
{
    const FSkillData* SkillData = Skills.Find(SkillType);
    return SkillData ? SkillData->Experience : 0.0f;
}

bool USkillsComponent::IsSkillLocked(ESkillType SkillType) const
{
    const FSkillData* SkillData = Skills.Find(SkillType);
    return SkillData ? SkillData->bIsLocked : false;
}

float USkillsComponent::GetRemainingSkillPoints() const
{
    return FMath::Max(0.0f, TotalSkillCap - CurrentTotalSkillPoints);
}

bool USkillsComponent::IsAtSkillCap() const
{
    return CurrentTotalSkillPoints >= TotalSkillCap;
}

TArray<ESkillType> USkillsComponent::GetHighestSkills(int32 Count) const
{
    TArray<TPair<ESkillType, float>> SkillPairs;
    
    for (const auto& SkillPair : Skills)
    {
        SkillPairs.Add(TPair<ESkillType, float>(SkillPair.Key, SkillPair.Value.CurrentValue));
    }
    
    // Sort by skill value (highest first)
    SkillPairs.Sort([](const TPair<ESkillType, float>& A, const TPair<ESkillType, float>& B)
    {
        return A.Value > B.Value;
    });
    
    TArray<ESkillType> Result;
    for (int32 i = 0; i < FMath::Min(Count, SkillPairs.Num()); i++)
    {
        Result.Add(SkillPairs[i].Key);
    }
    
    return Result;
}

void USkillsComponent::ResetAllSkills()
{
    for (auto& SkillPair : Skills)
    {
        SkillPair.Value.CurrentValue = 5.0f;
        SkillPair.Value.Experience = 0.0f;
        SkillPair.Value.Modifier = 0.0f;
        SkillPair.Value.bIsLocked = false;
    }
    
    UpdateTotalSkillPoints();
    
    UE_LOG(LogTemp, Log, TEXT("%s reset all skills"), 
           OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"));
}

void USkillsComponent::ResetSkill(ESkillType SkillType)
{
    FSkillData* SkillData = Skills.Find(SkillType);
    if (!SkillData)
        return;
        
    SkillData->CurrentValue = 5.0f;
    SkillData->Experience = 0.0f;
    SkillData->Modifier = 0.0f;
    SkillData->bIsLocked = false;
    
    UpdateTotalSkillPoints();
    BroadcastSkillChanged(SkillType);
}

void USkillsComponent::UpdateTotalSkillPoints()
{
    float NewTotal = 0.0f;
    
    for (const auto& SkillPair : Skills)
    {
        NewTotal += SkillPair.Value.CurrentValue;
    }
    
    CurrentTotalSkillPoints = NewTotal;
    
    // Check if we've exceeded the skill cap
    if (IsAtSkillCap())
    {
        HandleSkillCapExceeded();
    }
}

void USkillsComponent::HandleSkillCapExceeded()
{
    if (!bAllowSkillLoss || CurrentTotalSkillPoints <= TotalSkillCap)
        return;
        
    float ExcessPoints = CurrentTotalSkillPoints - TotalSkillCap;
    
    // Find unlocked skills to reduce
    TArray<ESkillType> UnlockedSkills;
    for (const auto& SkillPair : Skills)
    {
        if (!SkillPair.Value.bIsLocked && SkillPair.Value.CurrentValue > 0.0f)
        {
            UnlockedSkills.Add(SkillPair.Key);
        }
    }
    
    // Randomly reduce skills until we're under the cap
    while (ExcessPoints > 0.0f && UnlockedSkills.Num() > 0)
    {
        int32 RandomIndex = FMath::RandRange(0, UnlockedSkills.Num() - 1);
        ESkillType SkillToReduce = UnlockedSkills[RandomIndex];
        
        FSkillData* SkillData = Skills.Find(SkillToReduce);
        if (SkillData && SkillData->CurrentValue > 0.0f)
        {
            float ReductionAmount = FMath::Min(SkillLossRate, SkillData->CurrentValue);
            SkillData->CurrentValue -= ReductionAmount;
            SkillData->Experience = CalculateExperienceForLevel(SkillData->CurrentValue);
            
            ExcessPoints -= ReductionAmount;
            CurrentTotalSkillPoints -= ReductionAmount;
            
            BroadcastSkillChanged(SkillToReduce);
            
            UE_LOG(LogTemp, Warning, TEXT("%s lost %.1f points in %s due to skill cap"), 
                   OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"),
                   ReductionAmount, *UEnum::GetValueAsString(SkillToReduce));
        }
        
        // Remove skills that are at 0
        if (SkillData->CurrentValue <= 0.0f)
        {
            UnlockedSkills.RemoveAt(RandomIndex);
        }
    }
    
    OnSkillCapReached.Broadcast(CurrentTotalSkillPoints);
    OnSkillCapReachedBP(CurrentTotalSkillPoints);
}

void USkillsComponent::BroadcastSkillChanged(ESkillType SkillType)
{
    const FSkillData* SkillData = Skills.Find(SkillType);
    if (!SkillData)
        return;
        
    OnSkillChanged.Broadcast(SkillType, SkillData->CurrentValue, SkillData->Experience);
    OnSkillChangedBP(SkillType, SkillData->CurrentValue, SkillData->Experience);
}