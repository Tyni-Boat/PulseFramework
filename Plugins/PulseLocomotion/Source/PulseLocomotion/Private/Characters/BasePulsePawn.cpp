// Copyright © by Tyni Boat. All Rights Reserved.


#include "Characters/BasePulsePawn.h"

#include "Core/PulseDebugLibrary.h"
#include "Core/PulseMathLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"


// Sets default values
ABasePulsePawn::ABasePulsePawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

#pragma region Balance

EPulseLocomotionBalanceState ABasePulsePawn::GetCurrentBalanceState() const
{
	const float dot = GetActorUpVector() | CurrentBalance.GetSafeNormal();
	const float angle = FMath::Acos(dot);
	if (angle <= 0)
		return EPulseLocomotionBalanceState::PerfectlyBalanced;
	if (angle >= FMath::DegreesToRadians(BalanceLimit))
		return EPulseLocomotionBalanceState::Unbalanced;
	if (angle >= FMath::DegreesToRadians(CriticalBalance))
		return EPulseLocomotionBalanceState::CriticalBalance;
	return EPulseLocomotionBalanceState::Balanced;
}

EPulseLocomotionBalanceState ABasePulsePawn::GetTargetBalanceState() const
{
	const float dot = GetActorUpVector() | TargetBalance.GetSafeNormal();
	const float angle = FMath::Acos(dot);
	if (angle <= 0)
		return EPulseLocomotionBalanceState::PerfectlyBalanced;
	if (angle >= FMath::DegreesToRadians(BalanceLimit))
		return EPulseLocomotionBalanceState::Unbalanced;
	if (angle >= FMath::DegreesToRadians(CriticalBalance))
		return EPulseLocomotionBalanceState::CriticalBalance;
	return EPulseLocomotionBalanceState::Balanced;
}

FVector ABasePulsePawn::GetBalanceRecoveryVelocity(const float PawnHalfHeight) const
{
	float recoverySpeed = 0;
	const float dot = GetActorUpVector() | CurrentBalance.GetSafeNormal();
	const float angle = FMath::Acos(dot);
	if (angle >= FMath::DegreesToRadians(CriticalBalance))
		recoverySpeed = CriticalBalanceRecovery.Z;
	else if (angle > 0)
		recoverySpeed = NormalBalanceRecovery.Z;
	const FVector direction = FVector::VectorPlaneProject(CurrentBalance, GetActorUpVector()).GetSafeNormal();
	const float arcLenght = UPulseMathLibrary::CircleArcLenght(PawnHalfHeight, angle);
	return (direction * arcLenght * recoverySpeed) - FVector::VectorPlaneProject(GetVelocity(), GetActorUpVector());
}

void ABasePulsePawn::SetTargetBalance(const FVector& WorldDirection, const float AngleDegree, const ENumericOperator Operation)
{
	if (!bUseBalanceSystem)
		return;
	const float dot = GetActorUpVector() | TargetBalance.GetSafeNormal();
	float angle = FMath::Acos(dot);
	const FVector xDisplacement = FVector::VectorPlaneProject(WorldDirection, GetActorUpVector()).GetSafeNormal() * FMath::Sin(FMath::DegreesToRadians(AngleDegree));
	FVector yDisplacement = GetActorUpVector() * FMath::Cos(FMath::Asin(xDisplacement.Length()));
	switch (Operation)
	{
	case ENumericOperator::Set:
		TargetBalance = xDisplacement + yDisplacement;
		break;
	case ENumericOperator::Add:
		TargetBalance += (xDisplacement + yDisplacement);
		break;
	case ENumericOperator::Sub:
		TargetBalance -= (xDisplacement + yDisplacement);
		break;
	case ENumericOperator::Mul:
		{
			angle *= FMath::DegreesToRadians(AngleDegree);
			FVector xComp = FVector::VectorPlaneProject(CurrentBalance, GetActorUpVector()).GetSafeNormal() * FMath::Sin(angle);
			const FVector YComp = GetActorUpVector() * FMath::Cos(angle);
			TargetBalance = xComp + YComp;
		}
		break;
	case ENumericOperator::Div:
		{
			if (AngleDegree > 0)
				angle /= FMath::DegreesToRadians(AngleDegree);
			FVector xComp = FVector::VectorPlaneProject(CurrentBalance, GetActorUpVector()).GetSafeNormal() * FMath::Sin(angle);
			const FVector YComp = GetActorUpVector() * FMath::Cos(angle);
			TargetBalance = xComp + YComp;
		}
		break;
	case ENumericOperator::Mod:
		{
			angle = FMath::Modulo(angle, FMath::DegreesToRadians(AngleDegree));
			FVector xComp = FVector::VectorPlaneProject(CurrentBalance, GetActorUpVector()).GetSafeNormal() * FMath::Sin(angle);
			const FVector YComp = GetActorUpVector() * FMath::Cos(angle);
			TargetBalance = xComp + YComp;
		}
		break;
	}
	if (!TargetBalance.Normalize())
	{
		TargetBalance = GetActorUpVector();
	}
}

void ABasePulsePawn::DrawDebugBalance(const FVector& RelativeLocation, const float ArrowLenght, const float ArrowSize, const float Thickness, FLinearColor PerfectColor,
	FLinearColor NormaColor, FLinearColor CriticalColor, FLinearColor UnbalanceColor)
{
	FVector drawPt = GetActorLocation() + RelativeLocation;
	FVector dir = GetActorUpVector();
	FVector pointPt = drawPt + dir * ArrowSize;
	// circles draw
	float l = ArrowLenght - ArrowLenght * FMath::Cos(FMath::Asin(5/ArrowLenght));
	UPulseDebugLibrary::DrawDebugCircle(this, pointPt - dir * l, 5, dir, PerfectColor, 0, Thickness);
	l = ArrowLenght - ArrowLenght * FMath::Cos(FMath::Asin(CriticalBalance/ArrowLenght));
	UPulseDebugLibrary::DrawDebugCircle(this, pointPt - dir * l, CriticalBalance, dir, CriticalColor, 0, Thickness);
	l = ArrowLenght - ArrowLenght * FMath::Cos(FMath::Asin(BalanceLimit/ArrowLenght));
	UPulseDebugLibrary::DrawDebugCircle(this, pointPt - dir * l, BalanceLimit, dir, UnbalanceColor, 0, Thickness);
	// Arrow draw
	const float limit = BalanceLimit;
	const float crit = CriticalBalance;
	const auto GetColor = [PerfectColor, NormaColor, CriticalColor, UnbalanceColor, dir, crit, limit](const FVector& arrow) -> FLinearColor
	{
		const float dot = dir | arrow.GetSafeNormal();
		const float angle = FMath::Acos(dot);
		if (angle <= 0)
			return PerfectColor;
		if (angle >= FMath::DegreesToRadians(limit))
			return UnbalanceColor;
		if (angle >= FMath::DegreesToRadians(crit))
			return CriticalColor;
		return NormaColor;
	};
	UKismetSystemLibrary::DrawDebugArrow(this, drawPt, drawPt + TargetBalance * ArrowLenght * 0.85, ArrowSize, GetColor(TargetBalance), 0, Thickness + 1);
	UKismetSystemLibrary::DrawDebugArrow(this, drawPt, drawPt + CurrentBalance * ArrowLenght, ArrowSize, GetColor(CurrentBalance), 0, Thickness);
}

void ABasePulsePawn::BalanceLogicTick(const float DeltaTime)
{
	if (!bUseBalanceSystem)
	{
		if (CurrentBalance != GetActorUpVector())
			CurrentBalance = GetActorUpVector();
		if (TargetBalance != GetActorUpVector())
			TargetBalance = GetActorUpVector();
		return;
	}
	FVector usedRecovery = FVector(0);
	const float dot = GetActorUpVector() | CurrentBalance.GetSafeNormal();
	const float angle = FMath::Acos(dot);
	if (angle >= FMath::DegreesToRadians(CriticalBalance))
		usedRecovery = CriticalBalanceRecovery;
	else if (angle > 0)
		usedRecovery = NormalBalanceRecovery;
	// Auto balance recovery
	if (usedRecovery.Z > 0)
	{
		FQuat targetBalanceRot = UKismetMathLibrary::MakeRotFromX(TargetBalance).Quaternion();
		FQuat currentBalanceRot = UKismetMathLibrary::MakeRotFromX(CurrentBalance).Quaternion();
		const FQuat newRot = UKismetMathLibrary::QuaternionSpringInterp(currentBalanceRot, targetBalanceRot, _balanceSpringState, usedRecovery.X, usedRecovery.Y
			, usedRecovery.Z * DeltaTime, 1, 0, false);
		CurrentBalance = newRot.GetForwardVector();
		if (!CurrentBalance.Normalize())
			CurrentBalance = GetActorUpVector();
	}
	// Handle balance state
	const auto state = GetCurrentBalanceState();
	if (state != _lastBalanceState)
	{
		OnBalanceChanged.Broadcast(this, _lastBalanceState, state);
		_lastBalanceState = state;
	}
}

#pragma endregion

// Called when the game starts or when spawned
void ABasePulsePawn::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ABasePulsePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ABasePulsePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
