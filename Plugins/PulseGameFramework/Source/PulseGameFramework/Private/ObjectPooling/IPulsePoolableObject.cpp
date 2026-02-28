// Copyright © by Tyni Boat. All Rights Reserved.


#include "ObjectPooling/IPulsePoolableObject.h"

#include "ObjectPooling/PulseObjectPooling.h"


// Add default functionality here for any IIPoolingObject functions that are not pure virtual.


bool IIPulsePoolableObject::OnPoolQuery_Implementation(const FPoolingParams SpawnData)
{
	return true;
}

void IIPulsePoolableObject::OnPoolDispose_Implementation()
{
}

EPoolQueryResult IIPulsePoolableObject::Dispose()
{
	auto obj = Cast<UObject>(this);
	if (auto objPooling = UPulseObjectPooling::Get(obj))
	{
		return objPooling->DisposeObject(obj);
	}
	return EPoolQueryResult::Undefined;
}

EPoolQueryResult IIPulsePoolableObject::DisposeToPool_Implementation()
{
	return Dispose();
}