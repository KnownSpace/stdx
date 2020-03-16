#pragma once
#include<stdx/env.h>

namespace stdx
{
	template<typename _Derived, typename _Base>
	struct is_base_on
	{
	private:
		class true_t;
		class false_t;
		static true_t test(_Base*);
		static false_t test(void*);
	public:
		constexpr static bool value = std::is_same<decltype(test(stdx::declptr<_Derived>())), true_t>::value;
	};

	template<typename _Derived>
	struct is_base_on<_Derived,void>
	{
	public:
		constexpr static bool value = false;
	};

	template<typename _Base>
	struct is_base_on<void,_Base>
	{
	public:
		constexpr static bool value = false;
	};
}