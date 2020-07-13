#pragma once
#include <stdx/env.h>

namespace stdx
{
	template<uint64_t _A, uint64_t _B>
	struct _IsNotDiv
	{
		static constexpr bool value = _A % _B;
	};
	template<uint64_t _A, uint64_t _B>
	struct _LoopNotDiv
	{
		static constexpr bool value = stdx::_IsNotDiv<_A, _B>::value && stdx::_LoopNotDiv<_A, _B - 1>::value;
	};
	template<uint64_t _A>
	struct _LoopNotDiv<_A, 1>
	{
		static constexpr bool value = true;
	};
	template<uint64_t _A>
	struct _LoopNotDiv<_A, _A>
	{
		static constexpr bool value = stdx::_LoopNotDiv<_A, _A - 1>::value;
	};
	template<>
	struct _LoopNotDiv<1, 1>
	{
		static constexpr bool value = true;
	};
	template<uint64_t _N>
	struct is_prime_number
	{
		static  constexpr bool value = stdx::_LoopNotDiv<_N, _N>::value;
	};
}