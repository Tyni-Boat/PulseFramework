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
