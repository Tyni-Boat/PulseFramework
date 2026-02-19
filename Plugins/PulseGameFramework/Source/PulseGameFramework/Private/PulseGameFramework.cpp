// Copyright Epic Games, Inc. All Rights Reserved.

#include "PulseGameFramework.h"


#define LOCTEXT_NAMESPACE "FPulseGameFrameworkModule"
DEFINE_LOG_CATEGORY(LogPulseAssetManagement);
DEFINE_LOG_CATEGORY(LogPulseDownloader);
DEFINE_LOG_CATEGORY(LogPulseNetProxy);
DEFINE_LOG_CATEGORY(LogPulseObjectPooling);
DEFINE_LOG_CATEGORY(LogPulseSave);
DEFINE_LOG_CATEGORY(LogPulseTweening);
DEFINE_LOG_CATEGORY(LogPulseUserProfile);

void FPulseGameFrameworkModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FPulseGameFrameworkModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPulseGameFrameworkModule, PulseGameFramework)