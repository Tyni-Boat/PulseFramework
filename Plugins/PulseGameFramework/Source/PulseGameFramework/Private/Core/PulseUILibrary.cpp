// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/PulseUILibrary.h"

bool UPulseUILibrary::IsWidgetDisplayed(const UUserWidget* Widget)
{
	if (!IsValid(Widget))
	{
		return false;
	}
	
	if (Widget->IsConstructed())
	{
		TSharedPtr<SWidget> SlateWidget = Widget->GetCachedWidget();
		if (SlateWidget.IsValid())
		{
			// Widget is on screen
			return SlateWidget->GetVisibility().IsVisible();
		}
		return false;
	}

	// Directly added to viewport
	if (Widget->IsInViewport())
	{
		return true;
	}

	// Nested widget: visible + has parent
	const ESlateVisibility Visibility = Widget->GetVisibility();

	const bool bVisible =
		Visibility != ESlateVisibility::Collapsed &&
		Visibility != ESlateVisibility::Hidden;
	
	return bVisible && Widget->GetParent() != nullptr;
}
