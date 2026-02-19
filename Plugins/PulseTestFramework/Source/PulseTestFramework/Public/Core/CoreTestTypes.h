// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once
#include "Core/AssetManager/Types/Character/BasePulseCharacterAsset.h"
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



UCLASS(BlueprintType, Blueprintable)
class UTestPulseAsset : public UBasePulseCharacterAsset
{
	GENERATED_BODY()

public:
	
};
