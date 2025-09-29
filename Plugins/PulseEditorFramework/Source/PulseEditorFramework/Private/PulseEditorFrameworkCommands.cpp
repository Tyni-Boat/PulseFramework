// Copyright Epic Games, Inc. All Rights Reserved.

#include "PulseEditorFrameworkCommands.h"

#define LOCTEXT_NAMESPACE "FPulseEditorFrameworkModule"

void FPulseEditorFrameworkCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "PulseEditorFramework", "Bring up PulseEditorFramework window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
