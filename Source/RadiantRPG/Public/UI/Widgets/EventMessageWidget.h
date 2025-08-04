// Public/UI/EventMessageWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Engine/TimerHandle.h"
#include "UI/RadiantHUD.h"
#include "EventMessageWidget.generated.h"

/**
 * Individual event message widget that handles its own lifecycle and animations
 * Used by RadiantHUD to display event messages with sliding and fading animations
 */
UCLASS(Blueprintable)
class RADIANTRPG_API UEventMessageWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UEventMessageWidget(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // Widget components
    UPROPERTY(meta = (BindWidget))
    class UBorder* MessageBorder;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* MessageText;

    UPROPERTY(meta = (BindWidget))
    class UImage* MessageIcon;

    // Animation settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float SlideInDuration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float FadeOutDuration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float SlideDistance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UCurveFloat* SlideInCurve;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UCurveFloat* FadeOutCurve;

    // Message type colors and icons
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    TMap<EEventMessageType, FLinearColor> MessageTypeColors;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    TMap<EEventMessageType, UTexture2D*> MessageTypeIcons;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor BorderColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    float BorderOpacity;

private:
    // Message data
    FEventMessage CurrentMessage;
    
    // Animation state
    enum class EMessageState : uint8
    {
        SlidingIn,
        Displaying,
        FadingOut,
        Completed
    };
    
    EMessageState CurrentState;
    float AnimationTime;
    float DisplayTimeRemaining;
    
    // Position tracking
    FVector2D StartPosition;
    FVector2D TargetPosition;
    FVector2D CurrentPosition;
    
    // Timer handles
    FTimerHandle DisplayTimer;
    
    // Callbacks
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMessageCompleted, UEventMessageWidget*, CompletedWidget);
    
public:
    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnMessageCompleted OnMessageCompleted;

    // Public functions
    UFUNCTION(BlueprintCallable, Category = "Message")
    void InitializeMessage(const FEventMessage& Message);

    UFUNCTION(BlueprintCallable, Category = "Message")
    void StartSlideIn(const FVector2D& FromPosition, const FVector2D& ToPosition);

    UFUNCTION(BlueprintCallable, Category = "Message")
    void StartFadeOut();

    UFUNCTION(BlueprintCallable, Category = "Message")
    void ForceComplete();

    UFUNCTION(BlueprintCallable, Category = "Message")
    void SetTargetPosition(const FVector2D& NewTargetPosition, bool bAnimate = true);

    // Getters
    UFUNCTION(BlueprintPure, Category = "Message")
    bool IsAnimating() const;

    UFUNCTION(BlueprintPure, Category = "Message")
    EEventMessageType GetMessageType() const { return CurrentMessage.MessageType; }

    UFUNCTION(BlueprintPure, Category = "Message")
    FText GetMessageText() const { return CurrentMessage.MessageText; }

    UFUNCTION(BlueprintPure, Category = "Message")
    float GetRemainingDisplayTime() const { return DisplayTimeRemaining; }

protected:
    // Internal functions
    void UpdateAnimation(float DeltaTime);
    void UpdateSlideIn(float DeltaTime);
    void UpdateDisplay(float DeltaTime);
    void UpdateFadeOut(float DeltaTime);
    
    void ApplyMessageStyling();
    void SetupMessageIcon();
    void SetupMessageColors();
    
    FLinearColor GetMessageTypeColor(EEventMessageType MessageType) const;
    UTexture2D* GetMessageTypeIcon(EEventMessageType MessageType) const;
    
    // Animation helpers
    float EvaluateSlideInCurve(float Time) const;
    float EvaluateFadeOutCurve(float Time) const;
    
    // Timer callbacks
    UFUNCTION()
    void OnDisplayTimerExpired();

    // Blueprint events
    UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
    void OnSlideInStarted();

    UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
    void OnSlideInCompleted();

    UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
    void OnFadeOutStarted();

    UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
    void OnFadeOutCompleted();

    UFUNCTION(BlueprintImplementableEvent, Category = "Message")
    void OnMessageInitialized(const FEventMessage& Message);
};