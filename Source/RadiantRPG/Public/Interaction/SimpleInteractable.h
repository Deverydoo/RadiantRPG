// Public/Interaction/SimpleInteractable.h - Updated with crash safety functions

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "SimpleInteractable.generated.h"

UENUM(BlueprintType)
enum class EItemRarity : uint8
{
    Common UMETA(DisplayName = "Common"),
    Uncommon UMETA(DisplayName = "Uncommon"), 
    Rare UMETA(DisplayName = "Rare"),
    Epic UMETA(DisplayName = "Epic"),
    Legendary UMETA(DisplayName = "Legendary"),
    Unique UMETA(DisplayName = "Unique")
};

USTRUCT(BlueprintType)
struct FRarityColorData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity")
    EItemRarity Rarity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity")
    FLinearColor Color;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity")
    FString ActorTag; // For testing purposes

    FRarityColorData()
    {
        Rarity = EItemRarity::Common;
        Color = FLinearColor::White;
        ActorTag = TEXT("");
    }

    FRarityColorData(EItemRarity InRarity, FLinearColor InColor, const FString& InTag)
    {
        Rarity = InRarity;
        Color = InColor;
        ActorTag = InTag;
    }
};

/**
 * A simple interactable object that can be used as a base for doors, chests, levers, etc.
 * Works with any ABaseCharacter (player or NPC)
 * Supports visual highlighting through overlay materials with rarity-based colors
 * UE 5.6 Compatible with crash-safe collision and mesh handling
 */
UCLASS(Blueprintable)
class RADIANTRPG_API ASimpleInteractable : public AActor, public IInteractableInterface
{
    GENERATED_BODY()
    
public:    
    ASimpleInteractable();

protected:
    virtual void BeginPlay() override;

    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    // Interaction settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FInteractionData InteractionData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    bool bIsCurrentlyInteractable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    int32 MaxUseCount;

    UPROPERTY(BlueprintReadOnly, Category = "Interaction")
    int32 CurrentUseCount;

    // Visual feedback settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    UMaterialInterface* HighlightMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    bool bUseHighlightMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual", meta = (ClampMin = "0.0", ClampMax = "10000.0"))
    float HighlightMaxDrawDistance;

    UPROPERTY(BlueprintReadOnly, Category = "Visual")
    bool bIsHighlighted;

    // Rarity and Color System
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity")
    EItemRarity ItemRarity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity")
    bool bUseRarityColors;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity")
    FLinearColor DefaultHighlightColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity")
    TArray<FRarityColorData> RarityColors;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity")
    FName ColorParameterName;

    // Internal state for material management
    UPROPERTY()
    TArray<UMaterialInterface*> OriginalMaterials;

    UPROPERTY()
    UMaterialInstanceDynamic* DynamicHighlightMaterial; // Reference to shared material

    UPROPERTY()
    bool bHasStoredOriginalMaterials;

public:
    // IInteractableInterface implementation
    virtual bool OnInteract_Implementation(class ABaseCharacter* InteractingCharacter, const FVector& InteractionPoint, const FVector& InteractionNormal) override;
    virtual void OnInteractionFocusGained_Implementation(class ABaseCharacter* InteractingCharacter) override;
    virtual void OnInteractionFocusLost_Implementation(class ABaseCharacter* InteractingCharacter) override;
    virtual FInteractionData GetInteractionData_Implementation() const override;
    virtual bool CanInteract_Implementation(class ABaseCharacter* InteractingCharacter) const override;

    UFUNCTION(BlueprintCallable, Category = "Rarity")
    void SetItemRarity(EItemRarity NewRarity);

    UFUNCTION(BlueprintCallable, Category = "Rarity")
    EItemRarity GetItemRarity() const { return ItemRarity; }

    UFUNCTION(BlueprintCallable, Category = "Rarity")
    FLinearColor GetRarityColor() const;

    UFUNCTION(BlueprintCallable, Category = "Rarity")
    EItemRarity DetermineRarityFromTags() const;

    // NEW: Add this function to manually refresh rarity colors
    UFUNCTION(BlueprintCallable, Category = "Rarity", CallInEditor)
    void RefreshRarityColors();

    // NEW: Add this function to populate default rarity colors
    UFUNCTION(BlueprintCallable, Category = "Rarity", CallInEditor)
    void PopulateDefaultRarityColors();

    // NEW: Add this function to test rarity highlighting
    UFUNCTION(BlueprintCallable, Category = "Rarity", CallInEditor)
    void TestAllRarityColors();

    // Blueprint events
    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
    void OnInteractionSuccessful(class ABaseCharacter* InteractingCharacter);

    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
    void OnInteractionFailed(class ABaseCharacter* InteractingCharacter);

    UFUNCTION(BlueprintImplementableEvent, Category = "Visual")
    void OnHighlightStateChanged(bool bNewHighlighted);

    // Public functions
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void SetInteractable(bool bNewInteractable);

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void ResetUseCount();

    UFUNCTION(BlueprintCallable, Category = "Visual")
    void SetHighlight(bool bShouldHighlight);

    UFUNCTION(BlueprintCallable, Category = "Visual")
    void SetHighlightMaterial(UMaterialInterface* NewHighlightMaterial);

    UFUNCTION(BlueprintCallable, Category = "Visual")
    bool IsHighlighted() const { return bIsHighlighted; }

protected:
    // Internal highlight functions
    void ApplyHighlightMaterial();
    void RemoveHighlightMaterial();
    void StoreOriginalMaterials();
    void RestoreOriginalMaterials();
    void UpdateHighlightColor();
    void InitializeRarityColors();
    void CreateDynamicHighlightMaterial();
    
    // CRASH-SAFE collision setup functions
    void SetupCollisionSafely();
    void SetupCollisionDeferred();
    bool TrySetCollisionProfile();
    void ApplyManualCollisionSetup();
    void SetCollisionResponseSafely(ECollisionChannel Channel, ECollisionResponse Response);
    void ApplyMinimalCollision();
    
    // Shared material management
    UMaterialInstanceDynamic* GetSharedDynamicMaterial();
};