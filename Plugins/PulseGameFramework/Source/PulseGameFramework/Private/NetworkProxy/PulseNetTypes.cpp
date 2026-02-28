// Copyright © by Tyni Boat. All Rights Reserved.


#include "NetworkProxy/PulseNetTypes.h"
#include "Core/PulseSystemLibrary.h"



FString FPulseNetReplicatedData::ToString() const
{
	FString result;
	result.Append(TEXT(">>[ "));
	result.Append(FString::Printf(TEXT("Tag: %s"), *Tag.ToString()));
	result.Append(FString::Printf(TEXT("; Arrival: %lld"), ServerArrivalOrder));
	result.Append(FString::Printf(TEXT("; Owner: %d"), OwnerPlayerID));
	result.Append(FString::Printf(TEXT("; Version: %lld"), ItemVersion));
	if (SoftClassPtr != nullptr) result.Append(FString::Printf(TEXT("; SoftClass: %s"), *SoftClassPtr.ToString()));
	if (!NameValue.IsNone()) result.Append(FString::Printf(TEXT("; NameVal: %s"), *NameValue.ToString()));
	if (!StringValue.IsEmpty()) result.Append(FString::Printf(TEXT("; StringVal: %s"), *StringValue));
	if (EnumValue != 0) result.Append(FString::Printf(TEXT("; EnumVal: %d"), EnumValue));
	if (FlagValue != 0) result.Append(FString::Printf(TEXT("; FlagVal: %d"), FlagValue));
	if (IntegerValue != 0) result.Append(FString::Printf(TEXT("; IntegerVal: %d"), IntegerValue));
	if (!FMath::IsNearlyZero(DoubleValue)) result.Append(FString::Printf(TEXT("; IntegerVal: %f"), DoubleValue));
	if (!Float31Value.IsNearlyZero()) result.Append(FString::Printf(TEXT("; float31Val: (%s)"), *Float31Value.ToCompactString()));
	if (!Float32Value.IsNearlyZero()) result.Append(FString::Printf(TEXT("; float32Val: (%s)"), *Float32Value.ToCompactString()));
	if (!Float33Value.IsNearlyZero()) result.Append(FString::Printf(TEXT("; float33Val: (%s)"), *Float33Value.ToCompactString()));
	result.Append(TEXT(" ] "));
	const auto size = sizeof(Tag)
		+ sizeof(ServerArrivalOrder)
		+ sizeof(ItemVersion)
		+ sizeof(OwnerPlayerID)
		+ sizeof(ItemVersion)
		+ sizeof(SoftClassPtr) + SoftClassPtr.ToSoftObjectPath().GetAssetPathString().GetAllocatedSize()
		+ sizeof(NameValue)
		+ sizeof(StringValue) + StringValue.GetAllocatedSize()
		+ sizeof(EnumValue)
		+ sizeof(FlagValue)
		+ sizeof(IntegerValue)
		+ sizeof(DoubleValue)
		+ sizeof(Float31Value)
		+ sizeof(Float32Value)
		+ sizeof(Float33Value);
	result.Append(FString::Printf(TEXT("{ Size: %s}<<"), *UPulseSystemLibrary::FileSizeToString(size)));
	return result;
}
