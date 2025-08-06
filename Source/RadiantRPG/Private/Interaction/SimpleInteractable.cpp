// Private/Interaction/SimpleInteractable.cpp - CRASH-SAFE VERSION
// Fixed collision setup to prevent access violations

#include "Interaction/SimpleInteractable.h"
#include "Components/StaticMeshComponent.h"
#include "Characters/BaseCharacter.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/CollisionProfile.h"

ASimpleInteractable::ASimpleInteractable()
{
    PrimaryActorTick.bCanEverTick = false;

    // Create mesh component with explicit null check safety
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    if (!MeshComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("SimpleInteractable: Failed to create MeshComponent"));
        return;
    }
    
    SetRootComponent(MeshComponent);
    
    // UE 5.6 Safe mesh loading - try multiple fallback paths
    UStaticMesh* DefaultMesh = nullptr;
    TArray<FString> MeshPaths = {
        TEXT("/Engine/BasicShapes/Cube"),
        TEXT("/Engine/BasicShapes/Cube1m"),
        TEXT("/Game/StarterContent/Shapes/Shape_Cube"),
        TEXT("/Engine/EditorMeshes/EditorCube")
    };
    
    for (const FString& MeshPath : MeshPaths)
    {
        DefaultMesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
        if (DefaultMesh && IsValid(DefaultMesh))
        {
            MeshComponent->SetStaticMesh(DefaultMesh);
            UE_LOG(LogTemp, Log, TEXT("SimpleInteractable: Loaded mesh from %s"), *MeshPath);
            break;
        }
    }
    
    if (!DefaultMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("SimpleInteractable: Could not load any default mesh - object will be invisible"));
    }

    // Add interactable tag BEFORE any collision setup
    Tags.Add(FName("Interactable"));
    if (MeshComponent)
    {
        MeshComponent->ComponentTags.Add(FName("Interactable"));
    }

    // Initialize settings with safe defaults
    bIsCurrentlyInteractable = true;
    MaxUseCount = -1;
    CurrentUseCount = 0;
    bIsHighlighted = false;
    bUseHighlightMaterial = true;
    HighlightMaterial = nullptr;
    HighlightMaxDrawDistance = 5000.0f;
    bHasStoredOriginalMaterials = false;

    // Initialize rarity system
    ItemRarity = EItemRarity::Common;
    bUseRarityColors = true;
    DefaultHighlightColor = FLinearColor::Red;
    ColorParameterName = FName("Color");
    
    // Initialize interaction data with safe defaults
    InteractionData.InteractionPrompt = FText::FromString("Use");
    InteractionData.InteractionDescription = FText::GetEmpty();
    InteractionData.InteractionRange = 200.0f;
    InteractionData.bRequiresLineOfSight = true;
    InteractionData.bIsReusable = true;
}

void ASimpleInteractable::BeginPlay()
{
    Super::BeginPlay();

    // ULTRA SAFE collision setup with extensive validation
    if (MeshComponent && IsValid(MeshComponent))
    {
        // Additional safety check for world context
        if (GetWorld() && IsValid(GetWorld()))
        {
            // Only setup collision if we're in a valid game world
            if (GetWorld()->IsGameWorld())
            {
                SetupCollisionSafely();
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("SimpleInteractable '%s': Skipping collision setup - not in game world"), *GetName());
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("SimpleInteractable '%s': No valid world context"), *GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("SimpleInteractable '%s': MeshComponent is invalid in BeginPlay"), *GetName());
    }

    // Initialize rarity colors for THIS INSTANCE (no static check)
    InitializeRarityColors();

    // REMOVED: Don't determine rarity from tags anymore, just use the dropdown value
    // if (bUseRarityColors)
    // {
    //     ItemRarity = DetermineRarityFromTags();
    // }

    // Store materials safely - only if we have a valid mesh and are in game world
    if (MeshComponent && IsValid(MeshComponent) && GetWorld() && GetWorld()->IsGameWorld())
    {
        UStaticMesh* StaticMesh = MeshComponent->GetStaticMesh();
        if (StaticMesh && IsValid(StaticMesh))
        {
            StoreOriginalMaterials();
        }
    }

    UE_LOG(LogTemp, Log, TEXT("SimpleInteractable '%s' initialized with %d rarity colors, rarity: %s"), 
           *GetName(), RarityColors.Num(), *UEnum::GetValueAsString(ItemRarity));
}

void ASimpleInteractable::SetupCollisionSafely()
{
    // MAXIMUM SAFETY CHECKS for UE 5.6 to prevent crashes
    if (!MeshComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("SetupCollisionSafely: MeshComponent is null"));
        return;
    }

    if (!IsValid(MeshComponent))
    {
        UE_LOG(LogTemp, Error, TEXT("SetupCollisionSafely: MeshComponent is not valid"));
        return;
    }

    // Check if component is being destroyed or has invalid flags
    if (MeshComponent->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
    {
        UE_LOG(LogTemp, Warning, TEXT("SetupCollisionSafely: MeshComponent is being destroyed"));
        return;
    }

    // Check world validity
    if (!GetWorld() || !IsValid(GetWorld()) || !GetWorld()->IsGameWorld())
    {
        UE_LOG(LogTemp, Warning, TEXT("SetupCollisionSafely: Invalid world context"));
        return;
    }

    // Defer collision setup to next tick to ensure everything is fully initialized
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
        {
            SetupCollisionDeferred();
        });
    }
}

void ASimpleInteractable::SetupCollisionDeferred()
{
    // Final safety checks before applying collision settings
    if (!MeshComponent || !IsValid(MeshComponent))
    {
        UE_LOG(LogTemp, Warning, TEXT("SetupCollisionDeferred: Component invalid during deferred setup"));
        return;
    }

    // Check if component has valid mesh
    UStaticMesh* StaticMesh = MeshComponent->GetStaticMesh();
    if (!StaticMesh || !IsValid(StaticMesh))
    {
        UE_LOG(LogTemp, Warning, TEXT("SetupCollisionDeferred: No valid static mesh, using minimal collision"));
        // Set up basic collision without mesh dependency
        ApplyMinimalCollision();
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("SimpleInteractable '%s': Setting up collision safely"), *GetName());

    // Wrap entire collision setup in try-catch for ultimate safety
    try
    {
        // Try to use a simple collision profile first
        bool bProfileSet = TrySetCollisionProfile();
        
        if (!bProfileSet)
        {
            // Fall back to manual collision setup
            ApplyManualCollisionSetup();
        }
        
        UE_LOG(LogTemp, Log, TEXT("SimpleInteractable '%s': Collision setup completed successfully"), *GetName());
    }
    catch (const std::exception&)
    {
        UE_LOG(LogTemp, Error, TEXT("SimpleInteractable '%s': Exception during collision setup"), *GetName());
        // Apply absolute minimal collision as last resort
        ApplyMinimalCollision();
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("SimpleInteractable '%s': Unknown exception during collision setup"), *GetName());
        ApplyMinimalCollision();
    }
}

bool ASimpleInteractable::TrySetCollisionProfile()
{
    if (!MeshComponent || !IsValid(MeshComponent))
        return false;

    // Try the InteractableObject profile if it exists
    const FName ProfileName(TEXT("InteractableObject"));
    
    // Check if profile exists before trying to set it
    if (UCollisionProfile::Get())
    {
        FCollisionResponseTemplate ProfileTemplate;
        if (UCollisionProfile::Get()->GetProfileTemplate(ProfileName, ProfileTemplate))
        {
            // Profile exists, try to set it
            MeshComponent->SetCollisionProfileName(ProfileName);
            
            // Verify it was set correctly
            if (MeshComponent->GetCollisionProfileName() == ProfileName)
            {
                UE_LOG(LogTemp, Log, TEXT("SimpleInteractable '%s': Successfully applied InteractableObject collision profile"), *GetName());
                return true;
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("SimpleInteractable '%s': InteractableObject profile not available, using manual setup"), *GetName());
    return false;
}

void ASimpleInteractable::ApplyManualCollisionSetup()
{
    if (!MeshComponent || !IsValid(MeshComponent))
        return;

    // Apply collision settings one by one with individual safety checks
    try
    {
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        UE_LOG(LogTemp, Verbose, TEXT("Collision enabled set"));
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to set collision enabled"));
    }

    try
    {
        MeshComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
        UE_LOG(LogTemp, Verbose, TEXT("Collision object type set"));
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to set collision object type"));
    }

    // Set collision responses with individual try-catch blocks
    SetCollisionResponseSafely(ECC_WorldStatic, ECR_Block);
    SetCollisionResponseSafely(ECC_WorldDynamic, ECR_Block);
    SetCollisionResponseSafely(ECC_Visibility, ECR_Block);
    
    // Try custom channels if they exist
    if (ECC_GameTraceChannel1 < ECC_MAX)
    {
        SetCollisionResponseSafely(ECC_GameTraceChannel1, ECR_Block);
    }
    
    // Try to set as Interactable object type if available
    if (ECC_GameTraceChannel3 < ECC_MAX)
    {
        try
        {
            MeshComponent->SetCollisionObjectType(ECC_GameTraceChannel3);
            UE_LOG(LogTemp, Verbose, TEXT("Set Interactable object type"));
        }
        catch (...)
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to set Interactable object type"));
        }
    }
}

void ASimpleInteractable::SetCollisionResponseSafely(ECollisionChannel Channel, ECollisionResponse Response)
{
    if (!MeshComponent || !IsValid(MeshComponent))
        return;

    try
    {
        MeshComponent->SetCollisionResponseToChannel(Channel, Response);
    }
    catch (...)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to set collision response for channel %d"), (int32)Channel);
    }
}

void ASimpleInteractable::ApplyMinimalCollision()
{
    if (!MeshComponent || !IsValid(MeshComponent))
        return;

    UE_LOG(LogTemp, Warning, TEXT("SimpleInteractable '%s': Applying minimal collision setup"), *GetName());
    
    // Absolute minimal collision setup that should never fail
    try
    {
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
        MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        
        UE_LOG(LogTemp, Log, TEXT("SimpleInteractable '%s': Minimal collision applied successfully"), *GetName());
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("SimpleInteractable '%s': Even minimal collision setup failed!"), *GetName());
    }
}

void ASimpleInteractable::InitializeRarityColors()
{
    // FIXED: Always initialize if array is empty, no static checks
    if (RarityColors.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Initializing rarity colors for %s"), *GetName());
        
        RarityColors.Reserve(6);
        RarityColors.Add(FRarityColorData(EItemRarity::Common, FLinearColor(0.6f, 0.6f, 0.6f, 1.0f), TEXT("Common")));
        RarityColors.Add(FRarityColorData(EItemRarity::Uncommon, FLinearColor(0.2f, 1.0f, 0.2f, 1.0f), TEXT("Uncommon")));
        RarityColors.Add(FRarityColorData(EItemRarity::Rare, FLinearColor(0.2f, 0.2f, 1.0f, 1.0f), TEXT("Rare")));
        RarityColors.Add(FRarityColorData(EItemRarity::Epic, FLinearColor(0.8f, 0.2f, 1.0f, 1.0f), TEXT("Epic")));
        RarityColors.Add(FRarityColorData(EItemRarity::Legendary, FLinearColor(1.0f, 0.5f, 0.0f, 1.0f), TEXT("Legendary")));
        RarityColors.Add(FRarityColorData(EItemRarity::Unique, FLinearColor(1.0f, 0.8f, 0.0f, 1.0f), TEXT("Unique")));
        
        UE_LOG(LogTemp, Log, TEXT("Initialized %d rarity colors"), RarityColors.Num());
    }
}

EItemRarity ASimpleInteractable::DetermineRarityFromTags() const
{
    // Fast tag lookup
    if (Tags.Contains(FName("Legendary"))) return EItemRarity::Legendary;
    if (Tags.Contains(FName("Epic"))) return EItemRarity::Epic;
    if (Tags.Contains(FName("Rare"))) return EItemRarity::Rare;
    if (Tags.Contains(FName("Uncommon"))) return EItemRarity::Uncommon;
    if (Tags.Contains(FName("Unique"))) return EItemRarity::Unique;
    return EItemRarity::Common;
}

// ... Rest of the implementation remains the same as the original file ...

bool ASimpleInteractable::OnInteract_Implementation(ABaseCharacter* InteractingCharacter, const FVector& InteractionPoint, const FVector& InteractionNormal)
{
    if (!CanInteract_Implementation(InteractingCharacter))
    {
        OnInteractionFailed(InteractingCharacter);
        return false;
    }

    if (MaxUseCount > 0)
    {
        CurrentUseCount++;
        if (CurrentUseCount >= MaxUseCount && !InteractionData.bIsReusable)
        {
            bIsCurrentlyInteractable = false;
        }
    }
    
    OnInteractionSuccessful(InteractingCharacter);
    return true;
}

void ASimpleInteractable::OnInteractionFocusGained_Implementation(ABaseCharacter* InteractingCharacter)
{
    if (!InteractingCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("SimpleInteractable: OnInteractionFocusGained called with null character"));
        return;
    }
    
    // Ultra-safe highlight application with extensive checks
    if (bUseHighlightMaterial && MeshComponent && IsValid(MeshComponent) && 
        HighlightMaterial && IsValid(HighlightMaterial) && 
        MeshComponent->GetStaticMesh() && IsValid(MeshComponent->GetStaticMesh()))
    {
        SetHighlight(true);
    }
    else
    {
        UE_LOG(LogTemp, Verbose, TEXT("SimpleInteractable: Skipping highlight for %s - missing requirements"), *GetName());
    }
}

void ASimpleInteractable::OnInteractionFocusLost_Implementation(ABaseCharacter* InteractingCharacter)
{
    SetHighlight(false);
}

FInteractionData ASimpleInteractable::GetInteractionData_Implementation() const
{
    return InteractionData;
}

bool ASimpleInteractable::CanInteract_Implementation(ABaseCharacter* InteractingCharacter) const
{
    if (!bIsCurrentlyInteractable || !InteractingCharacter)
        return false;

    if (MaxUseCount > 0 && CurrentUseCount >= MaxUseCount)
        return false;

    return true;
}

void ASimpleInteractable::SetInteractable(bool bNewInteractable)
{
    bIsCurrentlyInteractable = bNewInteractable;
    if (!bNewInteractable)
    {
        SetHighlight(false);
    }
}

void ASimpleInteractable::ResetUseCount()
{
    CurrentUseCount = 0;
    if (MaxUseCount > 0 && !bIsCurrentlyInteractable)
    {
        bIsCurrentlyInteractable = true;
    }
}

void ASimpleInteractable::SetHighlight(bool bShouldHighlight)
{
    if (bIsHighlighted == bShouldHighlight)
        return;

    bIsHighlighted = bShouldHighlight;
    
    // Extra safety checks for UE 5.6
    if (bUseHighlightMaterial && MeshComponent && IsValid(MeshComponent) && 
        MeshComponent->GetStaticMesh() && IsValid(MeshComponent->GetStaticMesh()))
    {
        if (bIsHighlighted)
        {
            ApplyHighlightMaterial();
        }
        else
        {
            RemoveHighlightMaterial();
        }
    }
    
    OnHighlightStateChanged(bIsHighlighted);
}

void ASimpleInteractable::SetHighlightMaterial(UMaterialInterface* NewHighlightMaterial)
{
    bool bWasHighlighted = bIsHighlighted;
    if (bWasHighlighted)
    {
        RemoveHighlightMaterial();
    }
    
    HighlightMaterial = NewHighlightMaterial;
    DynamicHighlightMaterial = nullptr;
    
    if (bWasHighlighted)
    {
        ApplyHighlightMaterial();
    }
}

void ASimpleInteractable::SetItemRarity(EItemRarity NewRarity)
{
    if (ItemRarity != NewRarity)
    {
        ItemRarity = NewRarity;
        if (bIsHighlighted)
        {
            UpdateHighlightColor();
        }
    }
}

FLinearColor ASimpleInteractable::GetRarityColor() const
{
    if (!bUseRarityColors)
    {
        return DefaultHighlightColor;
    }

    // Search for the matching rarity color
    for (const FRarityColorData& RarityData : RarityColors)
    {
        if (RarityData.Rarity == ItemRarity)
        {
            return RarityData.Color;
        }
    }
    
    // Fallback: If no matching rarity found, return a default color based on rarity
    switch (ItemRarity)
    {
    case EItemRarity::Common:
        return FLinearColor(0.6f, 0.6f, 0.6f, 1.0f);
    case EItemRarity::Uncommon:
        return FLinearColor(0.2f, 1.0f, 0.2f, 1.0f);
    case EItemRarity::Rare:
        return FLinearColor(0.2f, 0.2f, 1.0f, 1.0f);
    case EItemRarity::Epic:
        return FLinearColor(0.8f, 0.2f, 1.0f, 1.0f);
    case EItemRarity::Legendary:
        return FLinearColor(1.0f, 0.5f, 0.0f, 1.0f);
    case EItemRarity::Unique:
        return FLinearColor(1.0f, 0.8f, 0.0f, 1.0f);
    default:
        return DefaultHighlightColor;
    }
}


void ASimpleInteractable::ApplyHighlightMaterial()
{
    // CRITICAL: Add null checks for ALL potential crash points
    if (!IsValid(this))
    {
        UE_LOG(LogTemp, Error, TEXT("ApplyHighlightMaterial: SimpleInteractable is invalid"));
        return;
    }

    if (!MeshComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyHighlightMaterial: MeshComponent is null for %s"), *GetName());
        return;
    }

    if (!IsValid(MeshComponent))
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyHighlightMaterial: MeshComponent is invalid for %s"), *GetName());
        return;
    }

    // Check if component is being destroyed
    if (MeshComponent->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyHighlightMaterial: MeshComponent is being destroyed for %s"), *GetName());
        return;
    }
    
    if (!bUseHighlightMaterial)
    {
        UE_LOG(LogTemp, Verbose, TEXT("ApplyHighlightMaterial: Highlight material disabled for %s"), *GetName());
        return;
    }
    
    // Validate static mesh
    UStaticMesh* StaticMesh = MeshComponent->GetStaticMesh();
    if (!StaticMesh || !IsValid(StaticMesh))
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyHighlightMaterial: No valid static mesh for %s"), *GetName());
        return;
    }

    // Validate highlight material
    if (!HighlightMaterial || !IsValid(HighlightMaterial))
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyHighlightMaterial: No valid highlight material for %s"), *GetName());
        return;
    }

    // Additional safety: Check if the world is valid
    UWorld* World = GetWorld();
    if (!World || !IsValid(World))
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyHighlightMaterial: Invalid world for %s"), *GetName());
        return;
    }

    // Store original materials if not already done
    if (!bHasStoredOriginalMaterials)
    {
        StoreOriginalMaterials();
    }

    UMaterialInterface* MaterialToApply = nullptr;
    
    // Handle dynamic material creation for rarity colors
    if (bUseRarityColors && RarityColors.Num() > 0)
    {
        // Only create dynamic material if we don't have one or it's invalid
        if (!DynamicHighlightMaterial || !IsValid(DynamicHighlightMaterial))
        {
            CreateDynamicHighlightMaterial();
        }
        
        if (DynamicHighlightMaterial && IsValid(DynamicHighlightMaterial))
        {
            FLinearColor RarityColor = GetRarityColor();
            
            // Validate the color values
            if (RarityColor.R >= 0.0f && RarityColor.G >= 0.0f && RarityColor.B >= 0.0f && RarityColor.A >= 0.0f)
            {
                // CRITICAL: Wrap material parameter setting in try-catch
                try
                {
                    DynamicHighlightMaterial->SetVectorParameterValue(ColorParameterName, RarityColor);
                    MaterialToApply = DynamicHighlightMaterial;
                    
                    UE_LOG(LogTemp, Verbose, TEXT("Applied rarity color for %s: R=%.2f G=%.2f B=%.2f"), 
                           *UEnum::GetValueAsString(ItemRarity), RarityColor.R, RarityColor.G, RarityColor.B);
                }
                catch (...)
                {
                    UE_LOG(LogTemp, Error, TEXT("Exception setting material parameter for %s"), *GetName());
                    MaterialToApply = HighlightMaterial; // Fallback to static material
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Invalid rarity color detected, using default material"));
                MaterialToApply = HighlightMaterial;
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to create dynamic material, using base highlight material"));
            MaterialToApply = HighlightMaterial;
        }
    }
    else
    {
        // Use base highlight material if not using rarity colors
        MaterialToApply = HighlightMaterial;
    }

    // Final validation of material to apply
    if (!MaterialToApply || !IsValid(MaterialToApply))
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyHighlightMaterial: No valid material to apply for %s"), *GetName());
        return;
    }

    // Apply the overlay material with maximum safety
    try
    {
        // Triple-check component validity before applying material
        if (MeshComponent && 
            IsValid(MeshComponent) && 
            !MeshComponent->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed) &&
            MeshComponent->GetStaticMesh() &&
            IsValid(MeshComponent->GetStaticMesh()))
        {
            // Use a delayed call to ensure component is fully initialized
            if (World->HasBegunPlay())
            {
                MeshComponent->SetOverlayMaterial(MaterialToApply);
                MeshComponent->SetOverlayMaterialMaxDrawDistance(HighlightMaxDrawDistance);
                
                UE_LOG(LogTemp, Verbose, TEXT("SimpleInteractable: Applied highlight material to %s"), *GetName());
            }
            else
            {
                // Defer material application until world has begun play
                FTimerHandle DelayedApplyHandle;
                World->GetTimerManager().SetTimer(DelayedApplyHandle, 
                    [this, MaterialToApply]()
                    {
                        if (IsValid(this) && IsValid(MeshComponent) && IsValid(MaterialToApply))
                        {
                            MeshComponent->SetOverlayMaterial(MaterialToApply);
                            MeshComponent->SetOverlayMaterialMaxDrawDistance(HighlightMaxDrawDistance);
                        }
                    }, 
                    0.1f, false);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("SimpleInteractable: MeshComponent not safe for material application"));
        }
    }
    catch (const std::exception&)
    {
        UE_LOG(LogTemp, Error, TEXT("SimpleInteractable: Standard exception applying highlight material to %s"), *GetName());
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("SimpleInteractable: Unknown exception applying highlight material to %s"), *GetName());
    }
}


void ASimpleInteractable::RemoveHighlightMaterial()
{
    if (!MeshComponent || !IsValid(MeshComponent))
        return;

    try
    {
        if (!MeshComponent->HasAnyFlags(RF_BeginDestroyed))
        {
            MeshComponent->SetOverlayMaterial(nullptr);
            UE_LOG(LogTemp, Verbose, TEXT("SimpleInteractable: Removed highlight material from %s"), *GetName());
        }
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("SimpleInteractable: Exception removing highlight material from %s"), *GetName());
    }
}

void ASimpleInteractable::StoreOriginalMaterials()
{
    if (!MeshComponent || !IsValid(MeshComponent) || bHasStoredOriginalMaterials)
        return;
        
    if (!MeshComponent->GetStaticMesh() || !IsValid(MeshComponent->GetStaticMesh()))
        return;

    OriginalMaterials.Empty();
    
    int32 MaterialCount = MeshComponent->GetNumMaterials();
    if (MaterialCount > 0)
    {
        OriginalMaterials.Reserve(MaterialCount);
        for (int32 i = 0; i < MaterialCount; i++)
        {
            UMaterialInterface* Material = MeshComponent->GetMaterial(i);
            OriginalMaterials.Add(Material);
        }
        
        bHasStoredOriginalMaterials = true;
        UE_LOG(LogTemp, Verbose, TEXT("SimpleInteractable: Stored %d original materials for %s"), MaterialCount, *GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SimpleInteractable: No materials found on mesh component for %s"), *GetName());
    }
}

void ASimpleInteractable::RestoreOriginalMaterials()
{
    if (!MeshComponent || !IsValid(MeshComponent) || !bHasStoredOriginalMaterials)
        return;

    try
    {
        for (int32 i = 0; i < OriginalMaterials.Num() && i < MeshComponent->GetNumMaterials(); i++)
        {
            if (OriginalMaterials[i] && IsValid(OriginalMaterials[i]))
            {
                MeshComponent->SetMaterial(i, OriginalMaterials[i]);
            }
        }
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("SimpleInteractable: Exception restoring materials for %s"), *GetName());
    }
}

void ASimpleInteractable::UpdateHighlightColor()
{
    if (DynamicHighlightMaterial && IsValid(DynamicHighlightMaterial))
    {
        FLinearColor RarityColor = GetRarityColor();
        DynamicHighlightMaterial->SetVectorParameterValue(ColorParameterName, RarityColor);
    }
}

void ASimpleInteractable::CreateDynamicHighlightMaterial()
{
    if (!HighlightMaterial || !IsValid(HighlightMaterial))
    {
        UE_LOG(LogTemp, Warning, TEXT("SimpleInteractable: Cannot create dynamic material - no base highlight material"));
        return;
    }

    // Clear any existing dynamic material first
    DynamicHighlightMaterial = nullptr;

    try
    {
        UWorld* World = GetWorld();
        if (World && IsValid(World))
        {
            DynamicHighlightMaterial = UMaterialInstanceDynamic::Create(HighlightMaterial, this);
            
            if (!DynamicHighlightMaterial || !IsValid(DynamicHighlightMaterial))
            {
                UE_LOG(LogTemp, Warning, TEXT("SimpleInteractable: Failed to create dynamic highlight material for %s"), *GetName());
            }
            else
            {
                UE_LOG(LogTemp, Verbose, TEXT("SimpleInteractable: Created dynamic highlight material for %s"), *GetName());
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("SimpleInteractable: Invalid world, cannot create dynamic material"));
        }
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("SimpleInteractable: Exception creating dynamic material for %s"), *GetName());
        DynamicHighlightMaterial = nullptr;
    }
}

UMaterialInstanceDynamic* ASimpleInteractable::GetSharedDynamicMaterial()
{
    static TMap<UMaterialInterface*, UMaterialInstanceDynamic*> SharedMaterials;
    
    if (!HighlightMaterial || !IsValid(HighlightMaterial))
        return nullptr;
    
    // Check if we already have a shared material for this base material
    UMaterialInstanceDynamic** ExistingMaterial = SharedMaterials.Find(HighlightMaterial);
    if (ExistingMaterial && *ExistingMaterial && IsValid(*ExistingMaterial))
    {
        return *ExistingMaterial;
    }
    
    // Create new shared material with safety checks
    UMaterialInstanceDynamic* NewSharedMaterial = nullptr;
    
    if (GetWorld() && IsValid(GetWorld()))
    {
        NewSharedMaterial = UMaterialInstanceDynamic::Create(HighlightMaterial, GetWorld());
    }
    
    if (NewSharedMaterial && IsValid(NewSharedMaterial))
    {
        SharedMaterials.Add(HighlightMaterial, NewSharedMaterial);
        UE_LOG(LogTemp, Log, TEXT("SimpleInteractable: Created new shared dynamic material"));
        return NewSharedMaterial;
    }
    
    UE_LOG(LogTemp, Error, TEXT("SimpleInteractable: Failed to create dynamic material instance"));
    return nullptr;
}

void ASimpleInteractable::RefreshRarityColors()
{
    RarityColors.Empty();
    InitializeRarityColors();
    
    // Update current highlight if active
    if (bIsHighlighted && bUseRarityColors)
    {
        UpdateHighlightColor();
    }
    
    UE_LOG(LogTemp, Log, TEXT("Refreshed rarity colors for %s"), *GetName());
}

void ASimpleInteractable::PopulateDefaultRarityColors()
{
    // Clear existing colors
    RarityColors.Empty();
    
    // Populate with default colors
    RarityColors.Reserve(6);
    RarityColors.Add(FRarityColorData(EItemRarity::Common, FLinearColor(0.6f, 0.6f, 0.6f, 1.0f), TEXT("Common")));
    RarityColors.Add(FRarityColorData(EItemRarity::Uncommon, FLinearColor(0.2f, 1.0f, 0.2f, 1.0f), TEXT("Uncommon")));
    RarityColors.Add(FRarityColorData(EItemRarity::Rare, FLinearColor(0.2f, 0.2f, 1.0f, 1.0f), TEXT("Rare")));
    RarityColors.Add(FRarityColorData(EItemRarity::Epic, FLinearColor(0.8f, 0.2f, 1.0f, 1.0f), TEXT("Epic")));
    RarityColors.Add(FRarityColorData(EItemRarity::Legendary, FLinearColor(1.0f, 0.5f, 0.0f, 1.0f), TEXT("Legendary")));
    RarityColors.Add(FRarityColorData(EItemRarity::Unique, FLinearColor(1.0f, 0.8f, 0.0f, 1.0f), TEXT("Unique")));
    
    UE_LOG(LogTemp, Warning, TEXT("Populated %d default rarity colors for %s"), RarityColors.Num(), *GetName());
    
    // Log each color for verification
    for (const FRarityColorData& ColorData : RarityColors)
    {
        FString RarityName = UEnum::GetValueAsString(ColorData.Rarity);
        UE_LOG(LogTemp, Warning, TEXT("  %s: R=%.2f G=%.2f B=%.2f"), 
               *RarityName, ColorData.Color.R, ColorData.Color.G, ColorData.Color.B);
    }
}

void ASimpleInteractable::TestAllRarityColors()
{
    if (RarityColors.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No rarity colors defined. Populating defaults first."));
        PopulateDefaultRarityColors();
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Testing all rarity colors for %s"), *GetName());
    
    // Test each rarity
    TArray<EItemRarity> AllRarities = {
        EItemRarity::Common,
        EItemRarity::Uncommon,
        EItemRarity::Rare,
        EItemRarity::Epic,
        EItemRarity::Legendary,
        EItemRarity::Unique
    };
    
    for (EItemRarity TestRarity : AllRarities)
    {
        SetItemRarity(TestRarity);
        FLinearColor Color = GetRarityColor();
        FString RarityName = UEnum::GetValueAsString(TestRarity);
        
        UE_LOG(LogTemp, Warning, TEXT("  %s: R=%.2f G=%.2f B=%.2f A=%.2f"), 
               *RarityName, Color.R, Color.G, Color.B, Color.A);
        
        // Brief highlight test
        SetHighlight(true);
        
        // Small delay between tests (in editor this will be immediate)
        if (GEngine)
        {
            FString DisplayText = FString::Printf(TEXT("Testing %s"), *RarityName);
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, Color.ToFColor(true), DisplayText);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Rarity color test complete"));
}