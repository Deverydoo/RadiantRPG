// Source/RadiantRPG/Public/RadiantRPG.h

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRadiantRPG, Log, All);

/**
 * Main module class for RadiantRPG
 * Handles module initialization and cleanup
 */
class FRadiantRPGModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool IsGameModule() const override;

private:
	/** Initialize gameplay systems during module startup */
	void InitializeGameplaySystems();
	
	/** Shutdown gameplay systems during module cleanup */
	void ShutdownGameplaySystems();
};