// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PulseUILibrary.generated.h"

/**
 * Library of helper functions for the UI
 */
UCLASS()
class PULSEGAMEFRAMEWORK_API UPulseUILibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	public:

	/**
	 * @brief Check if a user widget is actually displayed onscreen.
	 **/
	UFUNCTION(BlueprintPure, Category = "PulseUILibrary", meta=(CompactNodeTitle = "IsDisplayed"))
	static bool IsWidgetDisplayed(const UUserWidget* Widget);
};
