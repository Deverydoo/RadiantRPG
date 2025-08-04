// Private/UI/EventMessageWidget.cpp

#include "UI/Widgets/EventMessageWidget.h"

#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

UEventMessageWidget::UEventMessageWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Animation settings
    SlideInDuration = 0.5f;
    FadeOutDuration = 0.5f;
    SlideDistance = 200.0f;
    
    // Appearance settings
    BorderColor = FLinearColor(0.1f, 0.1f, 0.1f, 0.8f);
    BorderOpacity = 0.8f;
    
    // Initialize state
    CurrentState = EMessageState::SlidingIn;
    AnimationTime = 0.0f;
    DisplayTimeRemaining = 0.0f;
    StartPosition = FVector2D::ZeroVector;
    TargetPosition = FVector2D::ZeroVector;
    CurrentPosition = FVector2D::ZeroVector;
    
    // Setup default message type colors
    MessageTypeColors.Add(EEventMessageType::ExperienceGain, FLinearColor::Green);
    MessageTypeColors.Add(EEventMessageType::SkillLevelUp, FLinearColor::Yellow);
    MessageTypeColors.Add(EEventMessageType::LocationDiscovered, FLinearColor::Blue);
    MessageTypeColors.Add(EEventMessageType::QuestUpdated, FLinearColor(1.0f, 0.8f, 0.2f, 1.0f));
    MessageTypeColors.Add(EEventMessageType::QuestCompleted, FLinearColor(0.2f, 1.0f, 0.2f, 1.0f));
    MessageTypeColors.Add(EEventMessageType::ItemReceived, FLinearColor(0.8f, 0.2f, 1.0f, 1.0f));
    MessageTypeColors.Add(EEventMessageType::CombatEvent, FLinearColor::Red);
    MessageTypeColors.Add(EEventMessageType::SystemMessage, FLinearColor::White);
    MessageTypeColors.Add(EEventMessageType::Warning, FLinearColor(1.0f, 0.5f, 0.0f, 1.0f));
    MessageTypeColors.Add(EEventMessageType::Error, FLinearColor::Red);
}

void UEventMessageWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Apply initial styling
    ApplyMessageStyling();
    SetupMessageIcon();
    SetupMessageColors();
    
    UE_LOG(LogTemp, Verbose, TEXT("EventMessageWidget constructed"));
}

void UEventMessageWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    
    UpdateAnimation(InDeltaTime);
}

void UEventMessageWidget::InitializeMessage(const FEventMessage& Message)
{
    CurrentMessage = Message;
    DisplayTimeRemaining = Message.DisplayDuration;
    
    // Set message text
    if (MessageText)
    {
        MessageText->SetText(Message.MessageText);
        MessageText->SetColorAndOpacity(Message.MessageColor);
    }
    
    // Apply styling based on message type
    ApplyMessageStyling();
    SetupMessageIcon();
    SetupMessageColors();
    
    // Reset animation state
    CurrentState = EMessageState::SlidingIn;
    AnimationTime = 0.0f;
    
    OnMessageInitialized(Message);
    
    UE_LOG(LogTemp, Log, TEXT("EventMessageWidget initialized with message: %s"), 
           *Message.MessageText.ToString());
}

void UEventMessageWidget::StartSlideIn(const FVector2D& FromPosition, const FVector2D& ToPosition)
{
    StartPosition = FromPosition;
    TargetPosition = ToPosition;
    CurrentPosition = StartPosition;
    CurrentState = EMessageState::SlidingIn;
    AnimationTime = 0.0f;
    
    // Set initial position
    SetRenderTranslation(CurrentPosition);
    
    OnSlideInStarted();
    
    UE_LOG(LogTemp, Verbose, TEXT("EventMessageWidget starting slide in from (%.1f, %.1f) to (%.1f, %.1f)"), 
           FromPosition.X, FromPosition.Y, ToPosition.X, ToPosition.Y);
}

void UEventMessageWidget::StartFadeOut()
{
    CurrentState = EMessageState::FadingOut;
    AnimationTime = 0.0f;
    
    OnFadeOutStarted();
    
    UE_LOG(LogTemp, Verbose, TEXT("EventMessageWidget starting fade out"));
}

void UEventMessageWidget::ForceComplete()
{
    CurrentState = EMessageState::Completed;
    DisplayTimeRemaining = 0.0f;
    
    // Clear any active timers
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(DisplayTimer);
    }
    
    // Trigger completion
    OnMessageCompleted.Broadcast(this);
    
    UE_LOG(LogTemp, Verbose, TEXT("EventMessageWidget force completed"));
}

void UEventMessageWidget::SetTargetPosition(const FVector2D& NewTargetPosition, bool bAnimate)
{
    if (bAnimate && CurrentState == EMessageState::Displaying)
    {
        StartPosition = CurrentPosition;
        TargetPosition = NewTargetPosition;
        CurrentState = EMessageState::SlidingIn; // Reuse slide animation
        AnimationTime = 0.0f;
    }
    else
    {
        TargetPosition = NewTargetPosition;
        CurrentPosition = NewTargetPosition;
        SetRenderTranslation(CurrentPosition);
    }
}

bool UEventMessageWidget::IsAnimating() const
{
    return CurrentState == EMessageState::SlidingIn || CurrentState == EMessageState::FadingOut;
}

void UEventMessageWidget::UpdateAnimation(float DeltaTime)
{
    switch (CurrentState)
    {
        case EMessageState::SlidingIn:
            UpdateSlideIn(DeltaTime);
            break;
        case EMessageState::Displaying:
            UpdateDisplay(DeltaTime);
            break;
        case EMessageState::FadingOut:
            UpdateFadeOut(DeltaTime);
            break;
        case EMessageState::Completed:
            // No update needed
            break;
    }
}

void UEventMessageWidget::UpdateSlideIn(float DeltaTime)
{
    AnimationTime += DeltaTime;
    float Progress = FMath::Clamp(AnimationTime / SlideInDuration, 0.0f, 1.0f);
    
    // Apply easing curve if available
    float EasedProgress = EvaluateSlideInCurve(Progress);
    
    // Interpolate position
    CurrentPosition = FMath::Lerp(StartPosition, TargetPosition, EasedProgress);
    SetRenderTranslation(CurrentPosition);
    
    // Check if slide in is complete
    if (Progress >= 1.0f)
    {
        CurrentState = EMessageState::Displaying;
        AnimationTime = 0.0f;
        OnSlideInCompleted();
        
        // Start display timer
        if (GetWorld() && DisplayTimeRemaining > 0.0f)
        {
            FTimerDelegate TimerDelegate;
            TimerDelegate.BindUFunction(this, FName("OnDisplayTimerExpired"));
            GetWorld()->GetTimerManager().SetTimer(DisplayTimer, TimerDelegate, DisplayTimeRemaining, false);
        }
    }
}

void UEventMessageWidget::UpdateDisplay(float DeltaTime)
{
    // Display state - just count down time
    if (DisplayTimeRemaining > 0.0f)
    {
        DisplayTimeRemaining -= DeltaTime;
        
        if (DisplayTimeRemaining <= 0.0f)
        {
            StartFadeOut();
        }
    }
}

void UEventMessageWidget::UpdateFadeOut(float DeltaTime)
{
    AnimationTime += DeltaTime;
    float Progress = FMath::Clamp(AnimationTime / FadeOutDuration, 0.0f, 1.0f);
    
    // Apply easing curve if available
    float EasedProgress = EvaluateFadeOutCurve(Progress);
    
    // Apply fade out
    float Alpha = 1.0f - EasedProgress;
    SetRenderOpacity(Alpha);
    
    // Check if fade out is complete
    if (Progress >= 1.0f)
    {
        CurrentState = EMessageState::Completed;
        OnFadeOutCompleted();
        OnMessageCompleted.Broadcast(this);
    }
}

void UEventMessageWidget::ApplyMessageStyling()
{
    if (!MessageBorder)
        return;
    
    // Set border color and opacity
    MessageBorder->SetBrushColor(BorderColor);
    
    // Apply message-specific styling based on type
    FLinearColor TypeColor = GetMessageTypeColor(CurrentMessage.MessageType);
    
    // Create a subtle border highlight based on message type
    FLinearColor BorderHighlight = BorderColor;
    BorderHighlight.R = FMath::Lerp(BorderHighlight.R, TypeColor.R, 0.3f);
    BorderHighlight.G = FMath::Lerp(BorderHighlight.G, TypeColor.G, 0.3f);
    BorderHighlight.B = FMath::Lerp(BorderHighlight.B, TypeColor.B, 0.3f);
    
    MessageBorder->SetBrushColor(BorderHighlight);
}

void UEventMessageWidget::SetupMessageIcon()
{
    if (!MessageIcon)
        return;
    
    UTexture2D* IconTexture = GetMessageTypeIcon(CurrentMessage.MessageType);
    if (IconTexture)
    {
        MessageIcon->SetBrushFromTexture(IconTexture);
        MessageIcon->SetVisibility(ESlateVisibility::Visible);
    }
    else
    {
        MessageIcon->SetVisibility(ESlateVisibility::Hidden);
    }
}

void UEventMessageWidget::SetupMessageColors()
{
    if (MessageText)
    {
        FLinearColor TextColor = CurrentMessage.MessageColor;
        MessageText->SetColorAndOpacity(TextColor);
    }
}

FLinearColor UEventMessageWidget::GetMessageTypeColor(EEventMessageType MessageType) const
{
    const FLinearColor* FoundColor = MessageTypeColors.Find(MessageType);
    return FoundColor ? *FoundColor : FLinearColor::White;
}

UTexture2D* UEventMessageWidget::GetMessageTypeIcon(EEventMessageType MessageType) const
{
    const UTexture2D* const* FoundIcon = MessageTypeIcons.Find(MessageType);
    return FoundIcon ? const_cast<UTexture2D*>(*FoundIcon) : nullptr;
}

float UEventMessageWidget::EvaluateSlideInCurve(float Time) const
{
    if (SlideInCurve)
    {
        return SlideInCurve->GetFloatValue(Time);
    }
    
    // Default easing - ease out cubic
    return 1.0f - FMath::Pow(1.0f - Time, 3.0f);
}

float UEventMessageWidget::EvaluateFadeOutCurve(float Time) const
{
    if (FadeOutCurve)
    {
        return FadeOutCurve->GetFloatValue(Time);
    }
    
    // Default easing - ease in cubic
    return FMath::Pow(Time, 3.0f);
}

void UEventMessageWidget::OnDisplayTimerExpired()
{
    // Timer expired, start fade out
    StartFadeOut();
}