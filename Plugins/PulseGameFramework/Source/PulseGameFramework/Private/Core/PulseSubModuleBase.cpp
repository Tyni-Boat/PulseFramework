// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/PulseSubModuleBase.h"


FName UPulseSubModuleBase::GetSubModuleName() const
{
	return StaticClass()->GetFName();
}

bool UPulseSubModuleBase::WantToTick() const
{
	return false;
}

bool UPulseSubModuleBase::TickWhenPaused() const
{
	return false;
}

UPulseModuleBase* UPulseSubModuleBase::GetOwningModule()
{
	return _OwningModule;
}

void UPulseSubModuleBase::InitializeSubModule(UPulseModuleBase* OwningModule)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UPulseSubModuleBase::InitializeSubModule);
	_OwningModule = OwningModule;
}

void UPulseSubModuleBase::DeinitializeSubModule()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UPulseSubModuleBase::DeinitializeSubModule);
}

void UPulseSubModuleBase::TickSubModule(float DeltaTime, float CurrentTimeDilation, bool bIsGamePaused)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UPulseSubModuleBase::TickSubModule);
}
