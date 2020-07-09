#pragma once
#include <stdx/env.h>
#include <memory>
#include <functional>
#include <vector>
#include <atomic>
#include <stdx/async/thread_local_storer.h>

namespace stdx
{
	template<typename _Context,typename _KeyType>
	INTERFACE_CLASS basic_poller
	{
		using context_t = _Context;

		using key_t = _KeyType;

		INTERFACE_CLASS_HELPER(basic_poller);

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

		virtual _Context* get_at(size_t index)
		{
			NO_USED(index);
			return get();
		}

		virtual _Context* get_at(size_t index, uint32_t timeout_ms)
		{
			NO_USED(index);
			return get(timeout_ms);
		}

		virtual void notice() = 0;
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

		_Context* get_at(size_t index)
		{
			return m_impl->get_at(index);
		}

		_Context* get_at(size_t index, uint32_t timeout_ms)
		{
			return m_impl->get_at(index, timeout_ms);
		}

		void notice()
		{
			return m_impl->notice();
		}
	private:
		impl_t m_impl;
	};

	template<typename _Impl, typename ..._Args, typename _Context = typename _Impl::context_t, typename _KeyType = typename _Impl::key_t>
	inline stdx::poller<_Context, _KeyType> make_poller(_Args&&...args)
	{
		std::shared_ptr<stdx::basic_poller<_Context, _KeyType>> impl = std::make_shared<_Impl>(args...);
		return stdx::poller<_Context, _KeyType>(impl);
	}

	template<typename _Impl>
	class basic_multipoller:public stdx::basic_poller<typename _Impl::context_t,typename _Impl::key_t>
	{
		using base_t = stdx::basic_poller<typename _Impl::context_t, typename _Impl::key_t>;
	public:
		using context_t = typename base_t::context_t;
		using key_t = typename base_t::key_t;
		using dispath_t = std::function<size_t(const key_t &,size_t)>;
		using get_key_t = std::function<key_t(context_t *)>;
		using poller_t = stdx::poller<context_t, key_t>;

		template<typename ..._Args>
		basic_multipoller(size_t num_of_poller,dispath_t dispath,get_key_t key_getter,_Args &&...args)
			:base_t()
			,m_dispath(dispath)
			,m_key_getter(key_getter)
			,m_pollers()
			,m_pos(num_of_poller)
		{
			m_pollers.reserve(num_of_poller);
			for (size_t i = 0; i < num_of_poller; ++i)
			{
				m_pollers.push_back(stdx::make_poller<_Impl>(args...));
			}
		}

		~basic_multipoller() = default;

		virtual void bind(const key_t& object) override
		{
			poller_t& poller = _GetPollerByKey(object);
			poller.bind(object);
		}

		virtual void unbind(const key_t& object) override
		{
			poller_t& poller = _GetPollerByKey(object);
			poller.unbind(object);
		}

		virtual void unbind(const key_t& object, std::function<void(key_t)> deleter) override
		{
			poller_t& poller = _GetPollerByKey(object);
			poller.unbind(object,deleter);
		}

		virtual context_t* get() override
		{
			poller_t& poller = m_pollers.at(0);
			return poller.get();
		}

		virtual context_t* get(uint32_t timeout_ms) override
		{
			poller_t& poller = m_pollers.at(0);
			return poller.get(timeout_ms);
		}

		virtual void post(context_t* context) override
		{
			poller_t& poller = _GetPoller(context);
			poller.post(context);
		}

		virtual context_t* get_at(size_t index)
		{
			return _GetPoller(index).get();
		}

		virtual context_t* get_at(size_t index, uint32_t timeout_ms)
		{
			return _GetPoller(index).get(timeout_ms);
		}

		virtual void notice()
		{
			for (auto begin = m_pollers.begin(), end = m_pollers.end(); begin != end; begin++)
			{
				begin->notice();
			}
		}
	protected:
		dispath_t m_dispath;
		get_key_t m_key_getter;
		std::vector<poller_t> m_pollers;
		std::atomic_size_t m_pos;

		poller_t& _GetPoller(size_t index)
		{
			return m_pollers.at(index);
		}

		poller_t& _GetPollerByKey(const key_t& key)
		{
			size_t index = m_dispath(key, m_pollers.size());
			poller_t& poller = _GetPoller(index);
			return poller;
		}

		poller_t& _GetPoller(context_t* context)
		{
			key_t &&key = m_key_getter(context);
			return _GetPollerByKey(key);
		}
	};

	template<typename _Impl, typename ..._Args, typename _Context = typename _Impl::context_t, typename _KeyType = typename _Impl::key_t>
	inline stdx::poller<_Context, _KeyType> make_multipoller(size_t num_of_poller,typename stdx::basic_multipoller<_Impl>::dispath_t dispath, typename stdx::basic_multipoller<_Impl>::get_key_t getter,_Args &&...args)
	{
		std::shared_ptr<stdx::basic_poller<_Context, _KeyType>> impl = std::make_shared<stdx::basic_multipoller<_Impl>>(num_of_poller,dispath,getter,args...);
		return stdx::poller<_Context, _KeyType>(impl);
	}
}