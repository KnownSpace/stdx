#pragma once
#include <stdx/env.h>
#include <stdx/async/spin_lock.h>
#include <thread>
#include <queue>
#include <stdx/async/semaphore.h>
#include <stdx/function.h>
#include <stdx/async/cancel_token.h>
#include <stdx/async/worker.h>
#include <stdexcept>
#include <stdx/poller.h>

#define GET_CPU_CORES() std::thread::hardware_concurrency()

#ifndef STDX_LAZY_MAX_TIME
#define STDX_LAZY_MAX_TIME 16
#endif

namespace stdx
{
	INTERFACE_CLASS basic_thread_pool
	{
		INTERFACE_CLASS_HELPER(basic_thread_pool);

		virtual void run(std::function<void()> &&task) = 0;

		virtual void join_as_worker() = 0;

#ifdef WIN32
		using key_t = HANDLE;
#else
		using key_t = int;
#endif

		virtual stdx::poller<stdx::stand_context, key_t> get_poller()
		{
			throw std::logic_error("Unsupported operation");
		}
	};

	class _McmpThreadPool:public stdx::basic_thread_pool
	{
		using runable = std::function<void()>;
		using base_t = stdx::basic_thread_pool;
	public:
		_McmpThreadPool(uint32_t num_threads) noexcept;

		~_McmpThreadPool() noexcept;

		_McmpThreadPool(const _McmpThreadPool&) = delete;

		void run(std::function<void()> &&task)
		{
			std::unique_lock<std::mutex> _lock(*m_mutex);
			m_task_queue->push(std::move(task));
			m_cv->notify_one();
		}

		void join_as_worker();

	private:
		std::shared_ptr<bool> m_alive;
		std::shared_ptr<std::queue<runable>> m_task_queue;
		std::shared_ptr<std::condition_variable> m_cv;
		std::shared_ptr<std::mutex> m_mutex;

		void add_thread() noexcept;
		
		void init_threads(uint32_t num_threads) noexcept;
	};

	class _RoundRobinThreadPool:public stdx::basic_thread_pool
	{
		using base_t = stdx::basic_thread_pool;
	public:
		_RoundRobinThreadPool(uint32_t num_threads);

		~_RoundRobinThreadPool();

		virtual void run(std::function<void()> &&task) override;

		virtual void join_as_worker() override;

	private:
		stdx::spin_lock m_lock;
		std::atomic_size_t m_index;
		std::shared_ptr<std::atomic_bool> m_enable;
		std::vector<stdx::worker_thread*> m_workers;
		std::vector<std::shared_ptr<stdx::worker_context>> m_joiners;
		size_t m_size;

		size_t _GetIndex();
	};

	class _IoThreadPool:public stdx::basic_thread_pool
	{
		using poller_t = stdx::poller<stdx::stand_context,key_t>;
	public:
		_IoThreadPool(uint32_t num_threads);

		~_IoThreadPool();

		virtual void run(std::function<void()>&& task) override;

		virtual void join_as_worker() override;

		virtual stdx::poller<stdx::stand_context, key_t> get_poller()
		{
			return m_poller;
		}
	private:
		void _Join();

		void _Run(std::function<void()> task);

#ifndef WIN32
		bool _HandleTasks();
#endif

		poller_t m_poller;
		stdx::cancel_token m_token;
		std::vector<std::shared_ptr<std::thread>> m_threads;
#ifndef WIN32
		stdx::spin_lock m_lock;
		std::list<std::function<void()>> m_tasks;
#endif
	};

	class thread_pool
	{

	protected:
		using impl_t = std::shared_ptr<stdx::basic_thread_pool>;

	private:
		using self_t = stdx::thread_pool;

	public:
		thread_pool()
			:m_impl(nullptr)
		{}

		thread_pool(const impl_t &impl)
			:m_impl(impl)
		{}

		thread_pool(const self_t &other)
			:m_impl(other.m_impl)
		{}

		thread_pool(self_t&& other) noexcept
			:m_impl(std::move(other.m_impl))
		{}

		virtual ~thread_pool() = default;

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

		//执行任务
		template<typename _Fn,typename ..._Args>
		void run(_Fn &&fn,_Args &&...args) noexcept
		{
			m_impl->run(std::move(std::bind(fn, args...)));
		}

		void join_as_worker()
		{
			m_impl->join_as_worker();
		}

		template<typename _Fn, typename ..._Args,class = typename std::enable_if<stdx::is_callable<_Fn>::value>::type>
		void loop_run(stdx::cancel_token token,_Fn&& fn, _Args &&...args)
		{
			std::function<void()> call = std::bind(fn, args...);
			loop_do(token,call);
		}

		template<typename _Fn,typename ..._Args, class = typename std::enable_if<stdx::is_callable<_Fn>::value>::type>
		void lazy_run(uint64_t lazy_ms, _Fn &&fn,_Args &&...args)
		{
			std::function<void()> call = std::bind(fn, args...);
			lazy_do(lazy_ms,call,0);
		}

		template<typename _Fn, typename ..._Args, class = typename std::enable_if<stdx::is_callable<_Fn>::value>::type>
		void lazy_loop_run(stdx::cancel_token token,uint64_t lazy_ms,_Fn &&fn,_Args &&...args)
		{
			std::function<void()> call = std::bind(fn, args...);
			lazy_loop_do(token, lazy_ms, call);
		}

		operator bool() const
		{
			return (bool)m_impl;
		}

		bool operator==(const self_t& other) const
		{
			return m_impl == other.m_impl;
		}

		template<typename _Fn, typename ..._Args, class = typename std::enable_if<stdx::is_callable<_Fn>::value>::type>
		void long_loop(stdx::cancel_token token, _Fn&& fn, _Args&&...args)
		{
			std::function<void()> call = std::bind(fn, args...);
			run([](std::function<void()> call,stdx::cancel_token token) 
			{
					while (!token.is_cancel())
					{
						call();
					}
			},call,token);
		}
	private:
		void loop_do(stdx::cancel_token token, std::function<void()> call);

		void lazy_do(uint64_t lazy_ms, std::function<void()> call, uint64_t target_tick);

		void lazy_loop_do(stdx::cancel_token token,uint64_t lazy_ms,std::function<void()> call);

	protected:

		impl_t m_impl;
	};

	class io_thread_pool:public stdx::thread_pool
	{
		using base_t = stdx::thread_pool;

		using self_t = stdx::io_thread_pool;
	public:
		io_thread_pool()
			:base_t()
		{}

		io_thread_pool(impl_t &impl)
			:base_t(impl)
		{}

		io_thread_pool(const self_t &other)
			:base_t(other)
		{}

		io_thread_pool(self_t &&other) noexcept
			:base_t(std::move(other))
		{}

		virtual ~io_thread_pool() = default;

		self_t &operator=(const self_t &other)
		{
			base_t::operator=(other);
			return *this;
		}

		self_t& operator=(self_t&& other) noexcept
		{
			base_t::operator=(std::move(other));
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

		stdx::poller<stdx::stand_context, stdx::basic_thread_pool::key_t> get_poller()
		{
			return m_impl->get_poller();
		}
	private:

	};

	template<typename _Impl,typename ..._Args>
	inline stdx::thread_pool make_thread_pool(_Args &&...args)
	{
		std::shared_ptr<stdx::basic_thread_pool> impl = std::make_shared<_Impl>(args...);
		return stdx::thread_pool(impl);
	}

	extern stdx::thread_pool make_mcmp_thread_pool(uint32_t size);

	extern stdx::thread_pool make_round_robin_thread_pool(uint32_t size);

	extern stdx::io_thread_pool make_io_thread_pool(uint32_t size);

	extern stdx::io_thread_pool threadpool;
}