#pragma once
#include <functional>
#include <stdx/traits/type_list.h>

namespace stdx
{

	template<typename _Fn>
	struct _FunctionInfo;

	template<typename _Result, typename ..._Args>
	struct _FunctionInfo<_Result(*)(_Args...)>
	{
		using result = _Result;
		using arguments = stdx::type_list<_Args...>;
	};

	template<typename _Class, typename _Result, typename ..._Args>
	struct _FunctionInfo<_Result(_Class::*)(_Args...)>
	{
		using belong_type = _Class;
		using result = _Result;
		using arguments = stdx::type_list<_Args...>;
	};

	template<typename _Class, typename _Result, typename ..._Args>
	struct _FunctionInfo<_Result(_Class::*)(_Args...) const>
	{
		using belong_type = _Class;
		using result = _Result;
		using arguments = stdx::type_list<_Args...>;
	};

	template<typename _Fn>
	struct _FunctionInfo
	{
	private:
		using info = _FunctionInfo<decltype(&_Fn::operator())>;
	public:
		using result = typename info::result;
		using arguments = typename info::arguments;
		using belong_type = _Fn;
	};

	template<typename _Fn>
	struct _FunctionInfo<_Fn&>
	{
	private:
		using info = _FunctionInfo<_Fn>;
	public:
		using result = typename info::result;
		using arguments = typename info::arguments;
		using belong_type = _Fn;
	};

	template<typename _Fn>
	struct _FunctionInfo<_Fn&&>
	{
	private:
		using info = _FunctionInfo<_Fn>;
	public:
		using result = typename info::result;
		using arguments = typename info::arguments;
		using belong_type = _Fn;
	};

	template<typename _Fn>
	struct _FunctionInfo<const _Fn&>
	{
	private:
		using info = _FunctionInfo<_Fn>;
	public:
		using result = typename info::result;
		using arguments = typename info::arguments;
		using belong_type = _Fn;
	};

	template <typename T>
	struct is_callable 
	{
	private:
		class false_t;
	public:
		template<typename U>  static auto test(int)->decltype(&U::operator());
		template<typename U> static false_t test(...);
		constexpr static bool value = !std::is_same<decltype(test<T>(0)), false_t>::value;
	};

	template<typename _Fn, bool>
	struct _FunctionInfoChecker;

	template<typename _Fn>
	struct _FunctionInfoChecker<_Fn, false>
	{
		constexpr static bool is_callable = false;
	};

	template<typename _Fn>
	struct _FunctionInfoChecker<_Fn, true> :public _FunctionInfo<_Fn>
	{
		constexpr static bool is_callable = true;
	};

	template<typename _Fn>
	using function_info = _FunctionInfoChecker<_Fn, stdx::is_callable<_Fn>::value>;

	template<typename _Fn, typename ..._Args>
	struct _IsArgs
	{
	private:
		using info = stdx::function_info<_Fn>;
		using arguments = typename info::arguments;
	public:
		constexpr static bool value = IS_SAME(arguments, stdx::type_list<_Args...>);
	};


#ifdef WIN32
#define IS_ARGUMENTS_TYPE(_Fn,...) stdx::_IsArgs<_Fn,__VA_ARGS__>::value
#else
#define IS_ARGUMENTS_TYPE(_Fn,args...) stdx::_IsArgs<_Fn,##args>::value
#endif



	template<typename _Fn, typename _Result>
	struct _IsResult
	{
	private:
		using info = stdx::function_info<_Fn>;
		using result = typename info::result;
	public:
		constexpr static bool value = IS_SAME(result, _Result);
	};
#define IS_RESULT_TYPE(_Fn,_Result) stdx::_IsResult<_Fn,_Result>::value

	template<typename _Fn,typename ..._Args,class = typename std::enable_if<stdx::is_callable<_Fn>::value>::type>
	inline typename stdx::function_info<_Fn>::result invoke( _Fn &&callable,_Args ...args)
	{
		return callable(stdx::forward(args)...);
	}

	template<typename _R = void>
	INTERFACE_CLASS basic_runable
	{
	public:
		INTERFACE_CLASS_HELPER(basic_runable);
		virtual _R run() = 0;
	};

	template<typename _t, typename _fn>
	struct _ActionRunner
	{
		//static _t run(_fn &&fn)
		//{
		//	return fn();
		//}

		static _t run(_fn &fn)
		{
			return fn();
		}
	};

	template<typename _fn>
	struct _ActionRunner<void, _fn>
	{
		//static void run(_fn &&fn)
		//{
		//	fn();
		//	return;
		//}

		static void run(_fn &fn)
		{
			fn();
			return;
		}
	};

	template<typename _R,typename _Fn>
	class _Runable:public basic_runable<_R>
	{
	public:
		_Runable()
			:basic_runable<_R>()
		{}

		_Runable(_Fn fn)
			:basic_runable<_R>()
			,m_func(fn)
		{}

		~_Runable()=default;

		virtual _R run() override
		{
			return _ActionRunner<_R,_Fn>::run(m_func);
		}

		operator bool()
		{
			return m_func;
		}
	private:
		_Fn m_func;
	};

	template<typename T>
	using runable_ptr = std::shared_ptr<basic_runable<T>>;
	
	template<typename T, typename _Fn>
	inline runable_ptr<T> make_runable(_Fn &&fn)
	{
		return std::make_shared<_Runable<T,_Fn>>(fn);
	}

	template<typename T,typename _Fn,typename ..._Args>
	inline runable_ptr<T> make_runable(_Fn &&fn,_Args &&...args)
	{
		return make_runable<T>(std::bind(fn, args...));
	};

	template<typename T, typename _Fn, typename ..._Args>
	inline runable_ptr<T> make_runable(_Fn &fn, _Args &...args)
	{
		return make_runable<T>(std::bind(fn, args...));
	}

	template<typename _R,typename ..._Args>
	INTERFACE_CLASS _BasicFunction
	{
	public:
		INTERFACE_CLASS_HELPER(_BasicFunction);
		virtual _R run(const _Args&...) = 0;
	};
	template<typename _R,typename _Fn,typename ..._Args>
	class _Function:public _BasicFunction<_R,_Args...>
	{
	public:
		_Function(_Fn &&fn)
			:_BasicFunction<_R,_Args...>()
			,m_fn(fn)
		{}
		~_Function() = default;
		_R run(const _Args &...args) override
		{
			return stdx::invoke(m_fn,args...);
		}
	private:
		_Fn m_fn;
	};
	template<typename _R,typename ..._Args>
	class function
	{
		using impl_t = std::shared_ptr<_BasicFunction<_R,_Args...>>;
	public:
		function() = default;
		template<typename _Fn,class = typename std::enable_if<stdx::is_callable<_Fn>::value>::type>
		function(_Fn &&fn)
			:m_impl(new _Function<_R,_Fn,_Args...>(std::move(fn)))
		{
		}
		function(const function<_R,_Args...> &other)
			:m_impl(other.m_impl)
		{}
		function(function<_R,_Args...> &&other)
			:m_impl(std::move(other.m_impl))
		{}
		~function() = default;
		function<_R, _Args...> &operator=(const function<_R, _Args...> &other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		_R operator()(const _Args &...args)
		{
			return m_impl->run(args...);
		}
		operator bool()
		{
			return (bool)m_impl;
		}
	private:
		impl_t m_impl;
	};
}
