// Copyright Epic Games, Inc. All Rights Reserved.

#include "NavDataGeneratorPlugin.h"

#define LOCTEXT_NAMESPACE "FNavDataGeneratorPluginModule"

void FNavDataGeneratorPluginModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FNavDataGeneratorPluginModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FNavDataGeneratorPluginModule, NavDataGeneratorPlugin)