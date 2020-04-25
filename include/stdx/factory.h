#pragma once
#include <functional>
#include <memory>
#include <stdx/async/spin_lock.h>
#include <mutex>
#include <stdx/function.h>
#include <stdx/traits/type_list.h>

namespace stdx
{
	template<typename _T,typename ..._Args>
	struct basic_factory_unit
	{
		virtual ~basic_factory_unit() = default;
		virtual _T produce(_Args...) const = 0;
	};

	template<typename _T,typename ..._Args>
	class factory_unit
	{
		using self_t = stdx::factory_unit<_T, _Args...>;
	public:
		using impl_t = std::shared_ptr<stdx::basic_factory_unit<_T, _Args...>>;

		using product_t = _T;

		using args_tl = stdx::type_list<_Args...>;

		explicit factory_unit()
			:m_impl(nullptr)
		{}

		factory_unit(const impl_t &impl)
			:m_impl(impl)
		{}

		factory_unit(const self_t& other)
			:m_impl(other.m_impl)
		{}

		factory_unit(self_t&& other) noexcept
			:m_impl(other.m_impl)
		{}

		virtual ~factory_unit() =default;

		self_t& operator=(const self_t& other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		self_t& operator=(self_t&& other) noexcept
		{
			m_impl = other.m_impl;
			return *this;
		}

		bool operator==(const self_t& other) const
		{
			return m_impl == other.m_impl;
		}

		operator bool() const
		{
			return (bool)m_impl;
		}

		self_t& config(const impl_t& impl)
		{
			m_impl = impl;
		}

		_T produce(_Args ...args)
		{
			return m_impl->produce(args...);
		}

		bool check() const
		{
			return (bool)m_impl;
		}
	private:
		impl_t m_impl;
	};

	template<typename _Impl>
	struct _FactoryUnit
	{
		//nothing
	};

	template<typename _Class, typename _Result, typename ..._Args>
	struct _FactoryUnit<_Result(_Class::*)(_Args...) const>
	{
		using type = stdx::factory_unit<_Result, _Args...>;
	};


	template<typename _Impl,typename ..._ImplArgs,class _Factory = typename _FactoryUnit<decltype(&_Impl::produce)>::type>
	inline _Factory make_factory_unit(_ImplArgs ...args)
	{
		_Factory factory(std::make_shared<_Impl>(args...));
		return factory;
	}

	template<typename _Factories,typename _Unit,typename _Base,typename _CurrentUnit>
	struct _ConfigFactoriesUnit
	{
		static void to_do(_Factories& factories, const _Unit& unit)
		{
			_Base* base = (_Base*)&factories;
			base->config_unit(unit);
		}
	};

	template<typename _Factories, typename _Unit, typename _Base>
	struct _ConfigFactoriesUnit<_Factories,_Unit,_Base,_Unit>
	{
		static void to_do(_Factories& factories, const _Unit& unit)
		{
			_Unit* _unit = (_Unit*)&factories;
			*_unit = unit;
		}
	};

	template<typename _Unit,typename ..._Units>
	struct factories :protected stdx::factories<_Units...>,protected _Unit
	{
		using unit_t = _Unit;
		using base_t = stdx::factories<_Units...>;
		using tl_t = stdx::type_list<_Unit, _Units...>;
		using self_t = factories<_Unit, _Units...>;
	public:
		factories() = default;

		factories(const self_t& other)
			:base_t(other)
			,unit_t(other)
		{}

		factories(self_t&& other) noexcept
			:base_t(other)
			,unit_t(other)
		{}

		self_t& operator=(const self_t& other)
		{
			base_t::operator=(other);
			unit_t::operator=(other);
			return *this;
		}

		self_t& operator=(self_t&& other) noexcept
		{
			base_t::operator=(other);
			unit_t::operator=(other);
			return *this;
		}

		virtual ~factories() = default;

		template<typename _T,class = typename std::enable_if<stdx::type_include<_T,tl_t>::value,void>::type>
		self_t &config_unit(const _T &unit)
		{
			stdx::_ConfigFactoriesUnit<self_t, _T, base_t, unit_t>::to_do(*this, unit);
			return *this;
		}

		template<typename _T,typename ..._Args,typename _U = stdx::factory_unit<_T,_Args...>,class = typename std::enable_if<stdx::type_include<_U, tl_t>::value, void>::type>
		_U unit()
		{
			return *this;
		}

		template <typename _T, typename ..._Args,typename _U = stdx::factory_unit<_T, _Args...>, class = typename std::enable_if<stdx::type_include<_U, tl_t>::value, void>::type>
		_T produce(_Args ...args)
		{
			_U& unit = *this;
			return unit.produce(args...);
		}
	};

	template<typename _Unit>
	struct factories<_Unit>:protected _Unit
	{

		using unit_t = _Unit;
		using self_t = factories<_Unit>;
	public:
		factories() = default;

		factories(const self_t & other)
			:unit_t(other)
		{}

		factories(self_t&& other) noexcept
			:unit_t(other)
		{}

		self_t& operator=(const self_t& other)
		{
			unit_t::operator=(other);
			return *this;
		}

		self_t& operator=(self_t&& other) noexcept
		{
			unit_t::operator=(other);
			return *this;
		}

		virtual ~factories() = default;

		template <typename _T, class = typename std::enable_if<std::is_same<_T,unit_t>::value>::type>
		self_t& config_unit(const _T& unit)
		{
			unit_t::operator=(unit);
			return *this;
		}

		template<typename _T, typename ..._Args,typename _U = stdx::factory_unit<_T,_Args...>,class = typename  std::enable_if<std::is_same<_U, unit_t>::value>::type>
		_T produce(_Args ...args)
		{
			unit_t& unit = *this;
			return unit.produce(args...);
		}

		template<typename _T,typename ..._Args,typename _U = stdx::factory_unit<_T,_Args...>,class = typename std::enable_if<std::is_same<_U,unit_t>::value>::type>
		_U& unit()
		{
			return *this;
		}
	};
}