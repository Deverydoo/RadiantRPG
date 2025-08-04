// Private/Characters/PlayerCharacter.cpp
// Updated player character implementation with safe component architecture

#include "Characters/PlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"

// Component includes
#include "Components/StaminaComponent.h"
#include "Interaction/InteractableInterface.h"

APlayerCharacter::APlayerCharacter()
{
    // CRITICAL: Set character type FIRST so BaseCharacter constructor creates all components
    CharacterType = ECharacterType::Player;

    // Override tick settings for player - needs faster updates
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.0f; // Full framerate for player

    // Configure character movement for player
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = true;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
    GetCharacterMovement()->JumpZVelocity = 700.0f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 300.0f;
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.0f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;

    // Create third person camera boom
    ThirdPersonCameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("ThirdPersonCameraBoom"));
    ThirdPersonCameraBoom->SetupAttachment(RootComponent);
    ThirdPersonCameraBoom->TargetArmLength = 300.0f;
    ThirdPersonCameraBoom->bUsePawnControlRotation = true;
    ThirdPersonCameraBoom->bInheritPitch = true;
    ThirdPersonCameraBoom->bInheritYaw = true;
    ThirdPersonCameraBoom->bInheritRoll = false;
    ThirdPersonCameraBoom->bDoCollisionTest = true;
    ThirdPersonCameraBoom->bEnableCameraLag = true;
    ThirdPersonCameraBoom->CameraLagSpeed = 3.0f;

    // Create third person camera
    ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
    ThirdPersonCamera->SetupAttachment(ThirdPersonCameraBoom, USpringArmComponent::SocketName);
    ThirdPersonCamera->bUsePawnControlRotation = false;

    // Create first person camera
    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetMesh(), FName("head"));
    FirstPersonCamera->bUsePawnControlRotation = true;

    // Initialize camera settings
    CurrentCameraMode = ECameraMode::ThirdPerson;
    CurrentZoomLevel = EThirdPersonZoom::Close;
    CameraTransitionSpeed = 5.0f;
    ThirdPersonCloseDistance = 180.0f;
    ThirdPersonFarDistance = 350.0f;
    ThirdPersonCameraLag = 3.0f;
    FirstPersonCameraOffset = FVector::ZeroVector;

    // Initialize input settings (still stored on character for settings persistence)
    MouseSensitivity = 1.0f;
    GamepadSensitivity = 1.0f;
    bInvertMouseY = false;
    bUseTankControls = true; // Default to Oblivion-style controls
    RotationSpeed = 1.5f;

    // Initialize interaction settings with proper collision channel
    BaseInteractionDistance = 200.0f;
    InteractionSphereRadius = 15.0f;
    InteractionTraceChannel = ECC_GameTraceChannel1; // Our custom "Interaction" channel
    bShowInteractionDebug = false;

    // Initialize state
    CurrentInteractable = nullptr;
    CurrentInteractionPoint = FVector::ZeroVector;
    CurrentInteractionNormal = FVector::ZeroVector;
    bIsSprinting = false;
    MovementInput = FVector2D::ZeroVector;
    LookInput = FVector2D::ZeroVector;

    // Initialize cache
    CachedControlRotation = FRotator::ZeroRotator;
    LastControlRotationUpdate = 0.0f;
    CameraTransitionProgress = 1.0f;
    bIsTransitioningCamera = false;

    // Add default player tags
    CharacterActorTags.Add(FName("Player"));
    
    // Add gameplay tags for player capabilities
    CharacterTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Character.Player")));
    CharacterTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Character.Physical")));
    CharacterTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Character.Learning")));
}

void APlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Initialize player-specific components
    InitializePlayerComponents();

    // Set up camera mode
    UpdateCameraMode();

    UE_LOG(LogTemp, Log, TEXT("PlayerCharacter initialized with %s camera mode"), 
           CurrentCameraMode == ECameraMode::FirstPerson ? TEXT("First Person") : TEXT("Third Person"));
}

void APlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Update interaction detection every frame for responsiveness
    UpdateInteractionDetection();

    // Update sprint state
    UpdateSprintState();

    // Update camera transition if active
    if (bIsTransitioningCamera)
    {
        CameraTransitionProgress += DeltaTime * CameraTransitionSpeed;
        if (CameraTransitionProgress >= 1.0f)
        {
            CameraTransitionProgress = 1.0f;
            bIsTransitioningCamera = false;
        }
    }
}

// === INPUT PROCESSING FUNCTIONS (Called by PlayerController) ===

void APlayerCharacter::HandleMoveInput(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();
    MovementInput = MovementVector;

    if (Controller != nullptr && MovementVector.SizeSquared() > 0.0f)
    {
        if (bUseTankControls)
        {
            HandleTankMovement(MovementVector);
        }
        else
        {
            HandleModernMovement(MovementVector);
        }
    }
}

void APlayerCharacter::HandleLookInput(const FVector2D& ProcessedLookInput)
{
    // Input is already processed by controller (sensitivity and invert applied)
    LookInput = ProcessedLookInput;

    if (Controller != nullptr)
    {
        AddControllerYawInput(ProcessedLookInput.X);
        AddControllerPitchInput(ProcessedLookInput.Y);
    }
}

void APlayerCharacter::HandleZoomInput(const FInputActionValue& Value)
{
    float ZoomValue = Value.Get<float>();
    
    // Only zoom in third person mode
    if (CurrentCameraMode == ECameraMode::ThirdPerson)
    {
        if (ZoomValue > 0.0f)
        {
            // Zoom in
            if (CurrentZoomLevel == EThirdPersonZoom::Far)
            {
                SetZoomLevel(EThirdPersonZoom::Close);
            }
        }
        else if (ZoomValue < 0.0f)
        {
            // Zoom out
            if (CurrentZoomLevel == EThirdPersonZoom::Close)
            {
                SetZoomLevel(EThirdPersonZoom::Far);
            }
        }
    }
}

// === MOVEMENT PROCESSING ===

void APlayerCharacter::HandleTankMovement(const FVector2D& MovementVector)
{
    // Tank controls like Oblivion
    // Forward/Back: Move forward/backward relative to character facing
    // Left/Right: Rotate character left/right
    
    if (FMath::Abs(MovementVector.Y) > 0.0f)
    {
        // Forward/backward movement
        AddMovementInput(GetActorForwardVector(), MovementVector.Y);
    }
    
    if (FMath::Abs(MovementVector.X) > 0.0f)
    {
        // Left/right rotation
        AddControllerYawInput(MovementVector.X * RotationSpeed);
    }
}

void APlayerCharacter::HandleModernMovement(const FVector2D& MovementVector)
{
    // Modern controls - strafe left/right, camera-relative forward/back
    const FRotator Rotation = Controller->GetControlRotation();
    const FRotator YawRotation(0, Rotation.Yaw, 0);

    // Forward/backward movement
    if (FMath::Abs(MovementVector.Y) > 0.0f)
    {
        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        AddMovementInput(Direction, MovementVector.Y);
    }

    // Left/right movement (strafe)
    if (FMath::Abs(MovementVector.X) > 0.0f)
    {
        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        AddMovementInput(Direction, MovementVector.X);
    }
}

// === CAMERA INTERFACE ===

void APlayerCharacter::ToggleCameraMode()
{
    ECameraMode NewMode = (CurrentCameraMode == ECameraMode::ThirdPerson) ? 
                         ECameraMode::FirstPerson : ECameraMode::ThirdPerson;
    SetCameraMode(NewMode);
}

void APlayerCharacter::SetCameraMode(ECameraMode NewMode)
{
    if (CurrentCameraMode == NewMode)
        return;

    CurrentCameraMode = NewMode;
    UpdateCameraMode();
    
    // Start camera transition
    CameraTransitionProgress = 0.0f;
    bIsTransitioningCamera = true;

    // Adjust controls based on camera mode
    if (CurrentCameraMode == ECameraMode::FirstPerson)
    {
        bUseTankControls = false; // Use modern controls in first person
    }
    else
    {
        bUseTankControls = true; // Use tank controls in third person
    }

    OnCameraModeChanged.Broadcast(CurrentCameraMode);
    
    UE_LOG(LogTemp, Log, TEXT("Player switched to %s camera mode"), 
           CurrentCameraMode == ECameraMode::FirstPerson ? TEXT("First Person") : TEXT("Third Person"));
}

void APlayerCharacter::CycleZoomLevel()
{
    if (CurrentCameraMode != ECameraMode::ThirdPerson)
        return;

    EThirdPersonZoom NewZoom = (CurrentZoomLevel == EThirdPersonZoom::Close) ? 
                              EThirdPersonZoom::Far : EThirdPersonZoom::Close;
    SetZoomLevel(NewZoom);
}

void APlayerCharacter::SetZoomLevel(EThirdPersonZoom NewZoom)
{
    if (CurrentZoomLevel == NewZoom)
        return;

    CurrentZoomLevel = NewZoom;
    UpdateThirdPersonCameraSettings();

    UE_LOG(LogTemp, Log, TEXT("Player zoom level changed to %s"), 
           CurrentZoomLevel == EThirdPersonZoom::Close ? TEXT("Close") : TEXT("Far"));
}

UCameraComponent* APlayerCharacter::GetActiveCamera() const
{
    return (CurrentCameraMode == ECameraMode::FirstPerson) ? FirstPersonCamera : ThirdPersonCamera;
}

void APlayerCharacter::UpdateCameraMode()
{
    switch (CurrentCameraMode)
    {
        case ECameraMode::ThirdPerson:
        {
            ThirdPersonCamera->SetActive(true);
            FirstPersonCamera->SetActive(false);
            GetMesh()->SetVisibility(true);
            GetMesh()->SetOwnerNoSee(false);
            UpdateThirdPersonCameraSettings();
            break;
        }
        case ECameraMode::FirstPerson:
        {
            FirstPersonCamera->SetActive(true);
            ThirdPersonCamera->SetActive(false);
            GetMesh()->SetVisibility(false);
            GetMesh()->SetOwnerNoSee(true);
            break;
        }
    }
}

void APlayerCharacter::UpdateThirdPersonCameraSettings()
{
    if (!ThirdPersonCameraBoom)
        return;

    float TargetDistance = (CurrentZoomLevel == EThirdPersonZoom::Close) ? 
                          ThirdPersonCloseDistance : ThirdPersonFarDistance;
    
    ThirdPersonCameraBoom->TargetArmLength = TargetDistance;
    ThirdPersonCameraBoom->CameraLagSpeed = ThirdPersonCameraLag;
    ThirdPersonCameraBoom->CameraLagMaxDistance = 50.0f;
    ThirdPersonCameraBoom->SocketOffset = FVector(0.0f, 0.0f, 20.0f);
    ThirdPersonCameraBoom->TargetOffset = FVector(0.0f, 0.0f, 40.0f);
}

// === INTERACTION INTERFACE ===

void APlayerCharacter::UpdateInteractionDetection()
{
    if (!Controller)
        return;

    // Cache control rotation for performance (update at 30Hz max)
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastControlRotationUpdate > 0.0333f)
    {
        CachedControlRotation = Controller->GetControlRotation();
        LastControlRotationUpdate = CurrentTime;
    }

    // Perform interaction trace using our custom collision channel
    FHitResult HitResult;
    FVector TraceStart = GetInteractionTraceStart();
    FVector TraceDirection = GetInteractionTraceDirection();
    FVector TraceEnd = TraceStart + (TraceDirection * BaseInteractionDistance);
    
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    QueryParams.bTraceComplex = false;
    QueryParams.bReturnPhysicalMaterial = false;
    
    // Use sphere sweep on our custom Interaction trace channel
    bool bHit = GetWorld()->SweepSingleByChannel(
        HitResult,
        TraceStart,
        TraceEnd,
        FQuat::Identity,
        InteractionTraceChannel, // ECC_GameTraceChannel1 = "Interaction"
        FCollisionShape::MakeSphere(InteractionSphereRadius),
        QueryParams
    );
    
    // Alternative: Use object type query for Interactable objects
    // This will hit objects with the "Interactable" object type
    if (!bHit)
    {
        TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
        ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel3)); // "Interactable" object type
        
        FCollisionObjectQueryParams ObjectQueryParams(ObjectTypes);
        
        bHit = GetWorld()->SweepSingleByObjectType(
            HitResult,
            TraceStart,
            TraceEnd,
            FQuat::Identity,
            ObjectQueryParams,
            FCollisionShape::MakeSphere(InteractionSphereRadius),
            QueryParams
        );
    }
    
    AActor* PreviousInteractable = CurrentInteractable;
    AActor* NewInteractable = (bHit && IsActorInteractable(HitResult.GetActor())) ? HitResult.GetActor() : nullptr;
    
    // Handle interactable changes
    if (NewInteractable != PreviousInteractable)
    {
        // Handle focus lost
        if (PreviousInteractable && PreviousInteractable->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
        {
            IInteractableInterface::Execute_OnInteractionFocusLost(PreviousInteractable, this);
        }
        
        CurrentInteractable = NewInteractable;
        
        // Handle focus gained
        if (CurrentInteractable)
        {
            CurrentInteractionPoint = HitResult.Location;
            CurrentInteractionNormal = HitResult.Normal;
            
            if (CurrentInteractable->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
            {
                IInteractableInterface::Execute_OnInteractionFocusGained(CurrentInteractable, this);
            }
            
            OnInteractableDetected.Broadcast(CurrentInteractable, true);
        }
        else
        {
            CurrentInteractionPoint = FVector::ZeroVector;
            CurrentInteractionNormal = FVector::ZeroVector;
            OnInteractableDetected.Broadcast(nullptr, false);
        }
    }
    
    // Debug visualization
    if (bShowInteractionDebug)
    {
        FColor DebugColor = bHit ? (NewInteractable ? FColor::Green : FColor::Yellow) : FColor::Red;
        DrawDebugLine(GetWorld(), TraceStart, TraceEnd, DebugColor, false, 0.0f, 0, 2.0f);
        
        if (bHit)
        {
            DrawDebugSphere(GetWorld(), HitResult.Location, 5.0f, 8, FColor::Orange, false, 0.0f, 0, 1.0f);
            
            // Draw interaction sphere at hit location
            DrawDebugSphere(GetWorld(), HitResult.Location, InteractionSphereRadius, 12, DebugColor, false, 0.0f, 0, 1.0f);
        }
        
        // Always draw the trace sphere at the end point
        DrawDebugSphere(GetWorld(), TraceEnd, InteractionSphereRadius, 12, FColor::Blue, false, 0.0f, 0, 0.5f);
    }
}

FVector APlayerCharacter::GetInteractionTraceStart() const
{
    if (UCameraComponent* ActiveCamera = GetActiveCamera())
    {
        return ActiveCamera->GetComponentLocation();
    }
    
    // Fallback to character eye level
    return GetActorLocation() + FVector(0.0f, 0.0f, BaseEyeHeight);
}

FVector APlayerCharacter::GetInteractionTraceDirection() const
{
    if (UCameraComponent* ActiveCamera = GetActiveCamera())
    {
        return ActiveCamera->GetForwardVector();
    }
    
    // Fallback to control rotation
    return CachedControlRotation.Vector();
}

bool APlayerCharacter::IsActorInteractable(AActor* Actor) const
{
    if (!Actor)
        return false;
    
    // Check for interactable interface
    if (Actor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
    {
        return IInteractableInterface::Execute_CanInteract(Actor, const_cast<APlayerCharacter*>(this));
    }
    
    // Fallback to tag check
    return Actor->Tags.Contains(FName("Interactable"));
}

void APlayerCharacter::TryInteract()
{
    if (CurrentInteractable && CurrentInteractable->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
    {
        bool bInteractionSuccess = IInteractableInterface::Execute_OnInteract(
            CurrentInteractable, 
            this, 
            CurrentInteractionPoint, 
            CurrentInteractionNormal
        );
        
        if (bInteractionSuccess)
        {
            UE_LOG(LogTemp, Log, TEXT("Player successfully interacted with %s"), *CurrentInteractable->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Player interaction with %s failed"), *CurrentInteractable->GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No valid interactable target found"));
    }
}

// === MOVEMENT INTERFACE ===

void APlayerCharacter::StartSprinting()
{
    if (!CanSprint())
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot sprint - insufficient stamina or conditions not met"));
        return;
    }

    bIsSprinting = true;
    GetCharacterMovement()->MaxWalkSpeed = 600.0f; // Sprint speed
    
    // Start continuous stamina drain
    if (StaminaComponent)
    {
        FStaminaCostInfo SprintCost;
        SprintCost.BaseCost = 10.0f; // Cost per second
        SprintCost.Activity = EStaminaActivity::Sprinting;
        SprintCost.bIsContinuous = true;
        SprintCost.Actor = this;
        
        StaminaComponent->StartContinuousActivity(SprintCost);
    }
    
    UE_LOG(LogTemp, Log, TEXT("Player started sprinting"));
}

void APlayerCharacter::StopSprinting()
{
    if (!bIsSprinting)
        return;

    bIsSprinting = false;
    GetCharacterMovement()->MaxWalkSpeed = 300.0f; // Normal walk speed
    
    // Stop continuous stamina drain
    if (StaminaComponent)
    {
        StaminaComponent->StopContinuousActivity(EStaminaActivity::Sprinting);
    }
    
    UE_LOG(LogTemp, Log, TEXT("Player stopped sprinting"));
}

bool APlayerCharacter::CanSprint() const
{
    if (!StaminaComponent)
        return false;
        
    // Need at least 20% stamina to start sprinting and not be exhausted
    return StaminaComponent->GetStaminaPercent() >= 0.2f && !StaminaComponent->IsExhausted();
}

void APlayerCharacter::UpdateSprintState()
{
    // Auto-stop sprinting if conditions are no longer met
    if (bIsSprinting && !CanSprint())
    {
        StopSprinting();
    }
}

// === SETTINGS ===

void APlayerCharacter::SetMouseSensitivity(float NewSensitivity)
{
    MouseSensitivity = FMath::Clamp(NewSensitivity, 0.1f, 5.0f);
}

void APlayerCharacter::SetGamepadSensitivity(float NewSensitivity)
{
    GamepadSensitivity = FMath::Clamp(NewSensitivity, 0.1f, 5.0f);
}

void APlayerCharacter::SetInvertMouseY(bool bInvert)
{
    bInvertMouseY = bInvert;
}

// === INITIALIZATION ===

void APlayerCharacter::InitializePlayerComponents()
{
    // All player components are now created in constructor, so just log success
    UE_LOG(LogTemp, Log, TEXT("Player-specific component initialization completed"));
    
    // Verify all expected components exist
    if (StaminaComponent)
    {
        UE_LOG(LogTemp, Log, TEXT("StaminaComponent ready"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("StaminaComponent missing!"));
    }
    
    if (ManaComponent)
    {
        UE_LOG(LogTemp, Log, TEXT("ManaComponent ready"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ManaComponent missing!"));
    }
    
    if (SkillsComponent)
    {
        UE_LOG(LogTemp, Log, TEXT("SkillsComponent ready"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("SkillsComponent missing!"));
    }
}