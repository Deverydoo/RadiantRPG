// Public/AI/Interfaces/IARPG_FactionInterface.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTags.h"
#include "Types/FactionTypes.h"
#include "ARPG_FactionInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UARPG_FactionInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * Interface for actors that belong to factions
 * Provides faction identification and relationship data for AI systems
 */
class RADIANTRPG_API IARPG_FactionInterface
{
    GENERATED_BODY()

public:
    /**
     * Get the faction tag this actor belongs to
     * @return Faction gameplay tag
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Faction")
    FGameplayTag GetFactionTag() const;
    virtual FGameplayTag GetFactionTag_Implementation() const = 0;

    /**
     * Set the faction tag for this actor
     * @param NewFactionTag The new faction to belong to
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Faction")
    void SetFactionTag(const FGameplayTag& NewFactionTag);
    virtual void SetFactionTag_Implementation(const FGameplayTag& NewFactionTag) {}

    /**
     * Get faction role/rank within the faction
     * @return The role this actor has in their faction
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Faction")
    EFactionRole GetFactionRole() const;
    virtual EFactionRole GetFactionRole_Implementation() const { return EFactionRole::Civilian; }

    /**
     * Check if this actor is hostile to another faction
     * @param OtherFaction The faction to check hostility against
     * @return True if hostile to the other faction
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Faction")
    bool IsHostileToFaction(const FGameplayTag& OtherFaction) const;
    virtual bool IsHostileToFaction_Implementation(const FGameplayTag& OtherFaction) const;

    /**
     * Check if this actor is allied with another faction
     * @param OtherFaction The faction to check alliance with
     * @return True if allied with the other faction
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Faction")
    bool IsAlliedWithFaction(const FGameplayTag& OtherFaction) const;
    virtual bool IsAlliedWithFaction_Implementation(const FGameplayTag& OtherFaction) const;

    /**
     * Get the relationship stance towards another faction
     * @param OtherFaction The faction to check relationship with
     * @return Relationship stance value (-1.0 = hostile, 0.0 = neutral, 1.0 = allied)
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Faction")
    float GetFactionRelationship(const FGameplayTag& OtherFaction) const;
    virtual float GetFactionRelationship_Implementation(const FGameplayTag& OtherFaction) const;

    /**
     * Check if this actor can interact peacefully with another faction member
     * @param OtherActor The actor to check peaceful interaction with
     * @return True if peaceful interaction is possible
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Faction")
    bool CanInteractPeacefully(AActor* OtherActor) const;
    virtual bool CanInteractPeacefully_Implementation(AActor* OtherActor) const;

    // === C++ CONVENIENCE METHODS ===

    /**
     * C++ convenience method - get faction tag directly
     */
    virtual FGameplayTag GetActorFactionTag() const
    {
        return Execute_GetFactionTag(Cast<UObject>(this));
    }

    /**
     * C++ convenience method - check hostility directly
     */
    virtual bool IsActorHostileToFaction(const FGameplayTag& OtherFaction) const
    {
        return Execute_IsHostileToFaction(Cast<UObject>(this), OtherFaction);
    }
};