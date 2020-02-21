#pragma once

namespace stdx
{
	template<bool _Cond, typename _T1, typename _T2>
	struct _Max
	{
		using type = _T1;
	};

	template<typename _T1, typename _T2>
	struct _Max<false, _T1, _T2>
	{
		using type = _T2;
	};

	template<typename _T1, typename _T2>
	using max_type = typename stdx::_Max<(sizeof(_T1) > sizeof(_T2)),_T1,_T2>::type;
}