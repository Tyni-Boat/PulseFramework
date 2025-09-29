// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/PulseModuleBase.h"

#include "Core/PulseNetworkingModule/IPulseNetObject.h"
#include "ProfilingDebugging/TagTrace.h"
#include "Kismet/GameplayStatics.h"


FName UPulseModuleBase::GetModuleName() const
{
	return StaticClass()->GetFName();
}

TArray<FName> UPulseModuleBase::GetSubmoduleNames()
{
	TArray<FName> Result;
	for (const auto SubModule : _SubModuleMap)
		Result.Add(SubModule.Value ? SubModule.Value->GetSubModuleName() : FName());
	return Result;
}

UPulseSubModuleBase* UPulseModuleBase::GetSubmoduleByName(const FName& SubModuleName)
{
	for (const auto SubModule : _SubModuleMap)
		if (SubModule.Value && SubModule.Value->GetSubModuleName() == SubModuleName)
			return SubModule.Value;
	return nullptr;
}

UPulseSubModuleBase* UPulseModuleBase::GetSubmoduleByType(const TSubclassOf<UPulseSubModuleBase> Type, const FName SubModuleName)
{
	if (!SubModuleName.IsNone())
		return GetSubmoduleByName(SubModuleName);
	if (Type == nullptr)
		return nullptr;
	const FName subName = Type->StaticClass()->GetFName();
	return GetSubmoduleByName(subName);
}


TStatId UPulseModuleBase::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UPulseFrameworkModule, STATGROUP_Tickables);
}

UWorld* UPulseModuleBase::GetTickableGameObjectWorld() const
{
	return FTickableGameObject::GetTickableGameObjectWorld();
}

ETickableTickType UPulseModuleBase::GetTickableTickType() const
{
	return ETickableTickType::Never; // FTickableGameObject::GetTickableTickType();
}

bool UPulseModuleBase::IsTickable() const
{
	return false; //FTickableGameObject::IsTickable();
}

bool UPulseModuleBase::IsTickableInEditor() const
{
	return false; // FTickableGameObject::IsTickableInEditor();
}

bool UPulseModuleBase::IsTickableWhenPaused() const
{
	return false; // FTickableGameObject::IsTickableWhenPaused();
}

void UPulseModuleBase::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	TRACE_CPUPROFILER_EVENT_SCOPE(UPulseModuleBase::Initialize);
	if (!GetWorld())
		return;
	auto moduleTypes = GetSubmodulesTypes();
	for (int i = 0; i < moduleTypes.Num(); i++)
	{
		const auto moduleType = moduleTypes[i];
		if (_SubModuleMap.Contains(moduleType))
			continue;
		auto moduleInstance = NewObject<UPulseSubModuleBase>(this, moduleType, FName(moduleType->GetName()));
		_SubModuleMap.Add(moduleType, moduleInstance);
		// Bind Net comp
		if (auto asNetObj = Cast<IIPulseNetObject>(moduleInstance))
			asNetObj->BindNetworkManager();
		moduleInstance->InitializeSubModule(this);
	}
}

void UPulseModuleBase::Deinitialize()
{
	Super::Deinitialize();
	TRACE_CPUPROFILER_EVENT_SCOPE(UPulseModuleBase::Deinitialize);
	for (auto module : _SubModuleMap)
	{
		if (!module.Value)
			continue;
		if (auto asNetObj = Cast<IIPulseNetObject>(module.Value))
			asNetObj->UnbindNetworkManager();
		module.Value->DeinitializeSubModule();
	}
	_SubModuleMap.Empty();
}

void UPulseModuleBase::Tick(float DeltaTime)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UPulseModuleBase::Tick);
	auto world = GetWorld();
	float timeDilation = world ? UGameplayStatics::GetGlobalTimeDilation(world) : 1.0f;
	bool isPaused = world ? UGameplayStatics::IsGamePaused(world) : true;
	for (auto module : _SubModuleMap)
	{
		if (!module.Value)
			continue;
		if (!module.Value->WantToTick())
			continue;
		if (isPaused && !module.Value->TickWhenPaused())
			continue;
		module.Value->TickSubModule(DeltaTime, timeDilation, isPaused);
	}
}

TArray<TSubclassOf<UPulseSubModuleBase>> UPulseModuleBase::GetSubmodulesTypes() const
{
	return {};
}
