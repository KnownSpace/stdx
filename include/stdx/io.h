#pragma once
#include <stdx/env.h>
#include <memory>
#include <string>
#include <stdx/buffer.h>
#include <stdio.h>
#include <stdx/async/threadpool.h>
#include <stdx/finally.h>
#include <stdx/poller.h>

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
	class _IOCP:public stdx::basic_poller<_IOContext,HANDLE>
	{
		using base_t = stdx::basic_poller<_IOContext, HANDLE>;
	public:
		_IOCP()
			:base_t()
			,m_iocp(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0))
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

		virtual void bind(const HANDLE &file_handle) override
		{
			if (CreateIoCompletionPort(file_handle, m_iocp, (ULONG_PTR)file_handle, cpu_cores() * 2 + 2) == NULL)
			{
				_ThrowWinError
			}
		}

		template<typename _HandleType>
		void bind(const _HandleType &file_handle)
		{
			if (CreateIoCompletionPort((HANDLE)file_handle, m_iocp, file_handle, 0) == NULL)
			{
				_ThrowWinError
			}
		}

		virtual _IOContext *get() override
		{
			DWORD size = 0;
			OVERLAPPED *ol= nullptr;
			ULONG_PTR key = 0;
			bool r = GetQueuedCompletionStatus(m_iocp, &size,&key,&ol, INFINITE);
			if (!r)
			{
				//处理错误
				_ThrowWinError
			}
			if (ol == nullptr)
			{
				return nullptr;
			}
			return CONTAINING_RECORD(ol,_IOContext, m_ol);
		}

		virtual _IOContext* get(uint32_t ms) override
		{
			DWORD size = 0;
			OVERLAPPED* ol = nullptr;
			ULONG_PTR key = 0;
			bool r = GetQueuedCompletionStatus(m_iocp, &size, &key, &ol,ms);
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

		void post(DWORD size,_IOContext *context_ptr,OVERLAPPED *ol_ptr)
		{
			bool r = PostQueuedCompletionStatus(m_iocp, size, (ULONG_PTR)context_ptr, ol_ptr);
			if (!r)
			{
				//处理错误
				_ThrowWinError
			}
		}

		virtual void post(_IOContext *p) override
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

	//IOCP引用封装
	//template<typename _IOContext>
	//class iocp
	//{
	//	using impl_t = std::shared_ptr<stdx::_IOCP<_IOContext>>;
	//public:
	//	iocp()
	//		:m_impl(std::make_shared<stdx::_IOCP<_IOContext>>())
	//	{}

	//	iocp(const iocp<_IOContext> &other)
	//		:m_impl(other.m_impl)
	//	{}

	//	iocp(iocp<_IOContext> &&other) noexcept
	//		:m_impl(std::move(other.m_impl))
	//	{}

	//	~iocp() = default;

	//	iocp<_IOContext> &operator=(const iocp<_IOContext> &other)
	//	{
	//		m_impl = other.m_impl;
	//		return *this;
	//	}

	//	_IOContext *get()
	//	{
	//		return m_impl->get();
	//	}

	//	_IOContext* get(DWORD ms)
	//	{
	//		return m_impl->get(ms);
	//	}

	//	void bind(const HANDLE &file_handle)
	//	{
	//		m_impl->bind(file_handle);
	//	}

	//	template<typename _HandleType>
	//	void bind(const _HandleType &file_handle)
	//	{
	//		m_impl->bind<_HandleType>(file_handle);
	//	}

	//	void post(DWORD size, _IOContext *context_ptr, OVERLAPPED *ol_ptr)
	//	{
	//		m_impl->post(size, context_ptr, ol_ptr);
	//	}

	//	bool operator==(const iocp &other) const
	//	{
	//		return m_impl == other.m_impl;
	//	}

	//private:
	//	impl_t m_impl;
	//};

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
#include <queue>
#include <stdx/async/spin_lock.h>
#include <mutex>
#include <stdx/function.h>
#include <stdx/async/threadpool.h>
#define _ThrowLinuxError auto _ERROR_CODE = errno;\
						 throw std::system_error(std::error_code(_ERROR_CODE,std::system_category()),strerror(_ERROR_CODE)); 

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

		void add_event(int fd, epoll_event *event_ptr);

		void del_event(int fd);

		void update_event(int fd, epoll_event *event_ptr);

		int wait(epoll_event *event_ptr, const int &maxevents, const int &timeout);
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
		epoll(const epoll &other)
			:m_impl(other.m_impl)
		{}
		~epoll() = default;

		epoll &operator=(const epoll &other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		void add_event(int fd, epoll_event *event_ptr)
		{
			return m_impl->add_event(fd, event_ptr);
		}
		void update_event(int fd, epoll_event *event_ptr)
		{
			return m_impl->update_event(fd, event_ptr);
		}
		void del_event(int fd)
		{
			return m_impl->del_event(fd);
		}

		int wait(epoll_event *event_ptr,const int &maxevents,const int &timeout)
		{
			return m_impl->wait(event_ptr, maxevents, timeout);
		}

		epoll_event wait(const int &timeout)
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
		_AIOCP(unsigned nr_events=2048)
			:m_ctxid(0)
		{
			memset(&m_ctxid, 0, sizeof(aio_context_t));
			io_setup(nr_events, &m_ctxid);
		}
		~_AIOCP()
		{
			io_destroy(m_ctxid);
		}

		_IOContext *get(int64_t &res)
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
					catch (const std::exception &err)
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

		_IOContext* get(int64_t& res,int32_t ms)
		{
			io_event ev;
			timespec tm;
			tm.tv_nsec = ms*1000*1000;
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
		aiocp(const aiocp<_IOContext> &other)
			:m_impl(other.m_impl)
		{}
		aiocp(aiocp<_IOContext> &&other)
			:m_impl(std::move(other.m_impl))
		{}
		~aiocp()=default;
		aiocp &operator=(const aiocp<_IOContext> &other)
		{
			m_impl = other.m_impl;
			return *this;
		}
		aio_context_t get_context() const
		{
			return m_impl->get_context();
		}
		_IOContext *get(int64_t &res)
		{
			return m_impl->get(res);
		}
		_IOContext* get(int64_t& res, int32_t ms)
		{
			return m_impl->get(res,ms);
		}

		bool operator==(const aiocp &other) const
		{
			return m_impl == other.m_impl;
		}

	private:
		impl_t m_impl;
	};

	struct ev_queue
	{
		ev_queue()
			:m_lock()
			,m_existed(false)
			,m_queue()
		{
		}

		ev_queue(const ev_queue& other)
			:m_lock(other.m_lock)
			, m_existed(other.m_existed)
			, m_queue(other.m_queue)
		{}

		ev_queue(ev_queue &&other) noexcept
			:m_lock(std::cref(other.m_lock))
			,m_existed(other.m_existed)
			,m_queue(std::move(other.m_queue))
		{}
		~ev_queue() = default;
		ev_queue &operator=(ev_queue &&other) noexcept
		{
			m_lock = std::cref(other.m_lock);
			m_existed = other.m_existed;
			m_queue = std::move(other.m_queue);
			return *this;
		}
		ev_queue& operator=(const ev_queue& other)
		{
			m_lock = other.m_lock;
			m_existed = other.m_existed;
			m_queue = other.m_queue;
			return *this;
		}
		stdx::spin_lock m_lock;
		bool m_existed;
		std::list<epoll_event> m_queue;
	};

	template<typename _IOContext>
	class _EpollProactor:public stdx::basic_poller<_IOContext,int>
	{
	public:
		using clean_t = std::function<void(_IOContext*)>;
		using operate_t = std::function<bool(_IOContext*)>;
		using lock_t = stdx::spin_lock;
		using fd_getter_t = std::function<int(_IOContext*)>;
		using event_getter_t = std::function<uint32_t(_IOContext*)>;
	public:
		_EpollProactor(clean_t clean,operate_t io_operator,fd_getter_t fd_getter,event_getter_t event_getter)
			:m_epoll()
			,m_map()
			,m_clean(clean)
			,m_operate(io_operator)
			,m_fd_getter(fd_getter)
			,m_event_getter(event_getter)
		{}

		~_EpollProactor() = default;

		virtual _IOContext* get() override
		{
			epoll_event ev;
			int r = m_epoll.wait(&ev, 1,-1);
			if (r != 1)
			{
				r = m_epoll.wait(&ev, 1, -1);
			}
			_IOContext* context = (_IOContext*)ev.data.ptr;
			if (ev.events & stdx::epoll_events::hup || ev.events & stdx::epoll_events::err)
			{
				_Reset(m_fd_getter(context));
				m_clean(context);
				return nullptr;
			}
			else
			{
				if (!m_operate(context))
				{
					post(context);
					_Reset(m_fd_getter(context));
					return nullptr;
				}
			}
			_Reset(m_fd_getter(context));
			return context;
		}

		virtual _IOContext* get(uint32_t timeout_ms) override
		{
			epoll_event ev;
			int r = m_epoll.wait(&ev, 1, timeout_ms);
			if (r != 1)
			{
				return nullptr;
			}
			_IOContext* context = (_IOContext*)ev.data.ptr;
			if ((ev.events & stdx::epoll_events::hup) || (ev.events & stdx::epoll_events::err))
			{
				_Reset(m_fd_getter(context));
				m_clean(context);
				return nullptr;
			}
			else
			{
				if (!m_operate(context))
				{
					post(context);
					_Reset(m_fd_getter(context));
					return nullptr;
				}
			}
			_Reset(m_fd_getter(context));
			return context;
		}

		virtual void post(_IOContext* p) override
		{
			if (p == nullptr)
			{
				return;
			}
			int fd = m_fd_getter(p);
			epoll_event ev;
			ev.events = m_event_getter(p);
			ev.data.ptr = p;
			ev.events |= stdx::epoll_events::once;
			auto& obj = m_map[fd];
			std::unique_lock<stdx::spin_lock> lock(obj.m_lock);
			if (!obj.m_existed)
			{
				obj.m_existed = true;
				lock.unlock();
				m_epoll.add_event(fd, &ev);
			}
			else
			{
				obj.m_queue.push_back(std::move(ev));
			}
		}

		virtual void bind(const int &fd) override
		{
			m_map[fd] = std::move(stdx::ev_queue());
		}

		virtual void unbind(const int &fd) override
		{
			auto& obj = m_map[fd];
			std::unique_lock<stdx::spin_lock> lock(obj.m_lock);
			if (!obj.m_queue.empty())
			{
				for (auto qbegin = obj.m_queue.begin(), qend = obj.m_queue.end(); qbegin != qend; qbegin++)
				{
					auto ev = *qbegin;
					if (ev.data.ptr != nullptr)
					{
						m_clean((_IOContext*)ev.data.ptr);
					}
					qbegin = obj.m_queue.erase(qbegin);
				}
			}
			obj.m_existed = false;
		}

		virtual void unbind(const int& object, std::function<void(int)> deleter)
		{
			auto& obj = m_map[object];
			if (!obj.m_queue.empty())
			{
				for (auto qbegin = obj.m_queue.begin(), qend = obj.m_queue.end(); qbegin != qend; qbegin++)
				{
					auto ev = *qbegin;
					if (ev.data.ptr != nullptr)
					{
						m_clean((_IOContext*)ev.data.ptr);
					}
					qbegin = obj.m_queue.erase(qbegin);
				}
			}
			obj.m_existed = false;
			deleter(object);
		}
	private:
		void _Reset(int fd)
		{
			auto& obj = m_map[fd];
			std::unique_lock<stdx::spin_lock> lock(obj.m_lock);
			if (obj.m_existed)
			{
				if (!obj.m_queue.empty())
				{
					auto ev = obj.m_queue.front();
					obj.m_queue.pop_front();
					obj.m_existed = true;
					m_epoll.update_event(fd, &ev);
					return;
				}
				else
				{
					m_epoll.del_event(fd);
					obj.m_existed = false;
					return;
				}
			}
			else if (!obj.m_queue.empty())
			{
				lock.unlock();
				unbind(fd);
				return;
			}
		}

		stdx::epoll m_epoll;
		std::unordered_map<int, stdx::ev_queue> m_map;
		clean_t m_clean;
		operate_t m_operate;
		fd_getter_t m_fd_getter;
		event_getter_t m_event_getter;
	};

	template<typename _IOContext>
	inline stdx::io_poller<_IOContext> make_epoll_proactor_poller(typename stdx::_EpollProactor<_IOContext>::clean_t clean, 
		typename stdx::_EpollProactor<_IOContext>::operate_t io_operate,
		typename stdx::_EpollProactor<_IOContext>::fd_getter_t fd_getter,
		typename stdx::_EpollProactor<_IOContext>::event_getter_t event_getter)
	{
		return stdx::make_poller<stdx::_EpollProactor<_IOContext>>(clean,io_operate,fd_getter,event_getter);
	}

	template<typename _IOContext>
	class _BIOPoller:public stdx::basic_poller<_IOContext,int>
	{
	public:
		_BIOPoller()
			:m_lock()
			,m_cv()
			,m_list()
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
				auto status = m_cv.wait_for(lock,std::chrono::milliseconds(ms));
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

	//template<typename _IOContext>
	//class bio_poller
	//{
	//	using impl_t = std::shared_ptr<stdx::_BIOPoller<_IOContext>>;
	//	using self_t = stdx::bio_poller<_IOContext>;
	//public:
	//	bio_poller()
	//		:m_impl(std::make_shared<_BIOPoller<_IOContext>>())
	//	{}

	//	bio_poller(const self_t& other)
	//		:m_impl(other.m_impl)
	//	{}

	//	bio_poller(self_t&& other) noexcept
	//		:m_impl(std::move(other.m_impl))
	//	{}

	//	self_t& operator=(const self_t& other)
	//	{
	//		m_impl = other.m_impl;
	//		return *this;
	//	}

	//	self_t &operator=(self_t &&other) noexcept
	//	{
	//		m_impl = std::move(other.m_impl);
	//		return *this;
	//	}

	//	~bio_poller() = default;

	//	_IOContext* get()
	//	{
	//		return m_impl->get();
	//	}

	//	_IOContext* get(size_t ms)
	//	{
	//		return m_impl->get(ms);
	//	}

	//	void push(_IOContext* p)
	//	{
	//		return m_impl->post(p);
	//	}
	//	
	//	bool operator==(const self_t&& other) const
	//	{
	//		return m_impl == other.m_impl;
	//	}

	//	operator bool() const
	//	{
	//		return (bool)m_impl;
	//	}

	//private:
	//	impl_t m_impl;
	//};

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

	void _Fprintf(FILE *stream,stdx::string&& format, std::initializer_list<stdx::string>&& list);

	void _Plogf(stdx::string&& format, std::initializer_list<stdx::string>&& list);

	template<typename ..._Args>
	void printf(const stdx::string& format, const _Args&...args)
	{
		stdx::string _format(format);
		_Fprintf(stdout,std::move(_format),std::move(std::initializer_list<stdx::string>{ stdx::to_string(args)... }));
	}

	template<typename ..._Args>
	void printf(stdx::string &&format, const _Args&...args)
	{
		_Fprintf(stdout,std::move(format), std::move(std::initializer_list<stdx::string>{ stdx::to_string(args)... }));
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