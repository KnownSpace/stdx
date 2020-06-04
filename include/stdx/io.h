#pragma once
#include <stdx/env.h>
#include <memory>
#include <string>
#include <stdx/buffer.h>
#include <stdio.h>
#include <stdx/async/threadpool.h>
#include <stdx/finally.h>
#include <stdx/poller.h>
#include <stdx/async/callback_flag.h>

namespace stdx
{
#ifdef WIN32
	template<typename _IOContext>
	using io_poller = stdx::poller<_IOContext, HANDLE>;
#else
	template<typename _IOContext>
	using io_poller = stdx::poller<_IOContext, int>;
#endif
}

#ifdef WIN32

//定义抛出Windows错误宏
#ifdef WIN32
#define _ThrowWinError auto _ERROR_CODE = GetLastError(); \
						if(_ERROR_CODE != ERROR_IO_PENDING) \
						{ \
							throw std::system_error(std::error_code(_ERROR_CODE,std::system_category())); \
						}
#define _ThrowWSAError 	auto _ERROR_CODE = WSAGetLastError(); \
						if(_ERROR_CODE != WSA_IO_PENDING)\
						{\
							throw std::system_error(std::error_code(_ERROR_CODE,std::system_category()));\
						}
#endif

#ifdef LINUX
#define _ThrowLinuxError auto _ERROR_CODE = errno;\
						 throw std::system_error(std::error_code(_ERROR_CODE,std::system_category())); 
#endif

namespace stdx
{
	//IOCP封装
	template<typename _IOContext>
	class _IOCP :public stdx::basic_poller<_IOContext, HANDLE>
	{
		using base_t = stdx::basic_poller<_IOContext, HANDLE>;
	public:
		_IOCP()
			:base_t()
			, m_iocp(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0))
		{
		}

		~_IOCP()
		{
			if (m_iocp != INVALID_HANDLE_VALUE)
			{
				CloseHandle(m_iocp);
			}
		}

		delete_copy(_IOCP<_IOContext>);

		virtual void bind(const HANDLE& file_handle) override
		{
			if (CreateIoCompletionPort(file_handle, m_iocp, (ULONG_PTR)file_handle, cpu_cores() * 2 + 2) == NULL)
			{
				_ThrowWinError
			}
		}

		template<typename _HandleType>
		void bind(const _HandleType& file_handle)
		{
			if (CreateIoCompletionPort((HANDLE)file_handle, m_iocp, file_handle, 0) == NULL)
			{
				_ThrowWinError
			}
		}

		virtual _IOContext* get() override
		{
			DWORD size = 0;
			OVERLAPPED* ol = nullptr;
			ULONG_PTR key = 0;
			bool r = GetQueuedCompletionStatus(m_iocp, &size, &key, &ol, INFINITE);
			if (!r)
			{
				//处理错误
				_ThrowWinError
			}
			if (ol == nullptr)
			{
				return nullptr;
			}
			return CONTAINING_RECORD(ol, _IOContext, m_ol);
		}

		virtual _IOContext* get(uint32_t ms) override
		{
			DWORD size = 0;
			OVERLAPPED* ol = nullptr;
			ULONG_PTR key = 0;
			bool r = GetQueuedCompletionStatus(m_iocp, &size, &key, &ol, ms);
			if (!r)
			{
				if (GetLastError() == 258)
				{
					return nullptr;
				}
				//处理错误
				_ThrowWinError
			}
			if (ol == nullptr)
			{
				return nullptr;
			}
			return CONTAINING_RECORD(ol, _IOContext, m_ol);
		}

		void post(DWORD size, _IOContext* context_ptr, OVERLAPPED* ol_ptr)
		{
			bool r = PostQueuedCompletionStatus(m_iocp, size, (ULONG_PTR)context_ptr, ol_ptr);
			if (!r)
			{
				//处理错误
				_ThrowWinError
			}
		}

		virtual void post(_IOContext* p) override
		{
			if (p != nullptr)
			{
				if (!PostQueuedCompletionStatus(m_iocp, 0, (ULONG_PTR)p, &(p->m_ol)))
				{
					//处理错误
					_ThrowWinError
				}
			}
			else
			{
				if (!PostQueuedCompletionStatus(m_iocp, 0, (ULONG_PTR)p, nullptr))
				{
					//处理错误
					_ThrowWinError
				}
			}
		}

	private:
		HANDLE m_iocp;
	};


	template<typename _IOContext>
	inline stdx::io_poller<_IOContext> make_iocp_poller()
	{
		return stdx::make_poller<stdx::_IOCP<_IOContext>>();
	}
}
#undef _ThrowWinError
#endif

#ifdef LINUX
#include <memory>
#include <system_error>
#include <string.h>
#include <sys/epoll.h>
#include <errno.h>
#include <linux/aio_abi.h>
#include <sys/syscall.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <unordered_map>
#include <list>
#include <queue>
#include <stdx/async/spin_lock.h>
#include <mutex>
#include <stdx/function.h>
#include <stdx/async/threadpool.h>
#define _ThrowLinuxError auto _ERROR_CODE = errno;\
						 throw std::system_error(std::error_code(_ERROR_CODE,std::system_category())); 

namespace stdx
{
	struct epoll_events
	{
		enum
		{
			in = EPOLLIN,
			out = EPOLLOUT,
			err = EPOLLERR,
			hup = EPOLLHUP,
			et = EPOLLET,
			once = EPOLLONESHOT
		};
	};
	class _EPOLL
	{
	public:
		_EPOLL()
			:m_handle(epoll_create1(0))
		{}
		~_EPOLL()
		{
			close(m_handle);
		}

		void add_event(int fd, epoll_event* event_ptr);

		void del_event(int fd);

		void update_event(int fd, epoll_event* event_ptr);

		int wait(epoll_event* event_ptr, const int& maxevents, const int& timeout);
	private:
		int m_handle;
	};
	class epoll
	{
		using impl_t = std::shared_ptr<_EPOLL>;
	public:
		epoll()
			:m_impl(std::make_shared<_EPOLL>())
		{}
		epoll(const epoll& other)
			:m_impl(other.m_impl)
		{}
		~epoll() = default;

		epoll& operator=(const epoll& other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		void add_event(int fd, epoll_event* event_ptr)
		{
			return m_impl->add_event(fd, event_ptr);
		}
		void update_event(int fd, epoll_event* event_ptr)
		{
			return m_impl->update_event(fd, event_ptr);
		}
		void del_event(int fd)
		{
			return m_impl->del_event(fd);
		}

		int wait(epoll_event* event_ptr, const int& maxevents, const int& timeout)
		{
			return m_impl->wait(event_ptr, maxevents, timeout);
		}

		epoll_event wait(const int& timeout)
		{

			epoll_event ev;
			if (this->wait(&ev, 1, timeout) != 1)
			{
				ev.data.fd = -1;
			}
			return ev;
		}
	private:
		impl_t m_impl;
	};

	extern int io_setup(unsigned nr_events, aio_context_t* ctx_idp);

	extern int io_destroy(aio_context_t ctx_id);

	extern int io_submit(aio_context_t ctx_id, long nr, struct iocb** iocbpp);

	extern int io_getevents(aio_context_t ctx_id, long min_nr, long nr, struct io_event* events, struct timespec* timeout);

	extern int io_cancel(aio_context_t ctx_id, struct iocb* iocb, struct io_event* result);
#define INVALID_EVENTFD -1
	extern void aio_read(aio_context_t context, int fd, char* buf, size_t size, int64_t offset, int resfd, void* ptr);

	extern void aio_write(aio_context_t context, int fd, char* buf, size_t size, int64_t offset, int resfd, void* ptr);

	template<typename _IOContext>
	class _AIOCP
	{
	public:
		_AIOCP(unsigned nr_events = 2048)
			:m_ctxid(0)
		{
			memset(&m_ctxid, 0, sizeof(aio_context_t));
			io_setup(nr_events, &m_ctxid);
		}
		~_AIOCP()
		{
			io_destroy(m_ctxid);
		}

		_IOContext* get(int64_t& res)
		{
			io_event ev;
			timespec tm;
			tm.tv_nsec = 0;
			tm.tv_sec = 600;
			int r = io_getevents(m_ctxid, 1, 1, &ev, &tm);
			if (r < 1)
			{
#ifdef DEBUG
				if (r < 0)
				{
					try
					{
						if (errno != EINTR)
						{
							_ThrowLinuxError
						}
					}
					catch (const std::exception& err)
					{
						::printf("[Native AIO]发生意外错误:%s\n", err.what());
					}
				}
#endif
				return nullptr;
			}
			res = ev.res;
			return (_IOContext*)ev.data;
		}

		_IOContext* get(int64_t& res, int32_t ms)
		{
			io_event ev;
			timespec tm;
			tm.tv_nsec = ms * 1000 * 1000;
			tm.tv_sec = 0;
			int r = io_getevents(m_ctxid, 1, 1, &ev, &tm);
			if (r < 1)
			{
#ifdef DEBUG
				if (r < 0)
				{
					try
					{
						if (errno != EINTR)
						{
							_ThrowLinuxError
						}
					}
					catch (const std::exception& err)
					{
						::printf("[Native AIO]发生意外错误:%s\n", err.what());
					}
				}
#endif
				return nullptr;
			}
			res = ev.res;
			return (_IOContext*)ev.data;
		}

		aio_context_t get_context() const
		{
			return m_ctxid;
		}
	private:
		aio_context_t m_ctxid;
	};
	template<typename _IOContext>
	class aiocp
	{
		using impl_t = std::shared_ptr<_AIOCP<_IOContext>>;
	public:
		aiocp(unsigned nr_events)
			:m_impl(std::make_shared<_AIOCP<_IOContext>>(nr_events))
		{}
		aiocp(const aiocp<_IOContext>& other)
			:m_impl(other.m_impl)
		{}
		aiocp(aiocp<_IOContext>&& other)
			:m_impl(std::move(other.m_impl))
		{}
		~aiocp() = default;
		aiocp& operator=(const aiocp<_IOContext>& other)
		{
			m_impl = other.m_impl;
			return *this;
		}
		aio_context_t get_context() const
		{
			return m_impl->get_context();
		}
		_IOContext* get(int64_t& res)
		{
			return m_impl->get(res);
		}
		_IOContext* get(int64_t& res, int32_t ms)
		{
			return m_impl->get(res, ms);
		}

		bool operator==(const aiocp& other) const
		{
			return m_impl == other.m_impl;
		}

	private:
		impl_t m_impl;
	};

	struct epoll_event_model
	{
		epoll_event ev;
		bool is_exist;
		bool is_err_or_hup;
	};

	template<typename _IOContext>
	struct epoll_context_list
	{
		stdx::epoll_event_model model;
		std::list<_IOContext*> out_contexts;
		std::list<_IOContext*> in_contexts;
	};

	extern int make_eventfd(int flags);

	extern int make_semaphore_eventfd(int flags);

	template<typename _IOContext>
	class _EpollProactor :public stdx::basic_poller<_IOContext, int>
	{
	public:
		using clean_t = std::function<void(_IOContext*)>;
		using operate_t = std::function<bool(_IOContext*)>;
		using fd_getter_t = std::function<int(_IOContext*)>;
		using event_getter_t = std::function<uint32_t(_IOContext*)>;
		using task_t = std::function<void()>;
		using lock_t = stdx::spin_lock;
	public:
		_EpollProactor(clean_t clean, operate_t io_operator, fd_getter_t fd_getter, event_getter_t event_getter)
			:m_epoll()
			, m_map()
			, m_clean(clean)
			, m_operate(io_operator)
			, m_fd_getter(fd_getter)
			, m_event_getter(event_getter)
			, m_eventfd(stdx::make_eventfd(EFD_NONBLOCK))
			, m_ev_lock()
			, m_tasks()
			, m_completions()
		{
			epoll_event ev;
			ev.events = stdx::epoll_events::in;
			ev.data.fd = m_eventfd;
			m_epoll.add_event(m_eventfd, &ev);
		}

		~_EpollProactor()
		{
			::close(m_eventfd);
		}

		virtual _IOContext* get() override
		{
			{
				auto* cont = _CheckCompletions();
				if (cont)
				{
					return cont;
				}
			}
			while (true)
			{
				epoll_event ev;
				int r = m_epoll.wait(&ev, 1, -1);
				if (r == 1)
				{
					if (ev.data.fd == m_eventfd)
					{
						//is event fd
						if (_ReadEvFd())
						{
							_HandleTasks();
						}
						auto* cont = _CheckCompletions();
						if (cont)
						{
							return cont;
						}
					}
					else if (ev.events & (stdx::epoll_events::err | stdx::epoll_events::hup))
					{
						//has error(s)
						//clean operation
						int fd = ev.data.fd;
						_CleanFd(fd);
					}
					else
					{
						try
						{
							_IOContext* cont = _HandleIoEvent(ev);
							return cont;
						}
						catch (const std::exception& err)
						{
							return nullptr;
						}
					}
				}
			}
		}

		virtual _IOContext* get(uint32_t timeout_ms) override
		{
			{
				auto* cont = _CheckCompletions();
				if (cont)
				{
					return cont;
				}
			}
			epoll_event ev;
			int r = m_epoll.wait(&ev, 1, timeout_ms);
			if (r != 1)
			{
				return nullptr;
			}
			if (ev.data.fd == m_eventfd)
			{
				//is event fd
				if (_ReadEvFd())
				{
					_HandleTasks();
				}
				auto* cont = _CheckCompletions();
				if (cont)
				{
					return cont;
				}
				//return by timeout
				return nullptr;
			}
			if (ev.events & (stdx::epoll_events::err | stdx::epoll_events::hup))
			{
				//has error(s)
				//clean operation
				int fd = ev.data.fd;
				_CleanFd(fd);
				return nullptr;
			}
			try
			{
				_IOContext* cont = _HandleIoEvent(ev);
				return cont;
			}
			catch (const std::exception& err)
			{
				return nullptr;
			}
		}

		virtual void post(_IOContext* p) override
		{
			if (p == nullptr)
			{
				return;
			}
			_RunInLoop([this](_IOContext* p)
				{
					//get fd
					int fd = m_fd_getter(p);
					//get context manager
					stdx::epoll_context_list<_IOContext>& ev = m_map[fd];
					//is hup or error
					if (ev.model.is_err_or_hup)
					{
						//clean context
						m_clean(p);
						return;
					}
					//get events
					uint32_t events = m_event_getter(p);
					if (events & stdx::epoll_events::in)
					{
						//if (ev.in_contexts.empty())
						//{
						//	bool r = m_operate(p);
						//	if (r)
						//	{
						//		m_completions.push_back(p);
						//		return;
						//	}
						//}
						ev.in_contexts.push_back(p);
						//check if update epoll
						if (!(ev.model.ev.events & stdx::epoll_events::in))
						{
							//update epoll
							ev.model.ev.events |= stdx::epoll_events::in;
							_UpdateOrAddEvent(ev);
						}
					}
					else if (events & stdx::epoll_events::out)
					{
						if (ev.out_contexts.empty())
						{
							bool r = m_operate(p);
							if (r)
							{
								m_completions.push_back(p);
								return;
							}
						}
						ev.out_contexts.push_back(p);
						//check if update epoll
						if (!(ev.model.ev.events & stdx::epoll_events::out))
						{
							//update epoll
							ev.model.ev.events |= stdx::epoll_events::out;
							_UpdateOrAddEvent(ev);
						}
					}
				}, p);
		}

		virtual void bind(const int& fd) override
		{
			_RunInLoop([this](int fd)
				{
					stdx::epoll_context_list<_IOContext> ev;
					_InitModel(ev.model, fd);
					m_map[fd] = std::move(ev);
				}, fd);
		}

		virtual void unbind(const int& fd) override
		{
			_RunInLoop([this](int fd) mutable
				{
					auto& ev = m_map[fd];
					ev.model.is_err_or_hup = true;
					ev.model.is_exist = false;
					_CleanContexts(ev);
					//remove from epoll
					try
					{
						m_epoll.del_event(fd);
					}
					catch (const std::exception& err)
					{
#ifdef DEBUG
						::printf("[EpollProactor]Remove event failure: %s\n", err.what());
#endif
					}
				}, fd);
		}

		virtual void unbind(const int& object, std::function<void(int)> deleter)
		{
			_RunInLoop([this](int fd, std::function<void(int)> deleter) mutable
				{
					auto& ev = m_map[fd];
					ev.model.is_err_or_hup = true;
					ev.model.is_exist = false;
					_CleanContexts(ev);
					//remove from epoll
					try
					{
						m_epoll.del_event(fd);
						deleter(fd);
					}
					catch (const std::exception& err)
					{
#ifdef DEBUG
						::printf("[EpollProactor]Remove event or delete fd failure: %s\n", err.what());
#endif
					}
				}, object, deleter);
		}
	private:

		void __RunInLoop(task_t&& task)
		{
			{
				std::unique_lock<lock_t> lock(m_ev_lock);
				m_tasks.push_back(std::move(task));
			}
			_WokenUpFd();
		}

		template<typename _Fn, typename ..._Args, class = typename std::enable_if<stdx::is_callable<_Fn>::value>::type>
		void _RunInLoop(_Fn&& fn, _Args&&...args)
		{
			task_t task = std::bind(fn, args...);
			__RunInLoop(std::move(task));
		}

		void _WokenUpFd()
		{
			eventfd_t val = 1;
			::write(m_eventfd, &val, sizeof(eventfd_t));
		}

		bool _ReadEvFd()
		{
			eventfd_t val = UINT64_MAX;
			::read(m_eventfd, &val, sizeof(eventfd_t));
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				return false;
			}
			return true;
		}

		void _UpdateOrAddEvent(stdx::epoll_context_list<_IOContext>& ev)
		{
			if (ev.model.is_exist)
			{
				m_epoll.update_event(ev.model.ev.data.fd, &(ev.model.ev));
			}
			else
			{
				ev.model.is_exist = true;
				m_epoll.add_event(ev.model.ev.data.fd, &(ev.model.ev));
			}
		}

		_IOContext* _HandleIoEvent(epoll_event& ev)
		{
			int fd = ev.data.fd;
			stdx::epoll_context_list<_IOContext>& ev_ = m_map[fd];
			bool need_ctl = false;
			//handle in event first
			if (ev.events & stdx::epoll_events::in)
			{
				if (!ev_.in_contexts.empty())
				{
					_IOContext* cont = ev_.in_contexts.front();
					//I/O operation
					if (m_operate(cont))
					{
						//I/O operation finish
						ev_.in_contexts.pop_front();
						return cont;
					}
				}
				else
				{
					ev_.model.ev.events ^= stdx::epoll_events::in;
					need_ctl = true;
				}
			}
			//handle out event
			else if (ev.events & stdx::epoll_events::out)
			{
				if (ev_.out_contexts.size() > 1)
				{
					_IOContext* cont = ev_.out_contexts.front();
					//I/O operation
					if (m_operate(cont))
					{
						//I/O operation finish
						ev_.out_contexts.pop_front();
						return cont;
					}
				}
				else if (ev_.out_contexts.size() == 1)
				{
					_IOContext* cont = ev_.out_contexts.front();
					//I/O operation
					if (m_operate(cont))
					{
						//I/O operation finish
						ev_.out_contexts.pop_front();
						ev_.model.ev.events ^= stdx::epoll_events::out;
						m_epoll.update_event(fd, &(ev_.model.ev));
						return cont;
					}
				}
				else
				{
					ev_.model.ev.events ^= stdx::epoll_events::out;
					need_ctl = true;
				}
			}
			if (need_ctl)
			{
				m_epoll.update_event(fd, &(ev_.model.ev));
			}
			return nullptr;
		}

		void _CleanContexts(stdx::epoll_context_list<_IOContext>& ev)
		{
			auto* contexts = &(ev.in_contexts);
			for (auto begin = contexts->begin(), end = contexts->end(); begin != end; ++begin)
			{
				m_clean(*begin);
			}
			contexts->clear();
			contexts = &(ev.out_contexts);
			for (auto begin = contexts->begin(), end = contexts->end(); begin != end; ++begin)
			{
				m_clean(*begin);
			}
			contexts->clear();
		}

		void _CleanFd(int fd)
		{
			auto& ev = m_map[fd];
			ev.model.is_err_or_hup = true;
			ev.model.is_exist = false;
			_CleanContexts(ev);
			m_epoll.del_event(fd);
		}

		void _InitModel(stdx::epoll_event_model& model, int fd)
		{
			model.is_exist = false;
			model.is_err_or_hup = false;
			model.ev.data.fd = fd;
			model.ev.events = stdx::epoll_events::hup;
		}

		void _HandleTasks()
		{
			std::list<task_t> tasks;
			{
				std::unique_lock<lock_t> lock(m_ev_lock);
				std::swap(tasks, m_tasks);
			}
			while (!tasks.empty())
			{
				task_t task = tasks.front();
				tasks.pop_front();
				try
				{
					if (task)
					{
						task();
					}
				}
				catch (const std::exception& err)
				{
#ifdef DEBUG
					::printf("[EpollProactor]Execute task error: %s\n", err.what());
#endif
				}
			}
		}

		_IOContext* _CheckCompletions()
		{
			if (!m_completions.empty())
			{
				auto* p = m_completions.front();
				m_completions.pop_front();
				return p;
			}
			return nullptr;
		}

		stdx::epoll m_epoll;
		std::unordered_map<int, stdx::epoll_context_list<_IOContext>> m_map;
		clean_t m_clean;
		operate_t m_operate;
		fd_getter_t m_fd_getter;
		event_getter_t m_event_getter;
		int m_eventfd;
		lock_t m_ev_lock;
		std::list<task_t> m_tasks;
		std::list<_IOContext*> m_completions;
	};

	template<typename _IOContext>
	inline stdx::io_poller<_IOContext> make_epoll_proactor_poller(typename stdx::_EpollProactor<_IOContext>::clean_t clean,
		typename stdx::_EpollProactor<_IOContext>::operate_t io_operate,
		typename stdx::_EpollProactor<_IOContext>::fd_getter_t fd_getter,
		typename stdx::_EpollProactor<_IOContext>::event_getter_t event_getter)
	{
		return stdx::make_poller<stdx::_EpollProactor<_IOContext>>(clean, io_operate, fd_getter, event_getter);
	}

	template<typename _IOContext>
	inline stdx::io_poller<_IOContext> make_epoll_multipoller(size_t num_of_poller,
		typename stdx::_EpollProactor<_IOContext>::clean_t clean,
		typename stdx::_EpollProactor<_IOContext>::operate_t io_operate,
		typename stdx::_EpollProactor<_IOContext>::fd_getter_t fd_getter,
		typename stdx::_EpollProactor<_IOContext>::event_getter_t event_getter)
	{
		return stdx::make_multipoller<stdx::_EpollProactor<_IOContext>>(num_of_poller, [](const int& fd, size_t size)
			{
				size_t index = fd % size;
				return index;
			}, fd_getter, clean, io_operate, fd_getter, event_getter);
	}

	template<typename _IOContext>
	class _BIOPoller :public stdx::basic_poller<_IOContext, int>
	{
	public:
		_BIOPoller()
			:m_lock()
			, m_cv()
			, m_list()
		{}

		~_BIOPoller() = default;

		virtual _IOContext* get() override
		{
			std::unique_lock<std::mutex> lock(m_lock);
			while (m_list.empty())
			{
				m_cv.wait(lock);
			}
			_IOContext* p = m_list.front();
			m_list.pop_front();
			return p;
		}

		virtual _IOContext* get(uint32_t ms) override
		{
			std::unique_lock<std::mutex> lock(m_lock);
			while (m_list.empty())
			{
				auto status = m_cv.wait_for(lock, std::chrono::milliseconds(ms));
				if (status == std::cv_status::timeout)
				{
					return nullptr;
				}
			}
			_IOContext* p = m_list.front();
			m_list.pop_front();
			return p;
		}

		virtual void post(_IOContext* p) override
		{
			std::unique_lock<std::mutex> lock(m_lock);
			m_list.push_back(p);
			m_cv.notify_one();
		}

	private:
		std::mutex m_lock;
		std::condition_variable m_cv;
		std::list<_IOContext*> m_list;
	};

	template<typename _IOContext>
	inline stdx::io_poller<_IOContext> make_bio_poller()
	{
		return stdx::make_poller<stdx::_BIOPoller<_IOContext>>();
	}
}

#undef _ThrowLinuxError
#endif

namespace stdx
{
#ifdef WIN32
	extern std::wistream& cin();
	extern std::wostream& cout();
	extern std::wostream& cerr();
#else
	extern std::istream& cin();
	extern std::ostream& cout();
	extern std::ostream& cerr();
#endif

	void _Fprintf(FILE* stream, stdx::string&& format, std::initializer_list<stdx::string>&& list);

	void _Plogf(stdx::string&& format, std::initializer_list<stdx::string>&& list);

	template<typename ..._Args>
	void printf(const stdx::string& format, const _Args&...args)
	{
		stdx::string _format(format);
		_Fprintf(stdout, std::move(_format), std::move(std::initializer_list<stdx::string>{ stdx::to_string(args)... }));
	}

	template<typename ..._Args>
	void printf(stdx::string&& format, const _Args&...args)
	{
		_Fprintf(stdout, std::move(format), std::move(std::initializer_list<stdx::string>{ stdx::to_string(args)... }));
	}

	template<typename ..._Args>
	void perrorf(const stdx::string& format, const _Args&...args)
	{
		stdx::string _format(format);
		_Fprintf(stderr, std::move(_format), std::move(std::initializer_list<stdx::string>{ stdx::to_string(args)... }));
	}

	template<typename ..._Args>
	void perrorf(stdx::string&& format, const _Args&...args)
	{
		_Fprintf(stderr, std::move(format), std::move(std::initializer_list<stdx::string>{ stdx::to_string(args)... }));
	}

	template<typename ..._Args>
	void plogf(const stdx::string& format, const _Args&...args)
	{
		stdx::string _format(format);
		_Plogf(std::move(_format), std::move(std::initializer_list<stdx::string>{ stdx::to_string(args)... }));
	}

	template<typename ..._Args>
	void plogf(stdx::string&& format, const _Args&...args)
	{
		_Plogf(std::move(format), std::move(std::initializer_list<stdx::string>{ stdx::to_string(args)... }));
	}
}