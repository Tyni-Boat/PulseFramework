// Copyright © by Tyni Boat. All Rights Reserved.


#include "Actors/PulsePawnBase.h"

#include "Core/PulseDebugLibrary.h"
#include "Core/PulseMathLibrary.h"
#include "Core/PulseSystemLibrary.h"
#include "Kismet/KismetSystemLibrary.h"


// Sets default values
APulsePawnBase::APulsePawnBase()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Visual Body
	USkeletalMesh* DefaultBodyMesh = nullptr;
	TSubclassOf<UAnimInstance> DefaultBodyAnimInstance = nullptr;
	if (auto config = GetMutableDefault<UCoreProjectSetting>())
	{
		DefaultBodyMesh = config->DefaultPulsePawnBodyMesh.LoadSynchronous();
		DefaultBodyAnimInstance = config->DefaultPulsePawnBodyAnimInstance;
	}
	_MasterMeshComp = CreateOptionalDefaultSubobject<USkeletalMeshComponent>("BodyMesh");
	if (_MasterMeshComp)
	{
		_MasterMeshComp->AlwaysLoadOnClient = true;
		_MasterMeshComp->AlwaysLoadOnServer = true;
		_MasterMeshComp->bOwnerNoSee = false;
		_MasterMeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		_MasterMeshComp->bCastDynamicShadow = true;
		_MasterMeshComp->bAffectDynamicIndirectLighting = true;
		_MasterMeshComp->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		_MasterMeshComp->SetupAttachment(GetRootComponent());
		_MasterMeshComp->SetUsingAbsoluteLocation(false);
		_MasterMeshComp->SetUsingAbsoluteRotation(false);
		_MasterMeshComp->SetRelativeRotation({0,-90,0});
		FName MeshCollisionProfileName(TEXT("CharacterMesh"));
		_MasterMeshComp->SetCollisionProfileName(MeshCollisionProfileName);
		_MasterMeshComp->SetGenerateOverlapEvents(false);
		_MasterMeshComp->SetCanEverAffectNavigation(false);
		_SlaveMeshComp = CreateOptionalDefaultSubobject<USkeletalMeshComponent>("RetargetedBodyMesh");
		if (_SlaveMeshComp)
		{
			_SlaveMeshComp->AlwaysLoadOnClient = true;
			_SlaveMeshComp->AlwaysLoadOnServer = true;
			_SlaveMeshComp->bOwnerNoSee = false;
			_SlaveMeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
			_SlaveMeshComp->bCastDynamicShadow = true;
			_SlaveMeshComp->bAffectDynamicIndirectLighting = true;
			_SlaveMeshComp->PrimaryComponentTick.TickGroup = TG_PrePhysics;
			_SlaveMeshComp->SetupAttachment(_MasterMeshComp);
			_SlaveMeshComp->SetUsingAbsoluteLocation(false);
			_SlaveMeshComp->SetUsingAbsoluteRotation(false);
			_SlaveMeshComp->SetCollisionProfileName(MeshCollisionProfileName);
			_SlaveMeshComp->SetGenerateOverlapEvents(false);
			_SlaveMeshComp->SetCanEverAffectNavigation(false);
			InitializeBody(DefaultBodyMesh, DefaultBodyAnimInstance);
		}
	}
}

#pragma region Balance

EPulseLocomotionBalanceState APulsePawnBase::GetCurrentBalanceState() const
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

EPulseLocomotionBalanceState APulsePawnBase::GetTargetBalanceState() const
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

float APulsePawnBase::GetBalanceRatio() const
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

FVector APulsePawnBase::GetBalanceRecoveryVelocity(const float PawnHalfHeight) const
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

void APulsePawnBase::SetTargetBalance(const FVector& WorldDirection, const float AngleDegree, const ENumericOperator Operation)
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

void APulsePawnBase::DrawDebugBalance(const FVector& RelativeLocation, const float ArrowLenght, const float ArrowSize, const float Thickness, FLinearColor PerfectColor,
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
	const FQuat rot = UKismetMathLibrary::MakeRotFromXY(dir, FVector::VectorPlaneProject(CurrentBalance, dir)).Quaternion();
	UPulseDebugLibrary::DrawDebugArcCircle(this, drawPt, ArrowLenght, FMath::RadiansToDegrees(FMath::Acos(CurrentBalance | dir)), rot.GetUpVector(),
	                                       dir, GetColor(CurrentBalance, true), 0, Thickness, 0, 50, 36);
	UKismetSystemLibrary::DrawDebugArrow(this, drawPt, drawPt + TargetBalance * ArrowLenght, ArrowSize, GetColor(TargetBalance), 0, Thickness);
	UKismetSystemLibrary::DrawDebugLine(this, drawPt, drawPt + CurrentBalance * ArrowLenght, GetColor(CurrentBalance, true), 0, Thickness * 0.5);
}

void APulsePawnBase::BalanceLogicTick(const float DeltaTime)
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

void APulsePawnBase::DebugLimbs() const
{
	auto skm = GetBodyMesh();
	if (!skm)
		return;
	for (const FPulseLimbDefinition& Limb : Limbs)
	{
		auto col = Limb.bLocomotionLimb ? FLinearColor(1, 0, 1, 1) : FLinearColor(.3, .3, .3, 1);
		for (const auto& BoneSegment : Limb.LimbBoneChain)
		{
			BoneSegment.DebugDrawBoneSegment(skm, col, 1);
		}
	}
}

#pragma endregion

#pragma region SkeletalMeshComponent

void APulsePawnBase::GetPropSocketTransform(const FName Anchor, FName& OutSocket, FTransform& OutRelativeTransform) const
{
	OutSocket = "";
	OutRelativeTransform = FTransform();
	if (_PropRelativeTransformMap.Contains(Anchor))
		OutRelativeTransform = _PropRelativeTransformMap[Anchor];
	if (_PropSocketMap.Contains(Anchor))
		OutSocket = _PropSocketMap[Anchor];
}

void APulsePawnBase::InitializeBody(USkeletalMesh* SkeletalMesh, TSubclassOf<UAnimInstance> AnimInstance, bool bUseLiveRetargeting)
{
	_MasterMeshComp->SetHiddenInGame(bUseLiveRetargeting);
	_SlaveMeshComp->SetHiddenInGame(!bUseLiveRetargeting);
	_MasterMeshComp->SetVisibility(!bUseLiveRetargeting);
	_SlaveMeshComp->SetVisibility(bUseLiveRetargeting);
	_SlaveMeshComp->SetComponentTickEnabled(bUseLiveRetargeting);
	if (_CopyMasterAnimClass && !bUseLiveRetargeting) _CopyMasterAnimClass = nullptr;
	if (SkeletalMesh)
	{
		if (bUseLiveRetargeting) _SlaveMeshComp->SetSkeletalMesh(SkeletalMesh);
		else _MasterMeshComp->SetSkeletalMesh(SkeletalMesh);
	}
	if (AnimInstance)
	{
		if (bUseLiveRetargeting) _CopyMasterAnimClass = AnimInstance;
		if (bUseLiveRetargeting) _SlaveMeshComp->SetAnimInstanceClass(AnimInstance);
		else _MasterMeshComp->SetAnimInstanceClass(AnimInstance);
	}
	// Handle prop reallocation
	auto masterMesh = GetBodyMesh();
	if (!masterMesh)
		return;
	TArray<FName> propAnchors;
	_PropSkeletalMeshMap.GetKeys(propAnchors);
	for (const FName& Anchor : propAnchors)
	{
		if (!_PropSkeletalMeshMap[Anchor])
			continue;
		FName socketName;
		FTransform relativeTransform;
		GetPropSocketTransform(Anchor, socketName, relativeTransform);
		_PropSkeletalMeshMap[Anchor]->AttachToComponent(masterMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, socketName);
		_PropSkeletalMeshMap[Anchor]->SetRelativeTransform(relativeTransform);
	}
}

void APulsePawnBase::SetBodyProp(FName Anchor, USkeletalMesh* PropMesh, TSubclassOf<UAnimInstance> OptionalPropAnim, FName Socket, FVector RelativeOffset,
	FRotator RelativeRotation)
{
	auto masterMesh = GetBodyMesh();
	if (!masterMesh)
		return;
	if (!PropMesh)
	{
		// Try to disable prop
		if (!_PropSkeletalMeshMap.Contains(Anchor))
			return;
		if (!_PropSkeletalMeshMap[Anchor])
			return;
		_PropSkeletalMeshMap[Anchor]->SetSkeletalMesh(nullptr);
		_PropSkeletalMeshMap[Anchor]->SetAnimInstanceClass(nullptr);
		_PropSkeletalMeshMap[Anchor]->SetHiddenInGame(true);
		_PropSkeletalMeshMap[Anchor]->SetVisibility(false);
		_PropSkeletalMeshMap[Anchor]->SetComponentTickEnabled(false);
		_PropSkeletalMeshMap[Anchor]->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	}
	else
	{
		// Try to enable or create new prop
		if (!BodyPropTypes.Contains(Anchor))
			return;
		if (!_PropSkeletalMeshMap.Contains(Anchor))
		{
			if (auto prop_comp = CreateOptionalDefaultSubobject<USkeletalMeshComponent>(Anchor))
			{
				prop_comp->AlwaysLoadOnClient = true;
				prop_comp->AlwaysLoadOnServer = true;
				prop_comp->bOwnerNoSee = false;
				prop_comp->bCastDynamicShadow = true;
				prop_comp->bAffectDynamicIndirectLighting = true;
				prop_comp->PrimaryComponentTick.TickGroup = TG_PrePhysics;
				prop_comp->SetCollisionObjectType(ECC_Visibility);
				prop_comp->SetGenerateOverlapEvents(false);
				prop_comp->SetCanEverAffectNavigation(false);
				UPulseSystemLibrary::MapAddOrUpdateValue(_PropSocketMap, Anchor, Socket);
				UPulseSystemLibrary::MapAddOrUpdateValue(_PropRelativeTransformMap, Anchor, FTransform(RelativeRotation, RelativeOffset));
				_PropSkeletalMeshMap.Add(Anchor, prop_comp);
			}
			else
				return;
		}
		if (!_PropSkeletalMeshMap[Anchor])
			return;
		FName socketName;
		FTransform relativeTransform;
		GetPropSocketTransform(Anchor, socketName, relativeTransform);
		_PropSkeletalMeshMap[Anchor]->SetSkeletalMesh(PropMesh);
		_PropSkeletalMeshMap[Anchor]->SetAnimInstanceClass(OptionalPropAnim);
		_PropSkeletalMeshMap[Anchor]->AttachToComponent(masterMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, socketName);
		_PropSkeletalMeshMap[Anchor]->SetRelativeTransform(relativeTransform);
		_PropSkeletalMeshMap[Anchor]->SetHiddenInGame(false);
		_PropSkeletalMeshMap[Anchor]->SetVisibility(true);
		_PropSkeletalMeshMap[Anchor]->SetComponentTickEnabled(true);
		_PropSkeletalMeshMap[Anchor]->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
	}
}

void APulsePawnBase::ClearBodyProps()
{
	TArray<FName> propAnchors;
	_PropSkeletalMeshMap.GetKeys(propAnchors);
	for (const FName& Anchor : propAnchors)
	{
		SetBodyProp(Anchor, nullptr);
	}
}

#pragma endregion


// Called when the game starts or when spawned
void APulsePawnBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APulsePawnBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	BalanceLogicTick(DeltaTime);
}

