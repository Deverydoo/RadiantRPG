// Public/Characters/PlayerCharacter.h
// Updated player character header with proper collision setup

#pragma once

#include "CoreMinimal.h"
#include "Characters/BaseCharacter.h"
#include "InputActionValue.h"
#include "Engine/EngineTypes.h" // For collision enums
#include "PlayerCharacter.generated.h"

// Forward declarations
class USpringArmComponent;
class UCameraComponent;

UENUM(BlueprintType)
enum class ECameraMode : uint8
{
    ThirdPerson UMETA(DisplayName = "Third Person"),
    FirstPerson UMETA(DisplayName = "First Person")
};

UENUM(BlueprintType)
enum class EThirdPersonZoom : uint8
{
    Close UMETA(DisplayName = "Close"),
    Far UMETA(DisplayName = "Far")
};

// Player-specific events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCameraModeChanged, ECameraMode, NewCameraMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractableDetected, AActor*, InteractableActor, bool, bIsInRange);

/**
 * PlayerCharacter - Player-specific character class
 * 
 * Responsibilities:
 * - Camera management (first/third person switching, zoom)
 * - Movement execution (receives processed input from controller)
 * - Interaction detection and tracing with proper collision channels
 * - Player-specific UI integration
 * - Camera-based world interaction
 * 
 * Note: This class handles movement/action EXECUTION only.
 * Input processing is handled by RadiantPlayerController.
 */
UCLASS(Blueprintable)
class RADIANTRPG_API APlayerCharacter : public ABaseCharacter
{
    GENERATED_BODY()

public:
    APlayerCharacter();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // NOTE: SetupPlayerInputComponent removed - handled by PlayerController

    // === CAMERA COMPONENTS ===
    
    /** Third person camera boom */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    class USpringArmComponent* ThirdPersonCameraBoom;

    /** Third person camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    class UCameraComponent* ThirdPersonCamera;

    /** First person camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    class UCameraComponent* FirstPersonCamera;

    // === CAMERA SETTINGS ===
    
    /** Current camera mode */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    ECameraMode CurrentCameraMode;

    /** Current zoom level for third person */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    EThirdPersonZoom CurrentZoomLevel;

    /** Camera transition speed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float CameraTransitionSpeed;

    /** Third person close distance */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "100.0", ClampMax = "800.0"))
    float ThirdPersonCloseDistance;

    /** Third person far distance */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "200.0", ClampMax = "1200.0"))
    float ThirdPersonFarDistance;

    /** Camera lag speed for third person */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float ThirdPersonCameraLag;

    /** First person camera offset from head socket */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    FVector FirstPersonCameraOffset;

    // === MOVEMENT SETTINGS ===
    
    /** Mouse sensitivity */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float MouseSensitivity;

    /** Gamepad sensitivity */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float GamepadSensitivity;

    /** Whether to invert mouse Y axis */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    bool bInvertMouseY;

    /** Whether to use tank controls (like Oblivion) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    bool bUseTankControls;

    /** Rotation speed for tank controls */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float RotationSpeed;

    // === INTERACTION SETTINGS ===
    
    /** Base interaction distance */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "50.0", ClampMax = "500.0"))
    float BaseInteractionDistance;

    /** Interaction trace sphere radius */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "5.0", ClampMax = "50.0"))
    float InteractionSphereRadius;

    /** Collision channel for interaction traces - using custom Interaction channel */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    TEnumAsByte<ECollisionChannel> InteractionTraceChannel;

    /** Whether to show debug traces for interaction */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    bool bShowInteractionDebug;

    // === CURRENT STATE ===
    
    /** Currently detected interactable */
    UPROPERTY(BlueprintReadOnly, Category = "Interaction")
    AActor* CurrentInteractable;

    /** Last interaction point */
    UPROPERTY(BlueprintReadOnly, Category = "Interaction")
    FVector CurrentInteractionPoint;

    /** Last interaction normal */
    UPROPERTY(BlueprintReadOnly, Category = "Interaction")
    FVector CurrentInteractionNormal;

    /** Whether player is currently sprinting */
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bIsSprinting;

public:
    bool IsSprinting() const { return bIsSprinting; }

protected:
    /** Current movement input */
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    FVector2D MovementInput;

    /** Current look input */
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    FVector2D LookInput;

public:
    // === EVENTS ===
    
    UPROPERTY(BlueprintAssignable, Category = "Player Events")
    FOnCameraModeChanged OnCameraModeChanged;

    UPROPERTY(BlueprintAssignable, Category = "Player Events")
    FOnInteractableDetected OnInteractableDetected;

    // === INPUT PROCESSING FUNCTIONS (Called by PlayerController) ===
    
    /** Process movement input from controller */
    UFUNCTION(BlueprintCallable, Category = "Input Processing")
    void HandleMoveInput(const FInputActionValue& Value);

    /** Process look input from controller (pre-processed with sensitivity) */
    UFUNCTION(BlueprintCallable, Category = "Input Processing")
    void HandleLookInput(const FVector2D& ProcessedLookInput);

    /** Process zoom input from controller */
    UFUNCTION(BlueprintCallable, Category = "Input Processing")
    void HandleZoomInput(const FInputActionValue& Value);

    // === CAMERA INTERFACE ===
    
    /** Toggle between first and third person */
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void ToggleCameraMode();

    /** Set specific camera mode */
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void SetCameraMode(ECameraMode NewMode);

    /** Cycle zoom level in third person */
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void CycleZoomLevel();

    /** Set specific zoom level */
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void SetZoomLevel(EThirdPersonZoom NewZoom);

    /** Get current camera mode */
    UFUNCTION(BlueprintPure, Category = "Camera")
    ECameraMode GetCurrentCameraMode() const { return CurrentCameraMode; }

    /** Get currently active camera */
    UFUNCTION(BlueprintPure, Category = "Camera")
    UCameraComponent* GetActiveCamera() const;

    // === INTERACTION INTERFACE ===
    
    /** Try to interact with current interactable */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void TryInteract();

    /** Get current interactable */
    UFUNCTION(BlueprintPure, Category = "Interaction")
    AActor* GetCurrentInteractable() const { return CurrentInteractable; }

    // === MOVEMENT INTERFACE ===
    
    /** Start sprinting */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void StartSprinting();

    /** Stop sprinting */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void StopSprinting();

    /** Check if can sprint */
    UFUNCTION(BlueprintPure, Category = "Movement")
    bool CanSprint() const;

    // === SETTINGS ===
    
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetMouseSensitivity(float NewSensitivity);

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetGamepadSensitivity(float NewSensitivity);

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetInvertMouseY(bool bInvert);

protected:
    // === MOVEMENT PROCESSING ===
    
    /** Handle tank-style movement (like Oblivion) */
    void HandleTankMovement(const FVector2D& MovementVector);

    /** Handle modern movement (strafe-style) */
    void HandleModernMovement(const FVector2D& MovementVector);

    /** Update sprint state based on stamina */
    void UpdateSprintState();

    // === CAMERA MANAGEMENT ===
    
    /** Update camera mode and settings */
    void UpdateCameraMode();

    /** Update third person camera settings */
    void UpdateThirdPersonCameraSettings();

    // === INTERACTION DETECTION ===
    
    /** Update interaction detection - called every tick */
    void UpdateInteractionDetection();

    /** Get trace start position based on camera mode */
    FVector GetInteractionTraceStart() const;

    /** Get trace direction based on camera mode */
    FVector GetInteractionTraceDirection() const;

    /** Check if actor is interactable */
    bool IsActorInteractable(AActor* Actor) const;

    // === INITIALIZATION ===
    
    /** Initialize player-specific components */
    virtual void InitializePlayerComponents();

private:
    // === CACHED VALUES FOR PERFORMANCE ===
    
    /** Cached control rotation to avoid repeated calls */
    FRotator CachedControlRotation;

    /** Last time control rotation was updated */
    float LastControlRotationUpdate;

    /** Camera transition progress (0-1) */
    float CameraTransitionProgress;

    /** Whether camera is currently transitioning */
    bool bIsTransitioningCamera;
};