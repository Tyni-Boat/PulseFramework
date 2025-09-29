// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectConfig.h"
#include "PulseSubModuleBase.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "Core/CoreConcepts.h"
#include "PulseModuleBase.generated.h"


/**
 * The Mother class of all Module of the Pulse Framework. To be able to configure the module, derive from ConfigurableModule<T>
 * Where T is the type if the setting class (UDeveloperSettings).
 * use UCLASS(config = Game, defaultconfig, meta = (DisplayName = "XXX Config Name XXX")) on T class.
*/
UCLASS(Abstract, Within = GameInstance, NotBlueprintable, HideDropdown)
class PULSEGAMEFRAMEWORK_API UPulseModuleBase : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual FName GetModuleName() const;
	UFUNCTION(BlueprintPure, Category = "Pulse Module")
	TArray<FName> GetSubmoduleNames();
	UFUNCTION(BlueprintPure, Category = "Pulse Module")
	UPulseSubModuleBase* GetSubmoduleByName(const FName& SubModuleName);
	UFUNCTION(BlueprintPure, Category = "Pulse Module")
	UPulseSubModuleBase* GetSubmoduleByType(const TSubclassOf<UPulseSubModuleBase> Type, const FName SubModuleName = NAME_None);

	template <pulseCore::concepts::IsSubModule T, typename Q>
	requires pulseCore::concepts::IsSubclassOf<T, Q>
	inline T* GetSubModule(Q Type)
	{
		if (!_SubModuleMap.Contains(Type))
			return nullptr;
		return Cast<T>(_SubModuleMap[Type]);
	}

	template <pulseCore::concepts::IsSubModule T>
	inline T* GetSubModule()
	{
		return GetSubModule<T>(T::StaticClass());
	}

	virtual TStatId GetStatId() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override;
	virtual ETickableTickType GetTickableTickType() const override;

	virtual bool IsTickable() const override;
	virtual bool IsTickableInEditor() const override;
	virtual bool IsTickableWhenPaused() const override;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;

protected:

	// The order is important for sub-module inter dependency on initialization
	virtual TArray<TSubclassOf<UPulseSubModuleBase>> GetSubmodulesTypes() const;

	UPROPERTY(VisibleAnywhere)
	TMap<UClass*, UPulseSubModuleBase*> _SubModuleMap;
};
