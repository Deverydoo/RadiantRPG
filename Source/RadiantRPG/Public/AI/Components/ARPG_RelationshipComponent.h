// Public/AI/Components/ARPG_RelationshipComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTags.h"
#include "AI/Interfaces/ARPG_FactionInterface.h"
#include "ARPG_RelationshipComponent.generated.h"

/**
 * Simple relationship storage entry
 */
USTRUCT(BlueprintType)
struct RADIANTRPG_API FARPG_RelationshipEntry
{
    GENERATED_BODY()

    /** The actor this relationship is with */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TWeakObjectPtr<AActor> TargetActor;

    /** Relationship value (-1.0 = hostile, 0.0 = neutral, 1.0 = friendly) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RelationshipValue = 0.0f;

    /** Last time relationship was updated */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float LastUpdateTime = 0.0f;

    FARPG_RelationshipEntry()
    {
    }

    FARPG_RelationshipEntry(AActor* Actor, float Value)
        : TargetActor(Actor), RelationshipValue(Value)
    {
        LastUpdateTime = 0.0f;
    }
};

/**
 * Component for managing individual actor relationships
 * Provides relationship values for AI memory and decision making
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RADIANTRPG_API UARPG_RelationshipComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UARPG_RelationshipComponent();

protected:
    virtual void BeginPlay() override;

    /** Individual actor relationships */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relationships")
    TArray<FARPG_RelationshipEntry> ActorRelationships;

    /** Default relationship for unknown actors based on faction */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relationships", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float DefaultNeutralRelationship = 0.0f;
    FTimerHandle CleanupTimer;

public:
    /**
     * Get relationship value with a specific actor
     * @param TargetActor The actor to check relationship with
     * @return Relationship value (-1.0 to 1.0)
     */
    UFUNCTION(BlueprintPure, Category = "Relationships")
    float GetRelationshipValue(AActor* TargetActor) const;

    /**
     * Set relationship value with a specific actor
     * @param TargetActor The actor to set relationship with
     * @param Value New relationship value (-1.0 to 1.0)
     */
    UFUNCTION(BlueprintCallable, Category = "Relationships")
    void SetRelationshipValue(AActor* TargetActor, float Value);

    /**
     * Modify existing relationship by a delta amount
     * @param TargetActor The actor to modify relationship with
     * @param Delta Amount to change relationship by
     */
    UFUNCTION(BlueprintCallable, Category = "Relationships")
    void ModifyRelationshipValue(AActor* TargetActor, float Delta);

private:
    /** Find relationship entry for a specific actor */
    FARPG_RelationshipEntry* FindRelationshipEntry(AActor* TargetActor);
    const FARPG_RelationshipEntry* FindRelationshipEntry(AActor* TargetActor) const;

    /** Get faction-based relationship if no individual relationship exists */
    float GetFactionBasedRelationship(AActor* TargetActor) const;

    /** Clean up invalid weak pointers */
    void CleanupInvalidEntries();
};