// Source/RadiantRPG/Private/RadiantRPG.cpp

#include "RadiantRPG.h"
#include "Modules/ModuleManager.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY(LogRadiantRPG);
DEFINE_LOG_CATEGORY(LogARPG);

void FRadiantRPGModule::StartupModule()
{
	UE_LOG(LogRadiantRPG, Warning, TEXT("RadiantRPG module starting up..."));
	
	InitializeGameplaySystems();
	
	UE_LOG(LogRadiantRPG, Warning, TEXT("RadiantRPG module startup complete"));
}

void FRadiantRPGModule::ShutdownModule()
{
	UE_LOG(LogRadiantRPG, Warning, TEXT("RadiantRPG module shutting down..."));
	
	ShutdownGameplaySystems();
	
	UE_LOG(LogRadiantRPG, Warning, TEXT("RadiantRPG module shutdown complete"));
}

void FRadiantRPGModule::InitializeGameplaySystems()
{
	UE_LOG(LogRadiantRPG, Log, TEXT("Initializing gameplay systems..."));
	
	// TODO: Initialize custom systems when they're implemented
}

void FRadiantRPGModule::ShutdownGameplaySystems()
{
	UE_LOG(LogRadiantRPG, Log, TEXT("Shutting down gameplay systems..."));
	
	// TODO: Cleanup custom systems when they're implemented
}

bool FRadiantRPGModule::IsGameModule() const
{
	return true;
}

IMPLEMENT_PRIMARY_GAME_MODULE(FRadiantRPGModule, RadiantRPG, "RadiantRPG");