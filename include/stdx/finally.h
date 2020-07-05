#pragma once
#include <functional>
#include <stdx/function.h>
#ifdef DEBUG
#include <stdio.h>
#endif

namespace stdx
{
	struct finally
	{
	public:
		template<typename _Fn,class = typename std::enable_if<stdx::is_callable<_Fn>::value>::type>
		finally(_Fn &&fn)
			:m_fn(fn)
		{}
		template<typename _Fn,typename ..._Args, class = typename std::enable_if<stdx::is_callable<_Fn>::value>::type>
		finally(_Fn&& fn, _Args&&...args)
			:m_fn(std::bind(fn,args...))
		{}

		finally(const finally&) = delete;
		finally(finally&&) = delete;

		finally& operator=(const finally&) = delete;
		finally& operator=(finally&&) = delete;

		~finally()
		{
			try
			{
				if (m_fn)
				{
					m_fn();
				}
			}
			catch (const std::exception &err)
			{
				DBG_VAR(err);
#ifdef DEBUG
				::printf("[Finally]Error: %s",err.what());
#endif
			}
		}
	private:
		std::function<void()> m_fn;
	};
}