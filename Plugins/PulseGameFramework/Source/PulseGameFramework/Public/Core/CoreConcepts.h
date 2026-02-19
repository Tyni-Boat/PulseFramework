// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once
#include <concepts>
#include <type_traits>


// Don't use std:: concepts, since those C++20 features are not supported on mobile 
namespace pulseCore::concepts
{
	template <typename T>
	concept IsUStruct = requires { TBaseStructure<T>::Get(); };
	template <typename T, typename Q>
	concept IsDerivedFrom = requires { __is_base_of(Q, T)
	&& __is_convertible_to(const volatile T*, const volatile Q*); };
}
