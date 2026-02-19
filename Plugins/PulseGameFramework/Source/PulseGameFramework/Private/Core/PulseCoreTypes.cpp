// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/PulseCoreTypes.h"

#include "UserProfile/PulseUserProfile.h"

FCodedOperation::FCodedOperation()
{
}

FCodedOperation::~FCodedOperation()
{
}

bool FCodedOperation::Evaluate() const
{
	switch (Comparator)
	{
	case ELogicComparator::Equal:
		return AValue == BValue;
	case ELogicComparator::NotEqual:
		return AValue != BValue;
	case ELogicComparator::GreaterThan:
		return AValue > BValue;
	case ELogicComparator::LessThan:
		return AValue < BValue;
	case ELogicComparator::GreaterThanOrEqualTo:
		return AValue >= BValue;
	case ELogicComparator::LessThanOrEqualTo:
		return AValue <= BValue;
	}
	return false;
}

bool UBaseGameCondition::EvaluateCondition(const int32 Code, bool bInvalidCodeFallbackResponse) const
{
	return ConditionParam.Evaluate();
}

bool IIPulseCore::GetCurrentUserProfile(FUserProfile& OutProfile) const
{
	if (auto profileMgr = UPulseUserProfile::Get())
	{
		bool validUser = false;
		OutProfile = profileMgr->GetCurrentUserProfile(validUser);
		return validUser;
	}
	return false;
}

UCoreProjectSetting* IIPulseCore::GetProjectSettings() const
{
	return GetMutableDefault<UCoreProjectSetting>();
}
