#pragma once
#include <functional>
#include <memory>
#include <stdx/async/spin_lock.h>
#include <mutex>
#include <stdx/function.h>
#include <stdx/traits/type_list.h>

namespace stdx
{
	template<typename _T>
	struct basic_factory_unit
	{
		virtual ~basic_factory_unit() = default;
		virtual _T make() const = 0;
	};

	template<typename _T>
	struct default_factory_unit:public stdx::basic_factory_unit<_T>
	{
	public:
		default_factory_unit()
			:stdx::basic_factory_unit<_T>()
		{}

		default_factory_unit(const default_factory_unit<_T> &other)
			:stdx::basic_factory_unit<_T>()
		{}

		default_factory_unit(default_factory_unit<_T> &&other) noexcept
			:stdx::basic_factory_unit<_T>()
		{}

		virtual ~default_factory_unit() = default;

		stdx::default_factory_unit<_T> &operator=(const stdx::default_factory_unit<_T> &other)
		{
			return *this;
		}

		stdx::default_factory_unit<_T> &operator=(stdx::default_factory_unit<_T> &&other) noexcept
		{
			return *this;
		}

		virtual _T make() const override
		{
			return _T();
		}
	private:
	};

	template<typename _T>
	struct default_factory_unit<_T&> :public stdx::basic_factory_unit<_T&>
	{
	public:
		default_factory_unit()
			:stdx::basic_factory_unit<_T&>()
		{}

		default_factory_unit(const stdx::default_factory_unit<_T&>& other)
			:stdx::basic_factory_unit<_T&>()
		{}

		default_factory_unit(stdx::default_factory_unit<_T&>&& other) noexcept
			:stdx::basic_factory_unit<_T&>()
		{}

		virtual ~default_factory_unit() = default;

		stdx::default_factory_unit<_T&>& operator=(const stdx::default_factory_unit<_T> & other)
		{
			return *this;
		}

		stdx::default_factory_unit<_T&>& operator=(stdx::default_factory_unit<_T> && other) noexcept
		{
			return *this;
		}
	};

	template<typename _T>
	struct dynamic_factory_unit:public stdx::default_factory_unit<_T>
	{
	public:
		dynamic_factory_unit()
			:stdx::default_factory_unit<_T>()
			,m_maker()
		{}

		dynamic_factory_unit(std::function<_T()> &&maker)
			:stdx::default_factory_unit<_T>()
			,m_maker(std::move(maker))
		{}

		dynamic_factory_unit(std::function<_T()> &maker)
			:stdx::default_factory_unit<_T>()
			,m_maker(maker)
		{}

		dynamic_factory_unit(const stdx::dynamic_factory_unit<_T> &other)
			:stdx::default_factory_unit<_T>()
			,m_maker(other.m_maker)
		{}

		dynamic_factory_unit(stdx::dynamic_factory_unit<_T> &&other) noexcept
			:stdx::default_factory_unit<_T>()
			, m_maker(std::move(other.m_maker))
		{}

		virtual ~dynamic_factory_unit() = default;

		stdx::dynamic_factory_unit<_T>& operator=(const stdx::dynamic_factory_unit<_T>& other)
		{
			m_maker = other.m_maker;
			return *this;
		}

		stdx::dynamic_factory_unit<_T>& operator=(stdx::dynamic_factory_unit<_T> &&other)
		{
			m_maker = std::move(other.m_maker);
			return *this;
		}

		virtual _T make() const override
		{
			if (!m_maker)
			{
				throw std::logic_error("you should configurate first");
			}
			return m_maker();
		}

		void config(std::function<_T()> &&maker)
		{
			m_maker = std::move(maker);
		}

		void config(const std::function<_T()>& maker)
		{
			m_maker = maker;
		}

		template<typename _Fn,class = typename std::enable_if<stdx::is_callable<_Fn>::value && std::is_same<stdx::function_info<_Fn>::result,_T>::value>::type>
		void config()
		{
			m_maker = []() 
			{
				return _Fn()();
			};
		}
	protected:
		std::function<_T()> m_maker;
	};

	template<typename _T>
	struct singleton_factory_unit:public stdx::default_factory_unit<_T&>
	{
	public:
		singleton_factory_unit()
			:stdx::default_factory_unit<_T&>()
			,m_lock()
			,m_instance(nullptr)
		{}

		singleton_factory_unit(const stdx::singleton_factory_unit<_T> &other)
			:stdx::default_factory_unit<_T&>()
			,m_lock(other.m_lock)
			,m_instance(other.m_instance)
		{}

		virtual ~singleton_factory_unit() = default;

		stdx::singleton_factory_unit<_T> &operator=(const stdx::singleton_factory_unit<_T> &other)
		{
			m_lock = other.m_lock;
			m_instance = other.m_instance;
			return *this;
		}

		virtual _T& make() const override
		{
			std::unique_lock<stdx::spin_lock> lock(m_lock);
			if (!m_instance)
			{
				m_instance = std::make_shared<_T>();
			}
			return *m_instance;
		}
	protected:
		mutable stdx::spin_lock m_lock;
		mutable std::shared_ptr<_T> m_instance;
	};

	template<typename _T>
	struct dynamic_singleton_factory_unit:public stdx::singleton_factory_unit<_T>
	{
	public:
		dynamic_singleton_factory_unit()
			:stdx::singleton_factory_unit<_T>()
			,m_maker()
		{}
		
		dynamic_singleton_factory_unit(const dynamic_singleton_factory_unit &other)
			:stdx::singleton_factory_unit<_T>(other)
			,m_maker(other.m_maker)
		{}

		virtual ~dynamic_singleton_factory_unit() = default;

		virtual _T& make() const override
		{
			if (!m_maker)
			{
				throw std::logic_error("you should configurate first");
			}
			std::unique_lock<stdx::spin_lock> lock(m_lock);
			if (!m_instance)
			{
				m_instance = m_maker();
			}
			return *m_instance;
		}

		void config(std::function<std::shared_ptr<_T>()>&& maker)
		{
			m_maker = std::move(maker);
		}

		void config(const std::function<std::shared_ptr<_T>()>& maker)
		{
			m_maker = maker;
		}

		template<typename _Fn, class = typename std::enable_if<stdx::is_callable<_Fn>::value && std::is_same<stdx::function_info<_Fn>::result, std::shared_ptr<_T>>::value>::type>
		void config()
		{
			m_maker = []()
			{
				return _Fn()();
			};
		}
	private:
		std::function<std::shared_ptr<_T>()> m_maker;
	};

	template<template <class> class _Unit,typename ..._Type>
	class _UnitFactory;

	template<template <class> class _Unit>
	class _UnitFactory<_Unit>
	{
		/*do nothing*/
	};

	template<template <class> class _Unit, typename _T>
	class _UnitFactory<_Unit, _T>:protected _Unit<_T>
	{
	public:
		template<typename _T1>
		auto make() const ->decltype(_Unit<_T1>::make())
		{
			const _Unit<_T1>& unit = *this;
			return unit.make();
		}

		template<typename _T1>
		_Unit<_T1>& unit()
		{
			_Unit<_T1>& unit = *this;
			return unit;
		}

		template<typename _T1>
		const _Unit<_T1>& unit() const
		{
			const _Unit<_T1>& unit = *this;
			return unit;
		}
	private:
	};

	template<template <class> class _Unit,typename _T, typename ..._Type>
	class _UnitFactory<_Unit, _T, _Type...>:protected _Unit<_T>,protected stdx::_UnitFactory<_Unit,_Type...>
	{
	public:
		template<typename _T1>
		auto make() const ->decltype(_Unit<_T1>::make())
		{
			const _Unit<_T1>& unit = *this;
			return unit.make();
		}

		template<typename _T1>
		_Unit<_T1>& unit()
		{
			_Unit<_T1>& unit = *this;
			return unit;
		}

		template<typename _T1>
		const _Unit<_T1>& unit() const
		{
			const _Unit<_T1>& unit = *this;
			return unit;
		}
	};

	template<template<class> class _Unit,typename ..._Type>
	class unit_factory
	{
		using tl_t = stdx::type_list<_Type...>;
		using impl_t = std::shared_ptr<stdx::_UnitFactory<_Unit, _Type...>>;
	public:
		unit_factory()
			:m_impl(std::make_shared<stdx::_UnitFactory<_Unit, _Type...>>())
		{}

		unit_factory(const unit_factory& other)
			:m_impl(other.m_impl)
		{}

		virtual ~unit_factory() = default;

		unit_factory& operator=(const unit_factory& other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		template<typename _T1,class = typename std::enable_if<stdx::type_include<_T1,tl_t>::value>::type>
		auto make() const ->decltype(std::declval<_Unit<_T1>>().make())
		{
			return m_impl->make<_T1>();
		}

		template<typename _T1, class = typename std::enable_if<stdx::type_include<_T1, tl_t>::value>::type>
		_Unit<_T1>& unit()
		{
			return m_impl->unit<_T1>();
		}

		template<typename _T1, class = typename std::enable_if<stdx::type_include<_T1, tl_t>::value>::type>
		const _Unit<_T1>& unit() const
		{
			return m_impl->unit<_T1>();
		}

	private:
		impl_t m_impl;
	};

	//默认工厂 使用默认构造函数
	template<typename ..._Type>
	using default_factory = stdx::unit_factory<stdx::default_factory_unit, _Type...>;

	//动态工厂 使用用户提供的方式制造
	template<typename ..._Type>
	using dynamic_factory = stdx::unit_factory<stdx::dynamic_factory_unit, _Type...>;

	//单例工厂 使用默认构造函数
	template<typename ..._Type>
	using singleton_factory = stdx::unit_factory<stdx::singleton_factory_unit, _Type...>;

	//动态单例工厂 使用用户提供的方式制造
	template<typename ..._Type>
	using dynamic_singleton_factory = stdx::unit_factory<stdx::dynamic_singleton_factory_unit, _Type...>;
}