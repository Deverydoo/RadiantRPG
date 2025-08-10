// Public/Characters/PlayerCharacter.h
// Fixed player character header with sprint speed variables

#pragma once

#include "CoreMinimal.h"
#include "Characters/BaseCharacter.h"
#include "InputActionValue.h"
#include "Engine/EngineTypes.h"
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
 * - Movement execution with sprint support
 * - Interaction detection and tracing
 * - Player-specific UI integration
 * 
 * Note: Input processing is handled by RadiantPlayerController
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

    // === CAMERA COMPONENTS ===
    
    /** Third person camera boom positioning the camera behind the character */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    USpringArmComponent* ThirdPersonCameraBoom;

    /** Third person follow camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    UCameraComponent* ThirdPersonCamera;

    /** First person camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    UCameraComponent* FirstPersonCamera;

    // === CAMERA SETTINGS ===
    
    /** Current camera mode */
    UPROPERTY(BlueprintReadOnly, Category = "Camera")
    ECameraMode CurrentCameraMode;

    /** Current third person zoom level */
    UPROPERTY(BlueprintReadOnly, Category = "Camera")
    EThirdPersonZoom CurrentZoomLevel;

    /** Camera transition speed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
    float CameraTransitionSpeed;

    /** Third person camera distances */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
    float CloseZoomDistance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
    float FarZoomDistance;

    /** Third person camera offsets */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
    FVector CloseZoomOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
    FVector FarZoomOffset;

    /** First person camera offset from head socket */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
    FVector FirstPersonCameraOffset;

    // === INPUT SETTINGS ===
    
    /** Mouse look sensitivity */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Settings", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float MouseSensitivity;

    /** Gamepad look sensitivity */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Settings", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float GamepadSensitivity;

    /** Invert mouse Y axis */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Settings")
    bool bInvertMouseY;

    /** Use tank-style controls (Oblivion-style) instead of strafe controls */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Settings")
    bool bUseTankControls;

    /** Rotation speed for tank controls */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Settings")
    float RotationSpeed;

    // === INTERACTION SETTINGS ===
    
    /** Base interaction distance */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    float BaseInteractionDistance;

    /** Interaction sphere trace radius */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    float InteractionSphereRadius;

    /** Collision channel for interaction traces */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    TEnumAsByte<ECollisionChannel> InteractionChannel;

    /** Currently focused interactable actor */
    UPROPERTY(BlueprintReadOnly, Category = "Interaction")
    AActor* CurrentInteractable;

    /** Show interaction debug visualization */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowInteractionDebug;

    // === MOVEMENT SETTINGS ===
    
    /** Normal walking speed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "100", ClampMax = "600"))
    float NormalSpeed;

    /** Sprint speed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "300", ClampMax = "1200"))
    float SprintSpeed;
    
    /** Whether player is currently trying to sprint */
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bIsSprinting;

public:
    // === EVENTS ===
    
    /** Broadcast when camera mode changes */
    UPROPERTY(BlueprintAssignable, Category = "Camera")
    FOnCameraModeChanged OnCameraModeChanged;

    /** Broadcast when interactable detection state changes */
    UPROPERTY(BlueprintAssignable, Category = "Interaction")
    FOnInteractableDetected OnInteractableDetected;

    // === CAMERA INTERFACE ===
    
    /** Get the currently active camera component */
    UFUNCTION(BlueprintPure, Category = "Camera")
    UCameraComponent* GetActiveCamera() const;

    UFUNCTION(BlueprintCallable, Category = "Camera")
    void SetCameraMode(ECameraMode NewMode);

    UFUNCTION(BlueprintCallable, Category = "Camera")
    void SetThirdPersonZoom(EThirdPersonZoom NewZoom);

    UFUNCTION(BlueprintPure, Category = "Camera")
    ECameraMode GetCurrentCameraMode() const { return CurrentCameraMode; }

    UFUNCTION(BlueprintPure, Category = "Camera")
    EThirdPersonZoom GetCurrentZoomLevel() const { return CurrentZoomLevel; }

    // === MOVEMENT INTERFACE ===
    
    /** Move character based on input (called by PlayerController) */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void MoveCharacter(const FVector2D& MovementVector);

    /** Start sprinting */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void StartSprint();

    /** Stop sprinting */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void StopSprint();

    /** Start jumping */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void StartJump();

    /** Stop jumping */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void StopJump();

    // === INPUT INTERFACE ===
    
    /** Toggle between first and third person camera */
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void ToggleCameraMode();

    /** Toggle third person zoom level */
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void ToggleThirdPersonZoom();

    /** Interact with currently focused interactable */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void InteractWithTarget();

    // === INTERACTION INTERFACE ===
    
    /** Get current effective interaction distance */
    UFUNCTION(BlueprintPure, Category = "Interaction")
    float GetEffectiveInteractionDistance() const { return CalculateEffectiveInteractionDistance(); }

    /** Get current interactable if any */
    UFUNCTION(BlueprintPure, Category = "Interaction")
    AActor* GetCurrentInteractable() const { return CurrentInteractable; }

    /** Check if we can currently interact with something */
    UFUNCTION(BlueprintPure, Category = "Interaction")
    bool CanInteract() const { return CurrentInteractable != nullptr; }

    // === SETTINGS INTERFACE ===
    
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetMouseSensitivity(float NewSensitivity);

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetGamepadSensitivity(float NewSensitivity);

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetInvertMouseY(bool bInvert);

    // === INPUT HANDLING ===
    
    /** Handle movement input from controller */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void HandleMoveInput(const FInputActionValue& Value);

    /** Handle look input from controller */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void HandleLookInput(const FVector2D& LookInput);

    /** Try to interact with current target */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void TryInteract();

    /** Handle zoom input from mouse wheel */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void HandleZoomInput(const FInputActionValue& Value);

    /** Start sprinting (wrapper for controller) */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void StartSprinting();

    /** Stop sprinting (wrapper for controller) */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void StopSprinting();

    /** Check if character can currently sprint */
    UFUNCTION(BlueprintPure, Category = "Movement")
    bool CanSprint() const;

    /** Check if character is currently sprinting */
    UFUNCTION(BlueprintPure, Category = "Movement")
    bool IsSprinting() const;

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

    /** Calculate effective interaction distance based on camera mode */
    float CalculateEffectiveInteractionDistance() const;

    /** Get trace start position based on camera mode */
    FVector GetInteractionTraceStart() const;

    /** Get trace direction based on camera mode */
    FVector GetInteractionTraceDirection() const;

    /** Process hit result from interaction trace */
    void ProcessInteractionHit(bool bHit, const FHitResult& HitResult);

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