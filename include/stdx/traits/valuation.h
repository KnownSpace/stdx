#pragma once
#include <stdx/env.h>

namespace stdx
{
	template<typename _T>
	struct have_copy_valuation
	{
	private:
		class false_t;
		template<typename U>
		static false_t test(...);
		template<typename U>
		static auto test(int)->decltype(stdx::declref<U>() = stdx::declcref<U>());
	public:
		constexpr static bool value = !std::is_same<false_t, decltype(test<_T>(1))>::value;
	};

	template<>
	struct have_copy_valuation<void>
	{
		const static bool value = false;
	};

	template<typename _T>
	struct have_move_valuation
	{
	private:
		class false_t;
		template<typename U>
		static false_t test(...);
		template<typename U>
		static auto test(int)->decltype(stdx::declref<U>() = stdx::declrref<U>());
	public:
		constexpr static bool value = !std::is_same<false_t, decltype(test<_T>(1))>::value;
	};

	template<>
	struct have_move_valuation<void>
	{
		constexpr static bool value = false;
	};

	template<typename _T>
	struct have_valuation
	{
		constexpr static bool value = stdx::have_copy_valuation<_T>::value || stdx::have_move_valuation<_T>::value;
	};
}