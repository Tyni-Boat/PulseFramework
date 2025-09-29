// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once
#include "Core/PulseResourceManagement/Types/Character/BasePulseCharacterAsset.h"
#include "PulseGameFramework/Public/Core/PulseModuleBase.h"
#include "PulseGameFramework/Public/Core/ProjectConfig.h"
#include "PulseGameFramework/Public/Core/PulseSubModuleBase.h"
#include "CoreTestTypes.generated.h"


USTRUCT()
struct FSerializationTester1
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, Category=Test)
	float TestValue = 250;
};

USTRUCT()
struct FSerializationTester2
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, Category=Test)
	float TestValue = 250;
};


UCLASS(config = Game, defaultconfig, Category = Settings, meta = (DisplayName = "-->[Settings|Test Core Module]"))
class PULSETESTFRAMEWORK_API UCoreTestModuleConfig: public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, config, Category="Pulse Game Framework")
	float ModuleTestValue = 154;
	UPROPERTY(EditAnywhere, config, Category="Pulse Game Framework")
	float SubModuleTestValue = 256;
	UPROPERTY(EditAnywhere, config, Category="Pulse Game Framework")
	FSerializationTester1 Struct;
};

UCLASS(config = Game, defaultconfig, Category = Settings, meta = (DisplayName = "-->[Settings|Test Core Module]"))
class PULSETESTFRAMEWORK_API UCoreTestModuleConfigInherited: public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, config, Category="Pulse Game Framework")
	int32 ModuleTestValue = 154;
	UPROPERTY(EditAnywhere, config, Category="Pulse Game Framework")
	float SubModuleTestValue2 = 256;
	UPROPERTY(EditAnywhere, config, Category="Pulse Game Framework")
	FSerializationTester2 Struct;
};


UCLASS()
class PULSETESTFRAMEWORK_API UCoreTestModule : public UPulseModuleBase, public ConfigurableModule<UCoreTestModuleConfig>
{
	GENERATED_BODY()

public:
	
	virtual FName GetModuleName() const override;
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
	virtual TArray<TSubclassOf<UPulseSubModuleBase>> GetSubmodulesTypes() const override;

public:
	virtual UCoreTestModuleConfig* GetProjectConfig() const override { return GetMutableDefault<UCoreTestModuleConfig>(); }
};


UCLASS()
class PULSETESTFRAMEWORK_API UCoreTestSubModule : public UPulseSubModuleBase
{
	GENERATED_BODY()

public:
	virtual FName GetSubModuleName() const override;
	virtual bool WantToTick() const override;
	virtual bool TickWhenPaused() const override;
	virtual void InitializeSubModule(UPulseModuleBase* OwningModule) override;
	virtual void DeinitializeSubModule() override;
	virtual void TickSubModule(float DeltaTime, float CurrentTimeDilation = 1, bool bIsGamePaused = false) override;
};


UCLASS()
class PULSETESTFRAMEWORK_API UCoreTestSubModule2 : public UPulseSubModuleBase
{
	GENERATED_BODY()

public:
	virtual FName GetSubModuleName() const override;
	virtual bool WantToTick() const override;
	virtual bool TickWhenPaused() const override;
	virtual void InitializeSubModule(UPulseModuleBase* OwningModule) override;
	virtual void DeinitializeSubModule() override;
	virtual void TickSubModule(float DeltaTime, float CurrentTimeDilation = 1, bool bIsGamePaused = false) override;
};


UCLASS(BlueprintType, Blueprintable)
class UTestPulseAsset : public UBasePulseCharacterAsset
{
	GENERATED_BODY()

public:
	
};
