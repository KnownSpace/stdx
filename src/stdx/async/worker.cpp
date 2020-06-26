#include <stdx/async/worker.h>
#include <mutex>

stdx::worker_context::worker_context()
	:m_lock()
	,m_tasks()
	,m_cond()
	,m_sleep(true)
{}

void stdx::worker_context::push(task_t&& task)
{
	bool sleep = false;
	{
		std::unique_lock<lock_t> lock(m_lock);
		m_tasks.push_back(std::move(task));
		std::swap(m_sleep, sleep);
	}
	if (sleep)
	{
		m_cond.notify_one();
	}
}

typename stdx::worker_context::task_t stdx::worker_context::pop()
{
	std::unique_lock<lock_t> lock(m_lock);
	while (m_tasks.empty())
	{
		m_sleep = true;
		m_cond.wait(lock);
	}
	m_sleep = false;
	task_t task = m_tasks.front();
	m_tasks.pop_front();
	return task;
}

stdx::worker_thread::worker_thread()
	:m_context(std::make_shared<context_t>())
	,m_enable(true)
	,m_thread(std::bind(&stdx::worker_thread::_Run,this))
{}

stdx::worker_thread::~worker_thread()
{
	m_enable = false;
	push([]() {});
	if (m_thread.joinable())
	{
		m_thread.join();
	}
}