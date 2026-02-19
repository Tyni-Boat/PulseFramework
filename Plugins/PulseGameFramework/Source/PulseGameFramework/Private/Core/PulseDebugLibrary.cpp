// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/PulseDebugLibrary.h"
#include "Kismet/KismetSystemLibrary.h"


void UPulseDebugLibrary::DrawDebugTransform(const UObject* WorldContext, const FTransform Transform, FLinearColor Color, float Duration, float Size)
{
	if (!WorldContext)
		return;
	const FVector start = Transform.GetLocation();
	const FVector end = start + Transform.GetRotation().GetForwardVector() * Size * 10;
	UKismetSystemLibrary::DrawDebugArrow(WorldContext, start, end, Size * 10, Color, Duration, Size);
	UKismetSystemLibrary::DrawDebugPoint(WorldContext, start, Size * 5, Color, Duration);
}

void UPulseDebugLibrary::DrawDebugBasis(const UObject* WorldContext, const FVector DrawLocation, const FVector ForwardVector, const FVector RightVector, const FVector UpVector,
	float Duration, float Size)
{
	const FVector start = DrawLocation;
	const FVector end_f = start + ForwardVector.GetSafeNormal() * Size * 10;
	const FVector end_r = start + RightVector.GetSafeNormal() * Size * 10;
	const FVector end_u = start + UpVector.GetSafeNormal() * Size * 10;
	const FLinearColor f_col = FColor::Red;
	const FLinearColor r_col = FMath::IsNearlyEqual(ForwardVector.GetSafeNormal() | RightVector.GetSafeNormal(), 0)? FColor::Green : FColor::Silver; 
	const FLinearColor u_col = FMath::IsNearlyEqual(ForwardVector.GetSafeNormal() | UpVector.GetSafeNormal(), 0)? FColor::Blue : FColor::Silver;
	UKismetSystemLibrary::DrawDebugArrow(WorldContext, start, end_f, Size * 10, f_col, Duration, Size);
	UKismetSystemLibrary::DrawDebugArrow(WorldContext, start, end_r, Size * 10, r_col, Duration, Size);
	UKismetSystemLibrary::DrawDebugArrow(WorldContext, start, end_u, Size * 10, u_col, Duration, Size);
}

FString UPulseDebugLibrary::DebugNetLog(const UObject* Obj, const FString& message)
{
	return FString::Printf(TEXT("[%s-{%d}] - %s"), *NetModeStr(Obj), NetClientId(Obj), *message);
}

FString UPulseDebugLibrary::NetModeStr(const UObject* Obj)
{
	if (!Obj) return TEXT("NULL");

	UWorld* W = Obj->GetWorld();
	if (!W) return TEXT("NO_WORLD");

	switch (W->GetNetMode())
	{
	case NM_Standalone:    return TEXT("Standalone");
	case NM_Client:        return TEXT("Client");
	case NM_ListenServer:  return TEXT("ListenServer");
	case NM_DedicatedServer:return TEXT("Server");
	default:               return TEXT("Unknown");
	}
}

int32 UPulseDebugLibrary::NetClientId(const UObject* Obj)
{
#if WITH_EDITOR
	const UWorld* W = Obj ? Obj->GetWorld() : nullptr;
	return (W ? W->GetOutermost()->GetPIEInstanceID() : -1);
#else
	return -1;
#endif
}
