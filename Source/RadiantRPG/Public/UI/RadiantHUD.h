// Public/UI/RadiantHUD.h
// Redesigned HUD class focused purely on UI presentation and data display

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/ScrollBox.h"
#include "Components/Image.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/TimerHandle.h"
#include "RadiantHUD.generated.h"

class APlayerCharacter;
class UEventMessageWidget;

UENUM(BlueprintType)
enum class EEventMessageType : uint8
{
    ExperienceGain UMETA(DisplayName = "Experience Gain"),
    SkillLevelUp UMETA(DisplayName = "Skill Level Up"),
    LocationDiscovered UMETA(DisplayName = "Location Discovered"),
    QuestUpdated UMETA(DisplayName = "Quest Updated"),
    QuestCompleted UMETA(DisplayName = "Quest Completed"),
    ItemReceived UMETA(DisplayName = "Item Received"),
    CombatEvent UMETA(DisplayName = "Combat Event"),
    SystemMessage UMETA(DisplayName = "System Message"),
    Warning UMETA(DisplayName = "Warning"),
    Error UMETA(DisplayName = "Error")
};

USTRUCT(BlueprintType)
struct FEventMessage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    FText MessageText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    EEventMessageType MessageType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    FLinearColor MessageColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    float DisplayDuration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    bool bIsImportant;

    FEventMessage()
    {
        MessageText = FText::GetEmpty();
        MessageType = EEventMessageType::SystemMessage;
        MessageColor = FLinearColor::White;
        DisplayDuration = 5.0f;
        bIsImportant = false;
    }
};

// HUD-specific events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEventMessageAdded, const FEventMessage&, NewMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHUDVisibilityChanged, bool, bIsVisible);

/**
 * RadiantHUD - Main HUD Widget for RadiantRPG
 * 
 * Responsibilities:
 * - Display player stats (health, mana, stamina)
 * - Show crosshair and interaction feedback
 * - Display event messages and notifications
 * - Manage HUD panels (inventory, skills, etc.)
 * - Handle HUD animations and transitions
 * 
 * This class does NOT:
 * - Handle input (managed by PlayerController)
 * - Directly access game data (receives data via events)
 * - Manage game state (purely presentation layer)
 */
UCLASS(Blueprintable)
class RADIANTRPG_API URadiantHUD : public UUserWidget
{
    GENERATED_BODY()

public:
    URadiantHUD(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // === CROSSHAIR COMPONENTS ===
    UPROPERTY(meta = (BindWidget))
    class UImage* CrosshairImage;

    // === STAT BARS COMPONENTS ===
    UPROPERTY(meta = (BindWidget))
    class UHorizontalBox* StatBarsContainer;

    UPROPERTY(meta = (BindWidget))
    class UProgressBar* HealthBar;

    UPROPERTY(meta = (BindWidget))
    class UProgressBar* StaminaBar;

    UPROPERTY(meta = (BindWidget))
    class UProgressBar* ManaBar;

    // Individual stat bar containers for visibility control
    UPROPERTY(meta = (BindWidget))
    class UWidget* HealthContainer;

    UPROPERTY(meta = (BindWidget))
    class UWidget* StaminaContainer;

    UPROPERTY(meta = (BindWidget))
    class UWidget* ManaContainer;

    // === EVENT MESSAGES COMPONENTS ===
    UPROPERTY(meta = (BindWidget))
    class UScrollBox* EventMessagesScrollBox;

    UPROPERTY(meta = (BindWidget))
    class UVerticalBox* EventMessagesContainer;

    // === PANELS FOR FUTURE EXPANSION ===
    UPROPERTY(meta = (BindWidget))
    class UWidgetSwitcher* MainMenuSwitcher;

    // === HUD SETTINGS ===
    
    /** Whether to show crosshair */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Settings")
    bool bShowCrosshair;

    /** Crosshair colors */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Settings")
    FLinearColor DefaultCrosshairColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Settings")
    FLinearColor InteractableCrosshairColor;

    /** Stat bar visibility settings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Settings")
    float StatBarVisibilityDuration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Settings")
    float StatBarFadeSpeed;

    /** Event message settings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Settings")
    float EventMessageFadeSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Settings")
    int32 MaxEventMessages;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Settings")
    float EventMessageSlideSpeed;

    // === STAT BAR COLORS ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat Colors")
    FLinearColor HealthBarColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat Colors")
    FLinearColor StaminaBarColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat Colors")
    FLinearColor ManaBarColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat Colors")
    FLinearColor LowHealthColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat Colors")
    FLinearColor LowStaminaColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat Colors")
    FLinearColor LowManaColor;

private:
    // === CACHED DATA ===
    
    /** Cached player reference */
    UPROPERTY()
    APlayerCharacter* PlayerCharacter;

    /** Current stat values for change detection */
    float LastHealthPercent;
    float LastStaminaPercent;
    float LastManaPercent;

    /** Stat bar visibility timers */
    FTimerHandle HealthTimerHandle;
    FTimerHandle StaminaTimerHandle;
    FTimerHandle ManaTimerHandle;

    /** Event message management */
    UPROPERTY()
    TArray<UEventMessageWidget*> ActiveEventMessages;

    TQueue<FEventMessage> PendingEventMessages;

    /** Animation state */
    bool bIsHealthBarVisible;
    bool bIsStaminaBarVisible;
    bool bIsManaBarVisible;

    float HealthBarAlpha;
    float StaminaBarAlpha;
    float ManaBarAlpha;

public:
    // === EVENTS ===
    UPROPERTY(BlueprintAssignable, Category = "HUD Events")
    FOnEventMessageAdded OnEventMessageAdded;

    UPROPERTY(BlueprintAssignable, Category = "HUD Events")
    FOnHUDVisibilityChanged OnHUDVisibilityChanged;

    // === INITIALIZATION ===
    
    /** Initialize HUD with player character reference */
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void InitializeHUD(APlayerCharacter* InPlayerCharacter);

    // === CROSSHAIR INTERFACE ===
    
    /** Set crosshair visibility */
    UFUNCTION(BlueprintCallable, Category = "Crosshair")
    void SetCrosshairVisible(bool bVisible);

    /** Set crosshair to interactable state */
    UFUNCTION(BlueprintCallable, Category = "Crosshair")
    void SetCrosshairInteractable(bool bInteractable);

    // === STAT BAR INTERFACE ===
    
    /** Update health bar display */
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void UpdateHealthBar(float HealthPercent, float CurrentHealth, float MaxHealth);

    /** Update stamina bar display */
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void UpdateStaminaBar(float StaminaPercent, float CurrentStamina, float MaxStamina);

    /** Update mana bar display */
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void UpdateManaBar(float ManaPercent, float CurrentMana, float MaxMana);

    // === EVENT MESSAGE INTERFACE ===
    
    /** Add event message to display queue */
    UFUNCTION(BlueprintCallable, Category = "Events")
    void AddEventMessage(const FEventMessage& Message);

    /** Add simple event message */
    UFUNCTION(BlueprintCallable, Category = "Events")
    void AddEventMessageSimple(const FText& MessageText, EEventMessageType MessageType = EEventMessageType::SystemMessage);

    /** Clear all event messages */
    UFUNCTION(BlueprintCallable, Category = "Events")
    void ClearAllEventMessages();

    // === PANEL MANAGEMENT ===
    
    /** Show specific panel by index */
    UFUNCTION(BlueprintCallable, Category = "Panels")
    void ShowPanel(int32 PanelIndex);

    /** Hide all panels */
    UFUNCTION(BlueprintCallable, Category = "Panels")
    void HideAllPanels();

    /** Check if panel is visible */
    UFUNCTION(BlueprintPure, Category = "Panels")
    bool IsPanelVisible(int32 PanelIndex) const;

    // === HUD VISIBILITY ===
    
    /** Set overall HUD visibility */
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void SetHUDVisible(bool bVisible);

    /** Toggle HUD visibility */
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void ToggleHUDVisibility();

    /** Get HUD visibility */
    UFUNCTION(BlueprintPure, Category = "HUD")
    bool IsHUDVisible() const;

protected:
    // === INTERNAL UPDATE FUNCTIONS ===
    
    /** Update stat bar animations */
    void UpdateStatBars(float DeltaTime);

    /** Process pending event messages */
    void ProcessEventMessages();

    /** Update event message positions */
    void UpdateEventMessagePositions();

    // === STAT BAR MANAGEMENT ===
    
    /** Show stat bar with fade in */
    void ShowStatBar(int32 StatType); // 0=Health, 1=Stamina, 2=Mana

    /** Hide stat bar with fade out */
    void HideStatBar(int32 StatType);

    /** Start visibility timer for stat bar */
    void StartStatBarTimer(int32 StatType);

    /** Update stat bar visibility and alpha */
    void UpdateStatBarVisibility(int32 StatType, float DeltaTime);

    // === TIMER CALLBACKS ===
    
    UFUNCTION()
    void OnHealthTimerExpired();

    UFUNCTION()
    void OnStaminaTimerExpired();

    UFUNCTION()
    void OnManaTimerExpired();

    // === EVENT MESSAGE MANAGEMENT ===
    
    /** Create and display event message widget */
    void CreateEventMessageWidget(const FEventMessage& Message);

    /** Remove event message widget */
    void RemoveEventMessageWidget(UEventMessageWidget* MessageWidget);

    // === UTILITY FUNCTIONS ===
    
    /** Get stat bar color based on percentage */
    FLinearColor GetStatBarColor(int32 StatType, float Percent) const;

    /** Format stat text display */
    FText FormatStatText(float Current, float Max) const;

    /** Calculate stat bar alpha for animations */
    float CalculateStatBarAlpha(int32 StatType) const;

    // === BLUEPRINT EVENTS ===
    
    /** Called when health bar updates */
    UFUNCTION(BlueprintImplementableEvent, Category = "HUD Events")
    void OnHealthBarUpdated(float HealthPercent);

    /** Called when stamina bar updates */
    UFUNCTION(BlueprintImplementableEvent, Category = "HUD Events")
    void OnStaminaBarUpdated(float StaminaPercent);

    /** Called when mana bar updates */
    UFUNCTION(BlueprintImplementableEvent, Category = "HUD Events")
    void OnManaBarUpdated(float ManaPercent);

    /** Called when crosshair state changes */
    UFUNCTION(BlueprintImplementableEvent, Category = "HUD Events")
    void OnCrosshairStateChanged(bool bInteractable);

    /** Called when panel visibility changes */
    UFUNCTION(BlueprintImplementableEvent, Category = "HUD Events")
    void OnPanelVisibilityChanged(int32 PanelIndex, bool bVisible);

    /** Called when HUD visibility changes */
    UFUNCTION(BlueprintImplementableEvent, Category = "HUD Events")
    void OnHUDVisibilityChangedBP(bool bNewVisible);

protected:
    // === COMPONENT EVENT HANDLERS ===
    
    /** Handle health component changes */
    UFUNCTION()
    void OnHealthComponentChanged(float NewHealth, float NewMaxHealth);

    /** Handle stamina component changes */
    UFUNCTION()
    void OnStaminaComponentChanged(float NewStamina, float NewMaxStamina);

    /** Handle mana component changes */
    UFUNCTION()
    void OnManaComponentChanged(float NewMana, float NewMaxMana);

    /** Handle interaction detection */
    UFUNCTION()
    void OnInteractableDetected(AActor* InteractableActor, bool bIsInRange);

    /** Handle event message completion */
    UFUNCTION()
    void OnMessageCompleted(UEventMessageWidget* CompletedWidget);

    // === INITIALIZATION HELPERS ===
    
    /** Initialize stat bars with default settings */
    void InitializeStatBars();

    /** Bind to component events */
    void BindToComponentEvents();

    /** Update initial stat values */
    void UpdateInitialStatValues();
};