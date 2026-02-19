// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPulseAssetManagement, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogPulseDownloader, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogPulseNetProxy, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogPulseObjectPooling, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogPulseSave, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogPulseTweening, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogPulseUserProfile, Log, All);

class FPulseGameFrameworkModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
