#pragma once
#include <stdx/env.h>

namespace stdx
{
	template<typename _From,typename _To>
	struct convertable
	{
	private:
		class ture_t;
		class false_t;
		static ture_t test(_To& v);
		static false_t test(...);
	public:
		constexpr static bool value = std::is_same<decltype(test(stdx::declref<_From>())), ture_t>::value;
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