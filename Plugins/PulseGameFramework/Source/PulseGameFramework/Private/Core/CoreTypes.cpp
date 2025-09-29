// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/CoreTypes.h"

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

bool UBaseOperationModifier::TryApplyModifier(const float InitialAmount, float& FinalAmount, UObject* FromObject, UObject* ToObject, FName Context) const
{
	return false;
}
