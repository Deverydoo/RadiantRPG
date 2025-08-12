// Private/AI/Interfaces/IARPG_FactionInterface.cpp

#include "AI/Interfaces/ARPG_FactionInterface.h"
#include "Core/RadiantGameState.h"
#include "Engine/World.h"
#include "Core/RadiantGameplayTags.h"

bool IARPG_FactionInterface::IsHostileToFaction_Implementation(const FGameplayTag& OtherFaction) const
{
    FGameplayTag MyFaction = Execute_GetFactionTag(Cast<UObject>(this));
    
    if (!MyFaction.IsValid() || !OtherFaction.IsValid())
    {
        return false;
    }

    // Same faction is never hostile (unless it's the hostile faction)
    if (MyFaction == OtherFaction)
    {
        return MyFaction.MatchesTagExact(TAG_Faction_Hostile);
    }

    // Check if either faction is inherently hostile
    if (MyFaction.MatchesTagExact(TAG_Faction_Hostile) || 
        OtherFaction.MatchesTagExact(TAG_Faction_Hostile))
    {
        return true;
    }

    // Bandits are hostile to most non-bandit factions
    if (MyFaction.MatchesTag(TAG_Faction_Bandits))
    {
        return !OtherFaction.MatchesTag(TAG_Faction_Bandits) && 
               !OtherFaction.MatchesTagExact(TAG_Faction_Neutral);
    }

    if (OtherFaction.MatchesTag(TAG_Faction_Bandits))
    {
        return !MyFaction.MatchesTag(TAG_Faction_Bandits) && 
               !MyFaction.MatchesTagExact(TAG_Faction_Neutral);
    }

    // Wildlife is generally neutral unless hostile
    if (MyFaction.MatchesTagExact(TAG_Faction_Wildlife) || 
        OtherFaction.MatchesTagExact(TAG_Faction_Wildlife))
    {
        return false;
    }

    // Check game state for faction relationships
    if (const UWorld* World = Cast<UObject>(this)->GetWorld())
    {
        if (const ARadiantGameState* GameState = World->GetGameState<ARadiantGameState>())
        {
            return GameState->AreFactionsAtWar(MyFaction, OtherFaction);
        }
    }

    // Default to non-hostile
    return false;
}

bool IARPG_FactionInterface::IsAlliedWithFaction_Implementation(const FGameplayTag& OtherFaction) const
{
    FGameplayTag MyFaction = Execute_GetFactionTag(Cast<UObject>(this));
    
    if (!MyFaction.IsValid() || !OtherFaction.IsValid())
    {
        return false;
    }

    // Same faction is allied (unless hostile)
    if (MyFaction == OtherFaction)
    {
        return !MyFaction.MatchesTagExact(TAG_Faction_Hostile);
    }

    // Check for hierarchical relationships (parent/child factions)
    if (MyFaction.MatchesTag(OtherFaction) || OtherFaction.MatchesTag(MyFaction))
    {
        return true;
    }

    // Kingdom subfactions are allied
    if (MyFaction.MatchesTag(TAG_Faction_Kingdom) && OtherFaction.MatchesTag(TAG_Faction_Kingdom))
    {
        return true;
    }

    // Merchant subfactions are allied
    if (MyFaction.MatchesTag(TAG_Faction_Merchants) && OtherFaction.MatchesTag(TAG_Faction_Merchants))
    {
        return true;
    }

    // Bandit subfactions are allied
    if (MyFaction.MatchesTag(TAG_Faction_Bandits) && OtherFaction.MatchesTag(TAG_Faction_Bandits))
    {
        return true;
    }

    // Villager subfactions are allied
    if (MyFaction.MatchesTag(TAG_Faction_Villagers) && OtherFaction.MatchesTag(TAG_Faction_Villagers))
    {
        return true;
    }

    // Check game state for specific alliances
    if (const UWorld* World = Cast<UObject>(this)->GetWorld())
    {
        if (const ARadiantGameState* GameState = World->GetGameState<ARadiantGameState>())
        {
            float Relationship = GameState->GetFactionRelationship(MyFaction, OtherFaction);
            return Relationship > 0.5f; // Consider allied if relationship > 0.5
        }
    }

    // Default to non-allied
    return false;
}

float IARPG_FactionInterface::GetFactionRelationship_Implementation(const FGameplayTag& OtherFaction) const
{
    FGameplayTag MyFaction = Execute_GetFactionTag(Cast<UObject>(this));
    
    if (!MyFaction.IsValid() || !OtherFaction.IsValid())
    {
        return 0.0f; // Neutral
    }

    // Same faction has perfect relationship (unless hostile)
    if (MyFaction == OtherFaction)
    {
        return MyFaction.MatchesTagExact(TAG_Faction_Hostile) ? -1.0f : 1.0f;
    }

    // Check if hostile
    if (Execute_IsHostileToFaction(Cast<UObject>(this), OtherFaction))
    {
        return -1.0f;
    }

    // Check if allied
    if (Execute_IsAlliedWithFaction(Cast<UObject>(this), OtherFaction))
    {
        return 1.0f;
    }

    // Check game state for specific relationship values
    if (const UWorld* World = Cast<UObject>(this)->GetWorld())
    {
        if (const ARadiantGameState* GameState = World->GetGameState<ARadiantGameState>())
        {
            return GameState->GetFactionRelationship(MyFaction, OtherFaction);
        }
    }

    // Default to neutral
    return 0.0f;
}

bool IARPG_FactionInterface::CanInteractPeacefully_Implementation(AActor* OtherActor) const
{
    if (!OtherActor)
    {
        return false;
    }

    // Check if the other actor also implements faction interface
    if (IARPG_FactionInterface* OtherFactionInterface = Cast<IARPG_FactionInterface>(OtherActor))
    {
        FGameplayTag OtherFaction = Execute_GetFactionTag(OtherActor);
        
        // Can interact peacefully if not hostile
        return !Execute_IsHostileToFaction(Cast<UObject>(this), OtherFaction);
    }

    // If other actor doesn't have faction, assume peaceful interaction possible
    return true;
}