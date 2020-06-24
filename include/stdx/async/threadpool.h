#pragma once
#include <stdx/env.h>
#include <stdx/async/spin_lock.h>
#include <thread>
#include <queue>
#include <stdx/async/semaphore.h>
#include <stdx/function.h>
#include <stdx/async/cancel_token.h>
#include <stdx/async/worker.h>

#define cpu_cores() std::thread::hardware_concurrency()

#ifndef STDX_LAZY_MAX_TIME
#define STDX_LAZY_MAX_TIME 56
#endif

namespace stdx
{
	interface_class basic_thread_pool
	{
		interface_class_helper(basic_thread_pool);

		virtual void run(std::function<void()> &&task) = 0;

		virtual void join_as_worker() = 0;
	};

	//线程池
	class _FixedSizeThreadPool:public stdx::basic_thread_pool
	{
		using runable = std::function<void()>;
		using base_t = stdx::basic_thread_pool;
	public:
		//构造函数
		_FixedSizeThreadPool(uint32_t num_threads) noexcept;

		//析构函数
		~_FixedSizeThreadPool() noexcept;

		//删除复制构造函数
		_FixedSizeThreadPool(const _FixedSizeThreadPool&) = delete;

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
		//添加线程
		void add_thread() noexcept;
		
		//初始化线程池
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

	//线程池静态类
	class thread_pool
	{
		using impl_t = std::shared_ptr<stdx::basic_thread_pool>;

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

		~thread_pool() = default;

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

		impl_t m_impl;
	};

	template<typename _Impl,typename ..._Args>
	inline stdx::thread_pool make_thread_pool(_Args &&...args)
	{
		std::shared_ptr<stdx::basic_thread_pool> impl = std::make_shared<_Impl>(args...);
		return stdx::thread_pool(impl);
	}

	extern stdx::thread_pool make_fixed_size_thread_pool(uint32_t size);

	extern stdx::thread_pool make_round_robin_thread_pool(uint32_t size);

	extern stdx::thread_pool threadpool;
}