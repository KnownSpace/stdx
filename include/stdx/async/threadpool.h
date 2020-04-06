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

		static uint32_t expand_number_of_threads();


		void expand(uint32_t number_of_threads);

		//执行任务
		template<typename _Fn, typename ..._Args>
		void run(_Fn &&task, _Args &&...args) noexcept
		{
#ifdef DEBUG
			::printf("[Threadpool]正在投递任务\n");
#endif // DEBUG
			std::unique_lock<stdx::spin_lock> lock(m_queue_lock);
			m_task_queue->push(stdx::make_runable<void>(std::move(task), args...));
			lock.unlock();
			m_barrier.notify();
			if (((*m_free_count) == 0) || (m_task_queue->size() > (*m_free_count)))
			{
				std::unique_lock<stdx::spin_lock> _lock(m_count_lock);
				if (((*m_free_count) == 0) || (m_task_queue->size() > (*m_free_count)))
				{
#ifdef DEBUG
					::printf("[Threadpool]空闲线程数(%u)不足,创建新线程\n", *m_free_count);
#endif // DEBUG
					uint32_t num = expand_number_of_threads();
					*m_free_count = *m_free_count + num;
					_lock.unlock();
					expand(num);
					return;
				}
			}
		}

		void join_handle();

	private:
		std::shared_ptr<uint32_t> m_free_count;
		stdx::spin_lock m_count_lock;
		stdx::spin_lock m_queue_lock;
		std::shared_ptr<bool> m_alive;
		std::shared_ptr<std::queue<runable_ptr>> m_task_queue;
		stdx::semaphore m_barrier;

		//添加线程
		void add_thread() noexcept;
		
		//初始化线程池
		void init_threads() noexcept;
	};
	//线程池静态类
	class threadpool
	{
	public:
		~threadpool() = default;
		using impl_t = stdx::_Threadpool;
		//执行任务
		template<typename _Fn,typename ..._Args>
		static void run(_Fn &&fn,_Args &&...args) noexcept
		{
			m_impl.run(std::move(fn),args...);
		}
		static void join_handle()
		{
			m_impl.join_handle();
		}
	private:
		threadpool() = default;
		static impl_t m_impl;
	};
}