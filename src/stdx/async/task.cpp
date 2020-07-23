#include <stdx/async/task.h>
#include <chrono>
#include <stdx/datetime.h>

stdx::_TaskFlag::_TaskFlag()
	:m_lock()
	, m_locked(false)
	, m_wait_queue()
	, m_pool(nullptr)
{}

stdx::_TaskFlag::_TaskFlag(stdx::thread_pool& pool)
	:m_lock()
	,m_locked(false)
	,m_wait_queue()
	,m_pool(&pool)
{}

stdx::_TaskFlag::~_TaskFlag()
{
	if (m_locked)
	{
		bool i = !m_wait_queue.empty();
		while (i)
		{
			auto ce = m_wait_queue.front();
			m_wait_queue.pop();
			ce.set_exception(std::make_exception_ptr(std::logic_error("the flag has been free!")));
			ce.run();
			i = !m_wait_queue.empty();
		}
	}
}

stdx::task<void> stdx::_TaskFlag::lock()
{
	if (m_pool)
	{
		stdx::task_completion_event<void> ce(*m_pool);
		_RunOrPush(ce);
		return ce.get_task();
	}
	stdx::task_completion_event<void> ce;
	_RunOrPush(ce);
	return ce.get_task();
}

void stdx::_TaskFlag::unlock() noexcept
{
	std::lock_guard<stdx::spin_lock> guard(m_lock);
	if (!m_wait_queue.empty())
	{
		auto ce = m_wait_queue.front();
		m_wait_queue.pop();
		ce.set_value();
		ce.run();
	}
	else
	{
		m_locked = false;
	}
}

void stdx::_TaskFlag::_RunOrPush(stdx::task_completion_event<void>& ce)
{
	std::unique_lock<stdx::spin_lock> lock(m_lock);
	if (m_locked)
	{
		m_wait_queue.push(ce);
	}
	else
	{
		m_locked = true;
		lock.unlock();
		ce.set_value();
		ce.run_on_this_thread();
	}
}

stdx::task<void> stdx::lazy(uint64_t ms)
{
	stdx::task_completion_event<void> ce;
	stdx::threadpool.lazy_run(ms, [](stdx::task_completion_event<void> ce)
	{
			ce.set_value();
			ce.run_on_this_thread();
	},ce);
	return ce.get_task();
}

extern stdx::task<void> stdx::lazy(thread_pool& pool, uint64_t ms)
{
	stdx::task_completion_event<void> ce(pool);
	pool.lazy_run(ms, [](stdx::task_completion_event<void> ce)
		{
			ce.set_value();
			ce.run_on_this_thread();
		}, ce);
	return ce.get_task();
}

stdx::_RWFlag::_RWFlag()
	:m_lock()
	,m_state(stdx::_RWFlag::lock_state::free)
	,m_write_queue()
	,m_read_queue()
	,m_pool(nullptr)
	,m_read_ref(0)
{}

stdx::_RWFlag::~_RWFlag()
{
	if (m_state != stdx::_RWFlag::lock_state::free)
	{
		bool i = !m_write_queue.empty();
		while (i)
		{
			auto ce = m_write_queue.front();
			m_write_queue.pop();
			ce.set_exception(std::make_exception_ptr(std::logic_error("the flag has been free!")));
			ce.run();
			i = !m_write_queue.empty();
		}
		i = !m_read_queue.empty();
		while (i)
		{
			auto ce = m_read_queue.front();
			m_read_queue.pop();
			ce.set_exception(std::make_exception_ptr(std::logic_error("the flag has been free!")));
			ce.run();
			i = !m_read_queue.empty();
		}
	}
}

stdx::task<void> stdx::_RWFlag::lock_read()
{
	std::unique_lock<stdx::spin_lock> lock(m_lock);
	if (m_state != lock_state::write)
	{
		m_state = lock_state::read;
		m_read_ref += 1;
		lock.unlock();
		return stdx::complete_task();
	}
	if (m_pool)
	{
		stdx::task_completion_event<void> ce(*m_pool);
		m_read_queue.push(ce);
		return ce.get_task();
	}
	stdx::task_completion_event<void> ce;
	m_read_queue.push(ce);
	return ce.get_task();
}

stdx::task<void> stdx::_RWFlag::lock_write()
{
	std::unique_lock<stdx::spin_lock> lock(m_lock);
	if (m_state == lock_state::free)
	{
		m_state = lock_state::write;
		lock.unlock();
		return stdx::complete_task();
	}
	if (m_pool)
	{
		stdx::task_completion_event<void> ce(*m_pool);
		m_write_queue.push(ce);
		return ce.get_task();
	}
	stdx::task_completion_event<void> ce;
	m_write_queue.push(ce);
	return ce.get_task();
}

void stdx::_RWFlag::unlock() noexcept
{
	std::unique_lock<stdx::spin_lock> lock(m_lock);
	if (m_state == lock_state::read)
	{
		m_read_ref -= 1;
		if (m_read_ref == 0)
		{
			if (!m_write_queue.empty())
			{
				auto ce = m_write_queue.front();
				m_write_queue.pop();
				m_state = lock_state::write;
				lock.unlock();
				ce.set_value();
				ce.run();
				return;
			}
			else
			{
				m_state = lock_state::free;
				return;
			}
		}
	}
	else if (m_state == lock_state::write)
	{
		if (!m_write_queue.empty())
		{
			auto ce = m_write_queue.front();
			m_write_queue.pop();
			m_state = lock_state::write;
			lock.unlock();
			ce.set_value();
			ce.run();
			return;
		}
		else if(!m_read_queue.empty())
		{
			m_state = lock_state::read;
			m_read_ref += m_read_queue.size();
			std::queue<stdx::task_completion_event<void>> queue;
			queue.swap(m_read_queue);
			lock.unlock();
			bool i = !queue.empty();
			while (i)
			{
				auto ce = queue.front();
				queue.pop();
				ce.set_value();
				ce.run();
				i = !queue.empty();
			}
		}
		else
		{
			m_state = lock_state::free;
			return;
		}
	}
}

stdx::_RWFlag::_RWFlag(stdx::thread_pool & pool)
	:m_lock()
	, m_state(stdx::_RWFlag::lock_state::free)
	, m_write_queue()
	, m_read_queue()
	, m_pool(&pool)
	,m_read_ref(0)
{}
