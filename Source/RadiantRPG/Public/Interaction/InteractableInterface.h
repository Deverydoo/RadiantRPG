// Public/Interaction/InteractableInterface.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

class ABaseCharacter;

USTRUCT(BlueprintType)
struct FInteractionData
{
    GENERATED_BODY()

    // Display name for UI prompts
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FText InteractionPrompt;

    // Optional description
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FText InteractionDescription;

    // Interaction range override (0 = use default)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0.0"))
    float InteractionRange;

    // Whether this interaction requires line of sight
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    bool bRequiresLineOfSight;

    // Whether this interaction can be used multiple times
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    bool bIsReusable;

    FInteractionData()
    {
        InteractionPrompt = FText::FromString("Interact");
        InteractionDescription = FText::GetEmpty();
        InteractionRange = 0.0f;
        bRequiresLineOfSight = true;
        bIsReusable = true;
    }
};

UINTERFACE(MinimalAPI, Blueprintable)
class UInteractableInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * Interface for objects that can be interacted with by any character (player or NPC)
 * Uses ABaseCharacter* to avoid circular dependencies
 */
class RADIANTRPG_API IInteractableInterface
{
    GENERATED_BODY()

public:
    
    /**
     * Called when a character attempts to interact with this object
     * @param InteractingCharacter The character attempting the interaction
     * @param InteractionPoint The world location where the interaction trace hit
     * @param InteractionNormal The surface normal at the interaction point
     * @return true if the interaction was successful
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    bool OnInteract(ABaseCharacter* InteractingCharacter, const FVector& InteractionPoint, const FVector& InteractionNormal);
    virtual bool OnInteract_Implementation(ABaseCharacter* InteractingCharacter, const FVector& InteractionPoint, const FVector& InteractionNormal) { return false; }

    /**
     * Called when a character starts looking at this interactable (for highlighting/UI)
     * @param InteractingCharacter The character looking at this object
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void OnInteractionFocusGained(ABaseCharacter* InteractingCharacter);
    virtual void OnInteractionFocusGained_Implementation(ABaseCharacter* InteractingCharacter) {}

    /**
     * Called when a character stops looking at this interactable
     * @param InteractingCharacter The character that was looking at this object
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void OnInteractionFocusLost(ABaseCharacter* InteractingCharacter);
    virtual void OnInteractionFocusLost_Implementation(ABaseCharacter* InteractingCharacter) {}

    /**
     * Get the data describing this interaction
     * @return Interaction data for UI and behavior
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    FInteractionData GetInteractionData() const;
    virtual FInteractionData GetInteractionData_Implementation() const { return FInteractionData(); }

    /**
     * Check if this object can currently be interacted with
     * @param InteractingCharacter The character attempting the interaction
     * @return true if interaction is currently possible
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    bool CanInteract(ABaseCharacter* InteractingCharacter) const;
    virtual bool CanInteract_Implementation(ABaseCharacter* InteractingCharacter) const { return true; }
};