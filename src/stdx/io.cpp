#include <stdx/io.h>

#ifdef LINUX
#define _ThrowLinuxError auto _ERROR_CODE = errno;\
						 throw std::system_error(std::error_code(_ERROR_CODE,std::system_category()),strerror(_ERROR_CODE)); \

void stdx::_EPOLL::add_event(int fd, epoll_event * event_ptr)
{
	if (epoll_ctl(m_handle, EPOLL_CTL_ADD, fd, event_ptr) == -1)
	{
		_ThrowLinuxError
	}
}
void stdx::_EPOLL::del_event(int fd)
{
	if (epoll_ctl(m_handle, EPOLL_CTL_DEL, fd, NULL) == -1)
	{
		_ThrowLinuxError
	}
}
void stdx::_EPOLL::update_event(int fd, epoll_event * event_ptr)
{
	if (epoll_ctl(m_handle, EPOLL_CTL_MOD, fd, event_ptr) == -1)
	{
		_ThrowLinuxError
	}
}
void stdx::_EPOLL::wait(epoll_event * event_ptr, const int & maxevents, const int & timeout) const
{
	if (epoll_wait(m_handle, event_ptr, maxevents, timeout) == -1)
	{
		_ThrowLinuxError
	}
}
void stdx::_Reactor::bind(int fd)
{
	auto iterator = m_map.find(fd);
	if (iterator == std::end(m_map))
	{
		m_map.emplace(fd, std::move(make()));
	}
}

void stdx::_Reactor::unbind(int fd)
{
	auto iterator = m_map.find(fd);
	if (iterator != std::end(m_map))
	{
		m_map.erase(iterator);
	}
}

void stdx::_Reactor::push(int fd, epoll_event & ev)
{
	ev.events |= stdx::epoll_events::once;
	auto iterator = m_map.find(fd);
	if (iterator != std::end(m_map))
	{
		std::lock_guard<stdx::spin_lock> lock(iterator->second.m_lock);
		if (iterator->second.m_queue.empty() && (!iterator->second.m_existed))
		{
			m_poll.add_event(fd, &ev);
			iterator->second.m_existed = true;
		}
		else
		{
			iterator->second.m_queue.push(std::move(ev));
		}
	}
	else
	{
		throw std::invalid_argument("invalid argument: fd");
	}
}

void stdx::_Reactor::loop(int fd)
{
	auto iterator = m_map.find(fd);
	if (iterator != std::end(m_map))
	{
		std::unique_lock<stdx::spin_lock> lock();
		if (!iterator->second.m_queue.empty())
		{
			auto ev = iterator->second.m_queue.front();
			m_poll.update_event(fd, &ev);
			iterator->second.m_queue.pop();
		}
		else
		{
			m_poll.del_event(fd);
			iterator->second.m_existed = false;
		}
	}
}
#endif // LINUX