#pragma once
#include <stdx/env.h>
#include <stdx/async/spin_lock.h>
#include <thread>
#include <queue>
#include <stdx/async/semaphore.h>
#include <stdx/function.h>
#include <memory>

#define cpu_cores() std::thread::hardware_concurrency()
namespace stdx
{
	extern uint32_t suggested_threads_number();

	//线程池
	class _Threadpool
	{
		using runable_ptr = std::shared_ptr<stdx::basic_runable<void>>;
	public:
		//构造函数
		_Threadpool() noexcept;

		//析构函数
		~_Threadpool() noexcept;

		//删除复制构造函数
		_Threadpool(const _Threadpool&) = delete;

		//执行任务
		template<typename _Fn, typename ..._Args>
		void run(_Fn &&task, _Args &&...args) noexcept
		{
			m_count_lock.lock();
			if (*m_free_count ==0)
			{
				*m_free_count+=1;
				add_thread();
			}
			m_count_lock.unlock();
			m_task_queue->emplace(stdx::make_runable<void>(std::move(task), args...));
			m_barrier.notify();
		}

	private:
		std::shared_ptr<uint32_t> m_free_count;
		stdx::spin_lock m_count_lock;
		std::shared_ptr<bool> m_alive;
		std::shared_ptr<std::queue<runable_ptr>> m_task_queue;
		stdx::semaphore m_barrier;
		stdx::spin_lock m_lock;

		//添加线程
		void add_thread() noexcept;
		
		//初始化线程池
		void init_threads() noexcept;
	};
	using threadpool_ptr = std::shared_ptr<stdx::_Threadpool>;
	//线程池静态类
	class threadpool
	{
	public:
		~threadpool() = default;
		using impl_t = std::shared_ptr<stdx::_Threadpool>;
		//执行任务
		template<typename _Fn,typename ..._Args>
		static void run(_Fn &&fn,_Args &&...args) noexcept
		{
			m_impl->run(std::move(fn),args...);
		}
	private:
		threadpool() = default;
		const static impl_t m_impl;
	};
}