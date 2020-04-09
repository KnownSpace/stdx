#include <stdx/io.h>
#include <iostream>
#include <stdx/datetime.h>
#include <stdx/net/socket.h>

#ifdef LINUX
#define _ThrowLinuxError auto _ERROR_CODE = errno;\
							throw std::system_error(std::error_code(_ERROR_CODE,std::system_category()),strerror(_ERROR_CODE)); 

int stdx::io_setup(unsigned nr_events, aio_context_t* ctx_idp)
{
	return syscall(SYS_io_setup, nr_events, ctx_idp);
}

int stdx::io_destroy(aio_context_t ctx_id)
{
	return syscall(SYS_io_destroy, ctx_id);
}

int stdx::io_submit(aio_context_t ctx_id, long nr, struct iocb** iocbpp)
{
	return syscall(SYS_io_submit, ctx_id, nr, iocbpp);
}

int stdx::io_getevents(aio_context_t ctx_id, long min_nr, long nr, struct io_event* events, struct timespec* timeout)
{
	return syscall(SYS_io_getevents, ctx_id, min_nr, nr, events, timeout);
}

int stdx::io_cancel(aio_context_t ctx_id, struct iocb* iocb, struct io_event* result)
{
	return syscall(SYS_io_cancel, ctx_id, iocb, result);
}

void stdx::aio_read(aio_context_t context, int fd, char* buf, size_t size, int64_t offset, int resfd, void* ptr)
{
#ifdef DEBUG
	printf("[Native AIO]正在准备IO操作\n");
#endif // DEBUG
	iocb cbs[1], * p[1] = { &cbs[0] };
	memset(&(cbs[0]), 0, sizeof(iocb));
	(cbs[0]).aio_lio_opcode = IOCB_CMD_PREAD;
	(cbs[0]).aio_fildes = fd;
	(cbs[0]).aio_buf = (uint64_t)buf;
	(cbs[0]).aio_nbytes = size;
	(cbs[0]).aio_offset = offset;
	(cbs[0]).aio_data = (uint64_t)ptr;
	if (resfd != invalid_eventfd)
	{
		(cbs[0]).aio_flags = IOCB_FLAG_RESFD;
		(cbs[0]).aio_resfd = resfd;
	}
	if (stdx::io_submit(context, 1, p) != 1)
	{
#ifdef DEBUG
		printf("[Native AIO]IO操作提交失败\n");
#endif // DEBUG
		_ThrowLinuxError
	}
#ifdef DEBUG
	printf("[Native AIO]IO操作已提交\n");
#endif // DEBUG
	return;
}

void stdx::aio_write(aio_context_t context, int fd, char* buf, size_t size, int64_t offset, int resfd, void* ptr)
{
#ifdef DEBUG
	printf("[Native AIO]正在准备IO操作\n");
#endif // DEBUG
	iocb cbs[1], * p[1] = { &cbs[0] };
	memset(&(cbs[0]), 0, sizeof(iocb));
	(cbs[0]).aio_lio_opcode = IOCB_CMD_PWRITE;
	(cbs[0]).aio_fildes = fd;
	(cbs[0]).aio_buf = (uint64_t)buf;
	(cbs[0]).aio_nbytes = size;
	(cbs[0]).aio_offset = offset;
	(cbs[0]).aio_data = (uint64_t)ptr;
	if (resfd != invalid_eventfd)
	{
		(cbs[0]).aio_flags = IOCB_FLAG_RESFD;
		(cbs[0]).aio_resfd = resfd;
	}
	if (stdx::io_submit(context, 1, p) != 1)
	{
#ifdef DEBUG
		printf("[Native AIO]IO操作提交失败\n");
#endif // DEBUG
		_ThrowLinuxError
	}
#ifdef DEBUG
	printf("[Native AIO]IO操作已提交\n");
#endif // DEBUG
	return;
}

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
void stdx::_EPOLL::wait(epoll_event * event_ptr, const int & maxevents, const int & timeout) 
{
	if (epoll_wait(m_handle, event_ptr, maxevents, timeout) == -1)
	{
		_ThrowLinuxError
	}
}
void stdx::_Reactor::bind(int fd)
{
	std::unique_lock<stdx::spin_lock> _lock(m_lock);
	auto iterator = m_map.find(fd);
	if (iterator == std::end(m_map))
	{
		m_map.emplace(fd,make());
		_lock.unlock();
	}
}

void stdx::_Reactor::unbind(int fd)
{
	std::unique_lock<stdx::spin_lock> _lock(m_lock);
	auto iterator = m_map.find(fd);
	if (iterator != std::end(m_map))
	{
		std::unique_lock<stdx::spin_lock> lock(iterator->second.m_lock);
		_lock.unlock();
		auto& obj = iterator->second;
		if (!obj.m_queue.empty())
		{
//#ifdef DEBUG
			::printf("[Epoll]IO操作仍未完成,但FD已解除绑定,剩余队列长%zu\n",obj.m_queue.size());
//#endif
			//while (!obj.m_queue.empty())
			//{
			//	obj.m_queue.pop();
			//}
		}
		obj.m_existed = false;
	}
}

void stdx::_Reactor::push(int fd, epoll_event & ev)
{
	ev.events |= stdx::epoll_events::once;
	std::unique_lock<stdx::spin_lock> _lock(m_lock);
	auto iterator = m_map.find(fd);
	if (iterator != std::end(m_map))
	{
		std::lock_guard<stdx::spin_lock> lock(iterator->second.m_lock);
		_lock.unlock();
#ifdef DEBUG
		printf("[Epoll]准备更新监听事件\n");
#endif // DEBUG
		if (!iterator->second.m_existed)
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
		_lock.unlock();
#ifdef DEBUG
		::printf("[Epoll]无效FD\n");
#endif // DEBUG
		bind(fd);
		return push(fd, ev);
	}
}

void stdx::_Reactor::loop(int fd)
{
	std::unique_lock<stdx::spin_lock> _lock(m_lock);
	auto iterator = m_map.find(fd);
	if (iterator != std::end(m_map))
	{
		auto &obj = *iterator;
		std::unique_lock<stdx::spin_lock> lock(obj.second.m_lock);
		_lock.unlock();
		if (!obj.second.m_queue.empty())
		{
			auto ev = obj.second.m_queue.front();
			iterator->second.m_queue.pop();
			lock.unlock();
#ifdef DEBUG
			::printf("[Epoll]更新监听事件\n");
#endif // DEBUG
			m_poll.update_event(fd, &ev);
			return;
		}
#ifdef DEBUG
		::printf("[Epoll]到达循环尾\n");
#endif // DEBUG
		if (iterator->second.m_existed)
		{
			iterator->second.m_existed = false;
		}
		m_poll.del_event(fd);
	}
}
#endif // LINUX

#ifdef WIN32
std::wistream& stdx::cin()
{
	return std::wcin;
}
std::wostream& stdx::cout()
{
	return std::wcout;
}
std::wostream &stdx::cerr()
{
	return std::wcerr;
}
#else
std::istream &stdx::cin()
{
	return std::cin;
}
std::ostream &stdx::cout()
{
	return std::cout;
}
std::ostream &stdx::cerr()
{
	return std::cerr;
}
#endif

void stdx::_Fprintf(FILE *stream,stdx::string&& format, std::initializer_list<stdx::string>&& list)
{
	stdx::_FormatString(format, std::move(list));
#ifdef WIN32
	fputws(format.c_str(),stream);
#else
	fputs(format.c_str(),stream);
#endif
}

void stdx::_Plogf(stdx::string&& format, std::initializer_list<stdx::string>&& list)
{
	stdx::datetime &&now = stdx::datetime::now();
	stdx::string &&str = now.to_string(U("[%year-%mon-%day %hour:%min:%sec]"));
	str.append(format);
	stdx::_FormatString(str, std::move(list));
#ifdef WIN32
	fputws(str.c_str(), stdout);
#else
	fputs(str.c_str(), stdout);
#endif
}