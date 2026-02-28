// Copyright © by Tyni Boat. All Rights Reserved.


#include "ObjectPooling/PulsePoolingTypes.h"


bool FPoolingParams::IsValid() const
{
	if (!TransformParams.IsEmpty())
		return true;
	if (!VectorParams.IsEmpty())
		return true;
	if (!ColorParams.IsEmpty())
		return true;
	if (!RotationParams.IsEmpty())
		return true;
	if (!ValueParams.IsEmpty())
		return true;
	if (!AssetParams.IsEmpty())
		return true;
	if (!NamesParams.IsEmpty())
		return true;
	if (!CustomParams.IsEmpty())
		return true;
	return false;
}
