#pragma once
#include <vector>
#include <stdx/traits/max_type.h>
namespace stdx
{
	struct sort_way
	{
		enum
		{
			bigger = 0,
			smaller = 1
		};
	};
	//快速排序方法(从大到小)
	template<typename _T, typename _TContainer = std::vector<_T>>
	void quicksort_bigger(_TContainer &container, size_t begin, size_t end)
	{
		if (end == 0)
		{
			return;
		}
		end = end - 1;
		if (begin > end)
		{
			return;
		}
		auto basic = container[begin];
		auto begin_ps = begin;
		auto end_ps = end;
		while (begin != end)
		{
			while (container[end] <= basic && begin < end)
			{
				end--;
			}
			while (container[begin] >= basic && begin < end)
			{
				begin++;
			}
			if (begin<end)
			{
				auto temp = container[begin];
				container[begin] = container[end];
				container[end] = temp;
			}
		}
		container[begin_ps] = container[begin];
		container[begin] = basic;
		quicksort_bigger<_T, _TContainer>(container, begin_ps, begin);
		quicksort_bigger<_T, _TContainer>(container, begin + 1, end_ps + 1);
	}
	//快速排序方法(从小到大)
	template<typename T, typename TContainer = std::vector<T>>
	void quicksort_smaller(TContainer &container, size_t begin, size_t end)
	{
		if (end == 0)
		{
			return;
		}
		end = end - 1;
		if (begin > end)
		{
			return;
		}
		auto basic = container[begin];
		auto begin_ps = begin;
		auto end_ps = end;
		while (begin != end)
		{
			while (container[end] >= basic && begin < end)
			{
				end--;
			}
			while (container[begin] <= basic && begin < end)
			{
				begin++;
			}
			if (begin<end)
			{
				auto temp = container[begin];
				container[begin] = container[end];
				container[end] = temp;
			}
		}
		container[begin_ps] = container[begin];
		container[begin] = basic;
		quicksort_smaller<T, TContainer>(container, begin_ps, begin);
		quicksort_smaller<T, TContainer>(container, begin + 1, end_ps + 1);
	}

	//整型最大值
	template<typename _T1,typename _T2>
	inline stdx::max_type<_T1, _T2> max_value(const _T1& v1, const _T2& v2)
	{
		using ret_t = stdx::max_type<_T1, _T2>;
		if ((ret_t)v1>(ret_t)v2)
		{
			return (ret_t)v1;
		}
		return (ret_t)v2;
	}

	//整型最小值
	template<typename _T1, typename _T2>
	inline stdx::max_type<_T1, _T2> min_value(const _T1& v1, const _T2& v2)
	{
		using ret_t = stdx::max_type<_T1, _T2>;
		if ((ret_t)v1 < (ret_t)v2)
		{
			return (ret_t)v1;
		}
		return (ret_t)v2;
	}
}