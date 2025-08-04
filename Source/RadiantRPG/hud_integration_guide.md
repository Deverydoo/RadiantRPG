# RadiantRPG HUD Integration Guide

## Overview
This guide explains how to integrate the new HUD system into RadiantRPG and create the necessary UMG widgets.

## Files Created
1. `Public/UI/RadiantHUD.h` - Main HUD widget class
2. `Public/UI/EventMessageWidget.h` - Individual event message widget
3. `Private/UI/RadiantHUD.cpp` - HUD implementation
4. `Private/UI/EventMessageWidget.cpp` - Event message implementation

## Widget Blueprint Setup

### 1. Create WBP_RadiantHUD Blueprint

Create a new Widget Blueprint in your Content Browser:
- Right-click → User Interface → Widget Blueprint
- Name it `WBP_RadiantHUD`
- Set Parent Class to `RadiantHUD` (C++ class)

#### Widget Hierarchy Structure:
```
Canvas Panel (Fill Screen)
├── CrosshairImage (Image)
│   ├── Anchor: Center
│   ├── Size: 32x32
│   └── Z-Order: 100
├── StatBarsContainer (Horizontal Box)
│   ├── Anchor: Bottom Center
│   ├── Position: (0, -80)
│   ├── HealthContainer (Border)
│   │   ├── HealthBar (Progress Bar)
│   │   └── HealthText (Text Block)
│   ├── StaminaContainer (Border)
│   │   ├── StaminaBar (Progress Bar)
│   │   └── StaminaText (Text Block)
│   └── ManaContainer (Border)
│       ├── ManaBar (Progress Bar)
│       └── ManaText (Text Block)
├── EventMessagesScrollBox (Scroll Box)
│   ├── Anchor: Left Center
│   ├── Position: (20, 0)
│   ├── Size: 400x300
│   └── EventMessagesContainer (Vertical Box)
└── MainMenuSwitcher (Widget Switcher)
    ├── InventoryPanel (User Widget - hidden by default)
    ├── SkillsPanel (User Widget - hidden by default)
    ├── QuestPanel (User Widget - hidden by default)
    └── MapPanel (User Widget - hidden by default)
```

### 2. Create WBP_EventMessage Blueprint

Create another Widget Blueprint:
- Name it `WBP_EventMessage`
- Set Parent Class to `EventMessageWidget` (C++ class)

#### Widget Structure:
```
Canvas Panel
└── MessageBorder (Border)
    ├── Background Color: (0.1, 0.1, 0.1, 0.8)
    ├── Padding: 8px all sides
    └── Horizontal Box
        ├── MessageIcon (Image)
        │   ├── Size: 24x24
        │   └── Margin: (0, 0, 8, 0)
        └── MessageText (Text Block)
            ├── Font Size: 14
            ├── Color: White
            └── Auto Wrap: True
```

## Integration Steps

### 1. Update RadiantRPGCharacter

Add HUD creation and management to your character:

```cpp
// In RadiantRPGCharacter.h
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
TSubclassOf<class URadiantHUD> HUDWidgetClass;

UPROPERTY()
class URadiantHUD* HUDWidget;

// In RadiantRPGCharacter.cpp
void ARadiantRPGCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    // Create HUD widget
    if (HUDWidgetClass && IsLocallyControlled())
    {
        APlayerController* PC = Cast<APlayerController>(GetController());
        if (PC)
        {
            HUDWidget = CreateWidget<URadiantHUD>(PC, HUDWidgetClass);
            if (HUDWidget)
            {
                HUDWidget->AddToViewport();
                HUDWidget->InitializeHUD(this);
            }
        }
    }
}
```

### 2. Update InteractionUIComponent

Modify the existing interaction UI to work with the new HUD:

```cpp
// In InteractionUIComponent.cpp
void UInteractionUIComponent::OnInteractableDetected(AActor* InteractableActor, bool bIsInRange)
{
    // Instead of managing crosshair directly, use HUD
    if (OwnerCharacter && OwnerCharacter->GetHUDWidget())
    {
        OwnerCharacter->GetHUDWidget()->SetCrosshairInteractable(bIsInRange && InteractableActor != nullptr);
    }
}
```

### 3. Component Event Integration

Update your components to send messages to the HUD:

```cpp
// Example: In SkillsComponent when skill levels up
void USkillsComponent::TryLevelUpSkill(ESkillType SkillType)
{
    // ... existing level up logic ...
    
    // Send event message to HUD
    if (ARadiantRPGCharacter* Character = Cast<ARadiantRPGCharacter>(OwnerCharacter))
    {
        if (URadiantHUD* HUD = Character->GetHUDWidget())
        {
            FText Message = FText::FromString(FString::Printf(TEXT("%s increased to %.0f"), 
                *UEnum::GetValueAsString(SkillType), SkillData->CurrentValue));
            HUD->AddEventMessageSimple(Message, EEventMessageType::SkillLevelUp);
        }
    }
}
```

## Styling Guidelines

### Stat Bars
- **Health**: Red bars with darker red for low health
- **Stamina**: Green bars with orange for low stamina  
- **Mana**: Blue bars with darker blue for low mana
- Bars fade in/out smoothly over 2 seconds
- Text shows current/max values

### Event Messages
- **Experience Gain**: Green text
- **Skill Level Up**: Yellow text
- **Location Discovered**: Cyan text
- **Quest Updates**: Orange text
- **Item Received**: Purple text
- **Warnings**: Orange text
- **Errors**: Red text

### Crosshair
- **Default**: White, subtle
- **Interactable**: Green, slightly larger
- Hidden/shown based on camera mode

## Usage Examples

### Adding Event Messages

```cpp
// Simple message
HUDWidget->AddEventMessageSimple(
    FText::FromString("Location Discovered: Ancient Ruins"), 
    EEventMessageType::LocationDiscovered
);

// Complex message with custom settings
FEventMessage CustomMessage;
CustomMessage.MessageText = FText::FromString("Epic Sword of Power received!");
CustomMessage.MessageType = EEventMessageType::ItemReceived;
CustomMessage.MessageColor = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f); // Orange
CustomMessage.DisplayDuration = 8.0f; // Show longer for important items
CustomMessage.bIsImportant = true;
HUDWidget->AddEventMessage(CustomMessage);
```

### Managing Panels (Future Expansion)

```cpp
// Show inventory panel
HUDWidget->ShowPanel(0); // InventoryPanel index

// Hide all panels
HUDWidget->HideAllPanels();

// Check if panel is visible
bool bInventoryOpen = HUDWidget->IsPanelVisible(0);
```

## Build System Updates

### 1. Update RadiantRPG.Build.cs

Ensure UMG is included in your build dependencies:

```csharp
PublicDependencyModuleNames.AddRange(new string[] {
    "Core",
    "CoreUObject", 
    "Engine",
    "InputCore",
    "EnhancedInput",
    "AIModule",
    "StateTreeModule",
    "GameplayStateTreeModule",
    "UMG",           // Already included
    "Slate",         // Add if not present
    "SlateCore"      // Add if not present
});
```

## Testing the HUD System

### 1. Basic Functionality Test

Create a simple test function in your character:

```cpp
// In RadiantRPGCharacter.h
UFUNCTION(BlueprintCallable, Category = "Testing")
void TestHUDMessages();

// In RadiantRPGCharacter.cpp
void ARadiantRPGCharacter::TestHUDMessages()
{
    if (!HUDWidget) return;
    
    // Test different message types
    HUDWidget->AddEventMessageSimple(FText::FromString("Archery skill increased to 25"), EEventMessageType::SkillLevelUp);
    HUDWidget->AddEventMessageSimple(FText::FromString("Ancient Temple discovered"), EEventMessageType::LocationDiscovered);
    HUDWidget->AddEventMessageSimple(FText::FromString("Quest: Find the Lost Artifact updated"), EEventMessageType::QuestUpdated);
    HUDWidget->AddEventMessageSimple(FText::FromString("Iron Sword received"), EEventMessageType::ItemReceived);
    HUDWidget->AddEventMessageSimple(FText::FromString("Warning: Low health!"), EEventMessageType::Warning);
}
```

### 2. Stat Bar Testing

Test stat changes to verify bar behavior:

```cpp
// Test health changes
if (HealthComponent)
{
    HealthComponent->TakeDamageSimple(25.0f); // Should show health bar
    
    // After 3 seconds, heal
    FTimerHandle HealTimer;
    GetWorld()->GetTimerManager().SetTimer(HealTimer, [this]() {
        if (HealthComponent) HealthComponent->Heal(50.0f);
    }, 3.0f, false);
}
```

## Performance Considerations

### Event Message Optimization
- **Message Pooling**: Consider object pooling for frequent messages
- **Max Display Limit**: System automatically limits to 5 concurrent messages
- **Automatic Cleanup**: Messages self-destruct after display duration

### Stat Bar Efficiency
- **Change Detection**: Only updates when values actually change (0.01% threshold)
- **Visibility Optimization**: Bars are completely hidden when not needed
- **Timer Management**: Uses single timers per stat type to minimize overhead

### Memory Management
- **Widget Cleanup**: Event messages automatically remove themselves
- **Component Caching**: Player references cached for efficiency
- **Render Optimization**: Hidden elements don't consume render time

## Customization Options

### Theme Colors
All colors are exposed as `UPROPERTY(EditAnywhere)` for easy customization:

```cpp
// Modify in Blueprint or C++
HealthBarColor = FLinearColor(0.8f, 0.2f, 0.2f, 1.0f);
StaminaBarColor = FLinearColor(0.2f, 0.8f, 0.2f, 1.0f);
ManaBarColor = FLinearColor(0.2f, 0.2f, 0.8f, 1.0f);
```

### Animation Timing
Adjust animation speeds and durations:

```cpp
StatBarVisibilityDuration = 2.0f;  // How long bars stay visible
StatBarFadeSpeed = 5.0f;           // Fade in/out speed
EventMessageFadeSpeed = 3.0f;      // Message fade speed
EventMessageSlideSpeed = 2.0f;     // Message slide speed
```

### Message Appearance
Customize message types in the EventMessageWidget constructor:

```cpp
MessageTypeColors.Add(EEventMessageType::ExperienceGain, FLinearColor::Green);
MessageTypeColors.Add(EEventMessageType::SkillLevelUp, FLinearColor::Yellow);
// Add custom colors for your game's theme
```

## Future Expansion Areas

The HUD system is designed for easy expansion:

### 1. Minimap Integration
- Add minimap widget to the main canvas
- Integrate with world zone system
- Show faction territories and points of interest

### 2. Quest Tracker
- Add dedicated quest panel
- Show active objectives
- Update automatically with quest system

### 3. Hotbar System
- Add skill/item hotbars
- Support for multiple hotbar configurations
- Drag-and-drop functionality

### 4. Social Features
- Chat window integration
- Guild/faction information
- Player status indicators

### 5. Advanced Notifications
- Tooltip system for detailed information
- Modal dialog support
- Achievement notifications

## Troubleshooting

### Common Issues

**1. HUD Not Appearing**
- Verify HUDWidgetClass is set in Blueprint
- Check that character is locally controlled
- Ensure widget is added to viewport

**2. Stat Bars Not Updating**
- Confirm component event bindings in InitializeHUD()
- Check that components exist on character
- Verify change detection threshold (0.01%)

**3. Event Messages Not Sliding**
- Check EventMessageWidget parent class is set correctly
- Verify animation curves are properly configured
- Ensure proper widget hierarchy in Blueprint

**4. Performance Issues**
- Reduce MaxEventMessages if needed
- Increase tick intervals for less critical updates
- Profile widget blueprint complexity

### Debug Commands

Add these console commands for testing:

```cpp
// In your character class
UFUNCTION(CallInEditor = true, Category = "Debug")
void DebugHUD()
{
    if (HUDWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("HUD Active: %s"), HUDWidget ? TEXT("Yes") : TEXT("No"));
        UE_LOG(LogTemp, Warning, TEXT("Health Bar Visible: %s"), HUDWidget->IsHealthBarVisible() ? TEXT("Yes") : TEXT("No"));
        // Add more debug info as needed
    }
}
```

## Conclusion

The RadiantRPG HUD system provides a solid foundation for your game's user interface with:

- **Modular Design**: Easy to extend and customize
- **Performance Optimized**: Efficient rendering and memory usage  
- **Event-Driven**: Reactive to game state changes
- **Flexible Styling**: Easily themed and branded
- **Future-Proof**: Designed for expansion

The system integrates seamlessly with your existing component architecture and provides a professional, polished user experience that scales with your game's complexity.