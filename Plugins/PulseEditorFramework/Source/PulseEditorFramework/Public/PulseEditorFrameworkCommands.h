// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Framework/Commands/Commands.h"
#include "PulseEditorFrameworkStyle.h"

class FPulseEditorFrameworkCommands : public TCommands<FPulseEditorFrameworkCommands>
{
public:

	FPulseEditorFrameworkCommands()
		: TCommands<FPulseEditorFrameworkCommands>(TEXT("PulseEditorFramework"), NSLOCTEXT("Contexts", "PulseEditorFramework", "PulseEditorFramework Plugin"), NAME_None, FPulseEditorFrameworkStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};