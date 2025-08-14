// Source/RadiantRPG/Private/Characters/ARPG_BaseNPCCharacter.cpp

#include "Characters/ARPG_BaseNPCCharacter.h"
#include "AI/Core/ARPG_AIBrainComponent.h"
#include "AI/Core/ARPG_AIMemoryComponent.h"
#include "AI/Core/ARPG_AIPerceptionComponent.h"
#include "AI/Core/ARPG_AINeedsComponent.h"
#include "AI/Core/ARPG_AIPersonalityComponent.h"
#include "Types/ARPG_AITypes.h"
#include "GameplayTagsManager.h"
#include "Core/RadiantGameplayTags.h"

AARPG_BaseNPCCharacter::AARPG_BaseNPCCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.1f; // 10 FPS for NPCs

    // Set character type for BaseCharacter
    CharacterType = ECharacterType::NPC;

    // Replace the base NeedsComponent with AI-specific version
    if (NeedsComponent != nullptr)
    {
        NeedsComponent->DestroyComponent();
    }
    NeedsComponent = CreateDefaultSubobject<UARPG_AINeedsComponent>(TEXT("AINeedsComponent"));

    // Create other AI components
    BrainComponent = CreateDefaultSubobject<UARPG_AIBrainComponent>(TEXT("BrainComponent"));
    MemoryComponent = CreateDefaultSubobject<UARPG_AIMemoryComponent>(TEXT("MemoryComponent"));
    PerceptionComponent = CreateDefaultSubobject<UARPG_AIPerceptionComponent>(TEXT("PerceptionComponent"));
    PersonalityComponent = CreateDefaultSubobject<UARPG_AIPersonalityComponent>(TEXT("PersonalityComponent"));

    // Set default values
    NPCType = FGameplayTag::RequestGameplayTag(TEXT("NPC.Type.Generic"));
    Faction = FGameplayTag::RequestGameplayTag(TEXT("Faction.Neutral"));
    CurrentBehavior = FGameplayTag::RequestGameplayTag(TEXT("Behavior.Idle"));

    // Default configurations
    DefaultBrainConfig = FARPG_AIBrainConfiguration();
    DefaultNeedsConfig = FARPG_NeedsConfiguration();
}

void AARPG_BaseNPCCharacter::BeginPlay()
{
    Super::BeginPlay();

    InitializeAIComponents();
    SetupComponentReferences();

    // Bind to brain events
    if (BrainComponent)
    {
        BrainComponent->OnIntentChanged.AddDynamic(this, &AARPG_BaseNPCCharacter::OnIntentChanged);
    }

    BP_OnNPCInitialized();
}

void AARPG_BaseNPCCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clean up delegates
    if (BrainComponent)
    {
        BrainComponent->OnIntentChanged.RemoveAll(this);
    }

    Super::EndPlay(EndPlayReason);
}

void AARPG_BaseNPCCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Only tick if active
    if (!bIsActive)
    {
        return;
    }

    // Brain component handles its own ticking
    // This allows for custom NPC logic in derived classes
}

// === IARPG_AIBrainInterface Implementation ===

void AARPG_BaseNPCCharacter::InitializeBrain(const FARPG_AIBrainConfiguration& Config)
{
    if (BrainComponent)
    {
        BrainComponent->InitializeBrain(Config);
    }
}

void AARPG_BaseNPCCharacter::ProcessStimulus(const FARPG_AIStimulus& Stimulus)
{
    if (BrainComponent)
    {
        BrainComponent->ProcessStimulus(Stimulus);
    }
}

FARPG_AIIntent AARPG_BaseNPCCharacter::GenerateIntent()
{
    if (BrainComponent)
    {
        return BrainComponent->GenerateIntent();
    }
    return FARPG_AIIntent();
}

void AARPG_BaseNPCCharacter::UpdateBrain(float DeltaTime)
{
    if (BrainComponent)
    {
        BrainComponent->UpdateBrain(DeltaTime);
    }
}

bool AARPG_BaseNPCCharacter::ShouldActivateCuriosity() const
{
    if (BrainComponent)
    {
        return BrainComponent->ShouldActivateCuriosity();
    }
    return false;
}

FARPG_AIBrainState AARPG_BaseNPCCharacter::GetBrainState() const
{
    if (BrainComponent)
    {
        return BrainComponent->GetBrainState();
    }
    return FARPG_AIBrainState();
}

void AARPG_BaseNPCCharacter::SetBrainEnabled(bool bEnabled)
{
    if (BrainComponent)
    {
        BrainComponent->SetBrainEnabled(bEnabled);
    }
}

float AARPG_BaseNPCCharacter::GetIntentConfidence() const
{
    if (BrainComponent)
    {
        return BrainComponent->GetIntentConfidence();
    }
    return 0.0f;
}

FARPG_AIIntent AARPG_BaseNPCCharacter::GetCurrentIntent() const
{
    if (BrainComponent)
    {
        return BrainComponent->GetCurrentIntent();
    }
    
    return FARPG_AIIntent();
}

void AARPG_BaseNPCCharacter::ExecuteIntent(const FARPG_AIIntent& Intent)
{
    if (BrainComponent)
    {
        BrainComponent->ExecuteIntent(Intent);
    }
}

// === NPC Interface ===

void AARPG_BaseNPCCharacter::InitializeNPC(const FARPG_NPCConfiguration& Config)
{
    // Set NPC properties from config
    NPCType = Config.NPCType;
    Faction = Config.Faction;

    // Initialize AI systems with configuration
    if (BrainComponent)
    {
        BrainComponent->InitializeBrain(Config.BrainConfig);
    }

    if (NeedsComponent && Config.bUseCustomNeeds)
    {
        NeedsComponent->InitializeNeeds(Config.NeedsConfig, Config.InitialNeeds);
    }

    if (PersonalityComponent && Config.PersonalityTraits.Num() > 0)
    {
        for (const FARPG_AIPersonalityTrait& Trait : Config.PersonalityTraits)
        {
            PersonalityComponent->SetPersonalityTrait(Trait.TraitType, Trait.TraitStrength);
        }
    }
}

void AARPG_BaseNPCCharacter::SetFaction(FGameplayTag NewFaction)
{
    FGameplayTag OldFaction = Faction;
    Faction = NewFaction;

    // Notify about faction change
    OnBehaviorChanged.Broadcast(this, NewFaction);
    BP_OnFactionChanged(OldFaction, NewFaction);
}

bool AARPG_BaseNPCCharacter::IsHostileTo(AActor* Target) const
{
    return IsValidFactionTarget(Target, true);
}

bool AARPG_BaseNPCCharacter::IsFriendlyTo(AActor* Target) const
{
    return IsValidFactionTarget(Target, false);
}

// === Private Functions ===

void AARPG_BaseNPCCharacter::InitializeAIComponents()
{
    // Initialize brain with default config
    if (BrainComponent)
    {
        BrainComponent->InitializeBrain(DefaultBrainConfig);
    }

    // Initialize needs with default config
    if (NeedsComponent)
    {
        TArray<FARPG_AINeed> DefaultNeeds;
        // Create default needs
        for (int32 i = 0; i < static_cast<int32>(EARPG_NeedType::MAX); ++i)
        {
            FARPG_AINeed Need;
            Need.NeedType = static_cast<EARPG_NeedType>(i);
            Need.CurrentLevel = 0.5f; // Start at medium level
            Need.bIsActive = true;
            DefaultNeeds.Add(Need);
        }
        
        NeedsComponent->InitializeNeeds(DefaultNeedsConfig, DefaultNeeds);
    }

    // Initialize personality with default traits
    if (PersonalityComponent && DefaultPersonalityTraits.Num() > 0)
    {
        for (const FARPG_AIPersonalityTrait& Trait : DefaultPersonalityTraits)
        {
            PersonalityComponent->SetPersonalityTrait(Trait.TraitType, Trait.TraitStrength);
        }
    }
}

void AARPG_BaseNPCCharacter::SetupComponentReferences()
{
    // Components will find each other through the actor
    // This is handled in their respective BeginPlay methods
}

void AARPG_BaseNPCCharacter::OnIntentChanged(const FARPG_AIIntent& NewIntent)
{
    UpdateBehaviorFromIntent(NewIntent);
}

void AARPG_BaseNPCCharacter::UpdateBehaviorFromIntent(const FARPG_AIIntent& Intent)
{
    FGameplayTag OldBehavior = CurrentBehavior;
    
    // Map intent to behavior tag using the correct gameplay tag constants
    if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Survival_Flee))
    {
        CurrentBehavior = TAG_Behavior_Fleeing;
    }
    else if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Combat_Attack))
    {
        CurrentBehavior = TAG_Behavior_Combat;
    }
    else if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Social_Talk))
    {
        CurrentBehavior = TAG_Behavior_Socializing;
    }
    else if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Curiosity_Explore))
    {
        CurrentBehavior = TAG_Behavior_Moving; // or create TAG_Behavior_Exploring
    }
    else if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Wander))
    {
        CurrentBehavior = TAG_Behavior_Moving;
    }
    else if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Patrol))
    {
        CurrentBehavior = TAG_Behavior_Moving;
    }
    else if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Idle))
    {
        CurrentBehavior = TAG_Behavior_Idle;
    }
    else if (Intent.IntentTag.MatchesTag(TAG_AI_Intent_Guard))
    {
        CurrentBehavior = TAG_Behavior_Idle; // Standing guard
    }
    else
    {
        // Default fallback behavior
        CurrentBehavior = TAG_Behavior_Idle;
    }
    
    // Notify about behavior change if it actually changed
    if (OldBehavior != CurrentBehavior)
    {
        OnBehaviorChanged.Broadcast(this, CurrentBehavior);
        BP_OnBehaviorChanged(OldBehavior, CurrentBehavior);
    }
}

bool AARPG_BaseNPCCharacter::IsValidFactionTarget(AActor* Target, bool bCheckHostile) const
{
    if (!Target)
    {
        return false;
    }

    // Check if target is another NPC with faction
    if (AARPG_BaseNPCCharacter* TargetNPC = Cast<AARPG_BaseNPCCharacter>(Target))
    {
        FGameplayTag TargetFaction = TargetNPC->GetFaction();
        
        if (bCheckHostile)
        {
            // Check for hostile relationships
            // This could be expanded to use a faction relationship system
            return !Faction.MatchesTag(TargetFaction) && 
                   !Faction.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Faction.Neutral")));
        }
        else
        {
            // Check for friendly relationships
            return Faction.MatchesTag(TargetFaction) || 
                   Faction.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Faction.Neutral")));
        }
    }

    // Default behavior for non-NPC targets (like players)
    if (bCheckHostile)
    {
        // Hostile to players if not neutral faction
        return !Faction.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Faction.Neutral")));
    }
    else
    {
        // Friendly to players if neutral or friendly faction
        return Faction.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Faction.Neutral"))) ||
               Faction.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Faction.Friendly")));
    }
}