#include <stdx/io.h>
#include <iostream>
#include <stdx/datetime.h>
#include <stdx/net/socket.h>
#ifdef LINUX
#define _ThrowLinuxError auto _ERROR_CODE = errno;\
							throw std::system_error(std::error_code(_ERROR_CODE,std::system_category())); 
#include <signal.h>

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
	iocb cbs[1], * p[1] = { &cbs[0] };
	memset(&(cbs[0]), 0, sizeof(iocb));
	(cbs[0]).aio_lio_opcode = IOCB_CMD_PREAD;
	(cbs[0]).aio_fildes = fd;
	(cbs[0]).aio_buf = (uint64_t)buf;
	(cbs[0]).aio_nbytes = size;
	(cbs[0]).aio_offset = offset;
	(cbs[0]).aio_data = (uint64_t)ptr;
	if (resfd != INVALID_EVENTFD)
	{
		(cbs[0]).aio_flags = IOCB_FLAG_RESFD;
		(cbs[0]).aio_resfd = resfd;
	}
	if (stdx::io_submit(context, 1, p) != 1)
	{
		_ThrowLinuxError
	}
	return;
}

void stdx::aio_write(aio_context_t context, int fd, char* buf, size_t size, int64_t offset, int resfd, void* ptr)
{
	iocb cbs[1], * p[1] = { &cbs[0] };
	memset(&(cbs[0]), 0, sizeof(iocb));
	(cbs[0]).aio_lio_opcode = IOCB_CMD_PWRITE;
	(cbs[0]).aio_fildes = fd;
	(cbs[0]).aio_buf = (uint64_t)buf;
	(cbs[0]).aio_nbytes = size;
	(cbs[0]).aio_offset = offset;
	(cbs[0]).aio_data = (uint64_t)ptr;
	if (resfd != INVALID_EVENTFD)
	{
		(cbs[0]).aio_flags = IOCB_FLAG_RESFD;
		(cbs[0]).aio_resfd = resfd;
	}
	if (stdx::io_submit(context, 1, p) != 1)
	{
		_ThrowLinuxError
	}
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
int stdx::_EPOLL::wait(epoll_event * event_ptr, const int & maxevents, const int & timeout) 
{
	sigset_t newmask;
	sigemptyset(&newmask);
	sigaddset(&newmask, SIGINT);
	int r = epoll_pwait(m_handle, event_ptr, maxevents, timeout,&newmask);
	if (r == -1)
	{
		_ThrowLinuxError
	}
	return r;
}

int stdx::make_eventfd(int flags)
{
	int fd = ::eventfd(0,flags);
	if (fd == -1)
	{
		_ThrowLinuxError
	}
	return fd;
}

int stdx::make_semaphore_eventfd(int flags)
{
	return stdx::make_eventfd(EFD_SEMAPHORE);
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