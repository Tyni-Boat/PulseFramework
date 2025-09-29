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
