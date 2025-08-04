// Private/UI/RadiantHUD.cpp
// Redesigned HUD implementation focused purely on UI presentation

#include "UI/RadiantHUD.h"
#include "UI/Widgets/EventMessageWidget.h"
#include "Characters/PlayerCharacter.h"
#include "Components/HealthComponent.h"
#include "Components/ManaComponent.h"
#include "Components/StaminaComponent.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Blueprint/WidgetTree.h"

URadiantHUD::URadiantHUD(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Initialize HUD settings
    bShowCrosshair = true;
    DefaultCrosshairColor = FLinearColor::White;
    InteractableCrosshairColor = FLinearColor::Green;
    StatBarVisibilityDuration = 3.0f;
    StatBarFadeSpeed = 4.0f;
    EventMessageFadeSpeed = 3.0f;
    MaxEventMessages = 5;
    EventMessageSlideSpeed = 2.0f;

    // Initialize stat bar colors
    HealthBarColor = FLinearColor(0.8f, 0.2f, 0.2f, 1.0f);      // Red
    StaminaBarColor = FLinearColor(0.2f, 0.8f, 0.2f, 1.0f);     // Green
    ManaBarColor = FLinearColor(0.2f, 0.4f, 0.8f, 1.0f);        // Blue
    LowHealthColor = FLinearColor(0.9f, 0.1f, 0.1f, 1.0f);      // Bright red
    LowStaminaColor = FLinearColor(0.8f, 0.6f, 0.1f, 1.0f);     // Orange
    LowManaColor = FLinearColor(0.1f, 0.1f, 0.9f, 1.0f);        // Bright blue

    // Initialize state
    PlayerCharacter = nullptr;
    LastHealthPercent = 1.0f;
    LastStaminaPercent = 1.0f;
    LastManaPercent = 1.0f;
    
    bIsHealthBarVisible = false;
    bIsStaminaBarVisible = false;
    bIsManaBarVisible = false;
    
    HealthBarAlpha = 0.0f;
    StaminaBarAlpha = 0.0f;
    ManaBarAlpha = 0.0f;
}

void URadiantHUD::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Initialize crosshair
    if (CrosshairImage)
    {
        CrosshairImage->SetVisibility(bShowCrosshair ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
        CrosshairImage->SetColorAndOpacity(DefaultCrosshairColor);
        
        UE_LOG(LogTemp, Log, TEXT("RadiantHUD: Crosshair initialized"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("RadiantHUD: CrosshairImage widget not bound!"));
    }

    // Initialize stat bars as hidden
    InitializeStatBars();

    // Initialize event message system
    if (EventMessagesContainer)
    {
        EventMessagesContainer->ClearChildren();
        UE_LOG(LogTemp, Log, TEXT("RadiantHUD: Event message system initialized"));
    }

    // Hide all panels initially
    HideAllPanels();

    UE_LOG(LogTemp, Log, TEXT("RadiantHUD: NativeConstruct completed"));
}

void URadiantHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    
    // Update stat bar animations
    UpdateStatBars(InDeltaTime);
    
    // Process event messages
    ProcessEventMessages();
}

void URadiantHUD::InitializeHUD(APlayerCharacter* InPlayerCharacter)
{
    PlayerCharacter = InPlayerCharacter;
    
    if (!PlayerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("RadiantHUD: Cannot initialize with null player character"));
        return;
    }

    // Bind to player character component events
    BindToComponentEvents();

    // Initialize cached values
    UpdateInitialStatValues();

    UE_LOG(LogTemp, Log, TEXT("RadiantHUD: Initialized for player: %s"), *PlayerCharacter->GetName());
}

void URadiantHUD::InitializeStatBars()
{
    // Initialize health bar
    if (HealthContainer && HealthBar)
    {
        HealthContainer->SetVisibility(ESlateVisibility::Hidden);
        HealthBar->SetFillColorAndOpacity(HealthBarColor);
        HealthBar->SetPercent(1.0f);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("RadiantHUD: Health bar widgets not properly bound"));
    }

    // Initialize stamina bar
    if (StaminaContainer && StaminaBar)
    {
        StaminaContainer->SetVisibility(ESlateVisibility::Hidden);
        StaminaBar->SetFillColorAndOpacity(StaminaBarColor);
        StaminaBar->SetPercent(1.0f);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("RadiantHUD: Stamina bar widgets not properly bound"));
    }

    // Initialize mana bar
    if (ManaContainer && ManaBar)
    {
        ManaContainer->SetVisibility(ESlateVisibility::Hidden);
        ManaBar->SetFillColorAndOpacity(ManaBarColor);
        ManaBar->SetPercent(1.0f);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("RadiantHUD: Mana bar widgets not properly bound"));
    }
}

void URadiantHUD::BindToComponentEvents()
{
    if (!PlayerCharacter)
        return;

    // Bind to health component
    if (UHealthComponent* HealthComp = PlayerCharacter->GetHealthComponent())
    {
        HealthComp->OnHealthChanged.AddDynamic(this, &URadiantHUD::OnHealthComponentChanged);
        UE_LOG(LogTemp, Log, TEXT("RadiantHUD: Bound to health component events"));
    }

    // Bind to stamina component  
    if (UStaminaComponent* StaminaComp = PlayerCharacter->GetStaminaComponent())
    {
        StaminaComp->OnStaminaChanged.AddDynamic(this, &URadiantHUD::OnStaminaComponentChanged);
        UE_LOG(LogTemp, Log, TEXT("RadiantHUD: Bound to stamina component events"));
    }

    // Bind to mana component
    if (UManaComponent* ManaComp = PlayerCharacter->GetManaComponent())
    {
        ManaComp->OnManaChanged.AddDynamic(this, &URadiantHUD::OnManaComponentChanged);
        UE_LOG(LogTemp, Log, TEXT("RadiantHUD: Bound to mana component events"));
    }

    // Bind to interaction events
    PlayerCharacter->OnInteractableDetected.AddDynamic(this, &URadiantHUD::OnInteractableDetected);
}

void URadiantHUD::UpdateInitialStatValues()
{
    if (!PlayerCharacter)
        return;

    // Get initial health values
    if (UHealthComponent* HealthComp = PlayerCharacter->GetHealthComponent())
    {
        float CurrentHealth = HealthComp->GetHealth();
        float MaxHealth = HealthComp->GetMaxHealth();
        OnHealthComponentChanged(CurrentHealth, MaxHealth);
    }

    // Get initial stamina values
    if (UStaminaComponent* StaminaComp = PlayerCharacter->GetStaminaComponent())
    {
        float CurrentStamina = StaminaComp->GetStamina();
        float MaxStamina = StaminaComp->GetMaxStamina();
        OnStaminaComponentChanged(CurrentStamina, MaxStamina);
    }

    // Get initial mana values
    if (UManaComponent* ManaComp = PlayerCharacter->GetManaComponent())
    {
        float CurrentMana = ManaComp->GetMana();
        float MaxMana = ManaComp->GetMaxMana();
        OnManaComponentChanged(CurrentMana, MaxMana);
    }
}

void URadiantHUD::UpdateStatBars(float DeltaTime)
{
    // Update each stat bar's visibility and alpha
    UpdateStatBarVisibility(0, DeltaTime); // Health
    UpdateStatBarVisibility(1, DeltaTime); // Stamina  
    UpdateStatBarVisibility(2, DeltaTime); // Mana
}

void URadiantHUD::UpdateStatBarVisibility(int32 StatType, float DeltaTime)
{
    float* CurrentAlpha = nullptr;
    bool* IsVisible = nullptr;
    UWidget* Container = nullptr;

    switch (StatType)
    {
        case 0: // Health
            CurrentAlpha = &HealthBarAlpha;
            IsVisible = &bIsHealthBarVisible;
            Container = HealthContainer;
            break;
        case 1: // Stamina
            CurrentAlpha = &StaminaBarAlpha;
            IsVisible = &bIsStaminaBarVisible;
            Container = StaminaContainer;
            break;
        case 2: // Mana
            CurrentAlpha = &ManaBarAlpha;
            IsVisible = &bIsManaBarVisible;
            Container = ManaContainer;
            break;
        default:
            return;
    }

    if (!CurrentAlpha || !IsVisible || !Container)
        return;

    // Calculate target alpha
    float TargetAlpha = *IsVisible ? 1.0f : 0.0f;
    
    // Interpolate alpha
    *CurrentAlpha = FMath::FInterpTo(*CurrentAlpha, TargetAlpha, DeltaTime, StatBarFadeSpeed);
    
    // Update container visibility and opacity
    if (*CurrentAlpha > 0.01f)
    {
        Container->SetVisibility(ESlateVisibility::HitTestInvisible);
        Container->SetRenderOpacity(*CurrentAlpha);
    }
    else
    {
        Container->SetVisibility(ESlateVisibility::Hidden);
    }
}

void URadiantHUD::SetCrosshairVisible(bool bVisible)
{
    bShowCrosshair = bVisible;
    
    if (CrosshairImage)
    {
        CrosshairImage->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
    }
}

void URadiantHUD::SetCrosshairInteractable(bool bInteractable)
{
    if (CrosshairImage)
    {
        FLinearColor TargetColor = bInteractable ? InteractableCrosshairColor : DefaultCrosshairColor;
        CrosshairImage->SetColorAndOpacity(TargetColor);
    }
    
    OnCrosshairStateChanged(bInteractable);
}

void URadiantHUD::UpdateHealthBar(float HealthPercent, float CurrentHealth, float MaxHealth)
{
    if (FMath::Abs(HealthPercent - LastHealthPercent) < 0.01f)
        return; // No significant change

    LastHealthPercent = HealthPercent;
    
    if (HealthBar)
    {
        HealthBar->SetPercent(HealthPercent);
        
        // Update color based on health level
        FLinearColor BarColor = GetStatBarColor(0, HealthPercent);
        HealthBar->SetFillColorAndOpacity(BarColor);
    }
    
    ShowStatBar(0); // Health
    StartStatBarTimer(0);
    OnHealthBarUpdated(HealthPercent);
    
    UE_LOG(LogTemp, Verbose, TEXT("RadiantHUD: Health bar updated: %.1f%% (%.1f/%.1f)"), 
           HealthPercent * 100.0f, CurrentHealth, MaxHealth);
}

void URadiantHUD::UpdateStaminaBar(float StaminaPercent, float CurrentStamina, float MaxStamina)
{
    if (FMath::Abs(StaminaPercent - LastStaminaPercent) < 0.01f)
        return; // No significant change

    LastStaminaPercent = StaminaPercent;
    
    if (StaminaBar)
    {
        StaminaBar->SetPercent(StaminaPercent);
        
        // Update color based on stamina level
        FLinearColor BarColor = GetStatBarColor(1, StaminaPercent);
        StaminaBar->SetFillColorAndOpacity(BarColor);
    }
    
    ShowStatBar(1); // Stamina
    StartStatBarTimer(1);
    OnStaminaBarUpdated(StaminaPercent);
    
    UE_LOG(LogTemp, Verbose, TEXT("RadiantHUD: Stamina bar updated: %.1f%% (%.1f/%.1f)"), 
           StaminaPercent * 100.0f, CurrentStamina, MaxStamina);
}

void URadiantHUD::UpdateManaBar(float ManaPercent, float CurrentMana, float MaxMana)
{
    if (FMath::Abs(ManaPercent - LastManaPercent) < 0.01f)
        return; // No significant change

    LastManaPercent = ManaPercent;
    
    if (ManaBar)
    {
        ManaBar->SetPercent(ManaPercent);
        
        // Update color based on mana level
        FLinearColor BarColor = GetStatBarColor(2, ManaPercent);
        ManaBar->SetFillColorAndOpacity(BarColor);
    }
    
    ShowStatBar(2); // Mana
    StartStatBarTimer(2);
    OnManaBarUpdated(ManaPercent);
    
    UE_LOG(LogTemp, Verbose, TEXT("RadiantHUD: Mana bar updated: %.1f%% (%.1f/%.1f)"), 
           ManaPercent * 100.0f, CurrentMana, MaxMana);
}

void URadiantHUD::ShowStatBar(int32 StatType)
{
    switch (StatType)
    {
        case 0: // Health
            bIsHealthBarVisible = true;
            break;
        case 1: // Stamina
            bIsStaminaBarVisible = true;
            break;
        case 2: // Mana
            bIsManaBarVisible = true;
            break;
    }
}

void URadiantHUD::HideStatBar(int32 StatType)
{
    switch (StatType)
    {
        case 0: // Health
            bIsHealthBarVisible = false;
            break;
        case 1: // Stamina
            bIsStaminaBarVisible = false;
            break;
        case 2: // Mana
            bIsManaBarVisible = false;
            break;
    }
}

void URadiantHUD::StartStatBarTimer(int32 StatType)
{
    if (!GetWorld())
        return;

    FTimerHandle* TimerHandle = nullptr;
    FTimerDelegate TimerDelegate;

    switch (StatType)
    {
        case 0: // Health
            TimerHandle = &HealthTimerHandle;
            TimerDelegate.BindUFunction(this, FName("OnHealthTimerExpired"));
            break;
        case 1: // Stamina
            TimerHandle = &StaminaTimerHandle;
            TimerDelegate.BindUFunction(this, FName("OnStaminaTimerExpired"));
            break;
        case 2: // Mana
            TimerHandle = &ManaTimerHandle;
            TimerDelegate.BindUFunction(this, FName("OnManaTimerExpired"));
            break;
        default:
            return;
    }

    if (TimerHandle)
    {
        GetWorld()->GetTimerManager().ClearTimer(*TimerHandle);
        GetWorld()->GetTimerManager().SetTimer(*TimerHandle, TimerDelegate, StatBarVisibilityDuration, false);
    }
}

void URadiantHUD::OnHealthTimerExpired()
{
    HideStatBar(0);
}

void URadiantHUD::OnStaminaTimerExpired()
{
    HideStatBar(1);
}

void URadiantHUD::OnManaTimerExpired()
{
    HideStatBar(2);
}

void URadiantHUD::AddEventMessage(const FEventMessage& Message)
{
    PendingEventMessages.Enqueue(Message);
    OnEventMessageAdded.Broadcast(Message);
    
    UE_LOG(LogTemp, Log, TEXT("RadiantHUD: Event message added: %s"), *Message.MessageText.ToString());
}

void URadiantHUD::AddEventMessageSimple(const FText& MessageText, EEventMessageType MessageType)
{
    FEventMessage Message;
    Message.MessageText = MessageText;
    Message.MessageType = MessageType;
    Message.DisplayDuration = 5.0f;
    Message.bIsImportant = false;
    
    // Set color based on message type
    switch (MessageType)
    {
        case EEventMessageType::ExperienceGain:
            Message.MessageColor = FLinearColor::Green;
            break;
        case EEventMessageType::SkillLevelUp:
            Message.MessageColor = FLinearColor::Yellow;
            break;
        case EEventMessageType::LocationDiscovered:
            Message.MessageColor = FLinearColor::Blue;
            break;
        case EEventMessageType::QuestUpdated:
        case EEventMessageType::QuestCompleted:
            Message.MessageColor = FLinearColor(1.0f, 0.8f, 0.2f, 1.0f); // Orange
            break;
        case EEventMessageType::ItemReceived:
            Message.MessageColor = FLinearColor(0.8f, 0.2f, 1.0f, 1.0f); // Purple
            break;
        case EEventMessageType::Warning:
            Message.MessageColor = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f); // Orange
            break;
        case EEventMessageType::Error:
            Message.MessageColor = FLinearColor::Red;
            break;
        default:
            Message.MessageColor = FLinearColor::White;
            break;
    }
    
    AddEventMessage(Message);
}

void URadiantHUD::ProcessEventMessages()
{
    // Remove completed messages
    for (int32 i = ActiveEventMessages.Num() - 1; i >= 0; i--)
    {
        if (ActiveEventMessages[i] && !ActiveEventMessages[i]->IsAnimating() && 
            ActiveEventMessages[i]->GetRemainingDisplayTime() <= 0.0f)
        {
            RemoveEventMessageWidget(ActiveEventMessages[i]);
        }
    }

    // Add new messages if we have space and pending messages
    while (ActiveEventMessages.Num() < MaxEventMessages && !PendingEventMessages.IsEmpty())
    {
        FEventMessage NewMessage;
        if (PendingEventMessages.Dequeue(NewMessage))
        {
            CreateEventMessageWidget(NewMessage);
        }
    }

    UpdateEventMessagePositions();
}

void URadiantHUD::CreateEventMessageWidget(const FEventMessage& Message)
{
    if (!EventMessagesContainer || !GetWorld())
        return;

    // Create the widget
    UEventMessageWidget* MessageWidget = CreateWidget<UEventMessageWidget>(GetWorld(), UEventMessageWidget::StaticClass());
    if (!MessageWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("RadiantHUD: Failed to create EventMessageWidget"));
        return;
    }

    // Initialize the message
    MessageWidget->InitializeMessage(Message);
    
    // Bind completion event
    MessageWidget->OnMessageCompleted.AddDynamic(this, &URadiantHUD::OnMessageCompleted);

    // Add to container
    EventMessagesContainer->AddChild(MessageWidget);
    ActiveEventMessages.Add(MessageWidget);

    // Start slide-in animation
    FVector2D StartPos = FVector2D(300.0f, 0.0f); // Start off-screen to the right
    FVector2D TargetPos = FVector2D::ZeroVector;
    MessageWidget->StartSlideIn(StartPos, TargetPos);

    UE_LOG(LogTemp, Log, TEXT("RadiantHUD: Created event message widget for: %s"), *Message.MessageText.ToString());
}

void URadiantHUD::RemoveEventMessageWidget(UEventMessageWidget* MessageWidget)
{
    if (!MessageWidget || !EventMessagesContainer)
        return;

    // Remove from active list
    ActiveEventMessages.Remove(MessageWidget);
    
    // Remove from container
    EventMessagesContainer->RemoveChild(MessageWidget);
    
    UE_LOG(LogTemp, Verbose, TEXT("RadiantHUD: Removed event message widget"));
}

void URadiantHUD::UpdateEventMessagePositions()
{
    // Update positions to ensure proper stacking
    for (int32 i = 0; i < ActiveEventMessages.Num(); i++)
    {
        if (ActiveEventMessages[i])
        {
            FVector2D TargetPosition = FVector2D(0.0f, i * -35.0f); // Stack vertically with spacing
            ActiveEventMessages[i]->SetTargetPosition(TargetPosition, true);
        }
    }
}

void URadiantHUD::ClearAllEventMessages()
{
    // Force complete all active messages
    for (UEventMessageWidget* MessageWidget : ActiveEventMessages)
    {
        if (MessageWidget)
        {
            MessageWidget->ForceComplete();
        }
    }
    
    ActiveEventMessages.Empty();
    
    // Clear pending messages
    while (!PendingEventMessages.IsEmpty())
    {
        FEventMessage DummyMessage;
        PendingEventMessages.Dequeue(DummyMessage);
    }
    
    // Clear container
    if (EventMessagesContainer)
    {
        EventMessagesContainer->ClearChildren();
    }
    
    UE_LOG(LogTemp, Log, TEXT("RadiantHUD: Cleared all event messages"));
}

void URadiantHUD::ShowPanel(int32 PanelIndex)
{
    if (MainMenuSwitcher && PanelIndex >= 0 && PanelIndex < MainMenuSwitcher->GetNumWidgets())
    {
        MainMenuSwitcher->SetActiveWidgetIndex(PanelIndex);
        MainMenuSwitcher->SetVisibility(ESlateVisibility::Visible);
        OnPanelVisibilityChanged(PanelIndex, true);
        
        UE_LOG(LogTemp, Log, TEXT("RadiantHUD: Showing panel at index: %d"), PanelIndex);
    }
}

void URadiantHUD::HideAllPanels()
{
    if (MainMenuSwitcher)
    {
        MainMenuSwitcher->SetVisibility(ESlateVisibility::Hidden);
        OnPanelVisibilityChanged(-1, false);
        
        UE_LOG(LogTemp, Log, TEXT("RadiantHUD: Hiding all panels"));
    }
}

bool URadiantHUD::IsPanelVisible(int32 PanelIndex) const
{
    if (MainMenuSwitcher && PanelIndex >= 0 && PanelIndex < MainMenuSwitcher->GetNumWidgets())
    {
        return MainMenuSwitcher->GetVisibility() != ESlateVisibility::Hidden && 
               MainMenuSwitcher->GetActiveWidgetIndex() == PanelIndex;
    }
    return false;
}

void URadiantHUD::SetHUDVisible(bool bVisible)
{
    SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    OnHUDVisibilityChanged.Broadcast(bVisible);
    OnHUDVisibilityChangedBP(bVisible);
    
    UE_LOG(LogTemp, Log, TEXT("RadiantHUD: Visibility set to %s"), bVisible ? TEXT("Visible") : TEXT("Hidden"));
}

void URadiantHUD::ToggleHUDVisibility()
{
    bool bCurrentlyVisible = GetVisibility() == ESlateVisibility::Visible;
    SetHUDVisible(!bCurrentlyVisible);
}

bool URadiantHUD::IsHUDVisible() const
{
    return GetVisibility() == ESlateVisibility::Visible;
}

FLinearColor URadiantHUD::GetStatBarColor(int32 StatType, float Percent) const
{
    switch (StatType)
    {
        case 0: // Health
            return Percent < 0.25f ? LowHealthColor : HealthBarColor;
        case 1: // Stamina
            return Percent < 0.25f ? LowStaminaColor : StaminaBarColor;
        case 2: // Mana
            return Percent < 0.25f ? LowManaColor : ManaBarColor;
    }
    return FLinearColor::White;
}

FText URadiantHUD::FormatStatText(float Current, float Max) const
{
    return FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), Current, Max));
}

float URadiantHUD::CalculateStatBarAlpha(int32 StatType) const
{
    switch (StatType)
    {
        case 0: // Health
            return HealthBarAlpha;
        case 1: // Stamina
            return StaminaBarAlpha;
        case 2: // Mana
            return ManaBarAlpha;
    }
    return 0.0f;
}

// Component event handlers
void URadiantHUD::OnHealthComponentChanged(float NewHealth, float NewMaxHealth)
{
    float HealthPercent = NewMaxHealth > 0.0f ? NewHealth / NewMaxHealth : 0.0f;
    UpdateHealthBar(HealthPercent, NewHealth, NewMaxHealth);
}

void URadiantHUD::OnStaminaComponentChanged(float NewStamina, float NewMaxStamina)
{
    float StaminaPercent = NewMaxStamina > 0.0f ? NewStamina / NewMaxStamina : 0.0f;
    UpdateStaminaBar(StaminaPercent, NewStamina, NewMaxStamina);
}

void URadiantHUD::OnManaComponentChanged(float NewMana, float NewMaxMana)
{
    float ManaPercent = NewMaxMana > 0.0f ? NewMana / NewMaxMana : 0.0f;
    UpdateManaBar(ManaPercent, NewMana, NewMaxMana);
}

void URadiantHUD::OnInteractableDetected(AActor* InteractableActor, bool bIsInRange)
{
    SetCrosshairInteractable(bIsInRange && InteractableActor != nullptr);
}

void URadiantHUD::OnMessageCompleted(UEventMessageWidget* CompletedWidget)
{
    RemoveEventMessageWidget(CompletedWidget);
}