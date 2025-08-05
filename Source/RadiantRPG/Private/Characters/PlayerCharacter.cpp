// Private/Characters/PlayerCharacter.cpp
// Updated player character implementation with improved camera positioning and interaction tracing

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

    // Create third person camera boom (positioned to the right of player)
    ThirdPersonCameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("ThirdPersonCameraBoom"));
    ThirdPersonCameraBoom->SetupAttachment(RootComponent);
    ThirdPersonCameraBoom->TargetArmLength = 350.0f;
    ThirdPersonCameraBoom->bUsePawnControlRotation = true;
    ThirdPersonCameraBoom->bInheritPitch = true;
    ThirdPersonCameraBoom->bInheritYaw = true;
    ThirdPersonCameraBoom->bInheritRoll = false;
    ThirdPersonCameraBoom->bDoCollisionTest = true;
    ThirdPersonCameraBoom->bEnableCameraLag = true;
    ThirdPersonCameraBoom->CameraLagSpeed = 3.0f;
    ThirdPersonCameraBoom->CameraLagMaxDistance = 50.0f;
    
    // IMPORTANT: Position camera to the right of the player, not directly behind
    // This creates the over-the-shoulder view like in the reference image
    ThirdPersonCameraBoom->SocketOffset = FVector(0.0f, 80.0f, 40.0f); // Right and up offset
    ThirdPersonCameraBoom->TargetOffset = FVector(0.0f, 0.0f, 60.0f);   // Look slightly above character center

    // Create third person camera
    ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
    ThirdPersonCamera->SetupAttachment(ThirdPersonCameraBoom, USpringArmComponent::SocketName);
    ThirdPersonCamera->bUsePawnControlRotation = false;

    // Create first person camera attached to head/neck area
    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetMesh(), TEXT("head")); // Attach to head socket if available
    FirstPersonCamera->SetRelativeLocation(FVector(10.0f, 0.0f, 0.0f));
    FirstPersonCamera->bUsePawnControlRotation = true;

    // Initialize camera settings
    CurrentCameraMode = ECameraMode::ThirdPerson;
    CurrentZoomLevel = EThirdPersonZoom::Close;
    CameraTransitionSpeed = 5.0f;
    ThirdPersonCloseDistance = 250.0f;
    ThirdPersonFarDistance = 500.0f;
    ThirdPersonCameraLag = 3.0f;
    FirstPersonCameraOffset = FVector(10.0f, 0.0f, 0.0f);

    // Input settings
    MouseSensitivity = 1.0f;
    GamepadSensitivity = 1.0f;
    bInvertMouseY = false;
    bUseTankControls = false;
    RotationSpeed = 1.0f;

    // Interaction settings - CRITICAL: Adjust for camera distance
    BaseInteractionDistance = 300.0f; // Increased base distance
    InteractionSphereRadius = 15.0f;
    InteractionChannel = ECC_GameTraceChannel1; // Custom Interaction channel

    // Performance optimization
    LastControlRotationUpdate = 0.0f;
    CameraTransitionProgress = 1.0f;
    bIsTransitioningCamera = false;

    // Set proper collision for interaction
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
}

void APlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    InitializePlayerComponents();
    UpdateCameraMode();
}

void APlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    UpdateCameraMode();
    UpdateInteractionDetection();
    UpdateSprintState();
}

void APlayerCharacter::InitializePlayerComponents()
{
    // Additional player-specific initialization
    if (FirstPersonCamera && GetMesh())
    {
        // Try to attach to head socket, fallback to relative positioning
        if (GetMesh()->DoesSocketExist(TEXT("head")))
        {
            FirstPersonCamera->AttachToComponent(GetMesh(), 
                FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("head"));
        }
        else
        {
            FirstPersonCamera->SetRelativeLocation(FirstPersonCameraOffset);
        }
    }
}

// === CAMERA MANAGEMENT ===

UCameraComponent* APlayerCharacter::GetActiveCamera() const
{
    return (CurrentCameraMode == ECameraMode::FirstPerson) ? FirstPersonCamera : ThirdPersonCamera;
}

void APlayerCharacter::UpdateCameraMode()
{
    if (bIsTransitioningCamera)
    {
        CameraTransitionProgress = FMath::Clamp(CameraTransitionProgress + GetWorld()->GetDeltaSeconds() * CameraTransitionSpeed, 0.0f, 1.0f);
        
        if (CameraTransitionProgress >= 1.0f)
        {
            bIsTransitioningCamera = false;
        }
    }

    // Update third person camera settings
    UpdateThirdPersonCameraSettings();

    // Ensure proper camera activation
    if (ThirdPersonCamera && FirstPersonCamera)
    {
        ThirdPersonCamera->SetActive(CurrentCameraMode == ECameraMode::ThirdPerson);
        FirstPersonCamera->SetActive(CurrentCameraMode == ECameraMode::FirstPerson);
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
    
    // Maintain the right-side offset regardless of zoom
    ThirdPersonCameraBoom->SocketOffset = FVector(0.0f, 80.0f, 40.0f);
    ThirdPersonCameraBoom->TargetOffset = FVector(0.0f, 0.0f, 60.0f);
}

// === INTERACTION SYSTEM ===

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

    // Get trace parameters with camera distance compensation
    FVector TraceStart = GetInteractionTraceStart();
    FVector TraceDirection = GetInteractionTraceDirection();
    
    // CRITICAL FIX: Calculate effective interaction distance based on camera mode and distance
    float EffectiveInteractionDistance = CalculateEffectiveInteractionDistance();
    FVector TraceEnd = TraceStart + (TraceDirection * EffectiveInteractionDistance);
    
    // Enhanced collision query params
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    QueryParams.bTraceComplex = false;
    QueryParams.bReturnPhysicalMaterial = false;
    QueryParams.bIgnoreBlocks = false;
    
    // Perform primary interaction trace using sphere sweep for better reliability
    FHitResult HitResult;
    bool bHit = GetWorld()->SweepSingleByChannel(
        HitResult,
        TraceStart,
        TraceEnd,
        FQuat::Identity,
        InteractionChannel,
        FCollisionShape::MakeSphere(InteractionSphereRadius),
        QueryParams
    );

    // Debug visualization (only in development builds)
    #if WITH_EDITOR
    if (CVarDebugInteraction && CVarDebugInteraction->GetInt() > 0)
    {
        FColor DebugColor = bHit ? FColor::Green : FColor::Red;
        DrawDebugLine(GetWorld(), TraceStart, TraceEnd, DebugColor, false, 0.1f, 0, 2.0f);
        DrawDebugSphere(GetWorld(), TraceStart, 5.0f, 8, FColor::Blue, false, 0.1f);
        
        if (bHit)
        {
            DrawDebugSphere(GetWorld(), HitResult.Location, InteractionSphereRadius, 12, FColor::Yellow, false, 0.1f);
        }
    }
    #endif

    // Process hit result
    ProcessInteractionHit(bHit, HitResult);
}

float APlayerCharacter::CalculateEffectiveInteractionDistance() const
{
    float EffectiveDistance = BaseInteractionDistance;
    
    if (CurrentCameraMode == ECameraMode::ThirdPerson && ThirdPersonCameraBoom)
    {
        // CRITICAL FIX: Add camera distance to interaction range for third person
        // This compensates for the camera being behind the character
        float CameraDistance = ThirdPersonCameraBoom->TargetArmLength;
        EffectiveDistance += CameraDistance * 0.7f; // 70% of camera distance compensation
        
        // Additional compensation for camera offset to the right
        EffectiveDistance += 100.0f; // Extra range to account for angle offset
    }
    
    return EffectiveDistance;
}

FVector APlayerCharacter::GetInteractionTraceStart() const
{
    UCameraComponent* ActiveCamera = GetActiveCamera();
    if (ActiveCamera && IsValid(ActiveCamera))
    {
        FVector CameraLocation = ActiveCamera->GetComponentLocation();
        
        // For third person, start trace from camera but adjust for better targeting
        if (CurrentCameraMode == ECameraMode::ThirdPerson)
        {
            // Move trace start towards the character to account for camera offset
            FVector ToCharacter = (GetActorLocation() - CameraLocation).GetSafeNormal();
            CameraLocation += ToCharacter * 80.0f; // Move 80cm towards character
            
            // Also adjust for the right-side camera offset by angling slightly left
            FVector RightVector = ActiveCamera->GetRightVector();
            CameraLocation -= RightVector * 30.0f; // Adjust 30cm to the left
        }
        
        return CameraLocation;
    }
    
    // Enhanced fallback - use character's eye level with proper third-person offset
    FVector EyeLocation = GetActorLocation() + FVector(0.0f, 0.0f, BaseEyeHeight);
    
    if (CurrentCameraMode == ECameraMode::ThirdPerson)
    {
        // For third person fallback, start from in front of character
        FVector ForwardVector = GetActorForwardVector();
        FVector RightVector = GetActorRightVector();
        
        // Position similar to where camera would be looking from
        EyeLocation += ForwardVector * 100.0f;  // Forward offset
        EyeLocation += RightVector * 50.0f;     // Right offset to match camera
        EyeLocation += FVector(0.0f, 0.0f, 30.0f); // Slight height adjustment
    }
    
    return EyeLocation;
}

FVector APlayerCharacter::GetInteractionTraceDirection() const
{
    UCameraComponent* ActiveCamera = GetActiveCamera();
    if (ActiveCamera && IsValid(ActiveCamera))
    {
        FVector CameraForward = ActiveCamera->GetForwardVector();
        
        // For third person, adjust the trace direction to account for crosshair alignment
        if (CurrentCameraMode == ECameraMode::ThirdPerson)
        {
            // Calculate where the crosshair is actually pointing
            FVector TraceStart = GetInteractionTraceStart();
            FVector ScreenCenter = ActiveCamera->GetComponentLocation() + (CameraForward * 1000.0f);
            
            // Direct the trace towards where the crosshair points, not just camera forward
            FVector AdjustedDirection = (ScreenCenter - TraceStart).GetSafeNormal();
            return AdjustedDirection;
        }
        
        return CameraForward;
    }
    
    // Fallback to cached control rotation
    return CachedControlRotation.Vector();
}

void APlayerCharacter::ProcessInteractionHit(bool bHit, const FHitResult& HitResult)
{
    AActor* NewInteractable = nullptr;
    bool bCanInteract = false;

    if (bHit && HitResult.GetActor())
    {
        AActor* HitActor = HitResult.GetActor();
        if (IsActorInteractable(HitActor))
        {
            NewInteractable = HitActor;
            bCanInteract = true;
        }
    }

    // Update current interactable if changed
    if (CurrentInteractable != NewInteractable)
    {
        // Clear previous interactable
        if (CurrentInteractable && IsValid(CurrentInteractable))
        {
            if (CurrentInteractable->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
            {
                IInteractableInterface::Execute_EndFocus(CurrentInteractable, this);
            }
        }

        // Set new interactable
        CurrentInteractable = NewInteractable;

        // Focus new interactable
        if (CurrentInteractable && IsValid(CurrentInteractable))
        {
            if (CurrentInteractable->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
            {
                IInteractableInterface::Execute_StartFocus(CurrentInteractable, this);
            }
        }

        // Broadcast interaction state change
        OnInteractableDetected.Broadcast(CurrentInteractable, bCanInteract);
    }
}

bool APlayerCharacter::IsActorInteractable(AActor* Actor) const
{
    if (!Actor || !IsValid(Actor))
        return false;
    
    // Check for interactable interface first
    if (Actor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
    {
        return IInteractableInterface::Execute_CanInteract(Actor, const_cast<APlayerCharacter*>(this));
    }
    
    // Fallback to actor tags
    return Actor->ActorHasTag(FName("Interactable"));
}

// === MOVEMENT INTERFACE ===

void APlayerCharacter::MoveCharacter(const FVector2D& MovementVector)
{
    if (Controller && MovementVector.SizeSquared() > 0.0f)
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

void APlayerCharacter::HandleTankMovement(const FVector2D& MovementVector)
{
    // Tank controls: Forward/Back for movement, Left/Right for rotation
    float ForwardInput = MovementVector.Y;
    float RotationInput = MovementVector.X;

    // Apply forward/backward movement
    if (FMath::Abs(ForwardInput) > 0.0f)
    {
        FVector ForwardDirection = GetActorForwardVector();
        AddMovementInput(ForwardDirection, ForwardInput);
    }

    // Apply rotation
    if (FMath::Abs(RotationInput) > 0.0f)
    {
        float RotationRate = RotationSpeed * GetWorld()->GetDeltaSeconds();
        FRotator DeltaRotation(0.0f, RotationInput * RotationRate * 100.0f, 0.0f);
        AddActorWorldRotation(DeltaRotation);
    }
}

void APlayerCharacter::HandleModernMovement(const FVector2D& MovementVector)
{
    // Modern controls: WASD for movement relative to camera
    const FRotator ControllerRotation = Controller->GetControlRotation();
    const FRotator YawRotation(0, ControllerRotation.Yaw, 0);

    const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    AddMovementInput(ForwardDirection, MovementVector.Y);
    AddMovementInput(RightDirection, MovementVector.X);
}

void APlayerCharacter::UpdateSprintState()
{
    UStaminaComponent* StaminaComp = GetStaminaComponent();
    if (!StaminaComp)
        return;

    bool bWantsToSprint = bIsSprinting && GetVelocity().SizeSquared() > FMath::Square(50.0f);
    
    if (bWantsToSprint && StaminaComp->CanSprint())
    {
        if (!StaminaComp->IsSprintActive())
        {
            StaminaComp->StartSprint();
        }
    }
    else
    {
        if (StaminaComp->IsSprintActive())
        {
            StaminaComp->StopSprint();
        }
    }
}

// === INPUT INTERFACE ===

void APlayerCharacter::StartSprint()
{
    bIsSprinting = true;
}

void APlayerCharacter::StopSprint()
{
    bIsSprinting = false;
}

void APlayerCharacter::StartJump()
{
    Jump();
}

void APlayerCharacter::StopJump()
{
    StopJumping();
}

void APlayerCharacter::ToggleCameraMode()
{
    ECameraMode NewMode = (CurrentCameraMode == ECameraMode::FirstPerson) ? 
        ECameraMode::ThirdPerson : ECameraMode::FirstPerson;
    
    SetCameraMode(NewMode);
}

void APlayerCharacter::ToggleThirdPersonZoom()
{
    if (CurrentCameraMode == ECameraMode::ThirdPerson)
    {
        EThirdPersonZoom NewZoom = (CurrentZoomLevel == EThirdPersonZoom::Close) ?
            EThirdPersonZoom::Far : EThirdPersonZoom::Close;
        
        SetThirdPersonZoom(NewZoom);
    }
}

void APlayerCharacter::InteractWithTarget()
{
    if (CurrentInteractable && IsValid(CurrentInteractable))
    {
        if (CurrentInteractable->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
        {
            IInteractableInterface::Execute_Interact(CurrentInteractable, this);
        }
    }
}

// === CAMERA SETTINGS ===

void APlayerCharacter::SetCameraMode(ECameraMode NewMode)
{
    if (CurrentCameraMode != NewMode)
    {
        CurrentCameraMode = NewMode;
        bIsTransitioningCamera = true;
        CameraTransitionProgress = 0.0f;
        
        OnCameraModeChanged.Broadcast(CurrentCameraMode);
    }
}

void APlayerCharacter::SetThirdPersonZoom(EThirdPersonZoom NewZoom)
{
    if (CurrentZoomLevel != NewZoom)
    {
        CurrentZoomLevel = NewZoom;
        bIsTransitioningCamera = true;
        CameraTransitionProgress = 0.0f;
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