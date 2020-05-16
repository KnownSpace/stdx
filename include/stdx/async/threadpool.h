#pragma once
#include <stdx/env.h>
#include <stdx/async/spin_lock.h>
#include <thread>
#include <queue>
#include <stdx/async/semaphore.h>
#include <stdx/function.h>
#include <stdx/async/cancel_token.h>

#define cpu_cores() std::thread::hardware_concurrency()

#ifndef STDX_LAZY_MAX_TIME
#define STDX_LAZY_MAX_TIME 56
#endif
namespace stdx
{
	interface_class basic_thread_pool
	{
		virtual ~basic_thread_pool() = default;

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
			m_impl->run(std::bind(fn,args...));
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

	extern stdx::thread_pool threadpool;
}