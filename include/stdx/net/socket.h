#pragma once
#include <stdx/async/task.h>
#include <stdx/async/spin_lock.h>
#include <stdx/io.h>
#include <stdx/env.h>
#include <stdx/string.h>
#ifdef WIN32
#include <WinSock2.h>
#include <MSWSock.h>
#pragma comment(lib,"Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#endif 

#ifdef WIN32
#define _STDX_HAS_SOCKET
#define _ThrowWinError auto _ERROR_CODE = GetLastError(); \
						LPVOID _MSG;\
						if(_ERROR_CODE != ERROR_IO_PENDING) \
						{ \
							if(FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,NULL,_ERROR_CODE,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),(LPTSTR) &_MSG,0,NULL))\
							{ \
								throw std::runtime_error((char*)_MSG);\
							}else \
							{ \
								std::string _ERROR_MSG("windows system error:");\
								_ERROR_MSG.append(std::to_string(_ERROR_CODE));\
								throw std::system_error(std::error_code(_ERROR_CODE,std::system_category()),_ERROR_MSG.c_str()); \
							} \
						}\

#define _ThrowWSAError 	auto _ERROR_CODE = WSAGetLastError(); \
						if(_ERROR_CODE != WSA_IO_PENDING)\
						{\
							std::string _ERROR_STR("windows WSA error:");\
							_ERROR_STR.append(std::to_string(_ERROR_CODE));\
							throw std::system_error(std::error_code(_ERROR_CODE,std::system_category()),_ERROR_STR.c_str());\
						}\

namespace stdx
{
#pragma region Starter
	struct _WSAStarter
	{
		WSAData wsa;
		_WSAStarter() noexcept
			:wsa()
		{
			if (WSAStartup(MAKEWORD(2, 2), &wsa))
			{
				//致命错误
				std::terminate();
			}
		}
		~_WSAStarter() noexcept
		{
			if (WSACleanup())
			{
				//致命错误
				std::terminate();
			}
		}
	};
	extern _WSAStarter _wsastarter;
#pragma endregion

#pragma region Enum

	enum class protocol :int
	{
		ip = IPPROTO_IP,	//IP
		tcp = IPPROTO_TCP,	//TCP
		udp = IPPROTO_UDP	//UDP
	};

	enum class socket_type :int
	{
		raw = SOCK_RAW,			//原始套接字
		stream = SOCK_STREAM,	//流式套接字
		dgram = SOCK_DGRAM		//数据包套接字
	};
	enum class addr_family :int
	{
		ip = AF_INET,		//IPv4
		ipv6 = AF_INET6		//IPv6
	};

	int forward_protocol(const stdx::protocol &protocol);

	int forward_socket_type(const stdx::socket_type &sock_type);

	int forward_addr_family(const stdx::addr_family &addr_family);
#pragma endregion

#pragma region StructDef
	class network_addr
	{
	public:
		network_addr()
		{
			::memset(&m_handle, 0, sizeof(SOCKADDR_IN));
		}
		network_addr(unsigned long ip, const uint_16 &port)
		{
			m_handle.sin_family =stdx::forward_addr_family(addr_family::ip);
			m_handle.sin_addr.S_un.S_addr = ip;
			m_handle.sin_port = htons(port);
		}
		network_addr(const char *ip, const uint_16 &port)
			:network_addr(inet_addr(ip), port)
		{}
		network_addr(const network_addr &other)
			:m_handle(other.m_handle)
		{}
		network_addr(const SOCKADDR_IN &addr)
			:m_handle(addr)
		{}
		~network_addr() = default;
		operator SOCKADDR_IN* ()
		{
			return &m_handle;
		}

		operator sockaddr*()
		{
			return (sockaddr*)&m_handle;
		}

		network_addr &operator=(const network_addr &other)
		{
			m_handle = other.m_handle;
			return *this;
		}

		network_addr &operator=(network_addr &&other)
		{
			m_handle = other.m_handle;
			return *this;
		}

		const static int addr_len = sizeof(sockaddr);
		network_addr &port(const uint_16 &port)
		{
			m_handle.sin_port = htons(port);
			return *this;
		}
		uint_16 port() const
		{
			return ntohs(m_handle.sin_port);
		}

		template<typename _String = std::string,class = typename  std::enable_if<stdx::is_basic_string<_String>::value>::type>
		_String ip() const
		{
			using char_t = typename _String::value_type;
			const char* val = inet_ntoa(m_handle.sin_addr);
			return _String((char_t*)val);
		}

		network_addr &ip(const char *ip)
		{

			m_handle.sin_addr.S_un.S_addr = inet_addr(ip);
			return *this;
		}

		bool operator==(const stdx::network_addr &other) const
		{
			return (ip() == other.ip())&&(port() == other.port());
		}
	private:
		SOCKADDR_IN m_handle;
	};

	struct network_io_context
	{
		network_io_context()
		{
			std::memset(&m_ol, 0, sizeof(OVERLAPPED));
		}
		~network_io_context() = default;
		WSAOVERLAPPED m_ol;
		SOCKET this_socket;
		network_addr addr;
		WSABUF buffer;
		DWORD size;
		SOCKET target_socket;
		std::function <void(network_io_context*, std::exception_ptr)> *callback;
	};

	struct network_send_event
	{
		network_send_event()
			:sock(INVALID_SOCKET)
			, size(0)
		{}
		~network_send_event() = default;
		network_send_event(const network_send_event &other)
			:sock(other.sock)
			, size(other.size)
		{}
		network_send_event(network_send_event &&other)
			:sock(std::move(other.sock))
			, size(std::move(other.size))
		{}
		network_send_event &operator=(const network_send_event &other)
		{
			sock = other.sock;
			size = other.size;
			return *this;
		}
		network_send_event(network_io_context *ptr)
			:sock(ptr->this_socket)
			, size(ptr->size)
		{}
		SOCKET sock;
		size_t size;
	};

	struct network_recv_event
	{
		network_recv_event()
			:sock(INVALID_SOCKET)
			, buffer(0, nullptr)
			, size(0)
			, addr()
		{}
		~network_recv_event() = default;
		network_recv_event(const network_recv_event &other)
			:sock(other.sock)
			, buffer(other.buffer)
			, size(other.size)
			, addr(other.addr)
		{}
		network_recv_event(network_recv_event &&other)
			:sock(std::move(other.sock))
			, buffer(other.buffer)
			, size(other.size)
			,addr(other.addr)
		{}
		network_recv_event &operator=(const network_recv_event &other)
		{
			sock = other.sock;
			buffer = other.buffer;
			size = other.size;
			addr = other.addr;
			return *this;
		}
		network_recv_event(network_io_context *ptr)
			:sock(ptr->target_socket)
			,buffer(ptr->buffer.len, ptr->buffer.buf)
			,size(ptr->size)
			,addr(ptr->addr)
		{}
		SOCKET sock;
		stdx::buffer buffer;
		size_t size;
		stdx::network_addr addr;
	};
#pragma endregion


	

	//struct network_accept_event
	//{
	//	network_accept_event() = default;
	//	~network_accept_event() = default;
	//	network_accept_event(const network_accept_event &other)
	//		:accept(other.accept)
	//		,buffer(other.buffer)
	//		,size(other.size)
	//		,addr(other.addr)
	//	{}
	//	network_accept_event &operator=(const network_accept_event &other)
	//	{
	//		accept = other.accept;
	//		buffer = other.buffer;
	//		size = other.size;
	//		addr = other.addr;
	//		return *this;
	//	}
	//	network_accept_event(network_io_context *ptr)
	//		:accept(ptr->target_socket)
	//		,buffer(ptr->buffer.len-((sizeof(sockaddr)+16)*2),(ptr->buffer.buf+(sizeof(sockaddr)+16)*2))
	//		,size(ptr->size)
	//		,addr(ptr->addr)
	//	{}
	//	SOCKET accept;
	//	stdx::buffer buffer;
	//	size_t size;
	//	network_addr addr;
	//};

#pragma region IO_SERVICE
	class _NetworkIOService
	{
	public:
		using iocp_t = stdx::iocp<network_io_context>;
		_NetworkIOService();

		delete_copy(_NetworkIOService);

		~_NetworkIOService();

		SOCKET create_socket(const int &addr_family, const int &sock_type, const int &protocol);

		SOCKET create_wsasocket(const int &addr_family, const int &sock_type, const int &protocol);

		//发送数据
		void send(SOCKET sock, const char* data, const size_t &size, std::function<void(network_send_event, std::exception_ptr)> callback);

		void send_file(SOCKET sock, HANDLE file_with_cache, std::function<void(std::exception_ptr)> callback);

		//接收数据
		void recv(SOCKET sock, const size_t &size, std::function<void(network_recv_event, std::exception_ptr)> callback);

		void connect(SOCKET sock, stdx::network_addr &addr);

		SOCKET accept(SOCKET sock, network_addr &addr);

		SOCKET accept(SOCKET sock);

		void listen(SOCKET sock, int backlog);

		void bind(SOCKET sock, network_addr &addr);

		void send_to(SOCKET sock, const network_addr &addr, const char *data, const size_t &size, std::function<void(stdx::network_send_event, std::exception_ptr)> callback);

		void recv_from(SOCKET sock,const size_t &size, std::function<void(network_recv_event, std::exception_ptr)> callback);

		void close(SOCKET sock);

		network_addr get_local_addr(SOCKET sock) const;

		network_addr get_remote_addr(SOCKET sock) const;

		//void _GetAcceptEx(SOCKET s, LPFN_ACCEPTEX *ptr)
		//{
		//	GUID id = WSAID_ACCEPTEX;
		//	DWORD buf;
		//	if (WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &id, sizeof(id), ptr, sizeof(LPFN_ACCEPTEX), &buf, NULL, NULL)==SOCKET_ERROR)
		//	{
		//		_ThrowWSAError
		//	}
		//}
		//void _GetAcceptExSockaddr(SOCKET s, LPFN_GETACCEPTEXSOCKADDRS *ptr)
		//{
		//	GUID id = WSAID_GETACCEPTEXSOCKADDRS;
		//	DWORD buf;
		//	if (WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &id, sizeof(id), ptr, sizeof(LPFN_GETACCEPTEXSOCKADDRS), &buf, NULL, NULL)==SOCKET_ERROR)
		//	{
		//		_ThrowWSAError
		//	}
		//}
		//network_addr _GetSocketAddrEx(SOCKET sock,void *buffer,const size_t &size)
		//{
		//	if (!get_addr_ex)
		//	{
		//		_GetAcceptExSockaddr(sock,&get_addr_ex);
		//	}
		//	network_addr local;
		//	network_addr remote;
		//	auto local_ptr = (sockaddr*)local;
		//	auto remote_ptr = (sockaddr*)remote;
		//	DWORD len = sizeof(sockaddr);
		//	get_addr_ex(buffer, size, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,&local_ptr,(int*)&len,&remote_ptr,(int*)&len);
		//	return remote;
		//}
		//void _AcceptEx(SOCKET sock,const size_t &buffer_size,std::function<void(network_accept_event,std::exception_ptr)> &&callback,DWORD addr_family= stdx::addr_family::ip,DWORD socket_type=stdx::socket_type::stream,DWORD protocol = stdx::protocol::tcp)
		//{
		//	if (!accept_ex)
		//	{
		//		_GetAcceptEx(sock, &accept_ex);
		//	}
		//	network_io_context *context = new network_io_context;
		//	context->target_socket = WSASocket(addr_family, socket_type, protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
		//	context->buffer.len = buffer_size+ ((sizeof(sockaddr_in) + 16) * 2);
		//	context->buffer.buf = (char*)std::calloc(sizeof(char), context->buffer.len);
		//	context->this_socket = sock;
		//	auto *call = new std::function <void(network_io_context*, std::exception_ptr)>;
		//	*call = [callback,this,buffer_size](network_io_context *context_ptr, std::exception_ptr error)
		//	{
		//		if (error)
		//		{
		//			std::free(context_ptr->buffer.buf);
		//			delete context_ptr;
		//			callback(network_accept_event(), error);
		//			return;
		//		}
		//		context_ptr->addr = _GetSocketAddrEx(context_ptr->this_socket,(void*)context_ptr->buffer.buf,buffer_size);
		//		network_accept_event context(context_ptr);
		//		delete context_ptr;
		//		callback(context, nullptr);
		//	};
		//	if (!accept_ex(sock,context->target_socket,context->buffer.buf,buffer_size, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,&(context->size),&(context->m_ol)))
		//	{
		//		_ThrowWSAError
		//	}
		//	stdx::threadpool::run([](iocp_t iocp)
		//	{
		//		auto *context_ptr = iocp.get();
		//		std::exception_ptr error(nullptr);
		//		try
		//		{
		//			DWORD flag = 0;
		//			if (!WSAGetOverlappedResult(context_ptr->this_socket, &(context_ptr->m_ol), &(context_ptr->size), false, &flag))
		//			{
		//				//在这里出错
		//				_ThrowWSAError
		//			}
		//		}
		//		catch (const std::exception&)
		//		{
		//			error = std::current_exception();
		//		}
		//		auto *call = context_ptr->callback;
		//		try
		//		{
		//			(*call)(context_ptr, error);
		//		}
		//		catch (const std::exception&)
		//		{
		//		}
		//		delete call;
		//	}, m_iocp);
		//}
	private:
		iocp_t m_iocp;
		static DWORD recv_flag;
		std::shared_ptr<bool> m_alive;
		//static LPFN_ACCEPTEX accept_ex;
		//static LPFN_GETACCEPTEXSOCKADDRS get_addr_ex;
		void init_threadpoll() noexcept;
	};

	class network_io_service
	{
		using iocp_t = _NetworkIOService::iocp_t;
		using impl_t = std::shared_ptr<_NetworkIOService>;
	public:
		network_io_service()
			:m_impl(std::make_shared<_NetworkIOService>())
		{}

		//network_io_service(const iocp_t &iocp)
		//	:m_impl(std::make_shared<_NetworkIOService>(iocp))
		//{}

		network_io_service(const network_io_service &other)
			:m_impl(other.m_impl)
		{}

		network_io_service(network_io_service &&other)
			:m_impl(std::move(other.m_impl))
		{}

		network_io_service &operator=(const network_io_service &other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		~network_io_service() = default;

		SOCKET create_socket(const int &addr_family, const int &sock_type, const int &protocol)
		{
			return m_impl->create_wsasocket(addr_family, sock_type, protocol);
		}

		void send(SOCKET sock, const char* data, const size_t &size, std::function<void(network_send_event, std::exception_ptr)> &&callback)
		{
			m_impl->send(sock, data, size, std::move(callback));
		}

		void send_file(SOCKET sock, HANDLE file_with_cache, std::function<void(std::exception_ptr)> &&callback)
		{
			m_impl->send_file(sock, file_with_cache,callback);
		}

		void recv(SOCKET sock, const size_t &size, std::function<void(network_recv_event, std::exception_ptr)> &&callback)
		{
			m_impl->recv(sock, size, callback);
		}

		void connect(SOCKET sock, stdx::network_addr &addr)
		{
			m_impl->connect(sock, addr);
		}

		SOCKET accept(SOCKET sock, network_addr &addr)
		{
			return m_impl->accept(sock, addr);
		}

		SOCKET accept(SOCKET sock)
		{
			return m_impl->accept(sock);
		}

		void listen(SOCKET sock, int backlog)
		{
			m_impl->listen(sock, backlog);
		}

		void bind(SOCKET sock, network_addr &addr)
		{
			m_impl->bind(sock, addr);
		}

		void send_to(SOCKET sock, const network_addr &addr, const char *data, const size_t &size, std::function<void(stdx::network_send_event, std::exception_ptr)> &&callback)
		{
			m_impl->send_to(sock, addr, data, size, std::move(callback));
		}

		void recv_from(SOCKET sock,const size_t &size, std::function<void(network_recv_event, std::exception_ptr)> &&callback)
		{
			m_impl->recv_from(sock,size, callback);
		}

		void close(SOCKET sock)
		{
			m_impl->close(sock);
		}

		network_addr get_local_addr(SOCKET sock) const
		{
			return m_impl->get_local_addr(sock);
		}

		network_addr get_remote_addr(SOCKET sock) const
		{
			return m_impl->get_remote_addr(sock);
		}
		operator bool() const
		{
			return (bool)m_impl;
		}

		bool operator==(const network_io_service &other) const
		{
			return m_impl == other.m_impl;
		}
	private:
		impl_t m_impl;
	};
#pragma endregion

#pragma region Socket
	class _Socket
	{
		using io_service_t = network_io_service;
	public:
		//已弃用
		//explicit _Socket(const io_service_t &io_service, const int &addr_family, const int &sock_type, const int &protocol)
		//	:m_io_service(io_service)
		//	, m_handle(m_io_service.create_socket(addr_family, sock_type, protocol))
		//{}

		explicit _Socket(const io_service_t &io_service, SOCKET s);

		explicit _Socket(const io_service_t &io_service);

		delete_copy(_Socket);

		~_Socket();

		void init(const int &addr_family, const int &sock_type, const int &protocol)
		{
			m_handle = m_io_service.create_socket(addr_family, sock_type, protocol);
		}


		stdx::task<stdx::network_send_event> send(const char *data, const size_t &size);

		stdx::task<void> send_file(HANDLE file_handle);


		stdx::task<stdx::network_send_event> send_to(const network_addr &addr, const char *data, const size_t &size);


		stdx::task<stdx::network_recv_event> recv(const size_t &size);


		stdx::task<stdx::network_recv_event> recv_from(const size_t &size);

		void bind(network_addr &addr)
		{
			m_io_service.bind(m_handle, addr);
		}

		void listen(int backlog)
		{
			m_io_service.listen(m_handle, backlog);
		}

		SOCKET accept(network_addr &addr)
		{
			return m_io_service.accept(m_handle, addr);
		}

		SOCKET accept()
		{
			return m_io_service.accept(m_handle);
		}

		void close();

		void connect(network_addr &addr)
		{
			m_io_service.connect(m_handle, addr);
		}

		io_service_t io_service() const
		{
			return m_io_service;
		}

		network_addr local_addr() const
		{
			return m_io_service.get_local_addr(m_handle);
		}

		network_addr remote_addr() const
		{
			return m_io_service.get_remote_addr(m_handle);
		}

		//返回true则继续
		template<typename _Fn>
		void recv_utill(const size_t &size, _Fn call)
		{
			static_assert(is_arguments_type(_Fn, stdx::task_result<stdx::network_recv_event>) | is_arguments_type(_Fn, stdx::task_result<stdx::network_recv_event>&) | is_arguments_type(_Fn, const stdx::task_result<stdx::network_recv_event>&) | is_arguments_type(_Fn, stdx::task_result<stdx::network_recv_event> &&), "the input function not be allowed");
			static_assert(is_result_type(_Fn, bool), "the input function not be allowed");
			this->recv(size).then([this, size, call](stdx::task_result<network_recv_event> r) mutable
			{
				if (stdx::invoke(call, r))
				{
					recv_utill(size, call);
				}
			});
		}

		template<typename _Fn, typename _ErrHandler>
		void recv_utill_error(const size_t &size, _Fn call, _ErrHandler err_handler)
		{
			static_assert(is_arguments_type(_Fn, stdx::network_recv_event) | is_arguments_type(_Fn, stdx::network_recv_event&) | is_arguments_type(_Fn, const stdx::network_recv_event&) | is_arguments_type(_Fn, stdx::network_recv_event&&), "the input function not be allowed");
			return this->recv_utill(size, [call, err_handler](stdx::task_result<network_recv_event> &r) mutable
			{
				try
				{
					stdx::network_recv_event e = r.get();
					stdx::invoke(call, std::move(e));
				}
				catch (const std::exception &)
				{
					stdx::invoke(err_handler, std::current_exception());
					return false;
				}
				return true;
			});
		}

	private:
		io_service_t m_io_service;
		SOCKET m_handle;
	};

	class socket
	{
		using impl_t = std::shared_ptr<_Socket>;
		using self_t = socket;
		using io_service_t = network_io_service;
	public:
		//已弃用
		//socket(const io_service_t &io_service, const int &addr_family, const int &sock_type, const int &protocol)
		//	:m_impl(std::make_shared<_Socket>(io_service, addr_family, sock_type, protocol))
		//{}

		socket(const io_service_t &io_service)
			:m_impl(std::make_shared<_Socket>(io_service))
		{}
		socket(const self_t &other)
			:m_impl(other.m_impl)
		{}
		socket(self_t &&other)
			:m_impl(std::move(other.m_impl))
		{}
		self_t &operator=(const self_t &other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		void init(const int &addr_family, const int &sock_type, const int &protocol)
		{
			return m_impl->init(addr_family, sock_type, protocol);
		}

		void bind(network_addr &addr)
		{
			m_impl->bind(addr);
		}

		void listen(int backlog)
		{
			m_impl->listen(backlog);
		}

		self_t accept(network_addr &addr)
		{
			SOCKET s = m_impl->accept(addr);
			return socket(m_impl->io_service(), s);
		}

		self_t accept()
		{
			SOCKET s = m_impl->accept();
			return socket(m_impl->io_service(), s);
		}

		void close()
		{
			m_impl->close();
		}

		void connect(network_addr &addr)
		{
			m_impl->connect(addr);
		}

		network_addr local_addr() const
		{
			return m_impl->local_addr();
		}

		network_addr remote_addr() const
		{
			return m_impl->remote_addr();
		}

		stdx::task<network_send_event> send(const char *data, const size_t &size)
		{
			return m_impl->send(data, size);
		}

		stdx::task<void> send_file(HANDLE file_with_cache)
		{
			return m_impl->send_file(file_with_cache);
		}

		stdx::task<network_send_event> send_to(const network_addr &addr, const char *data, const size_t &size)
		{
			return m_impl->send_to(addr, data, size);
		}

		stdx::task<network_recv_event> recv(const size_t &size)
		{
			return m_impl->recv(size);
		}

		stdx::task<network_recv_event> recv_from(const size_t &size)
		{
			return m_impl->recv_from(size);
		}

		template<typename _Fn>
		void recv_utill(const size_t &size, _Fn &&call)
		{
			return m_impl->recv_utill(size,call);
		}

		template<typename _Fn, typename _ErrHandler>
		void recv_utill_error(const size_t &size, _Fn &&call, _ErrHandler &&err_handler)
		{
			return m_impl->recv_utill_error(size,call,err_handler);
		}

		bool operator==(const stdx::socket &other) const
		{
			return m_impl == other.m_impl;
		}
	private:
		impl_t m_impl;

		socket(const io_service_t &io_service, SOCKET s)
			:m_impl(std::make_shared<_Socket>(io_service, s))
		{}
	};
#pragma endregion
}

#pragma region APIs
namespace stdx
{
	extern stdx::socket open_socket(const stdx::network_io_service &io_service, const int &addr_family, const int &sock_type, const int &protocol);
	extern stdx::socket open_socket(const stdx::network_io_service &io_service, const stdx::addr_family &addr_family, const stdx::socket_type &sock_type, const stdx::protocol &protocol);
	extern stdx::socket open_tcpsocket(const stdx::network_io_service &io_service);
	extern stdx::socket open_udpsocket(const stdx::network_io_service &io_service);
}
#pragma endregion
#endif //Win32

#ifdef LINUX
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<sys/sendfile.h>
#define _STDX_HAS_SOCKET
#define _ThrowLinuxError auto _ERROR_CODE = errno;\
						 throw std::system_error(std::error_code(_ERROR_CODE,std::system_category()),strerror(_ERROR_CODE)); \

namespace stdx
{
#pragma region Enum
	enum class protocol :int
	{
		ip = IPPROTO_IP,	//IP
		tcp = IPPROTO_TCP,	//TCP
		udp = IPPROTO_UDP	//UDP
	};

	enum class socket_type :int
	{
		raw = SOCK_RAW,			//原始套接字
		stream = SOCK_STREAM,	//流式套接字
		dgram = SOCK_DGRAM		//数据包套接字
	};
	enum class addr_family :int
	{
		ip = AF_INET,		//IPv4
		ipv6 = AF_INET6		//IPv6
	};

	int forward_protocol(const stdx::protocol &protocol);

	int forward_socket_type(const stdx::socket_type &sock_type);

	int forward_addr_family(const stdx::addr_family &addr_family);
#pragma endregion

#pragma region StructDef
	class network_addr
	{
	public:
		network_addr()
		{
			::memset(&m_handle,0,sizeof(m_handle));
		}
		network_addr(unsigned long ip, const uint_16 &port)
		{
			m_handle.sin_family = stdx::forward_addr_family(addr_family::ip);
			m_handle.sin_addr.s_addr = ip;
			m_handle.sin_port = htons(port);
		}
		network_addr(const char *ip, const uint_16 &port)
			:network_addr(inet_addr(ip), port)
		{}
		network_addr(const network_addr &other)
			:m_handle(other.m_handle)
		{}

		network_addr(const sockaddr_in &addr)
			:m_handle(addr)
		{}

		~network_addr() = default;
		operator sockaddr_in* ()
		{
			return &m_handle;
		}

		operator sockaddr*() const
		{
			return (sockaddr*)&m_handle;
		}

		network_addr &operator=(const network_addr &other)
		{
			m_handle = other.m_handle;
			return *this;
		}

		network_addr &operator=(network_addr &&other)
		{
			m_handle = other.m_handle;
			return *this;
		}

		const static int addr_len = sizeof(sockaddr);
		network_addr &port(const uint_16 &port)
		{
			m_handle.sin_port = htons(port);
			return *this;
		}
		uint_16 port() const
		{
			return ntohs(m_handle.sin_port);
		}

		template<typename _String = std::string,class = typename  std::enable_if<stdx::is_basic_string<_String>::value>::type>
		_String ip() const
		{
			using char_t = typename _String::value_type;
			const char* val = inet_ntoa(m_handle.sin_addr);
			return _String((char_t*)val);
		}

		network_addr &ip(const char *ip)
		{
			m_handle.sin_addr.s_addr = inet_addr(ip);
			return *this;
		}

		bool operator==(const stdx::network_addr &other) const
		{
			return (ip() == other.ip()) && (port() == other.port());
		}
	private:
		sockaddr_in m_handle;
	};

	struct network_io_context
	{
		network_io_context() = default;
		~network_io_context() = default;
		int code;
		int this_socket;
		network_addr addr;
		char *buffer;
		size_t buffer_size;
		size_t size;
		int target_socket;
		std::function <void(network_io_context*, std::exception_ptr)> *callback;
	};

	struct network_io_context_code
	{
		enum
		{
			recv = 0,
			recvfrom = 1,
			send = 2,
			sendto = 3
		};
	};

	struct network_send_event
	{
		network_send_event()
			:sock(-1)
			, size(0)
		{}
		~network_send_event() = default;
		network_send_event(const network_send_event &other)
			:sock(other.sock)
			, size(other.size)
		{}
		network_send_event(network_send_event &&other)
			:sock(std::move(other.sock))
			, size(std::move(other.size))
		{}
		network_send_event &operator=(const network_send_event &other)
		{
			sock = other.sock;
			size = other.size;
			return *this;
		}
		network_send_event(network_io_context *ptr)
			:sock(ptr->this_socket)
			, size(ptr->size)
		{}
		int sock;
		size_t size;
	};

	struct network_recv_event
	{
		network_recv_event()
			:sock(-1)
			,buffer(0, nullptr)
			,size(0)
			,addr()
		{}
		~network_recv_event() = default;
		network_recv_event(const network_recv_event &other)
			:sock(other.sock)
			, buffer(other.buffer)
			, size(other.size)
			, addr(other.addr)
		{}
		network_recv_event(network_recv_event &&other)
			:sock(std::move(other.sock))
			,buffer(other.buffer)
			,size(other.size)
			,addr(other.addr)
		{}
		network_recv_event &operator=(const network_recv_event &other)
		{
			sock = other.sock;
			buffer = other.buffer;
			size = other.size;
			addr = other.addr;
			return *this;
		}
		network_recv_event(network_io_context *ptr)
			:sock(ptr->target_socket)
			,buffer(ptr->buffer_size, ptr->buffer)
			,size(ptr->size)
			,addr(ptr->addr)
		{}
		int sock;
		stdx::buffer buffer;
		size_t size;
		stdx::network_addr addr;
	};
#pragma endregion


	struct network_io_context_finder
	{
		static int find(epoll_event *ev);
	};

#pragma region IO_SERVICE
	class _NetworkIOService
	{
	public:
		_NetworkIOService()
			:m_reactor()
			, m_alive(std::make_shared<bool>(true))
		{
			init_threadpoll();
		}
		~_NetworkIOService()
		{
			*m_alive = false;
		}
		int create_socket(const int &addr_family, const int &sock_type, const int &protocol);

		void send(int sock, const char* data,size_t size, std::function<void(network_send_event, std::exception_ptr)> callback);

		void send_file(int sock, int file_handle, std::function<void(std::exception_ptr)> callback);

		void recv(int sock, const size_t &size, std::function<void(network_recv_event, std::exception_ptr)> callback);

		void connect(int sock, stdx::network_addr &addr);

		int accept(int sock, network_addr &addr);

		int accept(int sock);

		void listen(int sock, int backlog);

		void bind(int sock, network_addr &addr);

		void send_to(int sock, const network_addr &addr, const char *data,size_t size, std::function<void(stdx::network_send_event, std::exception_ptr)> callback);

		void recv_from(int sock, const size_t &size, std::function<void(network_recv_event, std::exception_ptr)> callback);

		void close(int sock);

		network_addr get_local_addr(int sock) const;

		network_addr get_remote_addr(int sock) const;
	private:
		stdx::reactor m_reactor;
		std::shared_ptr<bool> m_alive;
		void init_threadpoll() noexcept;
	};

	class network_io_service
	{
		using impl_t = std::shared_ptr<_NetworkIOService>;
	public:
		network_io_service()
			:m_impl(std::make_shared<_NetworkIOService>())
		{}

		network_io_service(const network_io_service &other)
			:m_impl(other.m_impl)
		{}

		network_io_service(network_io_service &&other)
			:m_impl(std::move(other.m_impl))
		{}

		network_io_service &operator=(const network_io_service &other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		~network_io_service() = default;

		int create_socket(const int &addr_family, const int &sock_type, const int &protocol)
		{
			return m_impl->create_socket(addr_family, sock_type, protocol);
		}

		void send(int sock, const char* data, const size_t &size, std::function<void(network_send_event, std::exception_ptr)> &&callback)
		{
			m_impl->send(sock, data, size, std::move(callback));
		}

		void send_file(int sock, int file_handle, std::function<void(std::exception_ptr)> &&callback)
		{
			m_impl->send_file(sock, file_handle, std::move(callback));
		}

		void recv(int sock, const size_t &size, std::function<void(network_recv_event, std::exception_ptr)> &&callback)
		{
			m_impl->recv(sock, size, std::move(callback));
		}

		void connect(int sock, stdx::network_addr &addr)
		{
			m_impl->connect(sock, addr);
		}

		int accept(int sock, network_addr &addr)
		{
			return m_impl->accept(sock, addr);
		}

		int accept(int sock)
		{
			return m_impl->accept(sock);
		}

		void listen(int sock, int backlog)
		{
			m_impl->listen(sock, backlog);
		}

		void bind(int sock, network_addr &addr)
		{
			m_impl->bind(sock, addr);
		}

		void send_to(int sock, const network_addr &addr, const char *data, const size_t &size, std::function<void(stdx::network_send_event, std::exception_ptr)> &&callback)
		{
			m_impl->send_to(sock, addr, data, size, std::move(callback));
		}

		void recv_from(int sock, const size_t &size, std::function<void(network_recv_event, std::exception_ptr)> &&callback)
		{
			m_impl->recv_from(sock,size,std::move(callback));
		}

		void close(int sock)
		{
			m_impl->close(sock);
		}

		network_addr get_local_addr(int sock) const
		{
			return m_impl->get_local_addr(sock);
		}

		network_addr get_remote_addr(int sock) const
		{
			return m_impl->get_remote_addr(sock);
		}
		operator bool() const
		{
			return (bool)m_impl;
		}

		bool operator==(const network_io_service &other) const
		{
			return m_impl == other.m_impl;
		}

	private:
		impl_t m_impl;
	};
#pragma endregion

#pragma region Socket
	class _Socket
	{
		using io_service_t = network_io_service;
	public:
		explicit _Socket(const io_service_t &io_service, int s);

		explicit _Socket(const io_service_t &io_service);

		delete_copy(_Socket);

		~_Socket();

		void init(const int &addr_family, const int &sock_type, const int &protocol)
		{
			m_handle = m_io_service.create_socket(addr_family, sock_type, protocol);
		}

		stdx::task<stdx::network_send_event> send(const char *data, const size_t &size);

		stdx::task<void> send_file(int file_handle);


		stdx::task<stdx::network_send_event> send_to(const network_addr &addr, const char *data, const size_t &size);

		stdx::task<stdx::network_recv_event> recv(const size_t &size);


		stdx::task<stdx::network_recv_event> recv_from(const size_t &size);

		void bind(network_addr &addr)
		{
			m_io_service.bind(m_handle, addr);
		}

		void listen(int backlog)
		{
			m_io_service.listen(m_handle, backlog);
		}

		int accept(network_addr &addr)
		{
			return m_io_service.accept(m_handle, addr);
		}

		int accept()
		{
			return m_io_service.accept(m_handle);
		}

		void close();

		void connect(network_addr &addr)
		{
			m_io_service.connect(m_handle, addr);
		}

		io_service_t io_service() const
		{
			return m_io_service;
		}

		network_addr local_addr() const
		{
			return m_io_service.get_local_addr(m_handle);
		}

		network_addr remote_addr() const
		{
			return m_io_service.get_remote_addr(m_handle);
		}

		//返回true则继续
		template<typename _Fn>
		void recv_utill(const size_t &size, _Fn &&call)
		{
			static_assert(is_arguments_type(_Fn, stdx::task_result<stdx::network_recv_event>) | is_arguments_type(_Fn, stdx::task_result<stdx::network_recv_event>&) | is_arguments_type(_Fn, const stdx::task_result<stdx::network_recv_event>&) | is_arguments_type(_Fn, stdx::task_result<stdx::network_recv_event> &&), "the input function not be allowed");
			static_assert(is_result_type(_Fn, bool), "the input function not be allowed");
			this->recv(size).then([this, size, call](stdx::task_result<network_recv_event> r) mutable
			{
				if (stdx::invoke(call, r))
				{
					recv_utill(size, call);
				}
			});
		}

		template<typename _Fn, typename _ErrHandler>
		void recv_utill_error(const size_t &size, _Fn &&call, _ErrHandler &&err_handler)
		{
			static_assert(is_arguments_type(_Fn, stdx::network_recv_event) | is_arguments_type(_Fn, stdx::network_recv_event&) | is_arguments_type(_Fn, const stdx::network_recv_event&) | is_arguments_type(_Fn, stdx::network_recv_event&&), "the input function not be allowed");
			return this->recv_utill(size, [call, err_handler](stdx::task_result<network_recv_event> r) mutable
			{
				try
				{
					auto e = r.get();
					stdx::invoke(call, e);
				}
				catch (const std::exception&)
				{
					stdx::invoke(err_handler, std::current_exception());
					return false;
				}
				return true;
			});
		}
	private:
		io_service_t m_io_service;
		int m_handle;
	};

	class socket
	{
		using impl_t = std::shared_ptr<_Socket>;
		using self_t = socket;
		using io_service_t = network_io_service;
	public:
		//已弃用
		//socket(const io_service_t &io_service, const int &addr_family, const int &sock_type, const int &protocol)
		//	:m_impl(std::make_shared<_Socket>(io_service, addr_family, sock_type, protocol))
		//{}

		socket(const io_service_t &io_service)
			:m_impl(std::make_shared<_Socket>(io_service))
		{}
		socket(const self_t &other)
			:m_impl(other.m_impl)
		{}
		socket(self_t &&other)
			:m_impl(std::move(other.m_impl))
		{}
		self_t &operator=(const self_t &other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		void init(const int &addr_family, const int &sock_type, const int &protocol)
		{
			return m_impl->init(addr_family, sock_type, protocol);
		}

		void bind(network_addr &addr)
		{
			m_impl->bind(addr);
		}

		void listen(int backlog)
		{
			m_impl->listen(backlog);
		}

		self_t accept(network_addr &addr)
		{
			int s = m_impl->accept(addr);
			return socket(m_impl->io_service(), s);
		}

		self_t accept()
		{
			int s = m_impl->accept();
			return socket(m_impl->io_service(), s);
		}

		void close()
		{
			m_impl->close();
		}

		void connect(network_addr &addr)
		{
			m_impl->connect(addr);
		}

		network_addr local_addr() const
		{
			return m_impl->local_addr();
		}

		network_addr remote_addr() const
		{
			return m_impl->remote_addr();
		}

		stdx::task<network_send_event> send(const char *data, const size_t &size)
		{
			return m_impl->send(data, size);
		}

		stdx::task<void> send_file(int file_handle)
		{
			return m_impl->send_file(file_handle);
		}

		stdx::task<network_send_event> send_to(const network_addr &addr, const char *data, const size_t &size)
		{
			return m_impl->send_to(addr, data, size);
		}

		stdx::task<network_recv_event> recv(const size_t &size)
		{
			return m_impl->recv(size);
		}

		stdx::task<network_recv_event> recv_from(const size_t &size)
		{
			return m_impl->recv_from(size);
		}

		template<typename _Fn>
		void recv_utill(const size_t &size, _Fn &&call)
		{
			return m_impl->recv_utill(size, std::move(call));
		}

		template<typename _Fn, typename _ErrHandler>
		void recv_utill_error(const size_t &size, _Fn &&call, _ErrHandler &&err_handler)
		{
			return m_impl->recv_utill_error(size, std::move(call), std::move(err_handler));
		}

		bool operator==(const socket &other) const
		{
			return m_impl == other.m_impl;
		}

	private:
		impl_t m_impl;

		socket(const io_service_t &io_service, int s)
			:m_impl(std::make_shared<_Socket>(io_service, s))
		{}
	};
#pragma endregion

	
}
#pragma region APIs
namespace stdx
{
	extern stdx::socket open_socket(const stdx::network_io_service &io_service, const int &addr_family, const int &sock_type, const int &protocol);
	extern stdx::socket open_socket(const stdx::network_io_service &io_service, const stdx::addr_family &addr_family, const stdx::socket_type &sock_type, const stdx::protocol &protocol);
	extern stdx::socket open_tcpsocket(const stdx::network_io_service &io_service);
	extern stdx::socket open_udpsocket(const stdx::network_io_service &io_service);
}
#pragma endregion


#undef _ThrowLinuxError
#endif //LINUX
