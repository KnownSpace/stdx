#pragma once

namespace stdx
{
	//��������
	template<typename _t>
	struct _RefType
	{
		using type = _t & ;
	};

	template<typename _t>
	struct _RefType<const _t>
	{
		using type = const _t & ;
	};

	template<typename _t>
	struct _RefType<_t&>
	{
		using type = _t & ;
	};

	template<typename _t>
	struct _RefType<const _t&>
	{
		using type = const _t &;
	};


	template<>
	struct _RefType<void>
	{
		using type = void;
	};

	template<typename T>
	using ref_t = typename _RefType<T>::type;
}