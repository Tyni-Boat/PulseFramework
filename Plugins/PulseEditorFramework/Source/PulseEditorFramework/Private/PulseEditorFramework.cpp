// Copyright Epic Games, Inc. All Rights Reserved.

#include "PulseEditorFramework.h"
#include "PulseEditorFrameworkStyle.h"
#include "PulseEditorFrameworkCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"

static const FName PulseEditorFrameworkTabName("PulseEditorFramework");

#define LOCTEXT_NAMESPACE "FPulseEditorFrameworkModule"

void FPulseEditorFrameworkModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FPulseEditorFrameworkStyle::Initialize();
	FPulseEditorFrameworkStyle::ReloadTextures();

	FPulseEditorFrameworkCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FPulseEditorFrameworkCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FPulseEditorFrameworkModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FPulseEditorFrameworkModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(PulseEditorFrameworkTabName, FOnSpawnTab::CreateRaw(this, &FPulseEditorFrameworkModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FPulseEditorFrameworkTabTitle", "PulseEditorFramework"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FPulseEditorFrameworkModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FPulseEditorFrameworkStyle::Shutdown();

	FPulseEditorFrameworkCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PulseEditorFrameworkTabName);
}

TSharedRef<SDockTab> FPulseEditorFrameworkModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	FText WidgetText = FText::Format(
		LOCTEXT("WindowWidgetText", "Add code to {0} in {1} to override this window's contents"),
		FText::FromString(TEXT("FPulseEditorFrameworkModule::OnSpawnPluginTab")),
		FText::FromString(TEXT("PulseEditorFramework.cpp"))
		);

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// Put your tab content here!
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(WidgetText)
			]
		];
}

void FPulseEditorFrameworkModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(PulseEditorFrameworkTabName);
}

void FPulseEditorFrameworkModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FPulseEditorFrameworkCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FPulseEditorFrameworkCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPulseEditorFrameworkModule, PulseEditorFramework)