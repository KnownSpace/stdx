#pragma once
#include <stdx/env.h>

namespace stdx
{
	template<typename _From,typename _To>
	struct convertable
	{
	private:
		class true_t;
		class false_t;
		static true_t test(_To&& v);
		static false_t test(...);
	public:
		constexpr static bool value = std::is_same<decltype(test(stdx::declrref<_From>())), true_t>::value;
	};

	template<typename _From>
	struct convertable<_From,void>
	{
	public:
		constexpr static bool value = false;
	};

	template<typename _To>
	struct convertable<void,_To>
	{
	public:
		constexpr static bool value = false;
	};
}