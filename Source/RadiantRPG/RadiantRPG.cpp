// Source/RadiantRPG/Private/RadiantRPG.cpp

#include "RadiantRPG.h"
#include "Modules/ModuleManager.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY(LogRadiantRPG);

void FRadiantRPGModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	UE_LOG(LogRadiantRPG, Warning, TEXT("RadiantRPG module starting up..."));
	
	// Initialize any global systems here if needed
	InitializeGameplaySystems();
	
	UE_LOG(LogRadiantRPG, Warning, TEXT("RadiantRPG module startup complete"));
}

void FRadiantRPGModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module
	UE_LOG(LogRadiantRPG, Warning, TEXT("RadiantRPG module shutting down..."));
	
	// Clean up any global systems here
	ShutdownGameplaySystems();
	
	UE_LOG(LogRadiantRPG, Warning, TEXT("RadiantRPG module shutdown complete"));
}

void FRadiantRPGModule::InitializeGameplaySystems()
{
	// Initialize any module-level systems here
	// This runs before any game classes are instantiated
	
	// Example: Register custom gameplay tags, asset types, etc.
	UE_LOG(LogRadiantRPG, Log, TEXT("Initializing gameplay systems..."));
	
	// TODO: Initialize custom systems when they're implemented
}

void FRadiantRPGModule::ShutdownGameplaySystems()
{
	// Clean up module-level systems here
	UE_LOG(LogRadiantRPG, Log, TEXT("Shutting down gameplay systems..."));
	
	// TODO: Cleanup custom systems when they're implemented
}

bool FRadiantRPGModule::IsGameModule() const
{
	return true;
}

IMPLEMENT_PRIMARY_GAME_MODULE(FRadiantRPGModule, RadiantRPG, "RadiantRPG");