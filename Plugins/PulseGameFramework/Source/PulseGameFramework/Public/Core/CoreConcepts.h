// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once
#include <concepts>
#include <type_traits>


class UPulseModuleBase;
class UPulseSubModuleBase;

namespace pulseCore::concepts
{
	template<typename T>
	concept IsNumber = std::integral<T> || std::floating_point<T>;
	template <typename T, typename U = T>
	concept Addable = requires(T a, U b) { { a + b } -> std::common_with<T>; };
	template <typename T, typename U = T>
	concept Multiplicable = requires(T a, U b) { { a * b } -> std::common_with<T>; };
	template <typename T, typename U = T>
	concept Divisible = requires(T a, U b) { { a / b } -> std::common_with<T>; };
	template <typename T>
	concept IsAModuleConfig = std::is_base_of_v<UDeveloperSettings, T>;
	template <typename T>
	concept IsUObject = std::is_base_of_v<UObject, T>;
	template <typename T>
	concept IsSubModule = std::derived_from<T, UPulseSubModuleBase>;
	template <typename T>
	concept IsModule = std::derived_from<T, UPulseModuleBase>;
	template <typename T, typename Q>
	concept IsSubclassOf = requires(Q b) { std::derived_from<Q, UClass> && T::StaticClass() == b; };
	template <typename T, typename Q, typename U, typename V>
	concept AreSubclassesOf = requires(Q b, V c) { std::derived_from<Q, UClass> && T::StaticClass() == b && std::derived_from<V, UClass> && U::StaticClass() == c; };
}
