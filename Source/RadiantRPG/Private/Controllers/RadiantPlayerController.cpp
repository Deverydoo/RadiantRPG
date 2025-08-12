// Private/Controllers/RadiantPlayerController.cpp
// Updated player controller implementation with proper Enhanced Input handling

#include "Controllers/RadiantPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

// Game-specific includes
#include "Characters/PlayerCharacter.h"
#include "UI/RadiantHUD.h"

ARadiantPlayerController::ARadiantPlayerController()
{
    // Initialize default values
    MouseSensitivity = 1.0f;
    GamepadSensitivity = 1.0f;
    bInvertMouseY = false;
    bInvertGamepadY = false;
    
    // Accessibility defaults
    bShowSubtitles = false;
    UIScale = 1.0f;
    bColorBlindSupport = false;
    
    // Initialize state
    CurrentInputMode = EInputMode::GameOnly;
    PreviousInputMode = EInputMode::GameOnly;
    PossessedPlayerCharacter = nullptr;
    HUDWidget = nullptr;
    
    bHUDInitialized = false;
    bInputContextsInitialized = false;
    
    // Initialize settings cache
    SettingsCache = FPlayerSettingsCache();
}

void ARadiantPlayerController::BeginPlay()
{
    Super::BeginPlay();
    
    // Load player settings from save file or defaults
    LoadPlayerSettings();
    
    UE_LOG(LogTemp, Log, TEXT("RadiantPlayerController initialized"));
}

void ARadiantPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    
    // Cache player character reference
    PossessedPlayerCharacter = Cast<APlayerCharacter>(InPawn);
    
    if (PossessedPlayerCharacter)
    {
        // Initialize input mapping contexts
        InitializeInputMappingContexts();
        
        // Initialize HUD
        InitializeHUD();
        
        // Apply settings to character
        ApplyInputSettings();
        
        // Bind to character events
        BindToPlayerCharacterEvents();
        
        // Broadcast possession event
        OnControllerPossessedPlayer.Broadcast(PossessedPlayerCharacter);
        
        UE_LOG(LogTemp, Log, TEXT("RadiantPlayerController possessed PlayerCharacter: %s"), 
               *PossessedPlayerCharacter->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("RadiantPlayerController possessed non-PlayerCharacter pawn"));
    }
}

void ARadiantPlayerController::OnUnPossess()
{
    if (PossessedPlayerCharacter)
    {
        UE_LOG(LogTemp, Log, TEXT("RadiantPlayerController unpossessing PlayerCharacter: %s"), 
               *PossessedPlayerCharacter->GetName());
    }
    
    // Clear cached reference
    PossessedPlayerCharacter = nullptr;
    
    // Clear input contexts
    ClearAllInputMappingContexts();
    
    Super::OnUnPossess();
}

void ARadiantPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    
    // Enhanced Input setup - bind actions to controller functions
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
    {
        // Movement
        if (MoveAction)
        {
            EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARadiantPlayerController::HandleMove);
        }

        // Looking
        if (LookAction)
        {
            EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ARadiantPlayerController::HandleLook);
        }

        // Jumping
        if (JumpAction)
        {
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ARadiantPlayerController::HandleJumpStart);
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ARadiantPlayerController::HandleJumpStop);
        }

        // Camera toggle
        if (ToggleCameraAction)
        {
            EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Started, this, &ARadiantPlayerController::HandleToggleCamera);
        }

        // Interaction
        if (InteractAction)
        {
            EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ARadiantPlayerController::HandleInteract);
        }

        // Zoom
        if (ZoomAction)
        {
            EnhancedInputComponent->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &ARadiantPlayerController::HandleZoom);
        }

        // Sprint
        if (SprintAction)
        {
            EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ARadiantPlayerController::HandleSprintStart);
            EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ARadiantPlayerController::HandleSprintStop);
        }
        
        UE_LOG(LogTemp, Log, TEXT("Enhanced Input actions bound to controller"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("RadiantPlayerController: Enhanced Input component not found!"));
    }
}

// Enhanced Input Action Handlers
void ARadiantPlayerController::HandleMove(const FInputActionValue& Value)
{
    if (PossessedPlayerCharacter)
    {
        PossessedPlayerCharacter->HandleMoveInput(Value);
    }
}

void ARadiantPlayerController::HandleLook(const FInputActionValue& Value)
{
    if (PossessedPlayerCharacter)
    {
        // Apply sensitivity and invert settings at controller level
        FVector2D LookInput = Value.Get<FVector2D>();
        
        // Apply sensitivity
        LookInput.X *= MouseSensitivity;
        LookInput.Y *= MouseSensitivity;
        
        // Apply invert Y if enabled
        if (bInvertMouseY)
        {
            LookInput.Y *= -1.0f;
        }
        
        // Pass processed input to character
        PossessedPlayerCharacter->HandleLookInput(LookInput);
    }
}

void ARadiantPlayerController::HandleJumpStart()
{
    if (PossessedPlayerCharacter)
    {
        PossessedPlayerCharacter->Jump();
    }
}

void ARadiantPlayerController::HandleJumpStop()
{
    if (PossessedPlayerCharacter)
    {
        PossessedPlayerCharacter->StopJumping();
    }
}

void ARadiantPlayerController::HandleToggleCamera()
{
    if (PossessedPlayerCharacter)
    {
        PossessedPlayerCharacter->ToggleCameraMode();
    }
}

void ARadiantPlayerController::HandleInteract()
{
    if (PossessedPlayerCharacter)
    {
        PossessedPlayerCharacter->TryInteract();
    }
}

void ARadiantPlayerController::HandleZoom(const FInputActionValue& Value)
{
    if (PossessedPlayerCharacter)
    {
        PossessedPlayerCharacter->HandleZoomInput(Value);
    }
}

void ARadiantPlayerController::HandleSprintStart()
{
    if (PossessedPlayerCharacter)
    {
        PossessedPlayerCharacter->StartSprinting();
    }
}

void ARadiantPlayerController::HandleSprintStop()
{
    if (PossessedPlayerCharacter)
    {
        PossessedPlayerCharacter->StopSprinting();
    }
}

void ARadiantPlayerController::InitializeInputMappingContexts()
{
    if (bInputContextsInitialized)
        return;
    
    // Add default input mapping contexts
    AddDefaultInputContexts();
    
    bInputContextsInitialized = true;
    
    UE_LOG(LogTemp, Log, TEXT("Input mapping contexts initialized"));
}

void ARadiantPlayerController::InitializeHUD()
{
    if (bHUDInitialized || !HUDWidgetClass)
        return;
    
    CreateHUD();
    bHUDInitialized = true;
    
    UE_LOG(LogTemp, Log, TEXT("HUD system initialized"));
}

void ARadiantPlayerController::BindToPlayerCharacterEvents()
{
    if (!PossessedPlayerCharacter)
        return;
    
    // Bind to camera mode changes
    PossessedPlayerCharacter->OnCameraModeChanged.AddDynamic(this, &ARadiantPlayerController::OnPlayerCameraModeChanged);
    
    // Bind to interaction detection
    PossessedPlayerCharacter->OnInteractableDetected.AddDynamic(this, &ARadiantPlayerController::OnPlayerInteractableDetected);
    
    // Bind to death event
    PossessedPlayerCharacter->OnCharacterDeath.AddDynamic(this, &ARadiantPlayerController::OnPlayerCharacterDeath);
    
    UE_LOG(LogTemp, Log, TEXT("Bound to player character events"));
}

void ARadiantPlayerController::SetRadiantInputMode(EInputMode NewInputMode)
{
    if (CurrentInputMode == NewInputMode)
        return;
    
    PreviousInputMode = CurrentInputMode;
    CurrentInputMode = NewInputMode;
    
    // Update Unreal's input mode based on our enum
    switch (CurrentInputMode)
    {
        case EInputMode::GameOnly:
        {
            FInputModeGameOnly InputMode;
            Super::SetInputMode(InputMode);
            RemoveUIInputContexts();
            break;
        }
        case EInputMode::UIOnly:
        {
            FInputModeUIOnly InputMode;
            Super::SetInputMode(InputMode);
            AddUIInputContexts();
            break;
        }
        case EInputMode::GameAndUI:
        {
            FInputModeGameAndUI InputMode;
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            Super::SetInputMode(InputMode);
            AddUIInputContexts();
            break;
        }
    }
    
    // Update cursor visibility
    bShowMouseCursor = (CurrentInputMode != EInputMode::GameOnly);
    
    // Broadcast change event
    OnInputModeChanged.Broadcast(CurrentInputMode);
    OnInputModeChangedBP(PreviousInputMode, CurrentInputMode);
    
    UE_LOG(LogTemp, Log, TEXT("Input mode changed from %s to %s"),
           *UEnum::GetValueAsString(PreviousInputMode),
           *UEnum::GetValueAsString(CurrentInputMode));
}

void ARadiantPlayerController::AddInputMappingContext(UInputMappingContext* MappingContext, int32 Priority)
{
    if (!MappingContext)
        return;
    
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        Subsystem->AddMappingContext(MappingContext, Priority);
        UE_LOG(LogTemp, Log, TEXT("Added input mapping context: %s (Priority: %d)"), 
               *MappingContext->GetName(), Priority);
    }
}

void ARadiantPlayerController::RemoveInputMappingContext(UInputMappingContext* MappingContext)
{
    if (!MappingContext)
        return;
    
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        Subsystem->RemoveMappingContext(MappingContext);
        UE_LOG(LogTemp, Log, TEXT("Removed input mapping context: %s"), *MappingContext->GetName());
    }
}

void ARadiantPlayerController::ClearAllInputMappingContexts()
{
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        Subsystem->ClearAllMappings();
        UE_LOG(LogTemp, Log, TEXT("Cleared all input mapping contexts"));
    }
    
    bInputContextsInitialized = false;
}

void ARadiantPlayerController::AddDefaultInputContexts()
{
    for (UInputMappingContext* Context : DefaultMappingContexts)
    {
        AddInputMappingContext(Context, 0);
    }
}

void ARadiantPlayerController::AddUIInputContexts()
{
    for (UInputMappingContext* Context : UIMappingContexts)
    {
        AddInputMappingContext(Context, 10); // Higher priority for UI
    }
}

void ARadiantPlayerController::RemoveUIInputContexts()
{
    for (UInputMappingContext* Context : UIMappingContexts)
    {
        RemoveInputMappingContext(Context);
    }
}

void ARadiantPlayerController::AddCombatInputContexts()
{
    for (UInputMappingContext* Context : CombatMappingContexts)
    {
        AddInputMappingContext(Context, 5); // Medium priority for combat
    }
}

void ARadiantPlayerController::RemoveCombatInputContexts()
{
    for (UInputMappingContext* Context : CombatMappingContexts)
    {
        RemoveInputMappingContext(Context);
    }
}

void ARadiantPlayerController::CreateHUD()
{
    if (HUDWidget || !HUDWidgetClass)
        return;
    
    HUDWidget = CreateWidget<URadiantHUD>(this, HUDWidgetClass);
    if (HUDWidget)
    {
        HUDWidget->AddToViewport();
        
        // Initialize HUD with player character
        if (PossessedPlayerCharacter)
        {
            HUDWidget->InitializeHUD(PossessedPlayerCharacter);
        }
        
        OnHUDCreatedBP(HUDWidget);
        
        UE_LOG(LogTemp, Log, TEXT("HUD widget created and added to viewport"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create HUD widget"));
    }
}

void ARadiantPlayerController::HideHUD()
{
    if (HUDWidget)
    {
        HUDWidget->SetVisibility(ESlateVisibility::Hidden);
        UE_LOG(LogTemp, Log, TEXT("HUD hidden"));
    }
}

void ARadiantPlayerController::ShowHUD()
{
    if (HUDWidget)
    {
        HUDWidget->SetVisibility(ESlateVisibility::Visible);
        UE_LOG(LogTemp, Log, TEXT("HUD shown"));
    }
}

void ARadiantPlayerController::ToggleHUD()
{
    if (HUDWidget)
    {
        if (HUDWidget->GetVisibility() == ESlateVisibility::Visible)
        {
            HideHUD();
        }
        else
        {
            ShowHUD();
        }
    }
}

void ARadiantPlayerController::ApplyInputSettings()
{
    if (!PossessedPlayerCharacter)
        return;
    
    // Apply cached settings to character
    PossessedPlayerCharacter->SetMouseSensitivity(SettingsCache.CachedMouseSensitivity);
    PossessedPlayerCharacter->SetGamepadSensitivity(SettingsCache.CachedGamepadSensitivity);
    PossessedPlayerCharacter->SetInvertMouseY(SettingsCache.bCachedInvertMouseY);
    
    UE_LOG(LogTemp, Log, TEXT("Applied input settings to player character"));
}

void ARadiantPlayerController::SavePlayerSettings()
{
    // Update settings cache with current values
    SettingsCache.CachedMouseSensitivity = MouseSensitivity;
    SettingsCache.CachedGamepadSensitivity = GamepadSensitivity;
    SettingsCache.bCachedInvertMouseY = bInvertMouseY;
    SettingsCache.bCachedInvertGamepadY = bInvertGamepadY;
    SettingsCache.CachedUIScale = UIScale;
    SettingsCache.bCachedSubtitles = bShowSubtitles;
    SettingsCache.bCachedColorBlindSupport = bColorBlindSupport;
    
    // TODO: Implement actual save to file/registry
    // For now, just log that we would save
    UE_LOG(LogTemp, Log, TEXT("Player settings saved"));
    OnPlayerSettingsSavedBP();
}

void ARadiantPlayerController::LoadPlayerSettings()
{
    // TODO: Implement actual load from file/registry
    // For now, use defaults and log
    
    MouseSensitivity = SettingsCache.CachedMouseSensitivity;
    GamepadSensitivity = SettingsCache.CachedGamepadSensitivity;
    bInvertMouseY = SettingsCache.bCachedInvertMouseY;
    bInvertGamepadY = SettingsCache.bCachedInvertGamepadY;
    UIScale = SettingsCache.CachedUIScale;
    bShowSubtitles = SettingsCache.bCachedSubtitles;
    bColorBlindSupport = SettingsCache.bCachedColorBlindSupport;
    
    UE_LOG(LogTemp, Log, TEXT("Player settings loaded"));
    OnPlayerSettingsLoadedBP();
}

void ARadiantPlayerController::SetMouseSensitivity(float NewSensitivity)
{
    MouseSensitivity = FMath::Clamp(NewSensitivity, 0.1f, 5.0f);
    SettingsCache.CachedMouseSensitivity = MouseSensitivity;
    
    if (PossessedPlayerCharacter)
    {
        PossessedPlayerCharacter->SetMouseSensitivity(MouseSensitivity);
    }
    
    UE_LOG(LogTemp, Log, TEXT("Mouse sensitivity set to: %.2f"), MouseSensitivity);
}

void ARadiantPlayerController::SetGamepadSensitivity(float NewSensitivity)
{
    GamepadSensitivity = FMath::Clamp(NewSensitivity, 0.1f, 5.0f);
    SettingsCache.CachedGamepadSensitivity = GamepadSensitivity;
    
    if (PossessedPlayerCharacter)
    {
        PossessedPlayerCharacter->SetGamepadSensitivity(GamepadSensitivity);
    }
    
    UE_LOG(LogTemp, Log, TEXT("Gamepad sensitivity set to: %.2f"), GamepadSensitivity);
}

void ARadiantPlayerController::SetInvertMouseY(bool bInvert)
{
    bInvertMouseY = bInvert;
    SettingsCache.bCachedInvertMouseY = bInvertMouseY;
    
    if (PossessedPlayerCharacter)
    {
        PossessedPlayerCharacter->SetInvertMouseY(bInvertMouseY);
    }
    
    UE_LOG(LogTemp, Log, TEXT("Mouse Y invert set to: %s"), bInvertMouseY ? TEXT("true") : TEXT("false"));
}

void ARadiantPlayerController::SetInvertGamepadY(bool bInvert)
{
    bInvertGamepadY = bInvert;
    SettingsCache.bCachedInvertGamepadY = bInvertGamepadY;
    
    // TODO: Apply to gamepad input when implemented
    
    UE_LOG(LogTemp, Log, TEXT("Gamepad Y invert set to: %s"), bInvertGamepadY ? TEXT("true") : TEXT("false"));
}

void ARadiantPlayerController::SetUIScale(float NewScale)
{
    UIScale = FMath::Clamp(NewScale, 0.5f, 2.0f);
    SettingsCache.CachedUIScale = UIScale;
    
    // TODO: Apply UI scale to HUD and other UI elements
    if (HUDWidget)
    {
        // Apply scale to HUD widget
        FVector2D Scale = FVector2D(UIScale, UIScale);
        HUDWidget->SetRenderScale(Scale);
    }
    
    UE_LOG(LogTemp, Log, TEXT("UI scale set to: %.2f"), UIScale);
}

void ARadiantPlayerController::SetSubtitlesEnabled(bool bEnabled)
{
    bShowSubtitles = bEnabled;
    SettingsCache.bCachedSubtitles = bShowSubtitles;
    
    // TODO: Apply subtitle settings to audio/dialogue system
    
    UE_LOG(LogTemp, Log, TEXT("Subtitles enabled: %s"), bShowSubtitles ? TEXT("true") : TEXT("false"));
}

void ARadiantPlayerController::SetColorBlindSupport(bool bEnabled)
{
    bColorBlindSupport = bEnabled;
    SettingsCache.bCachedColorBlindSupport = bColorBlindSupport;
    
    // TODO: Apply color blind support to UI and HUD
    
    UE_LOG(LogTemp, Log, TEXT("Color blind support enabled: %s"), bColorBlindSupport ? TEXT("true") : TEXT("false"));
}

void ARadiantPlayerController::SendPlayerCommand(const FString& Command, const FString& Parameters)
{
    if (!PossessedPlayerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot send player command - no possessed character"));
        return;
    }
    
    // Parse and execute common player commands
    if (Command.Equals(TEXT("ToggleCamera"), ESearchCase::IgnoreCase))
    {
        PossessedPlayerCharacter->ToggleCameraMode();
    }
    else if (Command.Equals(TEXT("ToggleSprint"), ESearchCase::IgnoreCase))
    {
        if (PossessedPlayerCharacter->CanSprint())
        {
            if (!PossessedPlayerCharacter->IsSprinting())
            {
                PossessedPlayerCharacter->StartSprinting();
            }
            else
            {
                PossessedPlayerCharacter->StopSprinting();
            }
        }
    }
    else if (Command.Equals(TEXT("Interact"), ESearchCase::IgnoreCase))
    {
        PossessedPlayerCharacter->TryInteract();
    }
    else if (Command.Equals(TEXT("ToggleHUD"), ESearchCase::IgnoreCase))
    {
        ToggleHUD();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Unknown player command: %s"), *Command);
    }
}

// Event handlers
void ARadiantPlayerController::OnPlayerCameraModeChanged(ECameraMode NewCameraMode)
{
    // Update input contexts based on camera mode if needed
    // For example, different contexts for first vs third person
    
    UE_LOG(LogTemp, Log, TEXT("Player camera mode changed to: %s"), 
           NewCameraMode == ECameraMode::FirstPerson ? TEXT("First Person") : TEXT("Third Person"));
}

void ARadiantPlayerController::OnPlayerInteractableDetected(AActor* InteractableActor, bool bIsInRange)
{
    // Update HUD to show interaction prompts
    if (HUDWidget)
    {
        HUDWidget->SetCrosshairInteractable(bIsInRange && InteractableActor != nullptr);
    }
    
    // Could also trigger UI animations, sound effects, etc.
    if (bIsInRange && InteractableActor)
    {
        UE_LOG(LogTemp, Verbose, TEXT("Player can interact with: %s"), *InteractableActor->GetName());
    }
}

void ARadiantPlayerController::OnPlayerCharacterDeath(ABaseCharacter* DeadCharacter)
{
    // Handle player death - switch to death screen, disable input, etc.
    SetRadiantInputMode(EInputMode::UIOnly);
    
    // TODO: Show death screen UI
    // TODO: Trigger death camera
    // TODO: Handle respawn logic
    
    UE_LOG(LogTemp, Warning, TEXT("Player character died: %s"), *DeadCharacter->GetName());
}