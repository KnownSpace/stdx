#pragma once

namespace stdx
{
	template<typename _T,typename _U>
	struct _IsSame
	{
		constexpr static bool value = false;
	};
	template<typename _T>
	struct _IsSame<_T,_T>
	{
		constexpr static bool value = true;
	};
#define IS_SAME(t,u) stdx::_IsSame<t,u>::value

}