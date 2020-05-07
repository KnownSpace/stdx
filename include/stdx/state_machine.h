#pragma once
#include <stdx/env.h>
#include <memory>

namespace stdx
{
	template<typename _State>
	class state_machine;

	template<typename _State>
	interface_class basic_state_machine
	{
		using state_t = _State;

		virtual stdx::state_machine<_State> move_next() = 0;

		virtual _State &state() = 0;

		virtual const _State& state() const = 0;

		virtual ~basic_state_machine() = default;

		virtual bool is_end() const = 0;

		virtual stdx::state_machine<_State> reset() = 0;

		virtual bool movable() const = 0;
	};

	template<typename _State>
	class state_machine
	{
		using self_t = stdx::state_machine<_State>;
		using impl_t = std::shared_ptr<stdx::basic_state_machine<_State>>;
	public:
		explicit state_machine()
			:m_impl(nullptr)
		{}

		state_machine(const impl_t &impl)
			:m_impl(impl)
		{}

		state_machine(const self_t &other)
			:m_impl(other.m_impl)
		{}

		state_machine(self_t &&other) noexcept
			:m_impl(other.m_impl)
		{}

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

		self_t move_next()
		{
			return m_impl->move_next();
		}

		bool is_end() const
		{
			return m_impl->is_end();
		}

		_State& state()
		{
			return m_impl->state();
		}

		const _State& state() const
		{
			return m_impl->state();
		}

		stdx::state_machine<_State> reset()
		{
			return m_impl->reset();
		}

		~state_machine() = default;

		bool operator==(const self_t& other)
		{
			return m_impl == other.m_impl;
		}

		operator bool() const
		{
			return (bool)m_impl;
		}

		bool movable() const
		{
			return m_impl->movable();
		}
	private:
		impl_t m_impl;
	};

	template<typename _Impl,typename ..._Args,typename _State = typename _Impl::state_t>
	inline stdx::state_machine<_State> make_state_machine(_Args &&...args)
	{
		std::shared_ptr<stdx::basic_state_machine<_State>> p = std::make_shared<_Impl>(args...);
		return stdx::state_machine<_State>(p);
	}
}