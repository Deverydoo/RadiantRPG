// Private/AI/Core/ARPG_AIPersonalityComponent.cpp

#include "AI/Core/ARPG_AIPersonalityComponent.h"
#include "AI/Core/ARPG_AIBrainComponent.h"
#include "Engine/DataTable.h"
#include "Components/ActorComponent.h"

UARPG_AIPersonalityComponent::UARPG_AIPersonalityComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);
    
    PersonalityDataTable = nullptr;
    DataTableRowName = NAME_None;
    
    // Initialize default configuration
    PersonalityConfig = FARPG_PersonalityConfiguration();
    EffectiveConfig = PersonalityConfig;
}

void UARPG_AIPersonalityComponent::SetPersonalityTrait(EARPG_PersonalityTrait TraitType, float Strength, float Flexibility)
{
    // Simply delegate to the existing SetTraitStrength function
    SetTraitStrength(TraitType, Strength, Flexibility);
}

void UARPG_AIPersonalityComponent::BeginPlay()
{
    Super::BeginPlay();
    
    LoadPersonalityConfiguration();
    InitializeComponentReferences();
    InitializeDefaultTraits();
}

void UARPG_AIPersonalityComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CurrentTraits.Empty();
    ExperienceHistory.Empty();
    BrainComponent.Reset();
    
    Super::EndPlay(EndPlayReason);
}

void UARPG_AIPersonalityComponent::LoadPersonalityConfiguration()
{
    FARPG_PersonalityConfiguration NewConfig;
    TArray<FARPG_AIPersonalityTrait> NewTraits;
    
    if (LoadConfigurationFromDataTable(NewConfig, NewTraits))
    {
        EffectiveConfig = NewConfig;
        DefaultTraits = NewTraits;
        
        if (EffectiveConfig.bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Log, TEXT("Personality Component: Loaded configuration from data table '%s' row '%s'"), 
                PersonalityDataTable ? *PersonalityDataTable->GetName() : TEXT("None"), 
                *DataTableRowName.ToString());
        }
    }
    else
    {
        EffectiveConfig = PersonalityConfig;
        
        if (EffectiveConfig.bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Log, TEXT("Personality Component: Using blueprint configuration"));
        }
    }
}

void UARPG_AIPersonalityComponent::SetDataTableConfig(UDataTable* DataTable, FName RowName)
{
    PersonalityDataTable = DataTable;
    DataTableRowName = RowName;
    
    if (HasBegunPlay())
    {
        LoadPersonalityConfiguration();
        InitializeDefaultTraits();
    }
}

FARPG_PersonalityConfiguration UARPG_AIPersonalityComponent::GetEffectiveConfiguration() const
{
    return EffectiveConfig;
}

bool UARPG_AIPersonalityComponent::LoadConfigurationFromDataTable(FARPG_PersonalityConfiguration& OutConfig, TArray<FARPG_AIPersonalityTrait>& OutTraits) const
{
    if (!PersonalityDataTable || DataTableRowName.IsNone())
    {
        return false;
    }

    // Try to find the row in the data table
    if (const FARPG_AIPersonalityTraitRow* Row = PersonalityDataTable->FindRow<FARPG_AIPersonalityTraitRow>(DataTableRowName, TEXT("LoadPersonalityConfig")))
    {
        // Create configuration based on data table values
        OutConfig.bAllowPersonalityChange = true; // Default for data table configs
        OutConfig.bEnablePersonalityInfluence = true;
        OutConfig.PersonalityChangeRate = 1.0f;
        OutConfig.ExperienceImpactMultiplier = 1.0f;
        OutConfig.bEnableDebugLogging = false;
        
        // Create traits from data table row
        OutTraits.Empty();
        
        // Add all the personality traits from the data table
        OutTraits.Add(CreateTraitFromDataTable(EARPG_PersonalityTrait::Aggression, Row->Aggression));
        OutTraits.Add(CreateTraitFromDataTable(EARPG_PersonalityTrait::Curiosity, Row->Curiosity));
        OutTraits.Add(CreateTraitFromDataTable(EARPG_PersonalityTrait::Sociability, Row->Sociability));
        OutTraits.Add(CreateTraitFromDataTable(EARPG_PersonalityTrait::Bravery, Row->Bravery));
        OutTraits.Add(CreateTraitFromDataTable(EARPG_PersonalityTrait::Greed, Row->Greed));
        OutTraits.Add(CreateTraitFromDataTable(EARPG_PersonalityTrait::Loyalty, Row->Loyalty));
        OutTraits.Add(CreateTraitFromDataTable(EARPG_PersonalityTrait::Intelligence, Row->Intelligence));
        OutTraits.Add(CreateTraitFromDataTable(EARPG_PersonalityTrait::Caution, Row->Caution));
        OutTraits.Add(CreateTraitFromDataTable(EARPG_PersonalityTrait::Impulsiveness, Row->Impulsiveness));
        
        return true;
    }
    
    return false;
}

FARPG_AIPersonalityTrait UARPG_AIPersonalityComponent::CreateTraitFromDataTable(EARPG_PersonalityTrait TraitType, float Strength) const
{
    FARPG_AIPersonalityTrait Trait;
    Trait.TraitType = TraitType;
    Trait.TraitStrength = FMath::Clamp(Strength, 0.0f, 1.0f);
    Trait.Flexibility = 0.2f; // Default flexibility
    return Trait;
}

float UARPG_AIPersonalityComponent::GetTraitStrength(EARPG_PersonalityTrait TraitType) const
{
    if (const FARPG_AIPersonalityTrait* Trait = CurrentTraits.Find(TraitType))
    {
        return Trait->TraitStrength;
    }
    return 0.5f; // Default neutral value
}

void UARPG_AIPersonalityComponent::SetTraitStrength(EARPG_PersonalityTrait TraitType, float Strength, float Flexibility)
{
    FARPG_AIPersonalityTrait& Trait = CurrentTraits.FindOrAdd(TraitType);
    Trait.TraitType = TraitType;
    Trait.TraitStrength = ValidateTraitStrength(Strength);
    Trait.Flexibility = FMath::Clamp(Flexibility, 0.0f, 1.0f);
    
    OnPersonalityTraitChanged.Broadcast(TraitType, Trait.TraitStrength);
    
    if (EffectiveConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("Personality: Set trait %s to %.2f"), 
            *GetTraitDisplayName(TraitType), Trait.TraitStrength);
    }
}

bool UARPG_AIPersonalityComponent::ModifyTrait(EARPG_PersonalityTrait TraitType, float Amount)
{
    if (!EffectiveConfig.bAllowPersonalityChange)
    {
        return false;
    }
    
    FARPG_AIPersonalityTrait& Trait = CurrentTraits.FindOrAdd(TraitType);
    if (Trait.TraitType == EARPG_PersonalityTrait::None)
    {
        Trait = GetDefaultTraitForType(TraitType);
    }
    
    float OldStrength = Trait.TraitStrength;
    float NewStrength = ValidateTraitStrength(OldStrength + Amount);
    
    // Check if change is within flexibility bounds
    float MaxChange = Trait.Flexibility * EffectiveConfig.PersonalityChangeRate;
    float ActualChange = FMath::Clamp(NewStrength - OldStrength, -MaxChange, MaxChange);
    
    Trait.TraitStrength = ValidateTraitStrength(OldStrength + ActualChange);
    
    if (FMath::Abs(ActualChange) > KINDA_SMALL_NUMBER)
    {
        OnPersonalityTraitChanged.Broadcast(TraitType, Trait.TraitStrength);
        
        if (EffectiveConfig.bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Log, TEXT("Personality: Modified trait %s by %.3f (%.2f -> %.2f)"), 
                *GetTraitDisplayName(TraitType), ActualChange, OldStrength, Trait.TraitStrength);
        }
        
        return true;
    }
    
    return false;
}

bool UARPG_AIPersonalityComponent::HasTrait(EARPG_PersonalityTrait TraitType) const
{
    return CurrentTraits.Contains(TraitType);
}

TMap<EARPG_PersonalityTrait, FARPG_AIPersonalityTrait> UARPG_AIPersonalityComponent::GetAllTraits() const
{
    return CurrentTraits;
}

void UARPG_AIPersonalityComponent::ModifyIntent(FARPG_AIIntent& Intent) const
{
    if (!EffectiveConfig.bEnablePersonalityInfluence)
    {
        return;
    }
    
    // Apply personality modifications to intent priority and type
    float ModificationFactor = GetActionWeight(Intent.IntentTag);
    
    // Convert enum to float, apply modification, then convert back
    float CurrentPriority = static_cast<float>(Intent.Priority);
    float ModifiedPriority = CurrentPriority * ModificationFactor;
    
    // Clamp to valid enum range and convert back
    int32 ClampedPriority = FMath::Clamp(FMath::RoundToInt(ModifiedPriority), 
                                        static_cast<int32>(EARPG_AIIntentPriority::Idle), 
                                        static_cast<int32>(EARPG_AIIntentPriority::Critical));
    Intent.Priority = static_cast<EARPG_AIIntentPriority>(ClampedPriority);
    
    if (EffectiveConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("Personality: Modified intent %s priority by %.2f (%.0f -> %d)"), 
            *Intent.IntentTag.ToString(), ModificationFactor, CurrentPriority, ClampedPriority);
    }
}


float UARPG_AIPersonalityComponent::GetActionWeight(FGameplayTag ActionTag) const
{
    if (!EffectiveConfig.bEnablePersonalityInfluence)
    {
        return 1.0f;
    }
    
    float Weight = 1.0f;
    
    // Apply trait influences based on action type
    for (const auto& TraitPair : CurrentTraits)
    {
        Weight *= GetTraitInfluenceOnAction(TraitPair.Key, ActionTag);
    }
    
    return FMath::Clamp(Weight, 0.1f, 2.0f);
}

bool UARPG_AIPersonalityComponent::SupportsAction(FGameplayTag ActionTag) const
{
    return GetActionWeight(ActionTag) > 0.1f;
}

float UARPG_AIPersonalityComponent::GetRelationshipModifier(const UARPG_AIPersonalityComponent* OtherPersonality) const
{
    if (!OtherPersonality || !EffectiveConfig.bEnablePersonalityInfluence)
    {
        return 1.0f;
    }
    
    float Compatibility = GetPersonalityCompatibility(OtherPersonality);
    return FMath::Lerp(0.5f, 1.5f, Compatibility);
}

float UARPG_AIPersonalityComponent::GetPersonalityCompatibility(const UARPG_AIPersonalityComponent* OtherPersonality) const
{
    if (!OtherPersonality)
    {
        return 0.5f;
    }
    
    float TotalCompatibility = 0.0f;
    int32 TraitCount = 0;
    
    for (const auto& TraitPair : CurrentTraits)
    {
        EARPG_PersonalityTrait TraitType = TraitPair.Key;
        float MyStrength = TraitPair.Value.TraitStrength;
        float OtherStrength = OtherPersonality->GetTraitStrength(TraitType);
        
        // Calculate compatibility based on trait similarity/difference
        float Difference = FMath::Abs(MyStrength - OtherStrength);
        float TraitCompatibility = 1.0f - Difference;
        
        TotalCompatibility += TraitCompatibility;
        TraitCount++;
    }
    
    return TraitCount > 0 ? TotalCompatibility / TraitCount : 0.5f;
}

TArray<FGameplayTag> UARPG_AIPersonalityComponent::GetPersonalityBasedIntents() const
{
    TArray<FGameplayTag> Intents;
    
    // Generate intents based on strong personality traits
    for (const auto& TraitPair : CurrentTraits)
    {
        if (TraitPair.Value.TraitStrength > 0.7f)
        {
            FGameplayTag IntentTag = GetGameplayTagForTrait(TraitPair.Key);
            if (IntentTag.IsValid())
            {
                Intents.Add(IntentTag);
            }
        }
    }
    
    return Intents;
}

TMap<FGameplayTag, float> UARPG_AIPersonalityComponent::GetPersonalityTraits() const
{
    TMap<FGameplayTag, float> TraitMap;
    
    for (const auto& TraitPair : CurrentTraits)
    {
        FGameplayTag TraitTag = GetGameplayTagForTrait(TraitPair.Key);
        if (TraitTag.IsValid())
        {
            TraitMap.Add(TraitTag, TraitPair.Value.TraitStrength);
        }
    }
    
    return TraitMap;
}

TMap<EARPG_PersonalityTrait, float> UARPG_AIPersonalityComponent::GetPersonalityTraitsAsEnumMap() const
{
    TMap<EARPG_PersonalityTrait, float> TraitMap;
    
    for (const auto& TraitPair : CurrentTraits)
    {
        TraitMap.Add(TraitPair.Key, TraitPair.Value.TraitStrength);
    }
    
    return TraitMap;
}

bool UARPG_AIPersonalityComponent::IsTraitStrong(EARPG_PersonalityTrait TraitType, float Threshold) const
{
    return GetTraitStrength(TraitType) >= Threshold;
}

bool UARPG_AIPersonalityComponent::IsTraitWeak(EARPG_PersonalityTrait TraitType, float Threshold) const
{
    return GetTraitStrength(TraitType) <= Threshold;
}

EARPG_PersonalityTrait UARPG_AIPersonalityComponent::GetDominantTrait() const
{
    EARPG_PersonalityTrait DominantTrait = EARPG_PersonalityTrait::None;
    float HighestStrength = 0.0f;
    
    for (const auto& TraitPair : CurrentTraits)
    {
        if (TraitPair.Value.TraitStrength > HighestStrength)
        {
            HighestStrength = TraitPair.Value.TraitStrength;
            DominantTrait = TraitPair.Key;
        }
    }
    
    return DominantTrait;
}

void UARPG_AIPersonalityComponent::AddExperience(FGameplayTag ExperienceTag, float Intensity)
{
    if (!EffectiveConfig.bAllowPersonalityChange)
    {
        return;
    }
    
    // Add to experience history
    float& ExistingExperience = ExperienceHistory.FindOrAdd(ExperienceTag);
    ExistingExperience += Intensity * EffectiveConfig.ExperienceImpactMultiplier;
    
    // Apply experience to relevant personality traits
    for (auto& TraitPair : CurrentTraits)
    {
        ApplyExperienceToTrait(TraitPair.Key, ExperienceTag, Intensity);
    }
    
    if (EffectiveConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("Personality: Added experience %s with intensity %.2f"), 
            *ExperienceTag.ToString(), Intensity);
    }
}

TMap<FGameplayTag, float> UARPG_AIPersonalityComponent::GetExperienceHistory() const
{
    return ExperienceHistory;
}

FString UARPG_AIPersonalityComponent::GetPersonalityDescription() const
{
    FString Description;
    
    EARPG_PersonalityTrait DominantTrait = GetDominantTrait();
    if (DominantTrait != EARPG_PersonalityTrait::None)
    {
        Description += FString::Printf(TEXT("Primarily %s"), *GetTraitDisplayName(DominantTrait));
        
        // Add secondary traits
        TArray<EARPG_PersonalityTrait> SecondaryTraits;
        for (const auto& TraitPair : CurrentTraits)
        {
            if (TraitPair.Key != DominantTrait && TraitPair.Value.TraitStrength > 0.6f)
            {
                SecondaryTraits.Add(TraitPair.Key);
            }
        }
        
        if (SecondaryTraits.Num() > 0)
        {
            Description += TEXT(", with strong ");
            for (int32 i = 0; i < SecondaryTraits.Num(); i++)
            {
                Description += GetTraitDisplayName(SecondaryTraits[i]);
                if (i < SecondaryTraits.Num() - 1)
                {
                    Description += TEXT(" and ");
                }
            }
            Description += TEXT(" tendencies");
        }
    }
    else
    {
        Description = TEXT("Balanced personality");
    }
    
    return Description;
}

void UARPG_AIPersonalityComponent::ResetPersonality()
{
    CurrentTraits.Empty();
    ExperienceHistory.Empty();
    InitializeDefaultTraits();
    
    OnPersonalityChanged.Broadcast(GetPersonalityDescription());
    
    if (EffectiveConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("Personality: Reset to default values"));
    }
}

FString UARPG_AIPersonalityComponent::GetPersonalitySummary() const
{
    FString Summary;
    
    for (const auto& TraitPair : CurrentTraits)
    {
        FString TraitName = GetTraitDisplayName(TraitPair.Key);
        float Strength = TraitPair.Value.TraitStrength;
        
        FString StrengthDesc;
        if (Strength > 0.8f) StrengthDesc = TEXT("Very High");
        else if (Strength > 0.6f) StrengthDesc = TEXT("High");
        else if (Strength > 0.4f) StrengthDesc = TEXT("Moderate");
        else if (Strength > 0.2f) StrengthDesc = TEXT("Low");
        else StrengthDesc = TEXT("Very Low");
        
        Summary += FString::Printf(TEXT("%s: %s (%.2f)\n"), *TraitName, *StrengthDesc, Strength);
    }
    
    return Summary;
}

float UARPG_AIPersonalityComponent::BP_GetCustomActionWeight(FGameplayTag ActionTag) const
{
    return GetActionWeight(ActionTag);
}

void UARPG_AIPersonalityComponent::InitializeComponentReferences()
{
    if (AActor* Owner = GetOwner())
    {
        BrainComponent = Owner->FindComponentByClass<UARPG_AIBrainComponent>();
    }
}

void UARPG_AIPersonalityComponent::InitializeDefaultTraits()
{
    CurrentTraits.Empty();
    
    // Initialize traits from default configuration
    for (const FARPG_AIPersonalityTrait& DefaultTrait : DefaultTraits)
    {
        CurrentTraits.Add(DefaultTrait.TraitType, DefaultTrait);
    }
    
    // Ensure all personality traits have values
    TArray<EARPG_PersonalityTrait> AllTraits = {
        EARPG_PersonalityTrait::Aggression,
        EARPG_PersonalityTrait::Curiosity,
        EARPG_PersonalityTrait::Sociability,
        EARPG_PersonalityTrait::Bravery,
        EARPG_PersonalityTrait::Greed,
        EARPG_PersonalityTrait::Loyalty,
        EARPG_PersonalityTrait::Intelligence,
        EARPG_PersonalityTrait::Caution,
        EARPG_PersonalityTrait::Impulsiveness
    };
    
    for (EARPG_PersonalityTrait TraitType : AllTraits)
    {
        if (!CurrentTraits.Contains(TraitType))
        {
            CurrentTraits.Add(TraitType, GetDefaultTraitForType(TraitType));
        }
    }
    
    if (EffectiveConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("Personality: Initialized %d traits"), CurrentTraits.Num());
    }
}

FARPG_AIPersonalityTrait UARPG_AIPersonalityComponent::GetDefaultTraitForType(EARPG_PersonalityTrait TraitType) const
{
    FARPG_AIPersonalityTrait DefaultTrait;
    DefaultTrait.TraitType = TraitType;
    DefaultTrait.TraitStrength = 0.5f; // Neutral default
    DefaultTrait.Flexibility = 0.2f;   // Moderate flexibility
    return DefaultTrait;
}

void UARPG_AIPersonalityComponent::ApplyExperienceToTrait(EARPG_PersonalityTrait TraitType, FGameplayTag ExperienceTag, float Intensity)
{
    // This is a simplified implementation - you could expand this with more complex logic
    // based on the relationship between experience types and personality traits
    
    float ChangeAmount = Intensity * EffectiveConfig.ExperienceImpactMultiplier * 0.01f; // Small changes
    
    // Apply trait-specific experience logic here
    // For now, just apply a small modification
    ModifyTrait(TraitType, ChangeAmount);
}

float UARPG_AIPersonalityComponent::GetTraitInfluenceOnAction(EARPG_PersonalityTrait TraitType, FGameplayTag ActionTag) const
{
    // This is where you'd implement trait-specific action influences
    // For now, return a simple influence based on trait strength
    
    float TraitStrength = GetTraitStrength(TraitType);
    
    // Example influences (customize based on your action tags)
    if (ActionTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Combat"))))
    {
        if (TraitType == EARPG_PersonalityTrait::Aggression)
        {
            return FMath::Lerp(0.5f, 1.5f, TraitStrength);
        }
        else if (TraitType == EARPG_PersonalityTrait::Caution)
        {
            return FMath::Lerp(1.5f, 0.5f, TraitStrength);
        }
    }
    
    return 1.0f; // No influence
}

float UARPG_AIPersonalityComponent::ValidateTraitStrength(float Strength) const
{
    return FMath::Clamp(Strength, 0.0f, 1.0f);
}

FGameplayTag UARPG_AIPersonalityComponent::GetGameplayTagForTrait(EARPG_PersonalityTrait TraitType) const
{
    switch (TraitType)
    {
        case EARPG_PersonalityTrait::Aggression:
            return FGameplayTag::RequestGameplayTag(TEXT("AI.Personality.Aggression"));
        case EARPG_PersonalityTrait::Curiosity:
            return FGameplayTag::RequestGameplayTag(TEXT("AI.Personality.Curiosity"));
        case EARPG_PersonalityTrait::Sociability:
            return FGameplayTag::RequestGameplayTag(TEXT("AI.Personality.Sociability"));
        case EARPG_PersonalityTrait::Bravery:
            return FGameplayTag::RequestGameplayTag(TEXT("AI.Personality.Bravery"));
        case EARPG_PersonalityTrait::Greed:
            return FGameplayTag::RequestGameplayTag(TEXT("AI.Personality.Greed"));
        case EARPG_PersonalityTrait::Loyalty:
            return FGameplayTag::RequestGameplayTag(TEXT("AI.Personality.Loyalty"));
        case EARPG_PersonalityTrait::Intelligence:
            return FGameplayTag::RequestGameplayTag(TEXT("AI.Personality.Intelligence"));
        case EARPG_PersonalityTrait::Caution:
            return FGameplayTag::RequestGameplayTag(TEXT("AI.Personality.Caution"));
        case EARPG_PersonalityTrait::Impulsiveness:
            return FGameplayTag::RequestGameplayTag(TEXT("AI.Personality.Impulsiveness"));
        default:
            return FGameplayTag::EmptyTag;
    }
}

FString UARPG_AIPersonalityComponent::GetTraitDisplayName(EARPG_PersonalityTrait TraitType) const
{
    switch (TraitType)
    {
        case EARPG_PersonalityTrait::Aggression: return TEXT("Aggressive");
        case EARPG_PersonalityTrait::Curiosity: return TEXT("Curious");
        case EARPG_PersonalityTrait::Sociability: return TEXT("Social");
        case EARPG_PersonalityTrait::Bravery: return TEXT("Brave");
        case EARPG_PersonalityTrait::Greed: return TEXT("Greedy");
        case EARPG_PersonalityTrait::Loyalty: return TEXT("Loyal");
        case EARPG_PersonalityTrait::Intelligence: return TEXT("Intelligent");
        case EARPG_PersonalityTrait::Caution: return TEXT("Cautious");
        case EARPG_PersonalityTrait::Impulsiveness: return TEXT("Impulsive");
        default: return TEXT("Unknown");
    }
}