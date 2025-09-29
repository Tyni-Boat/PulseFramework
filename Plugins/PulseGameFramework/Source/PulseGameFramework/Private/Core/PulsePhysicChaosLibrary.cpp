// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/PulsePhysicChaosLibrary.h"
#include "PhysicsProxy/SingleParticlePhysicsProxy.h"




void UPulsePhysicChaosLibrary::ChaosAddForce(UPrimitiveComponent* Component, FVector Force, bool bAccelChange, FName BoneName)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		if (bAccelChange)
		{
			const float RigidMass = RigidHandle->M();
			const Chaos::FVec3 Acceleration = Force * RigidMass;
			RigidHandle->AddForce(Acceleration, false);
		}
		else
		{
			RigidHandle->AddForce(Force, false);
		}
	}
}

void UPulsePhysicChaosLibrary::ChaosAddForceAtPosition(UPrimitiveComponent* Component, FVector Position, FVector Force, FName BoneName)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		const Chaos::FVec3 WorldCOM = Chaos::FParticleUtilitiesGT::GetCoMWorldPosition(RigidHandle);
		const Chaos::FVec3 WorldTorque = Chaos::FVec3::CrossProduct(Position - WorldCOM, Force);
		RigidHandle->AddForce(Force, false);
		RigidHandle->AddTorque(WorldTorque, false);
	}
}

void UPulsePhysicChaosLibrary::ChaosAddTorque(UPrimitiveComponent* Component, FVector Torque, bool bAccelChange, FName BoneName)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		if (bAccelChange)
		{
			RigidHandle->AddTorque(Chaos::FParticleUtilitiesXR::GetWorldInertia(RigidHandle) * Torque, false);
		}
		else
		{
			RigidHandle->AddTorque(Torque, false);
		}
	}
}

void UPulsePhysicChaosLibrary::ChaosAddImpulse(UPrimitiveComponent* Component, FVector Impulse, bool bVelChange, FName BoneName)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		if (bVelChange)
		{
			RigidHandle->SetLinearImpulse(RigidHandle->LinearImpulse() + RigidHandle->M() * Impulse, false);
		}
		else
		{
			RigidHandle->SetLinearImpulse(RigidHandle->LinearImpulse() + Impulse, false);
		}
	}
}

void UPulsePhysicChaosLibrary::ChaosAddImpulseAtPosition(UPrimitiveComponent* Component, FVector Position, FVector Impulse, FName BoneName)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		const Chaos::FVec3 WorldCOM = Chaos::FParticleUtilitiesGT::GetCoMWorldPosition(RigidHandle);
		const Chaos::FVec3 AngularImpulse = Chaos::FVec3::CrossProduct(Position - WorldCOM, Impulse);
		RigidHandle->SetLinearImpulse(RigidHandle->LinearImpulse() + Impulse, false);
		RigidHandle->SetAngularImpulse(RigidHandle->AngularImpulse() + AngularImpulse, false);
	}
}

void UPulsePhysicChaosLibrary::ChaosAddAngularImpulseInRadians(UPrimitiveComponent* Component, FVector Torque, bool bVelChange, FName BoneName)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		if (bVelChange)
		{
			const Chaos::FMatrix33 WorldI = Chaos::FParticleUtilitiesXR::GetWorldInertia(RigidHandle);
			RigidHandle->SetAngularImpulse(RigidHandle->AngularImpulse() + (WorldI * Torque), false);
		}
		else
		{
			RigidHandle->SetAngularImpulse(RigidHandle->AngularImpulse() + Torque, false);
		}
	}
}

void UPulsePhysicChaosLibrary::ChaosAddAngularImpulseInDegrees(UPrimitiveComponent* Component, FVector Torque, bool bVelChange, FName BoneName)
{
	ChaosAddAngularImpulseInRadians(Component, FMath::DegreesToRadians(Torque), bVelChange, BoneName);
}

FTransform UPulsePhysicChaosLibrary::ChaosGetTransform(UPrimitiveComponent* Component)
{
	if (const Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component))
	{
		const Chaos::FRigidTransform3 WorldCOM = Chaos::FParticleUtilitiesGT::GetActorWorldTransform(RigidHandle);
		return WorldCOM;
	}
	return Component ? Component->GetComponentTransform() : FTransform();
}

FVector UPulsePhysicChaosLibrary::ChaosGetLinearVelocity(UPrimitiveComponent* Component, FName BoneName)
{
	if (const Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		return RigidHandle->V();
	}
	return FVector::ZeroVector;
}

FVector UPulsePhysicChaosLibrary::ChaosGetCoMPosition(UPrimitiveComponent* Component)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component))
	{
		if (ensure(RigidHandle->CanTreatAsKinematic()))
		{
			const bool bIsRigid = RigidHandle->CanTreatAsRigid();
			return bIsRigid
				? Chaos::FParticleUtilitiesGT::GetCoMWorldPosition(RigidHandle)
				: static_cast<Chaos::FVec3>(Chaos::FParticleUtilitiesGT::GetActorWorldTransform(RigidHandle).
											GetTranslation());
		}
	}
	return FVector::ZeroVector;
}

FVector UPulsePhysicChaosLibrary::ChaosGetAngularVelocity(UPrimitiveComponent* Component, FName BoneName)
{
	if (const Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		return RigidHandle->W();
	}
	return FVector::ZeroVector;
}

void UPulsePhysicChaosLibrary::ChaosSetLinearVelocity(UPrimitiveComponent* Component, FVector Velocity, bool bAddToCurrent, FName BoneName)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		if (bAddToCurrent)
		{
			RigidHandle->SetV(RigidHandle->V() + Velocity);
		}
		else
		{
			RigidHandle->SetV(Velocity);
		}
	}
}

void UPulsePhysicChaosLibrary::ChaosSetAngularVelocityInRadians(UPrimitiveComponent* Component, FVector AngVelocity, bool bAddToCurrent, FName BoneName)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		if (bAddToCurrent)
		{
			RigidHandle->SetW(RigidHandle->W() + AngVelocity);
		}
		else
		{
			RigidHandle->SetW(AngVelocity);
		}
	}
}

void UPulsePhysicChaosLibrary::ChaosSetAngularVelocityInDegrees(UPrimitiveComponent* Component, FVector AngVelocity, bool bAddToCurrent, FName BoneName)
{
	ChaosSetAngularVelocityInRadians(Component, FMath::DegreesToRadians(AngVelocity), bAddToCurrent, BoneName);
}

void UPulsePhysicChaosLibrary::ChaosSetWorldLocation(USceneComponent* Component, FVector Location)
{
	if (!Component)
	{
		return;
	}

	if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component))
	{
		if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(PrimitiveComponent))
		{
			const Chaos::FVec3 P = Location - RigidHandle->R().RotateVector(RigidHandle->CenterOfMass());
			RigidHandle->SetX(P);
		}
	}
	else
	{
		Component->SetWorldLocation(Location);
	}
}

void UPulsePhysicChaosLibrary::ChaosSetWorldRotation(UPrimitiveComponent* Component, FRotator Rotation)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component))
	{
		const Chaos::FRotation3 Q = Rotation.Quaternion() * RigidHandle->RotationOfMass().Inverse();
		RigidHandle->SetR(Q);
	}
}

void UPulsePhysicChaosLibrary::ChaosSetWorldLocationAndRotation(UPrimitiveComponent* Component, FVector Location, FRotator Rotation)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component))
	{
		const Chaos::FRotation3 Q = Rotation.Quaternion() * RigidHandle->RotationOfMass().Inverse();
		const Chaos::FVec3 P = Location - Q.RotateVector(RigidHandle->CenterOfMass());
		RigidHandle->SetR(Q);
		RigidHandle->SetX(P);
	}
}

FVector UPulsePhysicChaosLibrary::ChaosGetLinearVelocityAtPoint(UPrimitiveComponent* Component, FVector Point, FName BoneName)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		if (ensure(RigidHandle->CanTreatAsKinematic()))
		{
			const bool bIsRigid = RigidHandle->CanTreatAsRigid();
			const Chaos::FVec3 COM = bIsRigid
				? Chaos::FParticleUtilitiesGT::GetCoMWorldPosition(RigidHandle)
				: static_cast<Chaos::FVec3>(
				Chaos::FParticleUtilitiesGT::GetActorWorldTransform(RigidHandle).
				GetTranslation());
			const Chaos::FVec3 Diff = Point - COM;
			return RigidHandle->V() - Chaos::FVec3::CrossProduct(Diff, RigidHandle->W());
		}
	}
	return FVector::ZeroVector;
}

Chaos::FRigidBodyHandle_Internal* UPulsePhysicChaosLibrary::GetInternalHandle(UPrimitiveComponent* Component, FName BoneName)
{
	if (IsValid(Component))
	{
		if (const FBodyInstance* BodyInstance = Component->GetBodyInstance(BoneName))
		{
			if (const auto Handle = BodyInstance->ActorHandle)
			{
				if (Chaos::FRigidBodyHandle_Internal* RigidHandle = Handle->GetPhysicsThreadAPI())
				{
					return RigidHandle;
				}
			}
		}
	}
	return nullptr;
}
