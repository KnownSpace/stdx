#pragma once
#include <stdx/async/spin_lock.h>
#include <list>
#include <functional>
#include <condition_variable>
#include <thread>
#include <stdx/env.h>

namespace stdx
{
	struct worker_context
	{
	private:
		using task_t = std::function<void()>;
		using lock_t = stdx::spin_lock;
		using self_t = stdx::worker_context;

		lock_t m_lock;
		std::list<task_t> m_tasks;
		std::condition_variable_any m_cond;
		bool m_sleep;
	public:
		worker_context();

		~worker_context() = default;

		void push(task_t&& task);

		task_t pop();
	};

	struct worker_thread
	{
		using context_t = stdx::worker_context;
		using context_ptr_t = std::shared_ptr<context_t>;
		using task_t = std::function<void()>;
	public:
		worker_thread();

		delete_copy(worker_thread);

		~worker_thread();

		context_ptr_t get_context()
		{
			return m_context;
		}

		void push(task_t&& task)
		{
			m_context->push(std::move(task));
		}

	private:
		context_ptr_t m_context;
		std::atomic_bool m_enable;
		std::thread m_thread;

		void _Run()
		{
			while (m_enable)
			{
				task_t &&task =  m_context->pop();
				task();
			}
		}
	};
}