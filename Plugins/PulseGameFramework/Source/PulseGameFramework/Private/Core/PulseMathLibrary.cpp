// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/PulseMathLibrary.h"

#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialExpressionChannelMaskParameterColor.h"

float UPulseMathLibrary::AlphaByCurve(const float InAlpha, EAlphaBlendOption CurveType, UCurveFloat* FloatCurve)
{
	const float alpha = FMath::Clamp(InAlpha, 0.0f, 1.0f);
	return FAlphaBlend::AlphaToBlendOption(alpha, CurveType, FloatCurve);
}

FTransform UPulseMathLibrary::TransformByTransform(const FTransform& PivotTransform, const FTransform& TransformToConvert)
{
	auto result = FTransform();
	result.SetLocation(PivotTransform.TransformPosition(TransformToConvert.GetLocation()));
	result.SetRotation(PivotTransform.TransformRotation(TransformToConvert.GetRotation()));
	result.SetScale3D(PivotTransform.GetScale3D() * TransformToConvert.GetScale3D());
	return result;
}

FTransform UPulseMathLibrary::InverseTransformByTransform(const FTransform& PivotTransform, const FTransform& TransformToConvert)
{
	auto result = FTransform();
	result.SetLocation(PivotTransform.InverseTransformPosition(TransformToConvert.GetLocation()));
	result.SetRotation(PivotTransform.InverseTransformRotation(TransformToConvert.GetRotation()));
	result.SetScale3D(TransformToConvert.GetScale3D() / PivotTransform.GetScale3D());
	return result;
}

bool UPulseMathLibrary::IsTransformValid(const FTransform& Transform)
{
	return Transform.IsValid() && Transform.GetScale3D().SquaredLength() > 0;
}

void UPulseMathLibrary::MakeOrthoBasis(const FVector& vector, FVector& OutRight, FVector& OutUp, float vectorRollInDegrees)
{
	OutRight = FVector::Zero();
	OutUp = FVector::Zero();
	FVector fwd = vector;
	if (!fwd.Normalize())
		return;
	auto rot = UKismetMathLibrary::MakeRotFromX(fwd).Quaternion();
	rot = rot.Inverse() * FQuat(rot.GetUpVector(), FMath::DegreesToRadians(vectorRollInDegrees));
	OutRight = rot.GetRightVector();
	rot = UKismetMathLibrary::MakeRotFromXY(fwd, OutRight).Quaternion();
	OutUp = rot.GetUpVector();
}

float UPulseMathLibrary::CircleArcLenght(const float Radius, const float radAngle)
{
	return Radius * radAngle;
}

bool UPulseMathLibrary::CircleArcPoints(const FVector& Location, float Radius, float DegreeAngle, const FVector& Axis, const FVector& HeadingVector, TArray<FVector>& OutPoints,
	int32 PiRadSegmentCount, bool bUseCentralHeading)
{
	if (DegreeAngle == 0)
		return false;
	if (HeadingVector.Length() <= 0)
		return false;
	if (Axis.Length() <= 0)
		return false;
	// Gathering parameters
	const int32 segmentFor360 = FMath::Abs(PiRadSegmentCount * 2);
	float degAngle = DegreeAngle > 0 ? FMath::Min(DegreeAngle, 360) : FMath::Min(DegreeAngle, -360);
	const float radAngle = FMath::DegreesToRadians(degAngle);
	const float perSegmentAngle = 360.0f / static_cast<float>(segmentFor360);
	const float perSegmentRad = FMath::DegreesToRadians(perSegmentAngle);
	const int32 segmentCount = FMath::CeilToInt(static_cast<float>(segmentFor360) * (FMath::Abs(degAngle) / 360.0f)) + 1;
	FVector newAxis = Axis.GetSafeNormal();
	FQuat newRotation = UKismetMathLibrary::MakeRotFromXZ(HeadingVector, newAxis).Quaternion();
	if (bUseCentralHeading)
	{
		FQuat offset = FQuat(-newRotation.GetAxisZ(), radAngle * 0.5f);
		newRotation = newRotation * offset;
	}
	const FVector u = newRotation.GetAxisX();
	const FVector v = newRotation.GetAxisY();
	// Placing points
	TArray<FVector> points = {};
	float cumulatedAngle = 0;
	for (int i = 0; i < segmentCount; i++)
	{
		FVector point = (u * FMath::Cos(perSegmentRad * i) + v * FMath::Sin(perSegmentRad * i)) * Radius;
		points.Add(point);
		cumulatedAngle += perSegmentAngle;
		if (cumulatedAngle >= FMath::Abs(degAngle))
		{
			point = (u * FMath::Cos(radAngle) + v * FMath::Sin(radAngle)) * Radius;
			points[points.Num() - 1] = point;
			break;
		}
	}
	// Transformation
	for (int i = 0; i < points.Num(); ++i)
	{
		points[i] += Location;
		OutPoints.Add(points[i]);
	}

	return points.Num() > 0;
}
