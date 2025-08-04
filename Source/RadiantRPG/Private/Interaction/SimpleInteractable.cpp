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

    // Initialize rarity colors with static safety check
    static bool bRarityColorsInitialized = false;
    if (!bRarityColorsInitialized)
    {
        InitializeRarityColors();
        bRarityColorsInitialized = true;
    }

    // Quick rarity determination
    if (bUseRarityColors)
    {
        ItemRarity = DetermineRarityFromTags();
    }

    // Store materials safely - only if we have a valid mesh and are in game world
    if (MeshComponent && IsValid(MeshComponent) && GetWorld() && GetWorld()->IsGameWorld())
    {
        UStaticMesh* StaticMesh = MeshComponent->GetStaticMesh();
        if (StaticMesh && IsValid(StaticMesh))
        {
            StoreOriginalMaterials();
        }
    }

    UE_LOG(LogTemp, Log, TEXT("SimpleInteractable '%s' initialized successfully"), *GetName());
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
    if (RarityColors.Num() > 0)
        return;
    
    RarityColors.Reserve(6);
    RarityColors.Emplace(EItemRarity::Common, FLinearColor(0.6f, 0.6f, 0.6f, 1.0f), TEXT("Common"));
    RarityColors.Emplace(EItemRarity::Uncommon, FLinearColor(0.2f, 1.0f, 0.2f, 1.0f), TEXT("Uncommon"));
    RarityColors.Emplace(EItemRarity::Rare, FLinearColor(0.2f, 0.2f, 1.0f, 1.0f), TEXT("Rare"));
    RarityColors.Emplace(EItemRarity::Epic, FLinearColor(0.8f, 0.2f, 1.0f, 1.0f), TEXT("Epic"));
    RarityColors.Emplace(EItemRarity::Legendary, FLinearColor(1.0f, 0.5f, 0.0f, 1.0f), TEXT("Legendary"));
    RarityColors.Emplace(EItemRarity::Unique, FLinearColor(1.0f, 0.8f, 0.0f, 1.0f), TEXT("Unique"));
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
    if (bUseRarityColors)
    {
        for (const FRarityColorData& RarityData : RarityColors)
        {
            if (RarityData.Rarity == ItemRarity)
            {
                return RarityData.Color;
            }
        }
    }
    return DefaultHighlightColor;
}

void ASimpleInteractable::ApplyHighlightMaterial()
{
    // ULTRA SAFE material application for UE 5.6
    if (!MeshComponent || !IsValid(MeshComponent))
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyHighlightMaterial: MeshComponent invalid"));
        return;
    }
    
    if (!bUseHighlightMaterial)
    {
        return;
    }
    
    if (!MeshComponent->GetStaticMesh() || !IsValid(MeshComponent->GetStaticMesh()))
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyHighlightMaterial: No valid static mesh"));
        return;
    }

    if (!HighlightMaterial || !IsValid(HighlightMaterial))
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyHighlightMaterial: No valid highlight material for %s"), *GetName());
        return;
    }

    // Store original materials if not already done
    if (!bHasStoredOriginalMaterials)
    {
        StoreOriginalMaterials();
    }

    UMaterialInterface* MaterialToApply = nullptr;
    
    // Handle dynamic material creation for rarity colors
    if (bUseRarityColors)
    {
        if (!DynamicHighlightMaterial)
        {
            CreateDynamicHighlightMaterial();
        }
        
        if (DynamicHighlightMaterial && IsValid(DynamicHighlightMaterial))
        {
            FLinearColor RarityColor = GetRarityColor();
            DynamicHighlightMaterial->SetVectorParameterValue(ColorParameterName, RarityColor);
            MaterialToApply = DynamicHighlightMaterial;
        }
        else
        {
            MaterialToApply = HighlightMaterial;
        }
    }
    else
    {
        MaterialToApply = HighlightMaterial;
    }

    if (!MaterialToApply || !IsValid(MaterialToApply))
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyHighlightMaterial: No valid material to apply for %s"), *GetName());
        return;
    }

    // Apply the overlay material with extreme safety for UE 5.6
    try
    {
        if (MeshComponent && IsValid(MeshComponent) && 
            !MeshComponent->HasAnyFlags(RF_BeginDestroyed))
        {
            MeshComponent->SetOverlayMaterial(MaterialToApply);
            MeshComponent->SetOverlayMaterialMaxDrawDistance(HighlightMaxDrawDistance);
            
            UE_LOG(LogTemp, Verbose, TEXT("SimpleInteractable: Applied highlight material to %s"), *GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("SimpleInteractable: MeshComponent not safe for material application"));
        }
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("SimpleInteractable: Exception applying highlight material to %s"), *GetName());
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
    if (!HighlightMaterial)
    {
        UE_LOG(LogTemp, Warning, TEXT("SimpleInteractable: Cannot create dynamic material - no base highlight material"));
        return;
    }

    DynamicHighlightMaterial = GetSharedDynamicMaterial();
    
    if (!DynamicHighlightMaterial)
    {
        UE_LOG(LogTemp, Warning, TEXT("SimpleInteractable: Failed to create dynamic highlight material for %s"), *GetName());
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