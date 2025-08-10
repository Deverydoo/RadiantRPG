// Source/RadiantRPG/Public/AI/Interfaces/IARPG_AIBrainInterface.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Types/ARPG_AITypes.h"
#include "IARPG_AIBrainInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UARPG_AIBrainInterface : public UInterface
{
 GENERATED_BODY()
};

/**
 * Interface for AI Brain systems
 * Allows different brain implementations while maintaining modular design
 */
class RADIANTRPG_API IARPG_AIBrainInterface
{
 GENERATED_BODY()

public:
 /**
  * Initialize the brain with configuration data
  * @param Config Brain-specific configuration
  */
 virtual void InitializeBrain(const FARPG_AIBrainConfiguration& Config) = 0;

 /**
  * Process incoming stimuli and update brain state
  * @param Stimulus The incoming stimulus to process
  */
 virtual void ProcessStimulus(const FARPG_AIStimulus& Stimulus) = 0;

 /**
  * Generate intent based on current brain state
  * @return Current dominant intent
  */
 virtual FARPG_AIIntent GenerateIntent() = 0;

 /**
  * Update brain processing - called each tick
  * @param DeltaTime Time since last update
  */
 virtual void UpdateBrain(float DeltaTime) = 0;

 /**
  * Check if brain is in curiosity mode (no recent stimuli)
  * @return True if curiosity should activate
  */
 virtual bool ShouldActivateCuriosity() const = 0;

 /**
  * Get current brain state for debugging
  * @return Current brain state information
  */
 virtual FARPG_AIBrainState GetBrainState() const = 0;

 /**
  * Set brain enabled/disabled state
  * @param bEnabled Whether brain should be active
  */
 virtual void SetBrainEnabled(bool bEnabled) = 0;

 /**
  * Get the brain's current intent confidence
  * @return Confidence in current intent (0-1)
  */
 virtual float GetIntentConfidence() const = 0;
};