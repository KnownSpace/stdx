#pragma once
#include <stdx/env.h>

namespace stdx
{
	template<typename _T>
	struct have_copy_construction
	{
	private:
		class false_t;
		template<typename _U>
		static false_t test(...);
		template<typename _U>
		static auto test(int)->decltype(_U(stdx::declcref<_U>()));
	public:
		constexpr static bool value = !std::is_same<false_t, decltype(test<_T>(1))>::value;
	};

	template<>
	struct have_copy_construction<void>
	{
		const static bool value = false;
	};

	template<typename _T>
	struct have_move_construction
	{
	private:
		class false_t;
		template<typename _U>
		static false_t test(...);
		template<typename _U>
		static auto test(int)->decltype(_U(stdx::declrref<_U>()));
	public:
		constexpr static bool value = !std::is_same<false_t, decltype(test<_T>(1))>::value;
	};

	template<>
	struct have_move_construction<void>
	{
		constexpr static bool value = false;
	};
}