#pragma once
#include <stdx/traits/is_same.h>

namespace stdx
{
	template<typename ..._T>
	struct type_list;

	template <typename ..._Types>
	struct _TypeList;
	template <typename _First, typename _Second, typename ..._More>
	struct _TypeList<_First,_Second,type_list<_More...>>
	{
		using type = type_list<_First, _Second, _More...>;
	};
	template<typename _First>
	struct _TypeList<_First>
	{
		using type = type_list<_First>;
	};
	template<typename _First,typename _Second>
	struct _TypeList<_First,_Second>
	{
		using type = type_list<_First, _Second>;
	};
	template<>
	struct _TypeList<>
	{
		using type = type_list<>;
	};

	template<>
	struct type_list<>
	{
		using First = void;
	};

	template<int index,typename _TL>
	struct _TypeAt
	{
		using type = typename _TypeAt<index - 2,typename _TL::More>::type;
	};
	template<typename _TL>
	struct _TypeAt<0,_TL>
	{
		using type = typename _TL::First;
	};
	template<typename _TL>
	struct _TypeAt<1,_TL>
	{
		using type = typename _TL::Second;
	};
	template<int index,typename tl>
	using type_at = typename _TypeAt<index, tl>::type;

	template<typename _Type,typename _TL>
	struct _TypePush;

	template<typename _Type,typename ..._Types>
	struct _TypePush<_Type,type_list<_Types...>>
	{
		using type = type_list<_Type,_Types...>;
	};

	template<typename _Type,typename _TL>
	struct _TypeAppend;

	template<typename _Type,typename ..._Types>
	struct _TypeAppend<_Type,type_list<_Types...>>
	{
		using type = type_list<_Types..., _Type>;
	};

	template<typename _Type, typename _TL>
	using type_push = typename _TypePush<_Type,_TL>::type;

	template<typename _Type, typename _TL>
	using type_append = typename _TypeAppend<_Type, _TL>::type;

	template<typename _Type, typename _TL>
	struct _TypeErase;

	template<typename _Type, typename _First>
	struct _TypeErase<_Type,type_list<_First>>
	{
		using type = type_list<_First>;
	};
	template<typename _Type>
	struct _TypeErase<_Type, type_list<_Type>>
	{
		using type = type_list<>;
	};

	template<typename _Type,typename _First,typename _Second>
	struct _TypeErase<_Type,type_list<_First,_Second>>
	{
		using type = type_list<_First, _Second>;
	};

	template<typename _Type,typename _Second>
	struct _TypeErase<_Type, type_list<_Type, _Second>>
	{
		using type = type_list<_Second>;
	};

	template<typename _Type, typename _First>
	struct _TypeErase<_Type, type_list<_First, _Type>>
	{
		using type = type_list<_First>;
	};

	template<typename _Type,typename _First,typename _Second,typename ..._Types>
	struct _TypeErase<_Type,type_list<_First,_Second,_Types...>>
	{
		using type = typename _TypeList<_First,_Second, _TypeErase<_Type,type_list<_Types...>>>::type;
	};

	template<typename _Type, typename _Second, typename ..._Types>
	struct _TypeErase<_Type, type_list<_Type, _Second, _Types...>>
	{
		using type = type_list<_Second,_Types...>;
	};

	template<typename _Type, typename _First, typename ..._Types>
	struct _TypeErase<_Type, type_list<_First, _Type, _Types...>>
	{
		using type = type_list<_First,_Types...>;
	};
	template<typename _Type, typename _TL>
	using type_erase = typename _TypeErase<_Type,_TL>::type;

	template<typename _T,typename _TL>
	struct type_include;

	template<typename _T>
	struct type_include<_T,stdx::type_list<>>
	{
		constexpr static bool value = false;
	};

	template<typename _T,typename _First>
	struct type_include<_T,stdx::type_list<_First>>
	{
		constexpr static bool value = IS_SAME(_T, _First);
	};

	template<typename _T, typename _First,typename _Second>
	struct type_include<_T,stdx::type_list<_First, _Second>>
	{
		constexpr static bool value = IS_SAME(_T, _First) || IS_SAME(_T, _Second);
	};

	template<typename _T, typename _First, typename _Second,typename ..._More>
	struct type_include<_T,stdx::type_list<_First, _Second, _More...>>
	{
		constexpr static bool value = IS_SAME(_T, _First) || IS_SAME(_T, _Second) || stdx::type_include<_T, stdx::type_list<_More...>>::value;
	};

	template<typename _First>
	struct type_list<_First>
	{
		using First = _First;
		constexpr static size_t size = 1;
	};

	template<typename _First,typename _Second>
	struct type_list<_First,_Second>
	{
		using First = _First;
		using Second = _Second;
		constexpr static size_t size = 2;
	};

	template<typename _First,typename _Second,typename ..._More>
	struct type_list<_First,_Second,_More...>
	{
		using First = _First;
		using Second = _Second;
		using More = type_list<_More...>;

		constexpr static size_t size = sizeof...(_More) + 2;
	};
}
