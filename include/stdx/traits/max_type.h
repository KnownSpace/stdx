#pragma once
#include <stdx/env.h>

namespace stdx
{
	template<typename _T1,typename _T2>
	extern auto _MaxTypeHelper() -> decltype(false?stdx::declval<_T1>():stdx::declval<_T2>());

	template<typename _T1, typename _T2>
	using max_type = typename decltype(stdx::_MaxTypeHelper<_T1,_T2>());
}