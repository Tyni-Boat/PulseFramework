// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CoreConcepts.h"
#include "UObject/Object.h"
#include "PulseSubModuleBase.generated.h"

class UPulseModuleBase;

/**
* The Mother class of all Sub-Module of the Pulse Framework. To be able to configure the submodule, derive from ConfigurableModule<T>
 * Where T is the type if the setting class.
 */
UCLASS(Abstract, Within = PulseModuleBase)
class PULSEGAMEFRAMEWORK_API UPulseSubModuleBase : public UObject
{
	GENERATED_BODY()

public:
	virtual FName GetSubModuleName() const;
	virtual bool WantToTick() const;
	virtual bool TickWhenPaused() const;
	UPulseModuleBase* GetOwningModule();

	virtual void InitializeSubModule(UPulseModuleBase* OwningModule);
	virtual void DeinitializeSubModule();
	virtual void TickSubModule(float DeltaTime, float CurrentTimeDilation = 1, bool bIsGamePaused = false);

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPulseModuleBase> _OwningModule;
};
