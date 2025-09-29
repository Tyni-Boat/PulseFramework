// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/PulseSystemLibrary.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Core/PulseModuleBase.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"


bool UPulseSystemLibrary::TryGetCameraRelativeInput(const UObject* WorldContext, const FVector2D& Input, FVector& OutDirection, const int32 PlayerIndex, const FVector& Normal,
                                                    const float SnapToAngle)
{
	FVector2D inp = Input;

	if (SnapToAngle > 0 && FMath::Abs(inp.X) > 0 && FMath::Abs(inp.Y) > 0)
	{
		const float lenght = inp.Length();
		const float snapDirA = FMath::Modulo(SnapToAngle, 90);
		const float snapVal = FMath::Abs(FMath::Sin(FMath::DegreesToRadians(snapDirA)));
		if (FMath::Abs(inp.X) > FMath::Abs(inp.Y))
		{
			float ySnap = snapVal > 0 ? FMath::RoundToDouble(FMath::Abs(inp.Y) / snapVal) * snapVal : 0;
			float xSnap = FMath::Cos(FMath::Asin(ySnap));
			inp = FVector2D(xSnap * FMath::Sign(inp.X), ySnap * FMath::Sign(inp.Y)).GetSafeNormal() * lenght;
		}
		else if (FMath::Abs(inp.X) < FMath::Abs(inp.Y))
		{
			float xSnap = snapVal > 0 ? FMath::RoundToDouble(FMath::Abs(inp.X) / snapVal) * snapVal : 0;
			float ySnap = FMath::Cos(FMath::Asin(xSnap));
			inp = FVector2D(xSnap * FMath::Sign(inp.X), ySnap * FMath::Sign(inp.Y)).GetSafeNormal() * lenght;
		}
	}

	OutDirection = FVector::ForwardVector * inp.Y + FVector::RightVector * inp.X;
	if (!WorldContext)
		return false;
	auto playerController = UGameplayStatics::GetPlayerController(WorldContext, PlayerIndex);
	if (!playerController)
		return false;
	const auto camMgr = playerController->PlayerCameraManager;
	if (!camMgr)
		return false;
	const auto camRot = camMgr->GetCameraRotation().Quaternion();
	FVector n = Normal;
	OutDirection = camRot.GetRightVector() * inp.X + camRot.GetForwardVector() * inp.Y;
	if (!n.Normalize())
		return true;
	const FVector fwd = FVector::VectorPlaneProject(camRot.GetForwardVector(), n).GetSafeNormal();
	const FVector rht = FVector::VectorPlaneProject(camRot.GetRightVector(), n).GetSafeNormal();
	OutDirection = fwd * inp.Y + rht * inp.X;
	return true;
}

void UPulseSystemLibrary::RotateComponentByInputs(USceneComponent*& Component, const FVector2D Inputs, FHitResult& SweepResult, const FVector& YawAxis, const FVector2D& TiltLimits, bool bInvertX,
	bool bInvertY, bool bUseSweep, ETeleportType TeleportType)
{
	if (!Component)
		return;
	FVector normal = YawAxis;
	if (!normal.Normalize())
		normal = FVector(0, 0, 1);
	const float currentTilt = Component->GetForwardVector() | normal;
	const float tilt = FMath::Clamp(currentTilt + Inputs.Y * (bInvertY? -1 : 1), TiltLimits.X, TiltLimits.Y);
	const float tiltDiff = tilt - currentTilt;
	FVector planarVector = FVector::VectorPlaneProject(Component->GetForwardVector(), normal).GetSafeNormal();
	FQuat planarOrientation = UKismetMathLibrary::MakeRotFromXZ(planarVector, normal).Quaternion();
	planarOrientation *= FQuat(normal, FMath::DegreesToRadians(Inputs.X * (bInvertX? -1 : 1)));
	planarOrientation.Normalize();
	FVector tiltAxis = FVector::CrossProduct(planarOrientation.GetForwardVector(), normal);
	planarOrientation *= FQuat(normal, FMath::DegreesToRadians(tiltDiff));
	planarOrientation.Normalize();

	Component->SetWorldRotation(planarOrientation, bUseSweep, &SweepResult, TeleportType);
}




void UPulseSystemLibrary::SimulateKey(const FKey Key, EInputEvent Event)
{
	if (!Key.IsValid())
		return;
	const FGamepadKeyNames::Type KeyName = Key.GetFName();
	FInputDeviceId PrimaryInputDevice = IPlatformInputDeviceMapper::Get().GetPrimaryInputDeviceForUser(FSlateApplicationBase::SlateAppPrimaryPlatformUser);
	auto ApplyInput = [PrimaryInputDevice](const FGamepadKeyNames::Type KeyName, bool bPressed)
	{
		if (bPressed)
			FSlateApplication::Get().OnControllerButtonPressed(KeyName, FSlateApplicationBase::SlateAppPrimaryPlatformUser, PrimaryInputDevice, false);
		else
			FSlateApplication::Get().OnControllerButtonReleased(KeyName, FSlateApplicationBase::SlateAppPrimaryPlatformUser, PrimaryInputDevice, false);
	};

	switch (Event)
	{
	case IE_Pressed:
		ApplyInput(KeyName, true);
		break;
	case IE_Released:
		ApplyInput(KeyName, false);
		break;
	case IE_Repeat:
		break;
	case IE_DoubleClick:
		break;
	case IE_Axis:
		break;
	case IE_MAX:
		break;
	default:
		break;
	}

}




float UPulseSystemLibrary::GetMontageFirstNotifyTriggerTime(const UAnimMontage* montage, TSubclassOf<UAnimNotify> notifyClass)
{
	if (!montage)
		return -1;
	for (int i = 0; i < montage->Notifies.Num(); i++)
	{
		if (!montage->Notifies[i].Notify)
			continue;
		if (montage->Notifies[i].Notify.IsA(notifyClass))
			return montage->Notifies[i].GetTriggerTime();
	}
	return -1;
}

FVector UPulseSystemLibrary::GetMontageFirstNotifyStateTime(const UAnimMontage* montage, TSubclassOf<UAnimNotifyState> notifyClass)
{
	if (!montage)
		return FVector(-1);
	for (int i = 0; i < montage->Notifies.Num(); i++)
	{
		if (!montage->Notifies[i].NotifyStateClass)
			continue;
		if (montage->Notifies[i].NotifyStateClass.IsA(notifyClass))
		{
			const float start = montage->Notifies[i].GetTriggerTime();
			const float duration = montage->Notifies[i].GetDuration();
			const float end = montage->Notifies[i].GetEndTriggerTime();
			return FVector(start, duration, end);
		}
	}
	return FVector(-1);
}

FVector2D UPulseSystemLibrary::GetMontageStartTimeFromSpeed(const FVector& Location, const FVector& Velocity, const FVector& MatchPoint, const float AnimationMatchTime)
{
	FVector nVel = Velocity;
	if (!nVel.Normalize())
		return FVector2D(0, 1);
	const float speed = Velocity.Length();
	const FVector distanceVector = (MatchPoint - Location).ProjectOnToNormal(nVel);
	float reachTime = distanceVector.Length() / speed;
	float plSpeed = 1;
	if (AnimationMatchTime > 0)
		plSpeed = AnimationMatchTime / reachTime;
	return FVector2D(reachTime, plSpeed);
}



bool UPulseSystemLibrary::HasChildTag(const FGameplayTagContainer& Container, FGameplayTag Tag)
{
	const FString namedTag = Tag.GetTagName().ToString();
	for (int i = 0; i < Container.Num(); i++)
	{
		if (Container.GetByIndex(i).GetTagName().ToString().Contains(namedTag))
			return true;
	}
	return false;
}

bool UPulseSystemLibrary::RemoveChildTags(UPARAM(ref)FGameplayTagContainer& Container, FGameplayTag Tag)
{
	const FString namedTag = Tag.GetTagName().ToString();
	bool result = false;
	for (int i = Container.Num() - 1; i >= 0; i--)
	{
		if (Container.GetByIndex(i).GetTagName().ToString().Contains(namedTag))
		{
			bool res = Container.RemoveTag(Container.GetByIndex(i));
			if (res && !result)
				result = true;
		}
	}
	return result;
}

bool UPulseSystemLibrary::AddChildTags(UPARAM(ref)FGameplayTagContainer& Container, FGameplayTag Tag)
{
	const FString namedTag = Tag.GetTagName().ToString();
	bool result = false;
	for (int i = Container.Num() - 1; i >= 0; i--)
	{
		if (Container.GetByIndex(i).GetTagName().ToString().Contains(namedTag))
		{
			Container.AddTag(Container.GetByIndex(i));
			result = true;
		}
	}
	return result;
}




bool UPulseSystemLibrary::EnableActor(AActor* Actor, bool Enable)
{
	if (!Actor)
		return false;
	if (!IsActorEnabled(Actor))
	{
		if (!Enable)
			return false;
		Actor->Tags.Remove("PulseActorDisabled");
	}
	Actor->SetActorHiddenInGame(!Enable);
	Actor->SetActorTickEnabled(Enable);
	Actor->SetActorEnableCollision(Enable);
	auto actorComps = Actor->GetComponents();
	for (auto comp : actorComps)
	{
		if (!comp)
			continue;
		//if (Enable)
		//	comp->Activate();
		//else
		//	comp->Deactivate();
		//comp->SetActiveFlag(Enable);
		if (USceneComponent* SceneComp = Cast<USceneComponent>(comp))
		{
			//SceneComp->SetVisibility(Enable);
			if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(SceneComp))
			{
				//PrimComp->SetCollisionEnabled(Enable ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
				//PrimComp->SetSimulatePhysics(Enable);
				PrimComp->SetPhysicsLinearVelocity(FVector::ZeroVector, false);
				PrimComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector, false);
			}
		}
	}
	if (!Enable)
		Actor->Tags.Add("PulseActorDisabled");
	return true;
}

bool UPulseSystemLibrary::IsActorEnabled(const AActor* Actor)
{
	if (!Actor)
		return false;
	return !Actor->ActorHasTag("PulseActorDisabled");
}

bool UPulseSystemLibrary::AddComponentAtRuntime(AActor* Actor, UActorComponent* Component)
{
	if (!Actor || !Component)
		return false;
	// Change owner
	Component->Rename(nullptr, Actor);
	// This is important for the component to function properly
	Component->RegisterComponent();
	// Add to the actor's components array
	Actor->AddInstanceComponent(Component);
	// If you need to attach to a scene component
	if (USceneComponent* ScnComp = Cast<USceneComponent>(Component))
	{
		if (USceneComponent* Root = Actor->GetRootComponent())
		{
			ScnComp->AttachToComponent(Root, FAttachmentTransformRules::SnapToTargetIncludingScale);
		}
	}
	// Initialize if needed
	//Component->InitializeComponent();
	
	// Activate the component if it is not already active
	Component->SetActive(true);

	return true;
}

bool UPulseSystemLibrary::RemoveComponentAtRuntime(AActor* Actor, UActorComponent* Component)
{
	if (!Actor || !Component)
		return false;

	if (Component && Component->GetOwner() == Actor)
	{
		// 0. Disable component
		Component->SetActive(false);

		// 1. Uninitialize component
		//Component->UninitializeComponent();

		// 2. Unregister the component (stops ticking and events)
		Component->UnregisterComponent();

		// 3. Remove from the actor's components array
		Actor->RemoveInstanceComponent(Component);

		// 4. Optional: Detach if it's a scene component
		if (USceneComponent* SceneComp = Cast<USceneComponent>(Component))
		{
			SceneComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		}

		// 5. Clear the owner (important!)
		Component->Rename(nullptr, nullptr, REN_DontCreateRedirectors | REN_ForceNoResetLoaders);

		return true;
	}
	return false;
}




bool UPulseSystemLibrary::SerializeObjectToBytes(UObject* object, TArray<uint8>& outBytes)
{
	if (!object)
		return false;
	FMemoryWriter MemoryWriter(outBytes, true);
	FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, false);
	object->Serialize(Ar);
	return true;
}

bool UPulseSystemLibrary::DeserializeObjectFromBytes(const TArray<uint8>& bytes, UObject* OutObject)
{
	if (!OutObject || bytes.Num() <= 0)
		return false;
	FMemoryReader MemoryReader(bytes, true);
	FObjectAndNameAsStringProxyArchive Ar(MemoryReader, true);
	OutObject->Serialize(Ar);
	return true;
}

