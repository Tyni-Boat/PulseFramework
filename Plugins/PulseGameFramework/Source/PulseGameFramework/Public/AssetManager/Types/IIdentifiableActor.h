// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include <UObject/Interface.h>
#include "IIdentifiableActor.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UIIdentifiableActor : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implement by any actor that can be identifier to an asset primary ID
 */
class PULSEGAMEFRAMEWORK_API IIIdentifiableActor
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	FPrimaryAssetId GetID();
};
