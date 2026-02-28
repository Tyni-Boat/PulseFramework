// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/PulseSystemLibrary.h"
#include "GameplayTagContainer.h"
#include "JsonObjectConverter.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "HAL/PlatformFilemanager.h"
#include "Async/Async.h"
#include "Engine/AssetManager.h"
#include "Engine/AssetManagerTypes.h"
#include "Misc/SecureHash.h"
#include "Internationalization/Regex.h"
//#include "JsonObjectConverter.h"
//#include "Engine/NetworkObjectList.h"


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

void UPulseSystemLibrary::RotateComponentByInputs(USceneComponent*& Component, const FVector2D Inputs, FHitResult& SweepResult, const FVector& YawAxis, const FVector2D& TiltLimits,
                                                  bool bInvertX,
                                                  bool bInvertY, bool bUseSweep, ETeleportType TeleportType)
{
	if (!Component)
		return;
	FVector normal = YawAxis;
	if (!normal.Normalize())
		normal = FVector(0, 0, 1);
	const float currentTilt = Component->GetForwardVector() | normal;
	const float tilt = FMath::Clamp(currentTilt + Inputs.Y * (bInvertY ? -1 : 1), TiltLimits.X, TiltLimits.Y);
	const float tiltDiff = tilt - currentTilt;
	FVector planarVector = FVector::VectorPlaneProject(Component->GetForwardVector(), normal).GetSafeNormal();
	FQuat planarOrientation = UKismetMathLibrary::MakeRotFromXZ(planarVector, normal).Quaternion();
	planarOrientation *= FQuat(normal, FMath::DegreesToRadians(Inputs.X * (bInvertX ? -1 : 1)));
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

bool UPulseSystemLibrary::RemoveChildTags(UPARAM(ref)
                                          FGameplayTagContainer& Container, FGameplayTag Tag)
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

bool UPulseSystemLibrary::AddChildTags(UPARAM(ref)
                                       FGameplayTagContainer& Container, FGameplayTag Tag)
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

FName UPulseSystemLibrary::ConstructNametag(TArray<FName> TagParts, const FString& Separator)
{
	FString result;
	for (int i = 0; i < TagParts.Num(); i++)
		if (i == TagParts.Num() - 1)
			result += TagParts[i].ToString();
		else
			result += FString::Printf(TEXT("%s%c"), *TagParts[i].ToString(), Separator.IsEmpty() ? '.' : Separator[0]);
	return FName(result);
}

bool UPulseSystemLibrary::ExtractNametagParts(FName Tag, TArray<FName>& OutTagParts, const FString& Separator)
{
	OutTagParts.Empty();
	FString strTag = Tag.ToString();
	strTag.RemoveSpacesInline();
	if (strTag.IsEmpty())
		return false;
	FString right = "";
	FString Sep = FString::Printf(TEXT("%c"), Separator.IsEmpty() ? '.' : Separator[0]);
	while (strTag.Split(Sep, &strTag, &right, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
	{
		OutTagParts.Insert(FName(right), 0);
	}
	OutTagParts.Insert(FName(strTag), 0);
	return OutTagParts.Num() > 0;
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

bool UPulseSystemLibrary::AddComponentAtRuntime(AActor* Actor, UActorComponent* Component, bool bReplicate)
{
	if (!Actor || !Component)
		return false;
	//Check if is already it's component
	if (Actor->GetComponents().Contains(Component))
		return true;
	// Change owner
	Component->Rename(nullptr, Actor);
	// This is important for the component to function properly
	Component->RegisterComponent();
	// Handle Replication
	if (bReplicate)
	{
		Component->SetIsReplicated(true);
		Actor->AddInstanceComponent(Component); // optional but recommended
		Actor->ReregisterAllComponents(); // updates replication layout
	}
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

void UPulseSystemLibrary::ForeachActorClass(const UObject* WorldContext, TSubclassOf<AActor> Class, TFunction<void(AActor*)> Action, TFunction<bool(AActor*)> Condition)
{
	if (!WorldContext)
		return;
	if (!Action)
		return;
	if (!Class)
		return;
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(WorldContext, Class, OutActors);
	for (auto* actor : OutActors)
	{
		if (!actor)
			continue;
		if (Condition && !Condition(actor))
			continue;
		Action(actor);
	}
}

void UPulseSystemLibrary::ForeachActorInterface(const UObject* WorldContext, TSubclassOf<UInterface> Interface, TFunction<void(AActor*)> Action, TFunction<bool(AActor*)> Condition)
{
	if (!WorldContext)
		return;
	if (!Action)
		return;
	if (!Interface)
		return;
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsWithInterface(WorldContext, Interface, OutActors);
	for (auto* actor : OutActors)
	{
		if (!actor)
			continue;
		if (Condition && !Condition(actor))
			continue;
		Action(actor);
	}
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

bool UPulseSystemLibrary::SerializeStructToJson(const FInstancedStruct& StructData, FString& OutJson)
{
	if (!StructData.IsValid())
	{
		OutJson = "{}";
		return false;
	}

	return FJsonObjectConverter::UStructToJsonObjectString(
		StructData.GetScriptStruct(),
		StructData.GetMemory(),
		OutJson,
		0, 0
	);
}

bool UPulseSystemLibrary::DeserializeStructFromJson(const FString& JsonString, FInstancedStruct& OutStruct, UScriptStruct* StructType)
{
	if (!StructType)
		return false;

	// 1. Parse JSON
	TSharedPtr<FJsonObject> JsonObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
		return false;

	// 2. Allocate struct
	OutStruct.InitializeAs(StructType);

	// 3. Convert JSON → Struct (runtime type!)
	return FJsonObjectConverter::JsonObjectToUStruct(
		JsonObject.ToSharedRef(),
		StructType,
		OutStruct.GetMutableMemory(),
		0,
		0
	);
}

bool UPulseSystemLibrary::SaveJsonToLocal(const FString& FileName, const FString& JsonString)
{
	if (FileName.IsEmpty() || JsonString.IsEmpty())
		return false;

	// Example path: Saved/Json/FileName.json
	FString SavePath = FPaths::ProjectSavedDir() / TEXT("Data") / FileName;
	FString Directory;
	FString File;
	SavePath.Split("/", &Directory, &File, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

	// Ensure folder exists
	if (!IFileManager::Get().MakeDirectory(*FPaths::GetPath(Directory), true))
	{
		UE_LOG(LogTemp, Warning, TEXT("Unable to Save Json: Impossible to create %s directory to save json at path %s"), *Directory, *SavePath);
	}

	if (!FFileHelper::SaveStringToFile(JsonString, *SavePath, FFileHelper::EEncodingOptions::ForceUTF8))
	{
		UE_LOG(LogTemp, Warning, TEXT("Unable to Save Json: path %s"), *SavePath);
		return false;
	}
	return true;
}

bool UPulseSystemLibrary::LoadJsonFromLocal(const FString& FileName, FString& OutJsonString)
{
	if (FileName.IsEmpty())
		return false;

	// Path must match your save code
	FString LoadPath = FPaths::ProjectSavedDir() / TEXT("Data") / FileName;

	if (!FFileHelper::LoadFileToString(OutJsonString, *LoadPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Unable to read Json from path: %s"), *LoadPath);
		return false;
	}
	return true;
}

bool UPulseSystemLibrary::LoadJsonFromLocalPath(const FString& FilePath, FString& OutJsonString)
{
	if (FilePath.IsEmpty())
		return false;
	return FFileHelper::LoadFileToString(OutJsonString, *FilePath);
}

bool UPulseSystemLibrary::IsAssetTypeDerivedFrom(FPrimaryAssetType AssetType, TSubclassOf<UObject> ClassToCheck)
{
	UAssetManager& AssetManager = UAssetManager::Get();
	FPrimaryAssetTypeInfo TypeInfo;
	if (AssetManager.GetPrimaryAssetTypeInfo(AssetType, TypeInfo))
		return false;
	if (!TypeInfo.GetAssetBaseClass().Get())
		return false;
	return TypeInfo.GetAssetBaseClass().Get()->IsChildOf(ClassToCheck);
}

bool UPulseSystemLibrary::FileExist(const FString& FilePath)
{
	if (FilePath.IsEmpty())
		return false;
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (PlatformFile.FileExists(*FilePath))
	{
		return true;
	}
	return false;
}

void UPulseSystemLibrary::FileComputeMD5Async(const FString& FilePath, FOnMD5Computed OnComplete, int32 BufferSize)
{
	if (!FileExist(FilePath))
	{
		OnComplete.ExecuteIfBound(false, "");
		return;
	}
	if (BufferSize <= 0)
		BufferSize = 1048576;
	Async(EAsyncExecution::ThreadPool, [FilePath, BufferSize, OnComplete]()
	{
		FString ResultMD5;
		bool bSuccess = false;

		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		TUniquePtr<IFileHandle> Handle(PlatformFile.OpenRead(*FilePath));

		if (Handle)
		{
			FMD5 MD5;
			TArray<uint8> Buffer;
			Buffer.SetNumUninitialized(BufferSize);

			int64 Remaining = Handle->Size();

			while (Remaining > 0)
			{
				const int64 ReadSize = FMath::Min(BufferSize, Remaining);
				if (!Handle->Read(Buffer.GetData(), ReadSize))
				{
					break;
				}

				MD5.Update(Buffer.GetData(), ReadSize);
				Remaining -= ReadSize;
			}

			if (Remaining <= 0)
			{
				uint8 Digest[16];
				MD5.Final(Digest);
				ResultMD5 = BytesToHex(Digest, 16).ToLower();
				bSuccess = true;
			}
		}

		// Back to game thread
		AsyncTask(ENamedThreads::GameThread, [OnComplete, bSuccess, ResultMD5]()
		{
			OnComplete.ExecuteIfBound(bSuccess, ResultMD5);
		});
	});
}

bool UPulseSystemLibrary::FileDelete(const FString& FilePath)
{
	if (!FileExist(FilePath))
		return true;
	return IFileManager::Get().Delete(*FilePath);
}

bool UPulseSystemLibrary::FileIsPathWritable(const FString& DirectoryPath)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	bool dirExist = PlatformFile.DirectoryExists(*DirectoryPath);
	if (!dirExist)
	{
		if (!IFileManager::Get().MakeDirectory(*FPaths::GetPath(DirectoryPath), true))
		{
			return false;
		}
	}

	const FString TestFilePath =
		DirectoryPath / FString::Printf(TEXT(".__writetest_%s.tmp"), *FGuid::NewGuid().ToString());

	// Try create + write
	TUniquePtr<IFileHandle> FileHandle(PlatformFile.OpenWrite(*TestFilePath));

	if (!FileHandle)
	{
		if (!dirExist)
			PlatformFile.DeleteDirectory(*DirectoryPath);
		return false;
	}

	FileHandle.Reset(); // Close file
	PlatformFile.DeleteFile(*TestFilePath);
	return true;
}

void UPulseSystemLibrary::FileGetAllFilesInDirectory(const FString& Directory, TArray<FString>& OutFiles, bool bRecursive, const FString& Extension)
{
	IFileManager& FileManager = IFileManager::Get();

	FString SearchPattern = Directory / TEXT("*");
	if (!Extension.IsEmpty())
	{
		SearchPattern += Extension.StartsWith(TEXT(".")) ? Extension : TEXT(".") + Extension;
	}

	if (bRecursive)
	{
		FileManager.FindFilesRecursive(
			OutFiles,
			*Directory,
			*SearchPattern,
			true, // Files
			false // Directories
		);
	}
	else
	{
		FileManager.FindFiles(
			OutFiles,
			*SearchPattern,
			true, // Files
			false // Directories
		);
	}

	// Convert to full paths
	for (FString& File : OutFiles)
	{
		File = Directory / File;
	}
}

FString UPulseSystemLibrary::FileSizeToString(const int64& ByteSize)
{
	double kb = (double)ByteSize / 1024;
	double mb = kb / 1024;
	double gb = mb / 1024;
	double tb = gb / 1024;
	double pb = tb / 1024;
	if (pb >= 1) return FString::Printf(TEXT("%.2f PB"), pb);
	if (tb >= 1) return FString::Printf(TEXT("%.2f TB"), tb);
	if (gb >= 1) return FString::Printf(TEXT("%.2f GB"), gb);
	if (mb >= 1) return FString::Printf(TEXT("%.2f MB"), mb);
	if (kb >= 1) return FString::Printf(TEXT("%.2f KB"), kb);
	return FString::Printf(TEXT("%lld B"), ByteSize);
}

bool UPulseSystemLibrary::TextRegex(const FString& InText, const FString& Regex)
{
	FRegexPattern RegexPattern(Regex);
	FRegexMatcher Matcher(RegexPattern, InText);
	if (Matcher.FindNext())
		return true;
	return false;
}
