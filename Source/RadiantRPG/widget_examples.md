# Widget Implementation Examples for RadiantRPG

## Crosshair Widget (WBP_Crosshair)

Create a UMG widget with these elements:

### Widget Structure:
```
Canvas Panel (Fill Screen)
└── Center Box (Center in Parent)
    └── Image (Crosshair)
        ├── Size: 32x32
        ├── Anchor: Center
        ├── Color: White (Default) / Green (Interactable)
```

### Blueprint Functions to Implement:

#### OnInteractableStateChanged (Custom Event)
- **Input**: Boolean `bCanInteract`
- **Logic**: 
  - If `bCanInteract` is true: Set Image color to Green
  - If `bCanInteract` is false: Set Image color to White

```cpp
// In Blueprint Event Graph
Event OnInteractableStateChanged
├── Branch (bCanInteract)
    ├── True: Set Color and Opacity (Green, Alpha 1.0)
    └── False: Set Color and Opacity (White, Alpha 0.8)
```

## Interaction Prompt Widget (WBP_InteractionPrompt)

Create a UMG widget with these elements:

### Widget Structure:
```
Canvas Panel (Fill Screen)
└── Vertical Box (Bottom Center)
    ├── Anchor: Bottom Center
    ├── Position: (0, -100) from bottom
    └── Text Block (Interaction Text)
        ├── Font Size: 18
        ├── Color: White
        ├── Shadow: Enabled
        ├── Horizontal Alignment: Center
```

### Blueprint Functions to Implement:

#### SetPromptAlpha (Custom Event)
- **Input**: Float `Alpha`
- **Logic**: Set the Text Block's render opacity to Alpha value

#### UpdatePromptText (Custom Event)  
- **Input**: Text `NewPrompt`
- **Logic**: Set the Text Block's text to NewPrompt

```cpp
// In Blueprint Event Graph
Event SetPromptAlpha
└── Set Render Opacity (Text Block, Alpha)

Event UpdatePromptText  
└── Set Text (Text Block, NewPrompt)
```

## C++ Widget Base Classes (Optional)

If you prefer C++ widgets, create these base classes:

### CrosshairWidget.h
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "CrosshairWidget.generated.h"

UCLASS()
class RADIANTRPG_API UCrosshairWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    UPROPERTY(meta = (BindWidget))
    class UImage* CrosshairImage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor DefaultColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor InteractableColor;

public:
    UFUNCTION(BlueprintCallable, Category = "Crosshair")
    void OnInteractableStateChanged(bool bCanInteract);
};
```

### InteractionPromptWidget.h
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "InteractionPromptWidget.generated.h"

UCLASS()
class RADIANTRPG_API UInteractionPromptWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* PromptText;

public:
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void SetPromptAlpha(float Alpha);

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void UpdatePromptText(const FText& NewPrompt);
};
```

## Setup Instructions

1. **Create the Widget Blueprints**:
   - Right-click in Content Browser → User Interface → Widget Blueprint
   - Name them `WBP_Crosshair` and `WBP_InteractionPrompt`

2. **Design the Widgets**:
   - Follow the structure above
   - Use appropriate fonts, colors, and sizing

3. **Implement Events**:
   - Add the custom events listed above
   - Connect them to update the visual elements

4. **Assign to Character**:
   - In your character Blueprint or C++ class
   - Set the `CrosshairWidgetClass` and `InteractionPromptWidgetClass` variables
   - Reference your created widget Blueprints

5. **Test the System**:
   - Place a SimpleInteractable in your level
   - Run the game and test camera switching and interaction detection
   - The crosshair should change color when targeting interactables
   - The prompt should fade in/out smoothly

## Styling Tips

### For a More Polished Look:
- Add subtle animations to the crosshair (scale pulse when targeting)
- Use gradient backgrounds for the interaction prompt
- Add key binding hints (e.g., "Press E to Use")
- Consider different prompt styles for different interaction types
- Add sound effects triggered by the UI component events

### Responsive Design:
- Use DPI scaling for different screen resolutions
- Test on various aspect ratios
- Consider safe zones for console platforms