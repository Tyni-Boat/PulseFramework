// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "PulseNetMgrActor.generated.h"


#pragma region Additionnal types

UENUM(BlueprintType)
enum class EReplicationEntryOperationType: uint8
{
	Update,
	AddNew,
	Remove,
};


USTRUCT(BlueprintType)
struct FReplicatedEntry
{
	GENERATED_BODY()
public:
	FReplicatedEntry(){}
	FReplicatedEntry(FName tag): Tag(tag){}
	FReplicatedEntry WithObject(UObject* objPtr){ this->WeakObjectPtr = objPtr;  return *this;}
	FReplicatedEntry WithName(FName name){ this->NameValue = name;  return *this;}
	FReplicatedEntry WithEnumValue(uint8 enumVal){ this->EnumValue = enumVal;  return *this;}
	FReplicatedEntry WithFlags(int32 flags){ this->FlagValue = flags;  return *this;}
	FReplicatedEntry WithInteger(int32 intValue){ this->IntegerValue = intValue;  return *this;}
	FReplicatedEntry WithDouble(double doubleValue){ this->DoubleValue = doubleValue;  return *this;}
	FReplicatedEntry WithFloat(float floatValue, int8 zeroEightIndex = 0)
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
	FReplicatedEntry WithVector(const FVector& vectorValue, int8 zeroTwoIndex = 0)
	{
		if (zeroTwoIndex == 0) this->Float31Value = vectorValue;
		if (zeroTwoIndex == 1) this->Float32Value = vectorValue;
		if (zeroTwoIndex == 2) this->Float33Value = vectorValue;
		return *this;
	}
	FReplicatedEntry WithQuaternion(const FQuat& quatValue)
	{
		FVector axis;
		float angle;
		quatValue.ToAxisAndAngle(axis, angle);
		this->Float32Value = axis * angle;
		return *this;
	}
	FReplicatedEntry WithTransform(const FTransform& transformValue)
	{
		FVector axis;
		float angle;
		transformValue.GetRotation().ToAxisAndAngle(axis, angle);
		this->Float31Value = transformValue.GetLocation();
		this->Float32Value = axis * angle;
		this->Float33Value = transformValue.GetScale3D();
		return *this;
	}
	bool IsValidNetEntry() const { return FGameplayTag::RequestGameplayTag(Tag, false).IsValid(); }
	FGameplayTag GTag() const { return FGameplayTag::RequestGameplayTag(Tag, false); }
	
	UPROPERTY()	FName Tag = "";
	UPROPERTY()	TWeakObjectPtr<UObject> WeakObjectPtr = nullptr;
	UPROPERTY()	FName NameValue = "";
	UPROPERTY()	uint8 EnumValue = 0;
	UPROPERTY()	int32 FlagValue = 0;
	UPROPERTY()	int32 IntegerValue = 0;
	UPROPERTY()	double DoubleValue = 0;
	UPROPERTY()	FVector_NetQuantize Float31Value = FVector_NetQuantize(FVector::ZeroVector);
	UPROPERTY()	FVector_NetQuantize Float32Value = FVector_NetQuantize(FVector::ZeroVector);
	UPROPERTY()	FVector_NetQuantize Float33Value = FVector_NetQuantize(FVector::ZeroVector);
};


USTRUCT()
struct FReplicatedItem: public FFastArraySerializerItem
{
	GENERATED_BODY()
	
	UPROPERTY()	FReplicatedEntry Entry;	
	
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

	// This function is required for delta serialization to work
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms);
};

template<>
struct TStructOpsTypeTraits<FReplicatedArray> : public TStructOpsTypeTraitsBase2<FReplicatedArray>
{
	enum { WithNetDeltaSerializer = true };
};

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnNetReplication_Raw, FGameplayTag Tag, FReplicatedEntry Value, EReplicationEntryOperationType Operation)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnNetRPC_Raw, FGameplayTag Tag, FReplicatedEntry Value)
DECLARE_MULTICAST_DELEGATE(FOnPulseNetInit_Raw);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPulseNetInit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnNetReplication, FGameplayTag, Tag, FReplicatedEntry, Value, EReplicationEntryOperationType, Operation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNetRPC, FGameplayTag, Tag, FReplicatedEntry, Value);

class UPulseNetManager;

#pragma endregion


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PULSEGAMEFRAMEWORK_API APulseNetMgrActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	APulseNetMgrActor();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:	
	UPROPERTY(ReplicatedUsing=OnRep_ReplicatedValues)
	FReplicatedArray _replicatedValues;

	UFUNCTION()
	void OnRep_ReplicatedValues();

	void PostMulticastRPC(FReplicatedEntry Value) const;
	
public:
	
	UPROPERTY()
	TWeakObjectPtr<UPulseNetManager> _manager;

	UFUNCTION(Server, Reliable)
	void ReplicateValue(FReplicatedEntry Value);
	
	UFUNCTION(Server, Reliable)
	void RemoveReplicatedItem(FName Tag, bool IncludeChildValues = true);

	UFUNCTION(Server, Reliable)
	void ReliableServerRPC(FReplicatedEntry Value);
	
	UFUNCTION(Server, UnReliable)
	void UnreliableServerRPC(FReplicatedEntry Value);
	
	UFUNCTION(NetMulticast, Reliable)
	void ReliableMulticastRPC(FReplicatedEntry Value);
	
	UFUNCTION(NetMulticast, UnReliable)
	void UnreliableMulticastRPC(FReplicatedEntry Value);

	// Get only the exact corresponding Tag. Return true if he tag is found. 
	bool GetReplicatedValue(FGameplayTag Tag, FReplicatedEntry& Value);
	
	// Get only the exact corresponding Tag (index 0 if any) including every children tags. return true if at least one tag where found
	bool GetReplicatedValues(FGameplayTag Tag, TArray<FReplicatedEntry>& Values);
};
