// Copyright © by Tyni Boat. All Rights Reserved.


#include "NetworkProxy/PulseNetEmitter.h"

#include "NetworkProxy/PulseNetManager.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"


// Sets default values
APulseNetEmitter::APulseNetEmitter()
{
	PrimaryActorTick.bCanEverTick = false;
	SetNetParams();
	bReplicates = true;
}

void APulseNetEmitter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APulseNetEmitter, TempPLayerID);
}

void APulseNetEmitter::SetNetParams(const float NetworkPriority, const float NetworkUpdateFrequency)
{
	SetNetUpdateFrequency(NetworkUpdateFrequency); // High update frequency.
	NetPriority = NetworkPriority; // High priority.
}

void APulseNetEmitter::BeginPlay()
{
	Super::BeginPlay();
	if (const auto Ctrl = Cast<AController>(Owner))
	{
		_cachedPs = Ctrl->GetPlayerState<APlayerState>();
	}
	UPulseNetManager::RegisterEmitter(this);
}

int32 APulseNetEmitter::GetPlayerID() const
{
	if (_cachedPs == nullptr)
		return TempPLayerID;
	return _cachedPs->GetPlayerId();
}

bool APulseNetEmitter::GetPendingMessages(TArray<FPulseNetReplicatedData>& OutPendingMessages) const
{
	int32 addCount = 0;
	for (const auto& pending : _PendingNetMessages)
	{
		addCount++;
		OutPendingMessages.Add(pending.Value);
	}
	return addCount > 0;
}

void APulseNetEmitter::BroadcastNetMessage(FPulseNetReplicatedData Value)
{
	if (Value.Tag.IsNone())
		return;
	// Add/Update pending
	if (_PendingNetMessages.Contains(Value.Tag))
		_PendingNetMessages[Value.Tag] = Value;
	else
		_PendingNetMessages.Add(Value.Tag, Value);
	// Broadcast
	Value.OwnerPlayerID = GetPlayerID();
	BroadcastNetMessage_Server(Value);
}

void APulseNetEmitter::DeleteNetEntry(const FName Tag)
{
	// Update pending
	if (_PendingNetMessages.Contains(Tag))
		_PendingNetMessages.Remove(Tag);
	// Ask delete
	DeleteNetEntry_Server(Tag);
}

void APulseNetEmitter::OnRep_ReplicatedValue(FName Tag, FPulseNetReplicatedData Value, EReplicationEntryOperationType Operation)
{
	if (_PendingNetMessages.Contains(Tag))
		_PendingNetMessages.Remove(Tag);
}

void APulseNetEmitter::BroadcastNetMessage_Server_Implementation(FPulseNetReplicatedData Value)
{
	if (auto receptor = UPulseNetManager::GetReceptor(this))
	{
		receptor->AddOrUpdateEntry_Server(Value);
	}
}

void APulseNetEmitter::DeleteNetEntry_Server_Implementation(const FName Tag)
{
	if (auto receptor = UPulseNetManager::GetReceptor(this))
	{
		receptor->RemoveEntry_Server(Tag);
	}
}


