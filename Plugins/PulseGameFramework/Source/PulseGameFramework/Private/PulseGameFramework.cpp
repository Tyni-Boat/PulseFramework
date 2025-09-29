// Copyright Epic Games, Inc. All Rights Reserved.

#include "PulseGameFramework.h"
#include "Core/PulseTweenSubModule/PulseTweenSystemCore.h"


#define LOCTEXT_NAMESPACE "FPulseGameFrameworkModule"

void FPulseGameFrameworkModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	PulseTweenSystemCore::Initialize();
}

void FPulseGameFrameworkModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	PulseTweenSystemCore::Deinitialize();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPulseGameFrameworkModule, PulseGameFramework)