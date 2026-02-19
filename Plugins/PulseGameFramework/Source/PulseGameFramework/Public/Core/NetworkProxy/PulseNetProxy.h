// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "PulseNetProxy.generated.h"


#pragma region Additionnal types

UENUM()
enum class EReplicationEntryOperationType: uint8
{
	Update,
	AddNew,
	Remove,
};


USTRUCT(BlueprintType)
struct FPulseNetReplicatedData
{
	GENERATED_BODY()
public:
	FPulseNetReplicatedData(){}
	FPulseNetReplicatedData(const FName& tag): Tag(tag){}
	FPulseNetReplicatedData WithClass(const UClass* classPtr){ this->SoftClassPtr = classPtr;  return *this;}
	FPulseNetReplicatedData WithString(const FString& string){ this->StringValue = string;  return *this;}
	FPulseNetReplicatedData WithName(FName name)
	{
		this->NameValue = name;
		return *this;
	}
	FPulseNetReplicatedData WithEnumValue(uint8 enumVal){ this->EnumValue = enumVal;  return *this;}
	FPulseNetReplicatedData WithFlags(int32 flags){ this->FlagValue = flags;  return *this;}
	FPulseNetReplicatedData WithInteger(int32 intValue){ this->IntegerValue = intValue;  return *this;}
	FPulseNetReplicatedData WithDouble(double doubleValue){ this->DoubleValue = doubleValue;  return *this;}
	FPulseNetReplicatedData WithFloat(float floatValue, int8 zeroEightIndex = 0)
	{
		if (zeroEightIndex == 0) this->Float31Value.X = floatValue;
		if (zeroEightIndex == 1) this->Float31Value.Y = floatValue;
		if (zeroEightIndex == 2) this->Float31Value.Z = floatValue;
		if (zeroEightIndex == 3) this->Float32Value.X = floatValue;
		if (zeroEightIndex == 4) this->Float32Value.Y = floatValue;
		if (zeroEightIndex == 5) this->Float32Value.Z = floatValue;
		if (zeroEightIndex == 6) this->Float33Value.X = floatValue;
		if (zeroEightIndex == 7) this->Float33Value.Y = floatValue;
		if (zeroEightIndex == 8) this->Float33Value.Z = floatValue;
		return *this;
	}
	FPulseNetReplicatedData WithVector(const FVector& vectorValue, int8 zeroTwoIndex = 0)
	{
		if (zeroTwoIndex == 0) this->Float31Value = vectorValue;
		if (zeroTwoIndex == 1) this->Float32Value = vectorValue;
		if (zeroTwoIndex == 2) this->Float33Value = vectorValue;
		return *this;
	}
	FPulseNetReplicatedData WithQuaternion(const FQuat& quatValue)
	{
		FVector axis;
		float angle;
		quatValue.ToAxisAndAngle(axis, angle);
		this->Float32Value = axis * angle;
		return *this;
	}
	FPulseNetReplicatedData WithTransform(const FTransform& transformValue)
	{
		FVector axis;
		float angle;
		transformValue.GetRotation().ToAxisAndAngle(axis, angle);
		this->Float31Value = transformValue.GetLocation();
		this->Float32Value = axis * angle;
		this->Float33Value = transformValue.GetScale3D();
		return *this;
	}
	bool operator==(const FPulseNetReplicatedData& Other) const
	{
		return Tag == Other.Tag;
	}

	bool operator!=(const FPulseNetReplicatedData& Other) const
	{
		return !(*this == Other);
	}
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PulseCore|Network")	FName Tag = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PulseCore|Network")	int64 ServerArrivalOrder = 0;
	UPROPERTY(SkipSerialization, VisibleAnywhere, BlueprintReadOnly, Category="PulseCore|Network")	int32 OwnerPlayerID = -1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="PulseCore|Network") TSoftClassPtr<UObject> SoftClassPtr = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="PulseCore|Network") FName NameValue = "";
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="PulseCore|Network") FString StringValue = "";
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="PulseCore|Network") uint8 EnumValue = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="PulseCore|Network") int32 FlagValue = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="PulseCore|Network") int32 IntegerValue = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="PulseCore|Network") double DoubleValue = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="PulseCore|Network") FVector_NetQuantize Float31Value = FVector_NetQuantize(FVector::ZeroVector);
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="PulseCore|Network") FVector_NetQuantize Float32Value = FVector_NetQuantize(FVector::ZeroVector);
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="PulseCore|Network") FVector_NetQuantize Float33Value = FVector_NetQuantize(FVector::ZeroVector);
};


USTRUCT()
struct FReplicatedItem: public FFastArraySerializerItem
{
	GENERATED_BODY()
	
	UPROPERTY()	FPulseNetReplicatedData Entry;	
	
	void PreReplicatedRemove(const struct FReplicatedArray& Serializer);
	void PostReplicatedAdd(const struct FReplicatedArray& Serializer);
	void PostReplicatedChange(const struct FReplicatedArray& Serializer);
};

// The container that holds and manages the replication replicated Entries
USTRUCT()
struct FReplicatedArray : public FFastArraySerializer
{
	GENERATED_BODY()

	// This TArray *must* be named "Items"
	UPROPERTY()
	TArray<FReplicatedItem> Items;
	
	UPROPERTY()
	TWeakObjectPtr<UObject> ArrayOwner;

	// This function is required for delta serialization to work
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms);
};

template<>
struct TStructOpsTypeTraits<FReplicatedArray> : public TStructOpsTypeTraitsBase2<FReplicatedArray>
{
	enum { WithNetDeltaSerializer = true };
};

USTRUCT()
struct FProxySaved
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TArray<FPulseNetReplicatedData> RepArray;
};

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnNetReplication_Raw, FName Tag, FPulseNetReplicatedData Value, EReplicationEntryOperationType Operation)
DECLARE_MULTICAST_DELEGATE(FOnPulseNetInit_Raw);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPulseNetInit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnNetReplication, FName, Tag, FPulseNetReplicatedData, Value, EReplicationEntryOperationType, Operation);

class UPulseNetManager;

#pragma endregion


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PULSEGAMEFRAMEWORK_API APulseNetProxy : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	APulseNetProxy();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void SetPlayerID(const int32 newPlayerID, bool Override = false);
	void SetNetParams(const bool NetAlwaysRelevant = true, const float NetworkPriority = 2.8f, const float NetworkUpdateFrequency = 100.0f);
	void InitFromSaved(const FProxySaved& ProxySaved);
	FProxySaved SaveProxy();
	int32 GetLocalPlayerControllerID() const;
	APlayerController* GetLocalPlayerController() const;
	bool TagExist(const FName& Tag) const;

private:	
	UPROPERTY(ReplicatedUsing=OnRep_ReplicatedValues)
	FReplicatedArray _replicatedValues;

	UFUNCTION()
	void OnRep_ReplicatedValues();

protected:

	UPROPERTY(Replicated, VisibleDefaultsOnly, BlueprintReadOnly, Category="PulseCore|Network")
	int32 PlayerID = -1;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="PulseCore|Network")
	int32 LocalPlayerControllerID = -1;
	
public:

	UFUNCTION(BlueprintPure, Category="PulseCore|Network")
	int32 GetPlayerID() const;

	UFUNCTION(Server, Reliable)
	void ReplicateValue(FPulseNetReplicatedData Value);
	
	UFUNCTION(Server, Reliable)
	void RemoveReplicatedItem(FName Tag);

	UFUNCTION(Server, Reliable)
	void ReliableServerRPC_AddOrUpdate(FPulseNetReplicatedData Value = FPulseNetReplicatedData());

	UFUNCTION(Server, Reliable)
	void ReliableServerRPC_Remove(FName Tag);
	
	// Get Replicated Net value from a Tag.
	// Can including every children tags on demand.
	// return true if the value or at least one child value where found.
	bool QueryNetValue(FName Tag, FPulseNetReplicatedData& OutValue, bool bIncludeChildValues, TArray<FPulseNetReplicatedData>& OutChildValues) const;
};
