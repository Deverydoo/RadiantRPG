// Private/AI/Components/ARPG_RelationshipComponent.cpp

#include "AI/Components/ARPG_RelationshipComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "AI/Interfaces/ARPG_FactionInterface.h"

UARPG_RelationshipComponent::UARPG_RelationshipComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bWantsInitializeComponent = true;
}

void UARPG_RelationshipComponent::BeginPlay()
{
    Super::BeginPlay();

    // Schedule periodic cleanup of invalid entries
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            CleanupTimer, 
            this, 
            &UARPG_RelationshipComponent::CleanupInvalidEntries, 
            30.0f, // Every 30 seconds
            true   // Repeat
        );
    }
}

float UARPG_RelationshipComponent::GetRelationshipValue(AActor* TargetActor) const
{
    if (!TargetActor || TargetActor == GetOwner())
    {
        return DefaultNeutralRelationship;
    }

    // Check for individual relationship first
    if (const FARPG_RelationshipEntry* Entry = FindRelationshipEntry(TargetActor))
    {
        return Entry->RelationshipValue;
    }

    // Fall back to faction-based relationship
    return GetFactionBasedRelationship(TargetActor);
}

void UARPG_RelationshipComponent::SetRelationshipValue(AActor* TargetActor, float Value)
{
    if (!TargetActor || TargetActor == GetOwner())
    {
        return;
    }

    // Clamp value to valid range
    Value = FMath::Clamp(Value, -1.0f, 1.0f);

    // Find existing entry or create new one
    if (FARPG_RelationshipEntry* Entry = FindRelationshipEntry(TargetActor))
    {
        Entry->RelationshipValue = Value;
        Entry->LastUpdateTime = GetWorld()->GetTimeSeconds();
    }
    else
    {
        ActorRelationships.Add(FARPG_RelationshipEntry(TargetActor, Value));
        ActorRelationships.Last().LastUpdateTime = GetWorld()->GetTimeSeconds();
    }
}

void UARPG_RelationshipComponent::ModifyRelationshipValue(AActor* TargetActor, float Delta)
{
    if (!TargetActor || TargetActor == GetOwner())
    {
        return;
    }

    float CurrentValue = GetRelationshipValue(TargetActor);
    float NewValue = FMath::Clamp(CurrentValue + Delta, -1.0f, 1.0f);
    
    SetRelationshipValue(TargetActor, NewValue);
}

FARPG_RelationshipEntry* UARPG_RelationshipComponent::FindRelationshipEntry(AActor* TargetActor)
{
    if (!TargetActor)
    {
        return nullptr;
    }

    for (FARPG_RelationshipEntry& Entry : ActorRelationships)
    {
        if (Entry.TargetActor.IsValid() && Entry.TargetActor.Get() == TargetActor)
        {
            return &Entry;
        }
    }

    return nullptr;
}

const FARPG_RelationshipEntry* UARPG_RelationshipComponent::FindRelationshipEntry(AActor* TargetActor) const
{
    if (!TargetActor)
    {
        return nullptr;
    }

    for (const FARPG_RelationshipEntry& Entry : ActorRelationships)
    {
        if (Entry.TargetActor.IsValid() && Entry.TargetActor.Get() == TargetActor)
        {
            return &Entry;
        }
    }

    return nullptr;
}

float UARPG_RelationshipComponent::GetFactionBasedRelationship(AActor* TargetActor) const
{
    // Get my faction
    IARPG_FactionInterface* MyFactionInterface = Cast<IARPG_FactionInterface>(GetOwner());
    if (!MyFactionInterface)
    {
        return DefaultNeutralRelationship;
    }

    // Get target's faction
    IARPG_FactionInterface* TargetFactionInterface = Cast<IARPG_FactionInterface>(TargetActor);
    if (!TargetFactionInterface)
    {
        return DefaultNeutralRelationship;
    }

    // Use faction interface to determine relationship
    FGameplayTag MyFaction = IARPG_FactionInterface::Execute_GetFactionTag(GetOwner());
    FGameplayTag TargetFaction = IARPG_FactionInterface::Execute_GetFactionTag(TargetActor);

    if (!MyFaction.IsValid() || !TargetFaction.IsValid())
    {
        return DefaultNeutralRelationship;
    }

    // Check faction relationships
    if (IARPG_FactionInterface::Execute_IsHostileToFaction(GetOwner(), TargetFaction))
    {
        return -0.8f; // Hostile but not maximum hatred
    }
    
    if (IARPG_FactionInterface::Execute_IsAlliedWithFaction(GetOwner(), TargetFaction))
    {
        return 0.6f; // Friendly but not maximum love
    }

    // Same faction
    if (MyFaction == TargetFaction)
    {
        return 0.4f; // Mildly positive toward faction members
    }

    return DefaultNeutralRelationship;
}

void UARPG_RelationshipComponent::CleanupInvalidEntries()
{
    // Remove entries with invalid weak pointers
    ActorRelationships.RemoveAll([](const FARPG_RelationshipEntry& Entry)
    {
        return !Entry.TargetActor.IsValid();
    });
}// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Components/ARPG_RelationshipComponent.h"
