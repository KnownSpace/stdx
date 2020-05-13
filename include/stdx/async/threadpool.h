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
#define STDX_LAZY_MAX_TIME 64
#endif
namespace stdx
{
	interface_class basic_threadpool
	{
		virtual ~basic_threadpool() = default;

		virtual void run(std::function<void()> &&task) = 0;

		virtual void join_as_worker() = 0;
	};

	//线程池
	class _Threadpool:public stdx::basic_threadpool
	{
		using runable = std::function<void()>;
	public:
		//构造函数
		_Threadpool() noexcept;

		//析构函数
		~_Threadpool() noexcept;

		//删除复制构造函数
		_Threadpool(const _Threadpool&) = delete;

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
		void init_threads() noexcept;
	};
	//线程池静态类
	class threadpool
	{
	public:
		threadpool() = default;

		~threadpool() = default;
		using impl_t = stdx::_Threadpool;
		//执行任务
		template<typename _Fn,typename ..._Args>
		static void run(_Fn &&fn,_Args &&...args) noexcept
		{
			m_impl.run(std::bind(fn,args...));
		}

		static void join_as_worker()
		{
			m_impl.join_as_worker();
		}

		template<typename _Fn, typename ..._Args,class = typename std::enable_if<stdx::is_callable<_Fn>::value>::type>
		static void loop_run(stdx::cancel_token token,_Fn&& fn, _Args &&...args)
		{
			std::function<void()> call = std::bind(fn, args...);
			stdx::threadpool::loop_do(token,call);
		}

		template<typename _Fn,typename ..._Args, class = typename std::enable_if<stdx::is_callable<_Fn>::value>::type>
		static void lazy_run(uint64_t lazy_ms, _Fn &&fn,_Args &&...args)
		{
			std::function<void()> call = std::bind(fn, args...);
			stdx::threadpool::lazy_do(lazy_ms,call,0);
		}

		template<typename _Fn, typename ..._Args, class = typename std::enable_if<stdx::is_callable<_Fn>::value>::type>
		static void lazy_loop_run(stdx::cancel_token token,uint64_t lazy_ms,_Fn &&fn,_Args &&...args)
		{
			std::function<void()> call = std::bind(fn, args...);
			stdx::threadpool::lazy_loop_do(token, lazy_ms, call);
		}
	private:
		static void loop_do(stdx::cancel_token token, std::function<void()> call);

		static void lazy_do(uint64_t lazy_ms, std::function<void()> call, uint64_t target_tick);

		static void lazy_loop_do(stdx::cancel_token token,uint64_t lazy_ms,std::function<void()> call);
		static impl_t m_impl;
	};
}