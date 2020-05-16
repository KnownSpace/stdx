#pragma once
#include <stdx/env.h>
#include <memory>
#include <functional>

namespace stdx
{
	template<typename _Context,typename _KeyType>
	interface_class basic_poller
	{
		using context_t = _Context;

		using key_t = _KeyType;

		virtual ~basic_poller() = default;

		virtual _Context* get() = 0;

		virtual _Context* get(uint32_t timeout_ms) = 0;

		virtual void post(_Context* context)
		{}

		virtual void bind(const _KeyType &object)
		{}

		virtual void unbind(const _KeyType& object)
		{}

		virtual void unbind(const _KeyType& object, std::function<void(_KeyType)> deleter)
		{}
	};

	template<typename _Context,typename _KeyType>
	class poller
	{
		using impl_t = std::shared_ptr<stdx::basic_poller<_Context,_KeyType>>;
		using self_t = stdx::poller<_Context,_KeyType>;
	public:
		poller()
			:m_impl(nullptr)
		{}

		poller(const impl_t &impl)
			:m_impl(impl)
		{}

		poller(const self_t &other)
			:m_impl(other.m_impl)
		{}

		poller(self_t&& other) noexcept
			:m_impl(std::move(other.m_impl))
		{}

		~poller() = default;

		self_t& operator=(const self_t& other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		self_t& operator=(self_t&& other) noexcept
		{
			m_impl = std::move(other.m_impl);
			return *this;
		}

		operator bool() const
		{
			return (bool)m_impl;
		}

		bool operator==(const self_t& other) const
		{
			return m_impl == other.m_impl;
		}

		void bind(const _KeyType& object)
		{
			m_impl->bind(object);
		}

		void unbind(const _KeyType& object)
		{
			m_impl->unbind(object);
		}

		void unbind(const _KeyType& object, std::function<void(_KeyType)> deleter)
		{
			m_impl->unbind(object, deleter);
		}

		_Context* get()
		{
			return m_impl->get();
		}

		_Context* get(uint32_t timeout_ms)
		{
			return m_impl->get(timeout_ms);
		}

		void post(_Context* context)
		{
			return m_impl->post(context);
		}
	private:
		impl_t m_impl;
	};

	template<typename _Impl,typename ..._Args,typename _Context = typename _Impl::context_t,typename _KeyType = typename _Impl::key_t>
	inline stdx::poller<_Context,_KeyType> make_poller(_Args &&...args)
	{
		std::shared_ptr<stdx::basic_poller<_Context,_KeyType>> impl = std::make_shared<_Impl>(args...);
		return stdx::poller<_Context,_KeyType>(impl);
	}
}