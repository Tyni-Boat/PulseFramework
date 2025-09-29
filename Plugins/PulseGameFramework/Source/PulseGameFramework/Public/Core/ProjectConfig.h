// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CoreConcepts.h"


// Derive from this class on the (Sub)Module class.
// Derive from UDeveloperSettings on the T class, the Developer setting class
// and use UCLASS(config = Game, defaultconfig, meta = (DisplayName = "XXX Config Name XXX"))
template <pulseCore::concepts::IsAModuleConfig T>
class ConfigurableModule
{
public:
	virtual ~ConfigurableModule() = default;

	virtual T* GetProjectConfig() const
	{
		return nullptr;
	}
};
