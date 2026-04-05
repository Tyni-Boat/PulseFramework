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
	// Mesh = CreateOptionalDefaultSubobject<USkeletalMeshComponent>(ACharacter::MeshComponentName);
	// if (Mesh)
	// {
	// 	Mesh->AlwaysLoadOnClient = true;
	// 	Mesh->AlwaysLoadOnServer = true;
	// 	Mesh->bOwnerNoSee = false;
	// 	Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
	// 	Mesh->bCastDynamicShadow = true;
	// 	Mesh->bAffectDynamicIndirectLighting = true;
	// 	Mesh->PrimaryComponentTick.TickGroup = TG_PrePhysics;
	// 	Mesh->SetupAttachment(CapsuleComponent);
	// 	static FName MeshCollisionProfileName(TEXT("CharacterMesh"));
	// 	Mesh->SetCollisionProfileName(MeshCollisionProfileName);
	// 	Mesh->SetGenerateOverlapEvents(false);
	// 	Mesh->SetCanEverAffectNavigation(false);
	// }
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

float ABasePulsePawn::GetBalanceRatio() const
{
	const float dot = GetActorUpVector() | CurrentBalance.GetSafeNormal();
	const float angle = FMath::Acos(dot);
	if (angle <= 0)
		return 1;
	if (angle >= FMath::DegreesToRadians(BalanceLimit))
		return 0;
	if (angle >= FMath::DegreesToRadians(CriticalBalance))
		return 0;
	return FMath::GetMappedRangeValueClamped(TRange<float>(0.0f, FMath::DegreesToRadians(CriticalBalance)), TRange<float>(1.0f, 0.0f), angle);
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
	FVector upVector = GetActorUpVector();
	const float dot = upVector | TargetBalance.GetSafeNormal();
	float angle = FMath::Acos(dot);
	const FQuat targetBalanceRot = UKismetMathLibrary::MakeRotFromX(TargetBalance).Quaternion();
	FVector dir = WorldDirection.GetSafeNormal();
	FVector rotAxis = FVector(0);
	FVector::CreateOrthonormalBasis(upVector, rotAxis, dir);
	FQuat finalBalanceRot = FQuat(rotAxis, FMath::DegreesToRadians(AngleDegree));
	switch (Operation)
	{
	case ENumericOperator::Set:
		TargetBalance = finalBalanceRot.GetUpVector();
		break;
	case ENumericOperator::Add:
		TargetBalance = (targetBalanceRot * finalBalanceRot).GetUpVector();
		break;
	case ENumericOperator::Sub:
		TargetBalance = (targetBalanceRot * finalBalanceRot.Inverse()).GetUpVector();
		break;
	case ENumericOperator::Mul:
		{
			angle *= FMath::DegreesToRadians(AngleDegree);
			FVector xComp = FVector::VectorPlaneProject(TargetBalance, GetActorUpVector()).GetSafeNormal() * FMath::Sin(angle);
			const FVector YComp = GetActorUpVector() * FMath::Cos(angle);
			TargetBalance = xComp + YComp;
		}
		break;
	case ENumericOperator::Div:
		{
			if (AngleDegree > 0)
				angle /= FMath::DegreesToRadians(AngleDegree);
			FVector xComp = FVector::VectorPlaneProject(TargetBalance, GetActorUpVector()).GetSafeNormal() * FMath::Sin(angle);
			const FVector YComp = GetActorUpVector() * FMath::Cos(angle);
			TargetBalance = xComp + YComp;
		}
		break;
	case ENumericOperator::Mod:
		{
			angle = FMath::Modulo(angle, FMath::DegreesToRadians(AngleDegree));
			FVector xComp = FVector::VectorPlaneProject(TargetBalance, GetActorUpVector()).GetSafeNormal() * FMath::Sin(angle);
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
	// Arrow draw
	const float limit = BalanceLimit;
	const float crit = CriticalBalance;
	const float ratio = GetBalanceRatio();
	const auto GetColor = [PerfectColor, NormaColor, CriticalColor, UnbalanceColor, dir, crit, limit, ratio](const FVector& arrow, bool lerp = false) -> FLinearColor
	{
		const float dot = dir | arrow.GetSafeNormal();
		const float angle = FMath::Acos(dot);
		if (angle <= 0)
			return PerfectColor;
		if (angle >= FMath::DegreesToRadians(limit))
			return UnbalanceColor;
		if (angle >= FMath::DegreesToRadians(crit))
			return CriticalColor;
		return lerp ? FLinearColor::LerpUsingHSV(CriticalColor, NormaColor, ratio) : NormaColor;
	};
	UPulseDebugLibrary::DrawDebugArcCircle(this, drawPt, ArrowLenght, FMath::RadiansToDegrees(FMath::Acos(CurrentBalance | dir)), FVector::VectorPlaneProject(CurrentBalance, dir),
	                                       dir, GetColor(CurrentBalance, true), 0, Thickness, 0, 50, 36);
	UKismetSystemLibrary::DrawDebugArrow(this, drawPt, drawPt + TargetBalance * ArrowLenght, ArrowSize, GetColor(TargetBalance), 0, Thickness);
	UKismetSystemLibrary::DrawDebugLine(this, drawPt, drawPt + CurrentBalance * ArrowLenght, GetColor(CurrentBalance, true), 0, Thickness * 0.5);
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
	else if (angle >= 0)
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

#pragma region Limb

void ABasePulsePawn::DebugLimbs(const float DeltaTime)
{
	if (!DebugSKM)
		return;
	for (const FPulseLimbDefinition& Limb : Limbs)
	{
		for (const auto& BoneSegment : Limb.LimbBoneChain)
		{
			BoneSegment.DebugDrawBoneSegment(DebugSKM, FLinearColor(1,0,1,1), 1);
		}
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
	BalanceLogicTick(DeltaTime);
	DebugLimbs(DeltaTime);
}

// Called to bind functionality to input
void ABasePulsePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ABasePulsePawn::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{
	IMoverInputProducerInterface::ProduceInput_Implementation(SimTimeMs, InputCmdResult);
	FCharacterDefaultInputs& CharacterInputs = InputCmdResult.InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();

	if (GetController() == nullptr)
	{
		if (GetLocalRole() == ENetRole::ROLE_Authority && GetRemoteRole() == ENetRole::ROLE_SimulatedProxy)
		{
			static const FCharacterDefaultInputs DoNothingInput;
			// If we get here, that means this pawn is not currently possessed and we're choosing to provide default do-nothing input
			CharacterInputs = DoNothingInput;
		}

		// We don't have a local controller so we can't run the code below. This is ok. Simulated proxies will just use previous input when extrapolating
		return;
	}

	CharacterInputs.ControlRotation = FRotator::ZeroRotator;

	CharacterInputs.SetMoveInput(EMoveInputType::Velocity, LocomotionInputVector);

	static float RotationMagMin(1e-3);

	const bool bHasAffirmativeMoveInput = (CharacterInputs.GetMoveInput().Size() >= RotationMagMin);

	// Figure out intended orientation
	CharacterInputs.OrientationIntent = FVector::ZeroVector;


	// set the intent to the actors movement direction
	//CharacterInputs.OrientationIntent = CharacterInputs.GetMoveInput().GetSafeNormal();

	CharacterInputs.bIsJumpPressed = false;
	CharacterInputs.bIsJumpJustPressed = false;

	// Convert inputs to be relative to the current movement base (depending on options and state)
	CharacterInputs.bUsingMovementBase = false;
}
