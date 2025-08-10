// Private/Characters/PlayerCharacter.cpp
// Fixed sprint implementation with proper speed changes and debug logging

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

// Console variable for sprint debug
static TAutoConsoleVariable<int32> CVarDebugSprint(
    TEXT("radiant.debug.sprint"),
    0,
    TEXT("Show sprint debug info (0=off, 1=on)"),
    ECVF_Default);

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
    ThirdPersonCameraBoom->TargetArmLength = 350.0f;
    ThirdPersonCameraBoom->SocketOffset = FVector(0.0f, 50.0f, 50.0f);
    ThirdPersonCameraBoom->bUsePawnControlRotation = true;
    ThirdPersonCameraBoom->bInheritPitch = true;
    ThirdPersonCameraBoom->bInheritYaw = true;
    ThirdPersonCameraBoom->bInheritRoll = false;
    ThirdPersonCameraBoom->bEnableCameraLag = true;
    ThirdPersonCameraBoom->CameraLagSpeed = 12.0f;

    // Create third person camera
    ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
    ThirdPersonCamera->SetupAttachment(ThirdPersonCameraBoom, USpringArmComponent::SocketName);
    ThirdPersonCamera->bUsePawnControlRotation = false;
    ThirdPersonCamera->FieldOfView = 90.0f;

    // Create first person camera
    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetMesh(), TEXT("head"));
    FirstPersonCamera->SetRelativeLocation(FVector(10.0f, 0.0f, 0.0f));
    FirstPersonCamera->bUsePawnControlRotation = true;
    FirstPersonCamera->FieldOfView = 90.0f;
    FirstPersonCamera->SetActive(false);

    // Default camera settings
    CurrentCameraMode = ECameraMode::ThirdPerson;
    CurrentZoomLevel = EThirdPersonZoom::Close;
    CameraTransitionSpeed = 5.0f;
    CloseZoomDistance = 350.0f;
    FarZoomDistance = 600.0f;
    CloseZoomOffset = FVector(0.0f, 50.0f, 50.0f);
    FarZoomOffset = FVector(0.0f, 0.0f, 100.0f);
    FirstPersonCameraOffset = FVector(10.0f, 0.0f, 0.0f);

    // Input settings
    MouseSensitivity = 1.0f;
    GamepadSensitivity = 1.0f;
    bInvertMouseY = false;
    bUseTankControls = false;
    RotationSpeed = 1.0f;

    // Interaction settings
    BaseInteractionDistance = 300.0f;
    InteractionSphereRadius = 15.0f;
    InteractionChannel = ECC_GameTraceChannel1;

    // Movement state
    bIsSprinting = false;
    NormalSpeed = 300.0f;
    SprintSpeed = 600.0f;

    // Set proper collision
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
}

void APlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    InitializePlayerComponents();
    UpdateCameraMode();
    
    // Ensure normal speed is set at start
    GetCharacterMovement()->MaxWalkSpeed = NormalSpeed;
    
    UE_LOG(LogTemp, Log, TEXT("PlayerCharacter initialized - Normal Speed: %f, Sprint Speed: %f"), 
           NormalSpeed, SprintSpeed);
}

void APlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    UpdateCameraMode();
    UpdateInteractionDetection();
    UpdateSprintState();
    
    // Debug logging
    if (CVarDebugSprint.GetValueOnGameThread() > 0)
    {
        FString SprintStatus = bIsSprinting ? TEXT("SPRINTING") : TEXT("WALKING");
        float CurrentSpeed = GetCharacterMovement()->MaxWalkSpeed;
        float ActualVelocity = GetVelocity().Size();
        
        GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Yellow, 
            FString::Printf(TEXT("Sprint Status: %s | Max Speed: %.0f | Actual Speed: %.0f"), 
            *SprintStatus, CurrentSpeed, ActualVelocity));
        
        if (UStaminaComponent* StaminaComp = GetStaminaComponent())
        {
            GEngine->AddOnScreenDebugMessage(2, 0.0f, FColor::Cyan, 
                FString::Printf(TEXT("Stamina: %.1f%% | Exhausted: %s"), 
                StaminaComp->GetStaminaPercent() * 100.0f,
                StaminaComp->IsExhausted() ? TEXT("YES") : TEXT("NO")));
        }
    }
}

// === SPRINT FUNCTIONS ===

void APlayerCharacter::StartSprint()
{
    UStaminaComponent* StaminaComp = GetStaminaComponent();
    
    // Debug log
    UE_LOG(LogTemp, Log, TEXT("StartSprint called - Current Speed: %f"), 
           GetCharacterMovement()->MaxWalkSpeed);
    
    if (!StaminaComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("StartSprint: No StaminaComponent found!"));
        return;
    }

    // Check if we can sprint
    if (StaminaComp->GetStaminaPercent() >= 0.1f && !StaminaComp->IsExhausted())
    {
        bIsSprinting = true;
        GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
        
        UE_LOG(LogTemp, Log, TEXT("Sprint STARTED - Speed set to %f"), SprintSpeed);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot sprint - Stamina: %f%%, Exhausted: %s"), 
               StaminaComp->GetStaminaPercent() * 100.0f,
               StaminaComp->IsExhausted() ? TEXT("Yes") : TEXT("No"));
    }
}

void APlayerCharacter::StopSprint()
{
    UE_LOG(LogTemp, Log, TEXT("StopSprint called"));
    
    bIsSprinting = false;
    GetCharacterMovement()->MaxWalkSpeed = NormalSpeed;
    
    UStaminaComponent* StaminaComp = GetStaminaComponent();
    if (StaminaComp)
    {
        StaminaComp->StopContinuousActivity(EStaminaActivity::Sprinting);
    }
    
    UE_LOG(LogTemp, Log, TEXT("Sprint STOPPED - Speed set to %f"), NormalSpeed);
}

void APlayerCharacter::UpdateSprintState()
{
    UStaminaComponent* StaminaComp = GetStaminaComponent();
    if (!StaminaComp)
        return;

    // Check if we're actually moving
    bool bIsMoving = GetVelocity().SizeSquared() > FMath::Square(50.0f);
    bool bWantsToSprint = bIsSprinting && bIsMoving;
    
    if (bWantsToSprint)
    {
        // Check if we still can sprint
        if (StaminaComp->GetStaminaPercent() >= 0.05f && !StaminaComp->IsExhausted())
        {
            // Ensure sprint speed is maintained
            if (GetCharacterMovement()->MaxWalkSpeed != SprintSpeed)
            {
                GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
                UE_LOG(LogTemp, Warning, TEXT("Sprint speed was reset, restoring to %f"), SprintSpeed);
            }
            
            // Start stamina drain if not already active
            if (!StaminaComp->IsActivityActive(EStaminaActivity::Sprinting))
            {
                FStaminaCostInfo SprintCost;
                SprintCost.BaseCost = 10.0f; // Cost per second
                SprintCost.Activity = EStaminaActivity::Sprinting;
                SprintCost.bIsContinuous = true;
                SprintCost.Actor = this;
                
                StaminaComp->StartContinuousActivity(SprintCost);
            }
        }
        else
        {
            // Out of stamina, force stop sprint
            StopSprint();
            UE_LOG(LogTemp, Warning, TEXT("Forced sprint stop - out of stamina"));
        }
    }
    else if (!bIsSprinting && GetCharacterMovement()->MaxWalkSpeed != NormalSpeed)
    {
        // Ensure we're at normal speed when not sprinting
        GetCharacterMovement()->MaxWalkSpeed = NormalSpeed;
        
        // Stop stamina drain
        if (StaminaComp->IsActivityActive(EStaminaActivity::Sprinting))
        {
            StaminaComp->StopContinuousActivity(EStaminaActivity::Sprinting);
        }
    }
}

// Wrapper functions for controller
void APlayerCharacter::StartSprinting()
{
    StartSprint();
}

void APlayerCharacter::StopSprinting()
{
    StopSprint();
}

bool APlayerCharacter::CanSprint() const
{
    UStaminaComponent* StaminaComp = GetStaminaComponent();
    if (!StaminaComp)
        return false;
    
    return StaminaComp->GetStaminaPercent() >= 0.1f && !StaminaComp->IsExhausted();
}

bool APlayerCharacter::IsSprinting() const
{
    return bIsSprinting && GetCharacterMovement()->MaxWalkSpeed == SprintSpeed;
}

// === MOVEMENT FUNCTIONS ===

void APlayerCharacter::MoveCharacter(const FVector2D& MovementVector)
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

void APlayerCharacter::HandleMoveInput(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();
    MoveCharacter(MovementVector);
}

void APlayerCharacter::HandleTankMovement(const FVector2D& MovementVector)
{
    // Tank controls: forward/back movement, left/right rotation
    float ForwardInput = MovementVector.Y;
    float RotationInput = MovementVector.X;

    if (FMath::Abs(ForwardInput) > 0.0f)
    {
        const FRotator Rotation = GetActorRotation();
        const FVector Direction = FRotationMatrix(Rotation).GetUnitAxis(EAxis::X);
        AddMovementInput(Direction, ForwardInput);
    }

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

// === OTHER INPUT FUNCTIONS ===

void APlayerCharacter::HandleLookInput(const FVector2D& LookInput)
{
    if (Controller)
    {
        AddControllerYawInput(LookInput.X);
        AddControllerPitchInput(LookInput.Y);
    }
}

void APlayerCharacter::StartJump()
{
    Jump();
}

void APlayerCharacter::StopJump()
{
    StopJumping();
}

void APlayerCharacter::TryInteract()
{
    InteractWithTarget();
}

void APlayerCharacter::HandleZoomInput(const FInputActionValue& Value)
{
    float ScrollValue = Value.Get<float>();
    
    if (CurrentCameraMode == ECameraMode::ThirdPerson && FMath::Abs(ScrollValue) > 0.1f)
    {
        ToggleThirdPersonZoom();
    }
}

// === CAMERA MANAGEMENT ===

void APlayerCharacter::ToggleCameraMode()
{
    ECameraMode NewMode = (CurrentCameraMode == ECameraMode::FirstPerson) ? 
        ECameraMode::ThirdPerson : ECameraMode::FirstPerson;
    
    SetCameraMode(NewMode);
}

void APlayerCharacter::SetCameraMode(ECameraMode NewMode)
{
    if (CurrentCameraMode == NewMode)
        return;

    CurrentCameraMode = NewMode;
    bIsTransitioningCamera = true;
    CameraTransitionProgress = 0.0f;

    // Hide/show mesh based on camera mode
    if (GetMesh())
    {
        GetMesh()->SetOwnerNoSee(CurrentCameraMode == ECameraMode::FirstPerson);
    }

    OnCameraModeChanged.Broadcast(CurrentCameraMode);
    
    UE_LOG(LogTemp, Log, TEXT("Camera mode changed to %s"),
           CurrentCameraMode == ECameraMode::FirstPerson ? TEXT("First Person") : TEXT("Third Person"));
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

void APlayerCharacter::SetThirdPersonZoom(EThirdPersonZoom NewZoom)
{
    CurrentZoomLevel = NewZoom;
    UpdateThirdPersonCameraSettings();
}

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

    UpdateThirdPersonCameraSettings();

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

    float TargetDistance = (CurrentZoomLevel == EThirdPersonZoom::Close) ? CloseZoomDistance : FarZoomDistance;
    FVector TargetOffset = (CurrentZoomLevel == EThirdPersonZoom::Close) ? CloseZoomOffset : FarZoomOffset;

    ThirdPersonCameraBoom->TargetArmLength = FMath::FInterpTo(
        ThirdPersonCameraBoom->TargetArmLength, 
        TargetDistance, 
        GetWorld()->GetDeltaSeconds(), 
        5.0f
    );

    ThirdPersonCameraBoom->SocketOffset = FMath::VInterpTo(
        ThirdPersonCameraBoom->SocketOffset,
        TargetOffset,
        GetWorld()->GetDeltaSeconds(),
        5.0f
    );
}

// === INTERACTION SYSTEM ===

void APlayerCharacter::UpdateInteractionDetection()
{
    FVector TraceStart = GetInteractionTraceStart();
    FVector TraceDirection = GetInteractionTraceDirection();
    float TraceDistance = CalculateEffectiveInteractionDistance();
    FVector TraceEnd = TraceStart + (TraceDirection * TraceDistance);

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    QueryParams.bTraceComplex = false;

    bool bHit = GetWorld()->SweepSingleByChannel(
        HitResult,
        TraceStart,
        TraceEnd,
        FQuat::Identity,
        InteractionChannel,
        FCollisionShape::MakeSphere(InteractionSphereRadius),
        QueryParams
    );

    ProcessInteractionHit(bHit, HitResult);

    // Debug visualization
    if (bShowInteractionDebug || CVarDebugSprint.GetValueOnGameThread() > 0)
    {
        FColor DebugColor = bHit ? FColor::Green : FColor::Red;
        DrawDebugLine(GetWorld(), TraceStart, TraceEnd, DebugColor, false, -1.0f, 0, 2.0f);
        DrawDebugSphere(GetWorld(), TraceEnd, InteractionSphereRadius, 12, DebugColor, false, -1.0f);
        
        if (bHit)
        {
            DrawDebugSphere(GetWorld(), HitResult.Location, 20.0f, 12, FColor::Yellow, false, -1.0f);
        }
    }
}

void APlayerCharacter::ProcessInteractionHit(bool bHit, const FHitResult& HitResult)
{
    AActor* HitActor = bHit ? HitResult.GetActor() : nullptr;
    
    if (bHit && IsActorInteractable(HitActor))
    {
        if (CurrentInteractable != HitActor)
        {
            CurrentInteractable = HitActor;
            OnInteractableDetected.Broadcast(CurrentInteractable, true);
        }
    }
    else
    {
        if (CurrentInteractable)
        {
            OnInteractableDetected.Broadcast(CurrentInteractable, false);
            CurrentInteractable = nullptr;
        }
    }
}

void APlayerCharacter::InteractWithTarget()
{
    if (!CurrentInteractable)
        return;

    if (CurrentInteractable->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
    {
        // Get interaction point and normal for the interface call
        FVector InteractionPoint = CurrentInteractable->GetActorLocation();
        FVector InteractionNormal = (GetActorLocation() - CurrentInteractable->GetActorLocation()).GetSafeNormal();
        
        // Call the interface function with all required parameters
        IInteractableInterface::Execute_OnInteract(
            CurrentInteractable,           // Target object
            this,                         // Interacting character
            InteractionPoint,             // Point of interaction
            InteractionNormal             // Normal at interaction point
        );
    }
}

bool APlayerCharacter::IsActorInteractable(AActor* Actor) const
{
    if (!Actor)
        return false;

    return Actor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()) ||
           Actor->ActorHasTag(TEXT("Interactable"));
}

FVector APlayerCharacter::GetInteractionTraceStart() const
{
    if (CurrentCameraMode == ECameraMode::FirstPerson)
    {
        return FirstPersonCamera ? FirstPersonCamera->GetComponentLocation() : GetActorLocation() + FVector(0, 0, 50);
    }
    else
    {
        return ThirdPersonCamera ? ThirdPersonCamera->GetComponentLocation() : GetActorLocation() + FVector(0, 0, 100);
    }
}

FVector APlayerCharacter::GetInteractionTraceDirection() const
{
    if (Controller)
    {
        return Controller->GetControlRotation().Vector();
    }
    
    return GetActorForwardVector();
}

float APlayerCharacter::CalculateEffectiveInteractionDistance() const
{
    float Distance = BaseInteractionDistance;
    
    if (CurrentCameraMode == ECameraMode::ThirdPerson && ThirdPersonCameraBoom)
    {
        Distance += ThirdPersonCameraBoom->TargetArmLength;
    }
    
    return Distance;
}

// === SETTINGS ===

void APlayerCharacter::InitializePlayerComponents()
{
    if (FirstPersonCamera && GetMesh())
    {
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