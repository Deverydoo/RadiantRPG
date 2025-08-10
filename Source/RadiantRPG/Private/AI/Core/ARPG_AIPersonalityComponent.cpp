// Source/RadiantRPG/Private/AI/Core/ARPG_AIPersonalityComponent.cpp

#include "AI/Core/ARPG_AIPersonalityComponent.h"
#include "AI/Core/ARPG_AIBrainComponent.h"
#include "Engine/World.h"
#include "Types/ARPG_AIEventTypes.h"

UARPG_AIPersonalityComponent::UARPG_AIPersonalityComponent()
{
    PrimaryComponentTick.bCanEverTick = false; // Personality doesn't need constant updates
    
    // Initialize default configuration
    PersonalityConfig = FARPG_PersonalityConfiguration();
    
    // Set up default personality traits (balanced personality)
    DefaultTraits.Empty();
    
    FARPG_AIPersonalityTrait Aggression;
    Aggression.TraitType = EARPG_PersonalityTrait::Aggression;
    Aggression.TraitStrength = 0.3f; // Slightly passive by default
    Aggression.Flexibility = 0.2f; // Can change moderately
    DefaultTraits.Add(Aggression);
    
    FARPG_AIPersonalityTrait Caution;
    Caution.TraitType = EARPG_PersonalityTrait::Caution;
    Caution.TraitStrength = 0.6f; // Moderately cautious
    Caution.Flexibility = 0.15f;
    DefaultTraits.Add(Caution);
    
    FARPG_AIPersonalityTrait Curiosity;
    Curiosity.TraitType = EARPG_PersonalityTrait::Curiosity;
    Curiosity.TraitStrength = 0.7f; // Naturally curious
    Curiosity.Flexibility = 0.1f; // Core trait, harder to change
    DefaultTraits.Add(Curiosity);
    
    FARPG_AIPersonalityTrait Sociability;
    Sociability.TraitType = EARPG_PersonalityTrait::Sociability;
    Sociability.TraitStrength = 0.5f; // Balanced social needs
    Sociability.Flexibility = 0.25f;
    DefaultTraits.Add(Sociability);
    
    FARPG_AIPersonalityTrait Loyalty;
    Loyalty.TraitType = EARPG_PersonalityTrait::Loyalty;
    Loyalty.TraitStrength = 0.6f; // Moderately loyal
    Loyalty.Flexibility = 0.05f; // Very stable trait
    DefaultTraits.Add(Loyalty);
    
    FARPG_AIPersonalityTrait Greed;
    Greed.TraitType = EARPG_PersonalityTrait::Greed;
    Greed.TraitStrength = 0.2f; // Low greed by default
    Greed.Flexibility = 0.3f; // Can be influenced by experiences
    DefaultTraits.Add(Greed);
    
    FARPG_AIPersonalityTrait Courage;
    Courage.TraitType = EARPG_PersonalityTrait::Courage;
    Courage.TraitStrength = 0.4f; // Moderately brave
    Courage.Flexibility = 0.2f;
    DefaultTraits.Add(Courage);
    
    FARPG_AIPersonalityTrait Intelligence;
    Intelligence.TraitType = EARPG_PersonalityTrait::Intelligence;
    Intelligence.TraitStrength = 0.5f; // Average intelligence
    Intelligence.Flexibility = 0.05f; // Very stable trait
    DefaultTraits.Add(Intelligence);
    
    UE_LOG(LogTemp, Log, TEXT("AIPersonalityComponent: Component created with %d default traits"), DefaultTraits.Num());
}

void UARPG_AIPersonalityComponent::BeginPlay()
{
    Super::BeginPlay();
    
    InitializeComponentReferences();
    InitializeDefaultTraits();
    
    UE_LOG(LogTemp, Log, TEXT("AIPersonalityComponent: Initialized for %s with %d traits"), 
           GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"), CurrentTraits.Num());
}

void UARPG_AIPersonalityComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CurrentTraits.Empty();
    ExperienceHistory.Empty();
    Super::EndPlay(EndPlayReason);
}

void UARPG_AIPersonalityComponent::InitializePersonality(const FARPG_PersonalityConfiguration& Config, const TArray<FARPG_AIPersonalityTrait>& InitialTraits)
{
    PersonalityConfig = Config;
    
    // Clear current traits and set new ones
    CurrentTraits.Empty();
    for (const FARPG_AIPersonalityTrait& Trait : InitialTraits)
    {
        CurrentTraits.Add(Trait.TraitType, Trait);
    }
    
    if (PersonalityConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("Personality initialized with %d custom traits"), InitialTraits.Num());
    }
}

void UARPG_AIPersonalityComponent::UpdatePersonalityConfiguration(const FARPG_PersonalityConfiguration& NewConfig)
{
    PersonalityConfig = NewConfig;
    
    if (PersonalityConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("Personality configuration updated"));
    }
}

float UARPG_AIPersonalityComponent::GetTraitStrength(EARPG_PersonalityTrait TraitType) const
{
    if (const FARPG_AIPersonalityTrait* Trait = CurrentTraits.Find(TraitType))
    {
        return Trait->TraitStrength;
    }
    return 0.5f; // Default neutral strength
}

void UARPG_AIPersonalityComponent::SetTraitStrength(EARPG_PersonalityTrait TraitType, float NewStrength)
{
    float ValidatedStrength = ValidateTraitStrength(NewStrength);
    
    if (FARPG_AIPersonalityTrait* Trait = CurrentTraits.Find(TraitType))
    {
        float OldStrength = Trait->TraitStrength;
        Trait->TraitStrength = ValidatedStrength;
        
        OnPersonalityTraitChanged.Broadcast(TraitType, ValidatedStrength);
        
        if (PersonalityConfig.bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Log, TEXT("Trait %d changed from %.2f to %.2f"), 
                   (int32)TraitType, OldStrength, ValidatedStrength);
        }
    }
    else
    {
        // Trait doesn't exist, create it with default settings
        FARPG_AIPersonalityTrait NewTrait = GetDefaultTraitForType(TraitType);
        NewTrait.TraitStrength = ValidatedStrength;
        CurrentTraits.Add(TraitType, NewTrait);
        
        OnPersonalityTraitChanged.Broadcast(TraitType, ValidatedStrength);
    }
}

void UARPG_AIPersonalityComponent::ModifyTraitStrength(EARPG_PersonalityTrait TraitType, float DeltaAmount)
{
    float CurrentStrength = GetTraitStrength(TraitType);
    SetTraitStrength(TraitType, CurrentStrength + DeltaAmount);
}

TArray<FARPG_AIPersonalityTrait> UARPG_AIPersonalityComponent::GetAllTraits() const
{
    TArray<FARPG_AIPersonalityTrait> AllTraits;
    CurrentTraits.GenerateValueArray(AllTraits);
    return AllTraits;
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
    EARPG_PersonalityTrait DominantTrait = EARPG_PersonalityTrait::MAX;
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

TArray<EARPG_PersonalityTrait> UARPG_AIPersonalityComponent::GetStrongTraits(float Threshold) const
{
    TArray<EARPG_PersonalityTrait> StrongTraits;
    
    for (const auto& TraitPair : CurrentTraits)
    {
        if (TraitPair.Value.TraitStrength >= Threshold)
        {
            StrongTraits.Add(TraitPair.Key);
        }
    }
    
    return StrongTraits;
}

void UARPG_AIPersonalityComponent::AddOrUpdateTrait(const FARPG_AIPersonalityTrait& Trait)
{
    FARPG_AIPersonalityTrait ValidatedTrait = Trait;
    ValidatedTrait.TraitStrength = ValidateTraitStrength(Trait.TraitStrength);
    ValidatedTrait.Flexibility = FMath::Clamp(Trait.Flexibility, 0.0f, 1.0f);
    
    CurrentTraits.Add(Trait.TraitType, ValidatedTrait);
    OnPersonalityTraitChanged.Broadcast(Trait.TraitType, ValidatedTrait.TraitStrength);
    
    if (PersonalityConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("Trait %d added/updated to %.2f"), 
               (int32)Trait.TraitType, ValidatedTrait.TraitStrength);
    }
}

void UARPG_AIPersonalityComponent::RemoveTrait(EARPG_PersonalityTrait TraitType)
{
    if (CurrentTraits.Remove(TraitType) > 0)
    {
        if (PersonalityConfig.bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Log, TEXT("Trait %d removed"), (int32)TraitType);
        }
    }
}

void UARPG_AIPersonalityComponent::SetTraitFlexibility(EARPG_PersonalityTrait TraitType, float NewFlexibility)
{
    if (FARPG_AIPersonalityTrait* Trait = CurrentTraits.Find(TraitType))
    {
        Trait->Flexibility = FMath::Clamp(NewFlexibility, 0.0f, 1.0f);
        
        if (PersonalityConfig.bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Log, TEXT("Trait %d flexibility set to %.2f"), 
                   (int32)TraitType, Trait->Flexibility);
        }
    }
}

void UARPG_AIPersonalityComponent::ProcessExperience(FGameplayTag ExperienceTag, float Intensity, AActor* RelatedActor)
{
    if (!PersonalityConfig.bAllowPersonalityChange || !PersonalityConfig.bEnablePersonalityInfluence)
    {
        return;
    }
    
    // Update experience history
    if (ExperienceHistory.Contains(ExperienceTag))
    {
        ExperienceHistory[ExperienceTag] += Intensity;
    }
    else
    {
        ExperienceHistory.Add(ExperienceTag, Intensity);
    }
    
    // Apply experience to relevant traits
    for (auto& TraitPair : CurrentTraits)
    {
        ApplyExperienceToTrait(TraitPair.Key, ExperienceTag, Intensity);
    }
    
    BP_OnExperienceProcessed(ExperienceTag, Intensity);
    
    if (PersonalityConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("Processed experience: %s (%.2f)"), 
               *ExperienceTag.ToString(), Intensity);
    }
}

void UARPG_AIPersonalityComponent::ReactToEvent(const FARPG_AIEvent& AIEvent)
{
    // Convert AI event to experience
    ProcessExperience(AIEvent.EventType, AIEvent.EventStrength, AIEvent.EventInstigator.Get());
}

void UARPG_AIPersonalityComponent::ApplyPersonalityDevelopment()
{
    // This would implement long-term personality changes based on accumulated experiences
    // For now, just log that it was called
    if (PersonalityConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("Applying personality development based on %d experiences"), 
               ExperienceHistory.Num());
    }
}

void UARPG_AIPersonalityComponent::ModifyIntent(FARPG_AIIntent& Intent) const
{
    if (!PersonalityConfig.bEnablePersonalityInfluence)
    {
        return;
    }
    
    // Try Blueprint implementation first
    FARPG_AIIntent ModifiedIntent = BP_ModifyIntentByPersonality(Intent);
    if (ModifiedIntent.IntentTag.IsValid())
    {
        Intent = ModifiedIntent;
        return;
    }
    
    // Apply personality-based modifications
    float PersonalityModifier = GetActionWeight(Intent.IntentTag);
    
    // Modify confidence based on personality alignment
    Intent.Confidence *= PersonalityModifier;
    
    // Aggressive personalities prefer combat intents
    if (Intent.IntentTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Combat"))))
    {
        float AggressionBoost = GetTraitStrength(EARPG_PersonalityTrait::Aggression);
        Intent.Confidence *= (1.0f + AggressionBoost * 0.5f);
    }
    
    // Cautious personalities prefer defensive intents
    if (Intent.IntentTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Defend"))) ||
        Intent.IntentTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Flee"))))
    {
        float CautionBoost = GetTraitStrength(EARPG_PersonalityTrait::Caution);
        Intent.Confidence *= (1.0f + CautionBoost * 0.3f);
    }
    
    // Curious personalities prefer exploration
    if (Intent.IntentTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Explore"))))
    {
        float CuriosityBoost = GetTraitStrength(EARPG_PersonalityTrait::Curiosity);
        Intent.Confidence *= (1.0f + CuriosityBoost * 0.4f);
    }
    
    // Social personalities prefer social interactions
    if (Intent.IntentTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Social"))))
    {
        float SocialBoost = GetTraitStrength(EARPG_PersonalityTrait::Sociability);
        Intent.Confidence *= (1.0f + SocialBoost * 0.3f);
    }
    
    // Clamp confidence to valid range
    Intent.Confidence = FMath::Clamp(Intent.Confidence, 0.0f, 1.0f);
}

float UARPG_AIPersonalityComponent::GetActionWeight(FGameplayTag ActionTag) const
{
    if (!PersonalityConfig.bEnablePersonalityInfluence)
    {
        return 1.0f;
    }
    
    // Try Blueprint implementation first
    float CustomWeight = BP_GetCustomActionWeight(ActionTag);
    if (CustomWeight > 0.0f)
    {
        return CustomWeight;
    }
    
    float BaseWeight = 1.0f;
    
    // Calculate weight based on personality traits
    for (const auto& TraitPair : CurrentTraits)
    {
        float TraitInfluence = GetTraitInfluenceOnAction(TraitPair.Key, ActionTag);
        BaseWeight *= (1.0f + TraitInfluence * TraitPair.Value.TraitStrength);
    }
    
    return FMath::Clamp(BaseWeight, 0.1f, 2.0f); // Allow 10x reduction to 2x increase
}

bool UARPG_AIPersonalityComponent::SupportsAction(FGameplayTag ActionTag) const
{
    return GetActionWeight(ActionTag) > 0.5f; // Support if weight is above neutral
}

TArray<FGameplayTag> UARPG_AIPersonalityComponent::GetPersonalityBasedIntents() const
{
    TArray<FGameplayTag> IntentSuggestions;
    
    if (!PersonalityConfig.bEnablePersonalityInfluence)
    {
        return IntentSuggestions;
    }
    
    // Suggest intents based on strong personality traits
    TArray<EARPG_PersonalityTrait> StrongTraits = GetStrongTraits(0.7f);
    
    for (EARPG_PersonalityTrait Trait : StrongTraits)
    {
        switch (Trait)
        {
            case EARPG_PersonalityTrait::Aggression:
                IntentSuggestions.Add(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Combat")));
                IntentSuggestions.Add(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Intimidate")));
                break;
            case EARPG_PersonalityTrait::Caution:
                IntentSuggestions.Add(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Observe")));
                IntentSuggestions.Add(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Hide")));
                break;
            case EARPG_PersonalityTrait::Curiosity:
                IntentSuggestions.Add(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Explore")));
                IntentSuggestions.Add(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Investigate")));
                break;
            case EARPG_PersonalityTrait::Sociability:
                IntentSuggestions.Add(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Socialize")));
                IntentSuggestions.Add(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Follow")));
                break;
            case EARPG_PersonalityTrait::Greed:
                IntentSuggestions.Add(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Loot")));
                IntentSuggestions.Add(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Trade")));
                break;
            case EARPG_PersonalityTrait::Courage:
                IntentSuggestions.Add(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Charge")));
                IntentSuggestions.Add(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Protect")));
                break;
            default:
                break;
        }
    }
    
    return IntentSuggestions;
}

float UARPG_AIPersonalityComponent::GetRelationshipModifier(const UARPG_AIPersonalityComponent* OtherPersonality) const
{
    if (!IsValid(OtherPersonality) || !PersonalityConfig.bEnablePersonalityInfluence)
    {
        return 1.0f; // Neutral relationship
    }
    
    float Compatibility = GetPersonalityCompatibility(OtherPersonality);
    
    // Convert compatibility to relationship modifier
    // 0.0 compatibility = 0.5x relationship strength
    // 1.0 compatibility = 1.5x relationship strength
    return 0.5f + Compatibility;
}

float UARPG_AIPersonalityComponent::GetPersonalityCompatibility(const UARPG_AIPersonalityComponent* OtherPersonality) const
{
    if (!IsValid(OtherPersonality))
    {
        return 0.5f; // Neutral compatibility
    }
    
    float TotalCompatibility = 0.0f;
    int32 TraitCount = 0;
    
    // Compare each trait for compatibility
    for (const auto& TraitPair : CurrentTraits)
    {
        EARPG_PersonalityTrait TraitType = TraitPair.Key;
        float MyStrength = TraitPair.Value.TraitStrength;
        float OtherStrength = OtherPersonality->GetTraitStrength(TraitType);
        
        float TraitCompatibility = 1.0f;
        
        // Some traits are compatible when similar, others when different
        switch (TraitType)
        {
            case EARPG_PersonalityTrait::Aggression:
                // Aggressive and non-aggressive personalities clash
                TraitCompatibility = 1.0f - FMath::Abs(MyStrength - OtherStrength);
                break;
            case EARPG_PersonalityTrait::Sociability:
                // Social personalities get along well
                TraitCompatibility = 1.0f - FMath::Abs(MyStrength - OtherStrength) * 0.5f;
                break;
            case EARPG_PersonalityTrait::Loyalty:
                // Loyal personalities trust each other
                TraitCompatibility = (MyStrength + OtherStrength) * 0.5f;
                break;
            case EARPG_PersonalityTrait::Intelligence:
                // Similar intelligence levels get along better
                TraitCompatibility = 1.0f - FMath::Abs(MyStrength - OtherStrength) * 0.3f;
                break;
            default:
                // Default: similar personalities are more compatible
                TraitCompatibility = 1.0f - FMath::Abs(MyStrength - OtherStrength) * 0.7f;
                break;
        }
        
        TotalCompatibility += TraitCompatibility;
        TraitCount++;
    }
    
    return TraitCount > 0 ? FMath::Clamp(TotalCompatibility / TraitCount, 0.0f, 1.0f) : 0.5f;
}

FString UARPG_AIPersonalityComponent::GetPersonalityDescription() const
{
    TArray<FString> TraitDescriptions;
    
    for (const auto& TraitPair : CurrentTraits)
    {
        if (TraitPair.Value.TraitStrength >= 0.7f)
        {
            FString TraitName;
            switch (TraitPair.Key)
            {
                case EARPG_PersonalityTrait::Aggression: TraitName = TEXT("Aggressive"); break;
                case EARPG_PersonalityTrait::Caution: TraitName = TEXT("Cautious"); break;
                case EARPG_PersonalityTrait::Curiosity: TraitName = TEXT("Curious"); break;
                case EARPG_PersonalityTrait::Sociability: TraitName = TEXT("Social"); break;
                case EARPG_PersonalityTrait::Loyalty: TraitName = TEXT("Loyal"); break;
                case EARPG_PersonalityTrait::Greed: TraitName = TEXT("Greedy"); break;
                case EARPG_PersonalityTrait::Courage: TraitName = TEXT("Brave"); break;
                case EARPG_PersonalityTrait::Intelligence: TraitName = TEXT("Intelligent"); break;
                default: TraitName = TEXT("Unknown"); break;
            }
            TraitDescriptions.Add(TraitName);
        }
    }
    
    if (TraitDescriptions.Num() == 0)
    {
        return TEXT("Balanced personality");
    }
    else if (TraitDescriptions.Num() == 1)
    {
        return TraitDescriptions[0];
    }
    else
    {
        FString Result = TraitDescriptions[0];
        for (int32 i = 1; i < TraitDescriptions.Num() - 1; i++)
        {
            Result += TEXT(", ") + TraitDescriptions[i];
        }
        Result += TEXT(" and ") + TraitDescriptions.Last();
        return Result;
    }
}

void UARPG_AIPersonalityComponent::ResetPersonality()
{
    CurrentTraits.Empty();
    ExperienceHistory.Empty();
    InitializeDefaultTraits();
    
    if (PersonalityConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("Personality reset to defaults"));
    }
}

void UARPG_AIPersonalityComponent::GenerateRandomPersonality()
{
    CurrentTraits.Empty();
    
    // Generate random traits
    for (int32 i = 0; i < (int32)EARPG_PersonalityTrait::MAX; i++)
    {
        EARPG_PersonalityTrait TraitType = (EARPG_PersonalityTrait)i;
        FARPG_AIPersonalityTrait NewTrait = GetDefaultTraitForType(TraitType);
        NewTrait.TraitStrength = FMath::RandRange(0.1f, 0.9f);
        NewTrait.Flexibility = FMath::RandRange(0.05f, 0.3f);
        
        CurrentTraits.Add(TraitType, NewTrait);
    }
    
    if (PersonalityConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("Generated random personality: %s"), *GetPersonalityDescription());
    }
}

void UARPG_AIPersonalityComponent::InitializeComponentReferences()
{
    if (AActor* Owner = GetOwner())
    {
        BrainComponent = Owner->FindComponentByClass<UARPG_AIBrainComponent>();
        
        if (!BrainComponent.IsValid() && PersonalityConfig.bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Warning, TEXT("AIPersonalityComponent: No brain component found on %s"), *Owner->GetName());
        }
    }
}

void UARPG_AIPersonalityComponent::InitializeDefaultTraits()
{
    CurrentTraits.Empty();
    
    for (const FARPG_AIPersonalityTrait& DefaultTrait : DefaultTraits)
    {
        CurrentTraits.Add(DefaultTrait.TraitType, DefaultTrait);
    }
    
    if (PersonalityConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("Initialized %d default personality traits"), CurrentTraits.Num());
    }
}

FARPG_AIPersonalityTrait UARPG_AIPersonalityComponent::GetDefaultTraitForType(EARPG_PersonalityTrait TraitType) const
{
    FARPG_AIPersonalityTrait DefaultTrait;
    DefaultTrait.TraitType = TraitType;
    DefaultTrait.TraitStrength = 0.5f; // Neutral by default
    DefaultTrait.Flexibility = 0.1f; // Low flexibility by default
    
    return DefaultTrait;
}

void UARPG_AIPersonalityComponent::ApplyExperienceToTrait(EARPG_PersonalityTrait TraitType, FGameplayTag ExperienceTag, float Intensity)
{
    FARPG_AIPersonalityTrait* Trait = CurrentTraits.Find(TraitType);
    if (!Trait)
    {
        return;
    }
    
    float ChangeAmount = 0.0f;
    
    // Determine how the experience affects this trait
    if (ExperienceTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Experience.Combat"))))
    {
        if (TraitType == EARPG_PersonalityTrait::Aggression)
        {
            ChangeAmount = Intensity * 0.05f; // Combat increases aggression
        }
        else if (TraitType == EARPG_PersonalityTrait::Caution)
        {
            ChangeAmount = Intensity * 0.03f; // Combat increases caution
        }
        else if (TraitType == EARPG_PersonalityTrait::Courage)
        {
            ChangeAmount = Intensity * 0.02f; // Combat builds courage
        }
    }
    else if (ExperienceTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Experience.Social"))))
    {
        if (TraitType == EARPG_PersonalityTrait::Sociability)
        {
            ChangeAmount = Intensity * 0.04f; // Social experiences increase sociability
        }
    }
    else if (ExperienceTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Experience.Discovery"))))
    {
        if (TraitType == EARPG_PersonalityTrait::Curiosity)
        {
            ChangeAmount = Intensity * 0.03f; // Discovery increases curiosity
        }
    }
    
    // Apply change if significant and trait is flexible enough
    if (FMath::Abs(ChangeAmount) > 0.001f)
    {
        float EffectiveChange = ChangeAmount * Trait->Flexibility * PersonalityConfig.PersonalityChangeRate;
        float OldStrength = Trait->TraitStrength;
        Trait->TraitStrength = ValidateTraitStrength(Trait->TraitStrength + EffectiveChange);
        
        if (!FMath::IsNearlyEqual(OldStrength, Trait->TraitStrength, 0.01f))
        {
            OnPersonalityTraitChanged.Broadcast(TraitType, Trait->TraitStrength);
            
            if (PersonalityConfig.bEnableDebugLogging)
            {
                UE_LOG(LogTemp, VeryVerbose, TEXT("Experience %s changed trait %d by %.3f"), 
                       *ExperienceTag.ToString(), (int32)TraitType, EffectiveChange);
            }
        }
    }
}

float UARPG_AIPersonalityComponent::GetTraitInfluenceOnAction(EARPG_PersonalityTrait TraitType, FGameplayTag ActionTag) const
{
    // Return influence multiplier (-1.0 to 1.0)
    // Positive values increase action weight, negative values decrease it
    
    if (ActionTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Combat"))))
    {
        if (TraitType == EARPG_PersonalityTrait::Aggression)
            return 0.5f; // Aggressive personalities favor combat
        if (TraitType == EARPG_PersonalityTrait::Caution)
            return -0.3f; // Cautious personalities avoid combat
        if (TraitType == EARPG_PersonalityTrait::Courage)
            return 0.2f; // Brave personalities are more willing to fight
    }
    else if (ActionTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Flee"))))
    {
        if (TraitType == EARPG_PersonalityTrait::Caution)
            return 0.4f; // Cautious personalities flee more readily
        if (TraitType == EARPG_PersonalityTrait::Courage)
            return -0.5f; // Brave personalities resist fleeing
    }
    else if (ActionTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Explore"))))
    {
        if (TraitType == EARPG_PersonalityTrait::Curiosity)
            return 0.6f; // Curious personalities love exploring
        if (TraitType == EARPG_PersonalityTrait::Caution)
            return -0.2f; // Cautious personalities are hesitant to explore
    }
    else if (ActionTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("AI.Intent.Trade"))))
    {
        if (TraitType == EARPG_PersonalityTrait::Greed)
            return 0.4f; // Greedy personalities favor trading
        if (TraitType == EARPG_PersonalityTrait::Sociability)
            return 0.2f; // Social personalities enjoy trading interactions
    }
    
    return 0.0f; // No influence by default
}

float UARPG_AIPersonalityComponent::ValidateTraitStrength(float Strength) const
{
    return FMath::Clamp(Strength, 0.0f, 1.0f);
}

TMap<FGameplayTag, float> UARPG_AIPersonalityComponent::GetPersonalityTraits() const
{
    TMap<FGameplayTag, float> TraitsMap;
    
    for (const auto& TraitPair : CurrentTraits)
    {
        FGameplayTag TraitTag = GetGameplayTagForTrait(TraitPair.Key);
        if (TraitTag.IsValid())
        {
            TraitsMap.Add(TraitTag, TraitPair.Value.TraitStrength);
        }
    }
    
    return TraitsMap;
}

TMap<EARPG_PersonalityTrait, float> UARPG_AIPersonalityComponent::GetPersonalityTraitsAsEnumMap() const
{
    TMap<EARPG_PersonalityTrait, float> TraitsMap;
    
    for (const auto& TraitPair : CurrentTraits)
    {
        TraitsMap.Add(TraitPair.Key, TraitPair.Value.TraitStrength);
    }
    
    return TraitsMap;
}

FGameplayTag UARPG_AIPersonalityComponent::GetGameplayTagForTrait(EARPG_PersonalityTrait TraitType) const
{
    switch (TraitType)
    {
    case EARPG_PersonalityTrait::Aggression:
        return FGameplayTag::RequestGameplayTag(TEXT("Personality.Aggression"));
    case EARPG_PersonalityTrait::Caution:
        return FGameplayTag::RequestGameplayTag(TEXT("Personality.Caution"));
    case EARPG_PersonalityTrait::Curiosity:
        return FGameplayTag::RequestGameplayTag(TEXT("Personality.Curiosity"));
    case EARPG_PersonalityTrait::Sociability:
        return FGameplayTag::RequestGameplayTag(TEXT("Personality.Sociability"));
    case EARPG_PersonalityTrait::Loyalty:
        return FGameplayTag::RequestGameplayTag(TEXT("Personality.Loyalty"));
    case EARPG_PersonalityTrait::Greed:
        return FGameplayTag::RequestGameplayTag(TEXT("Personality.Greed"));
    case EARPG_PersonalityTrait::Courage:
        return FGameplayTag::RequestGameplayTag(TEXT("Personality.Courage"));
    case EARPG_PersonalityTrait::Intelligence:
        return FGameplayTag::RequestGameplayTag(TEXT("Personality.Intelligence"));
    default:
        return FGameplayTag::EmptyTag;
    }
}

void UARPG_AIPersonalityComponent::AddExperience(FGameplayTag ExperienceTag, float Intensity)
{
    ProcessExperience(ExperienceTag, Intensity, nullptr);
}

TMap<FGameplayTag, float> UARPG_AIPersonalityComponent::GetExperienceHistory() const
{
    return ExperienceHistory;
}

FString UARPG_AIPersonalityComponent::GetPersonalitySummary() const
{
    if (CurrentTraits.Num() == 0)
    {
        return TEXT("No personality traits defined");
    }
    
    FString Summary;
    TArray<EARPG_PersonalityTrait> StrongTraits = GetStrongTraits(0.6f);
    
    if (StrongTraits.Num() > 0)
    {
        Summary += TEXT("Strong traits: ");
        for (int32 i = 0; i < StrongTraits.Num(); i++)
        {
            FString TraitName = GetTraitDisplayName(StrongTraits[i]);
            Summary += TraitName;
            if (i < StrongTraits.Num() - 1)
            {
                Summary += TEXT(", ");
            }
        }
    }
    else
    {
        Summary = TEXT("Balanced personality with no dominant traits");
    }
    
    return Summary;
}

bool UARPG_AIPersonalityComponent::FavorsActionType(FGameplayTag ActionTag, float Threshold) const
{
    return GetActionWeight(ActionTag) >= Threshold;
}

bool UARPG_AIPersonalityComponent::OpposesActionType(FGameplayTag ActionTag, float Threshold) const
{
    return GetActionWeight(ActionTag) <= (1.0f - Threshold);
}

FARPG_AIIntent UARPG_AIPersonalityComponent::ModifyIntentByPersonality(const FARPG_AIIntent& BaseIntent) const
{
    FARPG_AIIntent ModifiedIntent = BaseIntent;
    ModifyIntent(ModifiedIntent);
    return ModifiedIntent;
}

float UARPG_AIPersonalityComponent::GetCustomActionWeight(FGameplayTag ActionTag) const
{
    return GetActionWeight(ActionTag);
}

// Helper function to get trait display name
FString UARPG_AIPersonalityComponent::GetTraitDisplayName(EARPG_PersonalityTrait TraitType) const
{
    switch (TraitType)
    {
        case EARPG_PersonalityTrait::Aggression: return TEXT("Aggressive");
        case EARPG_PersonalityTrait::Caution: return TEXT("Cautious");
        case EARPG_PersonalityTrait::Curiosity: return TEXT("Curious");
        case EARPG_PersonalityTrait::Sociability: return TEXT("Social");
        case EARPG_PersonalityTrait::Courage: return TEXT("Brave");
        case EARPG_PersonalityTrait::Greed: return TEXT("Greedy");
        case EARPG_PersonalityTrait::Loyalty: return TEXT("Loyal");
        case EARPG_PersonalityTrait::Intelligence: return TEXT("Intelligent");
        case EARPG_PersonalityTrait::Creativity: return TEXT("Creative");
        case EARPG_PersonalityTrait::Patience: return TEXT("Patient");
        default: return TEXT("Unknown");
    }
}

void UARPG_AIPersonalityComponent::SetPersonalityTrait(EARPG_PersonalityTrait TraitType, float NewStrength)
{
    SetTraitStrength(TraitType, NewStrength);
}