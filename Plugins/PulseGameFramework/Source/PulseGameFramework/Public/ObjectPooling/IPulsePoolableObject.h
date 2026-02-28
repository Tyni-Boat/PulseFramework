// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ObjectPooling/PulsePoolingTypes.h"
#include "IPulsePoolableObject.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UIPulsePoolableObject : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface to make an object compatible with tha pulse pooling system.
 */
class PULSEGAMEFRAMEWORK_API IIPulsePoolableObject
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pooling System")
	bool OnPoolQuery(const FPoolingParams SpawnData);

	virtual bool OnPoolQuery_Implementation(const FPoolingParams SpawnData);


	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pooling System")
	void OnPoolDispose();

	virtual void OnPoolDispose_Implementation();
	
	EPoolQueryResult Dispose();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pooling System")
	EPoolQueryResult DisposeToPool();

	virtual EPoolQueryResult DisposeToPool_Implementation();
};
