// Public/Controllers/RadiantPlayerController.h
// Updated player controller header with Enhanced Input Actions

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "RadiantPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class URadiantHUD;
class APlayerCharacter;

UENUM(BlueprintType)
enum class EInputMode : uint8
{
    GameOnly UMETA(DisplayName = "Game Only"),
    UIOnly UMETA(DisplayName = "UI Only"),
    GameAndUI UMETA(DisplayName = "Game and UI")
};

// Input-related events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInputModeChanged, EInputMode, NewInputMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnControllerPossessedPlayer, APlayerCharacter*, PossessedPlayer);

/**
 * RadiantPlayerController - The Player's "Brain"
 * 
 * Responsibilities:
 * - Enhanced Input Action binding and processing
 * - Input mapping context management and switching
 * - HUD creation and management  
 * - Input mode switching (Game/UI/Both)
 * - Player settings management
 * - Communication bridge between input and character actions
 * - Save/Load player preferences
 * 
 * This class handles ALL input processing before delegating to character
 */
UCLASS(Blueprintable)
class RADIANTRPG_API ARadiantPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ARadiantPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void SetupInputComponent() override;

    // === ENHANCED INPUT ACTIONS ===
    
    /** Movement input action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input", meta = (AllowPrivateAccess = "true"))
    class UInputAction* MoveAction;

    /** Look input action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input", meta = (AllowPrivateAccess = "true"))
    class UInputAction* LookAction;

    /** Jump input action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input", meta = (AllowPrivateAccess = "true"))
    class UInputAction* JumpAction;

    /** Toggle camera mode input action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input", meta = (AllowPrivateAccess = "true"))
    class UInputAction* ToggleCameraAction;

    /** Interact input action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input", meta = (AllowPrivateAccess = "true"))
    class UInputAction* InteractAction;

    /** Zoom input action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input", meta = (AllowPrivateAccess = "true"))
    class UInputAction* ZoomAction;

    /** Sprint input action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input", meta = (AllowPrivateAccess = "true"))
    class UInputAction* SprintAction;

    // === INPUT MAPPING CONTEXTS ===
    
    /** Default input mapping contexts to add on possess */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    TArray<UInputMappingContext*> DefaultMappingContexts;

    /** UI-specific input mapping contexts */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    TArray<UInputMappingContext*> UIMappingContexts;

    /** Combat-specific input mapping contexts */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    TArray<UInputMappingContext*> CombatMappingContexts;

    // === HUD MANAGEMENT ===
    
    /** HUD widget class to create */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<URadiantHUD> HUDWidgetClass;

    /** Current HUD widget instance */
    UPROPERTY(BlueprintReadOnly, Category = "UI")
    URadiantHUD* HUDWidget;

    // === INPUT SETTINGS ===
    
    /** Current input mode */
    UPROPERTY(BlueprintReadOnly, Category = "Input")
    EInputMode CurrentInputMode;

    /** Player input settings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Settings")
    float MouseSensitivity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Settings")
    float GamepadSensitivity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Settings")
    bool bInvertMouseY;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Settings")
    bool bInvertGamepadY;

    // === ACCESSIBILITY SETTINGS ===
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accessibility")
    bool bShowSubtitles;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accessibility")
    float UIScale;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accessibility")
    bool bColorBlindSupport;

    // === CACHED REFERENCES ===
    
    /** Cached reference to possessed player character */
    UPROPERTY(BlueprintReadOnly, Category = "Player")
    APlayerCharacter* PossessedPlayerCharacter;

public:
    // === EVENTS ===
    
    UPROPERTY(BlueprintAssignable, Category = "Controller Events")
    FOnInputModeChanged OnInputModeChanged;

    UPROPERTY(BlueprintAssignable, Category = "Controller Events")
    FOnControllerPossessedPlayer OnControllerPossessedPlayer;

    // === ENHANCED INPUT ACTION HANDLERS ===
    
    /** Handle movement input and delegate to character */
    void HandleMove(const FInputActionValue& Value);

    /** Handle look input with sensitivity applied */
    void HandleLook(const FInputActionValue& Value);

    /** Handle jump start */
    void HandleJumpStart();

    /** Handle jump stop */
    void HandleJumpStop();

    /** Handle camera toggle */
    void HandleToggleCamera();

    /** Handle interaction */
    void HandleInteract();

    /** Handle zoom input */
    void HandleZoom(const FInputActionValue& Value);

    /** Handle sprint start */
    void HandleSprintStart();

    /** Handle sprint stop */
    void HandleSprintStop();

    // === INPUT MODE MANAGEMENT ===
    
    /** Set input mode (Game/UI/Both) */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void SetInputMode(EInputMode NewInputMode);

    /** Get current input mode */
    UFUNCTION(BlueprintPure, Category = "Input")
    EInputMode GetInputMode() const { return CurrentInputMode; }

    /** Add input mapping context */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void AddInputMappingContext(UInputMappingContext* MappingContext, int32 Priority = 0);

    /** Remove input mapping context */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void RemoveInputMappingContext(UInputMappingContext* MappingContext);

    /** Clear all input mapping contexts */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void ClearAllInputMappingContexts();

    // === HUD INTERFACE ===
    
    /** Get current HUD widget */
    UFUNCTION(BlueprintPure, Category = "UI")
    URadiantHUD* GetHUDWidget() const { return HUDWidget; }

    /** Create and show HUD */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void CreateHUD();

    /** Hide HUD */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void HideHUD();

    /** Show HUD */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ShowHUD();

    /** Toggle HUD visibility */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ToggleHUD();

    // === PLAYER SETTINGS ===
    
    /** Apply input settings to possessed character */
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void ApplyInputSettings();

    /** Save player settings */
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SavePlayerSettings();

    /** Load player settings */
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void LoadPlayerSettings();

    // === INPUT SETTINGS INTERFACE ===
    
    UFUNCTION(BlueprintCallable, Category = "Input Settings")
    void SetMouseSensitivity(float NewSensitivity);

    UFUNCTION(BlueprintCallable, Category = "Input Settings")
    void SetGamepadSensitivity(float NewSensitivity);

    UFUNCTION(BlueprintCallable, Category = "Input Settings")
    void SetInvertMouseY(bool bInvert);

    UFUNCTION(BlueprintCallable, Category = "Input Settings")
    void SetInvertGamepadY(bool bInvert);

    // === ACCESSIBILITY INTERFACE ===
    
    UFUNCTION(BlueprintCallable, Category = "Accessibility")
    void SetUIScale(float NewScale);

    UFUNCTION(BlueprintCallable, Category = "Accessibility")
    void SetSubtitlesEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Accessibility")
    void SetColorBlindSupport(bool bEnabled);

    // === PLAYER CHARACTER INTERFACE ===
    
    /** Get possessed player character */
    UFUNCTION(BlueprintPure, Category = "Player")
    APlayerCharacter* GetPlayerCharacter() const { return PossessedPlayerCharacter; }

    /** Send command to player character */
    UFUNCTION(BlueprintCallable, Category = "Player")
    void SendPlayerCommand(const FString& Command, const FString& Parameters = "");

protected:
    // === INITIALIZATION ===
    
    /** Initialize input mapping contexts */
    void InitializeInputMappingContexts();

    /** Initialize HUD system */
    void InitializeHUD();

    /** Bind to player character events */
    void BindToPlayerCharacterEvents();

    // === INPUT CONTEXT MANAGEMENT ===
    
    /** Add default input contexts */
    void AddDefaultInputContexts();

    /** Add UI input contexts */
    void AddUIInputContexts();

    /** Remove UI input contexts */
    void RemoveUIInputContexts();

    /** Add combat input contexts */
    void AddCombatInputContexts();

    /** Remove combat input contexts */
    void RemoveCombatInputContexts();

    // === EVENT HANDLERS ===
    
    /** Called when player character changes camera mode */
    UFUNCTION()
    void OnPlayerCameraModeChanged(ECameraMode NewCameraMode);

    /** Called when player detects interactable */
    UFUNCTION()
    void OnPlayerInteractableDetected(AActor* InteractableActor, bool bIsInRange);

    /** Called when player character dies */
    UFUNCTION()
    void OnPlayerCharacterDeath(ABaseCharacter* DeadCharacter);

    // === BLUEPRINT EVENTS ===
    
    /** Called when input mode changes */
    UFUNCTION(BlueprintImplementableEvent, Category = "Controller Events")
    void OnInputModeChangedBP(EInputMode OldMode, EInputMode NewMode);

    /** Called when HUD is created */
    UFUNCTION(BlueprintImplementableEvent, Category = "UI Events")
    void OnHUDCreatedBP(URadiantHUD* CreatedHUD);

    /** Called when player settings are loaded */
    UFUNCTION(BlueprintImplementableEvent, Category = "Settings Events")
    void OnPlayerSettingsLoadedBP();

    /** Called when player settings are saved */
    UFUNCTION(BlueprintImplementableEvent, Category = "Settings Events")
    void OnPlayerSettingsSavedBP();

private:
    // === INTERNAL STATE ===
    
    /** Whether HUD has been initialized */
    bool bHUDInitialized;

    /** Whether input contexts have been initialized */
    bool bInputContextsInitialized;

    /** Previous input mode for change detection */
    EInputMode PreviousInputMode;

    // === SETTINGS CACHE ===
    
    /** Cached settings for performance */
    struct FPlayerSettingsCache
    {
        float CachedMouseSensitivity;
        float CachedGamepadSensitivity;
        bool bCachedInvertMouseY;
        bool bCachedInvertGamepadY;
        float CachedUIScale;
        bool bCachedSubtitles;
        bool bCachedColorBlindSupport;
        
        FPlayerSettingsCache()
        {
            CachedMouseSensitivity = 1.0f;
            CachedGamepadSensitivity = 1.0f;
            bCachedInvertMouseY = false;
            bCachedInvertGamepadY = false;
            CachedUIScale = 1.0f;
            bCachedSubtitles = false;
            bCachedColorBlindSupport = false;
        }
    } SettingsCache;
};