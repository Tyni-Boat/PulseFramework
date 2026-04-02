// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LocomotionTypes.generated.h"



// Represent the balance state of a pulse pawn.
UENUM(BlueprintType)
enum EPulseLocomotionBalanceState
{
	PerfectlyBalanced = 0 UMETA(DisplayName = "Perfect Balance"),
	Balanced = 1 UMETA(DisplayName = "Mostly Balanced"),
	CriticalBalance = 2 UMETA(DisplayName = "Critical Balance"),
	Unbalanced = 3 UMETA(DisplayName = "Completely Unbalanced"),
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPulsePawnBalanceEvent, APawn*, Pawn, EPulseLocomotionBalanceState, oldState, EPulseLocomotionBalanceState, newState);