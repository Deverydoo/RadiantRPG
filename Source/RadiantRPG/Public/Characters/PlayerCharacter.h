// Public/Characters/PlayerCharacter.h
// Updated player character header with improved camera positioning and interaction tracing

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
 * PlayerCharacter - Player-specific character class with improved camera and interaction systems
 * 
 * Responsibilities:
 * - Camera management (first/third person switching, zoom) with over-the-shoulder positioning
 * - Movement execution (receives processed input from controller)
 * - Interaction detection and tracing with camera distance compensation
 * - Player-specific UI integration
 * - Camera-based world interaction with proper third-person support
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
    
    /** Third person camera boom (positioned to the right for over-the-shoulder view) */
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
    
    /** Base interaction distance (compensated for camera distance in third person) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "50.0", ClampMax = "500.0"))
    float BaseInteractionDistance;

    /** Interaction trace sphere radius */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "5.0", ClampMax = "50.0"))
    float InteractionSphereRadius;

    /** Collision channel for interaction traces - using custom Interaction channel */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    TEnumAsByte<ECollisionChannel> InteractionChannel;

    /** Currently focused interactable */
    UPROPERTY(BlueprintReadOnly, Category = "Interaction")
    AActor* CurrentInteractable;

    /** Current interaction point in world space */
    UPROPERTY(BlueprintReadOnly, Category = "Interaction")
    FVector CurrentInteractionPoint;

    /** Current interaction surface normal */
    UPROPERTY(BlueprintReadOnly, Category = "Interaction")
    FVector CurrentInteractionNormal;

    // === DEBUG SETTINGS ===
    
    /** Show interaction debug visualization */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowInteractionDebug;

    // === MOVEMENT STATE ===
    
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
    
    /** Get current effective interaction distance (compensated for camera mode) */
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

    UFUNCTION(BlueprintCallable, Category = "Input")
    void HandleMoveInput(const FInputActionValue& Value);

    /** Handle look input from controller (with sensitivity already applied) */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void HandleLookInput(const FVector2D& LookInput);

    /** Try to interact with current target (alternative name for existing function) */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void TryInteract();

    /** Handle zoom input from mouse wheel */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void HandleZoomInput(const FInputActionValue& Value);

    /** Start sprinting (wrapper for existing StartSprint) */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void StartSprinting();

    /** Stop sprinting (wrapper for existing StopSprint) */
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
    void LogActiveStaminaActivities() const;

    // === CAMERA MANAGEMENT ===
    
    /** Update camera mode and settings */
    void UpdateCameraMode();

    /** Update third person camera settings */
    void UpdateThirdPersonCameraSettings();

    // === INTERACTION DETECTION ===
    
    /** Update interaction detection - called every tick with camera distance compensation */
    void UpdateInteractionDetection();

    /** Calculate effective interaction distance based on camera mode and position */
    float CalculateEffectiveInteractionDistance() const;

    /** Get trace start position based on camera mode with proper offset compensation */
    FVector GetInteractionTraceStart() const;

    /** Get trace direction based on camera mode with crosshair alignment */
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

    // === DEBUG CONSOLE VARIABLES ===
    #if WITH_EDITOR
    /** Console variable for interaction debug visualization */
    static class IConsoleVariable* CVarDebugInteraction;
    #endif
};