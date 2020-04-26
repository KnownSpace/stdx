#pragma once
#include <stdx/async/task.h>
#include <stdx/async/spin_lock.h>
#include <stdx/io.h>
#include <stdx/env.h>
#include <stdx/string.h>
#ifdef WIN32
#include <WinSock2.h>
#include <MSWSock.h>
#include<WS2tcpip.h>
#pragma comment(lib,"Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#define _STDX_HAS_SOCKET
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
#endif 

#ifdef LINUX
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<sys/sendfile.h>
#define _STDX_HAS_SOCKET
#define _ThrowLinuxError auto _ERROR_CODE = errno;\
						 throw std::system_error(std::error_code(_ERROR_CODE,std::system_category())); 
#endif // LINUX


namespace stdx
{
#ifdef WIN32
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
#endif // WIN32
#ifdef _STDX_HAS_SOCKET

#ifdef WIN32
	using socket_size_t = DWORD;
#else
	using socket_size_t = size_t;
#endif

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

	int forward_protocol(const stdx::protocol& protocol);

	int forward_socket_type(const stdx::socket_type& sock_type);

	int forward_addr_family(const stdx::addr_family& addr_family);
#endif // LINUX
#ifdef _STDX_HAS_SOCKET
	class ipv4_addr
	{
	public:
		ipv4_addr()
			:m_handle()
		{
#ifdef WIN32
			memset(&m_handle, 0, sizeof(SOCKADDR_IN));
#else
			memset(&m_handle, 0, sizeof(sockaddr_in));
#endif // WIN32
		}
		ipv4_addr(unsigned long ip, const uint16_t& port)
		{
			m_handle.sin_family = stdx::forward_addr_family(addr_family::ip);
#ifdef WIN32
			m_handle.sin_addr.S_un.S_addr = ip;
#else
			m_handle.sin_addr.s_addr = ip;
#endif // WIN32
			m_handle.sin_port = htons(port);
		}
		ipv4_addr(const stdx::string& ip, const uint16_t& port)
			:m_handle()
		{
			m_handle.sin_family = stdx::forward_addr_family(addr_family::ip);
			m_handle.sin_port = htons(port);
#ifdef WIN32
			InetPtonW(stdx::forward_addr_family(stdx::addr_family::ip), ip.c_str(), &(m_handle.sin_addr));
#else
			inet_pton(stdx::forward_addr_family(stdx::addr_family::ip), ip.c_str(), &(m_handle.sin_addr));
#endif
		}
		ipv4_addr(const ipv4_addr& other)
			:m_handle(other.m_handle)
		{}
#ifdef WIN32
		ipv4_addr(const SOCKADDR_IN& addr)
			: m_handle(addr)
		{}
#else
		ipv4_addr(const sockaddr_in& addr)
			: m_handle(addr)
		{}
#endif
		~ipv4_addr() = default;

#ifdef WIN32
		operator SOCKADDR_IN* ()
		{
			return &m_handle;
		}
#else
		operator sockaddr_in* ()
		{
			return &m_handle;
		}
#endif // WIN32

		operator sockaddr* ()
		{
			return (sockaddr*)&m_handle;
		}

		ipv4_addr& operator=(const ipv4_addr& other)
		{
			m_handle = other.m_handle;
			return *this;
		}

		ipv4_addr& operator=(ipv4_addr&& other) noexcept
		{
			m_handle = other.m_handle;
			return *this;
		}

		const static int addr_len = sizeof(sockaddr);
		ipv4_addr& port(const uint16_t& port)
		{
			m_handle.sin_port = htons(port);
			return *this;
		}
		uint16_t port() const
		{
			return ntohs(m_handle.sin_port);
		}

		stdx::string ip() const
		{
#ifdef WIN32
			wchar_t buf[20];
			InetNtopW(stdx::forward_addr_family(stdx::addr_family::ip), &(m_handle.sin_addr), buf, 20);
#else
			char buf[20];
			inet_ntop(stdx::forward_addr_family(stdx::addr_family::ip), &(m_handle.sin_addr), buf, 20);
#endif
			return stdx::string(buf);
		}

		ipv4_addr& ip(const stdx::string& ip)
		{
#ifdef WIN32
			InetPtonW(stdx::forward_addr_family(stdx::addr_family::ip), ip.c_str(), &(m_handle.sin_addr));
#else
			inet_pton(stdx::forward_addr_family(stdx::addr_family::ip), ip.c_str(), &(m_handle.sin_addr));
#endif
			return *this;
		}

		bool operator==(const stdx::ipv4_addr& other) const
		{
#ifdef WIN32
			return (m_handle.sin_port == m_handle.sin_port) && (m_handle.sin_addr.S_un.S_addr == m_handle.sin_addr.S_un.S_addr);
#else
			return (m_handle.sin_port == m_handle.sin_port) && (m_handle.sin_addr.s_addr == m_handle.sin_addr.s_addr);
#endif
		}
	private:
#ifdef WIN32
		SOCKADDR_IN m_handle;
#else
		sockaddr_in m_handle;
#endif
	};

	class ipv6_addr
	{
	public:
		ipv6_addr()
			:m_handle()
		{
#ifdef WIN32
			memset(&m_handle, 0, sizeof(SOCKADDR_IN6));
#else
			memset(&m_handle, 0, sizeof(sockaddr_in6));
#endif
		}
		ipv6_addr(const stdx::string& ip, const uint16_t& port)
			:m_handle()
		{
			m_handle.sin6_family = stdx::forward_addr_family(addr_family::ipv6);
			m_handle.sin6_port = htons(port);
#ifdef WIN32
			InetPtonW(stdx::forward_addr_family(stdx::addr_family::ipv6), ip.c_str(), &(m_handle.sin6_addr));
#else
			inet_pton(stdx::forward_addr_family(stdx::addr_family::ipv6), ip.c_str(), &(m_handle.sin6_addr));
#endif
		}
		ipv6_addr(const ipv6_addr& other)
			:m_handle(other.m_handle)
		{}
#ifdef WIN32
		ipv6_addr(const SOCKADDR_IN6& addr)
			: m_handle(addr)
		{}
#else
		ipv6_addr(const sockaddr_in6& addr)
			: m_handle(addr)
		{}
#endif
		~ipv6_addr() = default;

#ifdef WIN32
		operator SOCKADDR_IN6* ()
		{
			return &m_handle;
		}
#else
		operator sockaddr_in6* ()
		{
			return &m_handle;
		}
#endif // WIN32

		operator sockaddr* ()
		{
			return (sockaddr*)&m_handle;
		}

		ipv6_addr& operator=(const ipv6_addr& other)
		{
			m_handle = other.m_handle;
			return *this;
		}

		ipv6_addr& operator=(ipv6_addr&& other) noexcept
		{
			m_handle = other.m_handle;
			return *this;
		}

		const static int addr_len = sizeof(sockaddr);
		ipv6_addr& port(const uint16_t& port)
		{
			m_handle.sin6_port = htons(port);
			return *this;
		}
		uint16_t port() const
		{
			return ntohs(m_handle.sin6_port);
		}

		stdx::string ip() const
		{
#ifdef WIN32
			wchar_t buf[39];
			InetNtopW(stdx::forward_addr_family(stdx::addr_family::ipv6), &(m_handle.sin6_addr), buf, 39);
#else
			char buf[39];
			inet_ntop(stdx::forward_addr_family(stdx::addr_family::ipv6), &(m_handle.sin6_addr), buf, 39);
#endif
			return stdx::string(buf);
		}

		ipv6_addr& ip(const stdx::string& ip)
		{
#ifdef WIN32
			InetPtonW(stdx::forward_addr_family(stdx::addr_family::ipv6), ip.c_str(), &(m_handle.sin6_addr));
#else
			inet_pton(stdx::forward_addr_family(stdx::addr_family::ipv6), ip.c_str(), &(m_handle.sin6_addr));
#endif
			return *this;
		}

		bool operator==(const stdx::ipv4_addr& other) const
		{
#ifdef WIN32
			return (m_handle.sin6_port == m_handle.sin6_port) && (m_handle.sin6_addr.u.Word == m_handle.sin6_addr.u.Word);
#else
			return (m_handle.sin6_port == m_handle.sin6_port) && (m_handle.sin6_addr.__in6_u.__u6_addr32 == m_handle.sin6_addr.__in6_u.__u6_addr32);
#endif // WIN32
		}
	private:
#ifdef WIN32
		SOCKADDR_IN6 m_handle;
#else
		sockaddr_in6 m_handle;
#endif
	};

	struct network_io_context
	{
#ifdef WIN32
		network_io_context()
		{
			std::memset(&m_ol, 0, sizeof(OVERLAPPED));
		}
#else
		network_io_context() = default;
#endif


		~network_io_context() = default;
#ifdef WIN32
		WSAOVERLAPPED m_ol;
#else
		int code;
#endif 
#ifdef WIN32
		SOCKET this_socket;
		SOCKET target_socket;
#else
		int this_socket;
		int target_socket;
#endif
		ipv4_addr addr;
#ifdef WIN32
		WSABUF buffer;
#else
		char* buffer;
		size_t buffer_size;
#endif
		stdx::socket_size_t size;
		
		std::function <void(network_io_context*, std::exception_ptr)>* callback;
	};

#ifdef LINUX
	struct network_io_context_code
	{
		enum
		{
			recv = 0,
			recvfrom = 1,
			send = 2,
			sendto = 3,
			accept = 4,
			connect = 5,
			sendfile = 6
		};
	};
#endif

	struct network_send_event
	{
		network_send_event()
#ifdef WIN32
			:sock(INVALID_SOCKET)
#else
			: sock(-1)
#endif
			, size(0)
		{}
		~network_send_event() = default;
		network_send_event(const network_send_event& other)
			:sock(other.sock)
			, size(other.size)
		{}
		network_send_event(network_send_event&& other) noexcept
			:sock(std::move(other.sock))
			, size(std::move(other.size))
		{}
		network_send_event& operator=(const network_send_event& other)
		{
			sock = other.sock;
			size = other.size;
			return *this;
		}

		network_send_event& operator=(network_send_event&& other) noexcept
		{
			sock = other.sock;
			size = other.size;
#ifdef WIN32
			other.sock = INVALID_SOCKET;
			other.size = 0;
#else
			other.sock = -1;
			other.size = 0;
#endif
			return *this;
		}

		network_send_event(network_io_context* ptr)
			:sock(ptr->this_socket)
			, size(ptr->size)
		{}

#ifdef WIN32
		SOCKET sock;
#else
		int sock;
#endif
		size_t size;
	};

	struct network_recv_event
	{
		network_recv_event()
#ifdef WIN32
			:sock(INVALID_SOCKET)
#else
			:sock(-1)
#endif
			, buffer(0, nullptr)
			, size(0)
			, addr()
		{}
		~network_recv_event() = default;
		network_recv_event(const network_recv_event& other)
			:sock(other.sock)
			, buffer(other.buffer)
			, size(other.size)
			, addr(other.addr)
		{}
		network_recv_event(network_recv_event&& other) noexcept
			:sock(std::move(other.sock))
			, buffer(other.buffer)
			, size(other.size)
			, addr(other.addr)
		{}
		network_recv_event& operator=(const network_recv_event& other)
		{
			sock = other.sock;
			buffer = other.buffer;
			size = other.size;
			addr = other.addr;
			return *this;
		}

		network_recv_event& operator=(network_recv_event&& other) noexcept
		{
			sock = other.sock;
			buffer = other.buffer;
			size = other.size;
			addr = other.addr;
#ifdef WIN32
			other.sock = INVALID_SOCKET;
			other.size = 0;
#else
			other.sock = -1;
			other.size = 0;
#endif
			return *this;
		}

		network_recv_event(network_io_context* ptr)
			:sock(ptr->target_socket)
#ifdef WIN32
			, buffer(ptr->buffer.len, ptr->buffer.buf)
#else
			, buffer(ptr->buffer_size, ptr->buffer)
#endif // WIN32
			, size(ptr->size)
			, addr(ptr->addr)
		{}
#ifdef WIN32
		SOCKET sock;
#else
		int sock;
#endif // WIN32
		stdx::buffer buffer;
		size_t size;
		stdx::ipv4_addr addr;
	};

#ifdef LINUX
	struct network_io_context_finder
	{
		static int find(epoll_event* ev);
	};
#endif // LINUX

	struct network_accept_event
	{
		network_accept_event()
#ifdef WIN32
			:accept(INVALID_SOCKET)
#else
			:accept(-1)
#endif
			, addr()
		{}
		~network_accept_event() = default;
		network_accept_event(const network_accept_event& other)
			:accept(other.accept)
			, addr(other.addr)
		{}

		network_accept_event(network_accept_event&& other) noexcept
			:accept(other.accept)
			, addr(other.addr)
		{}

		network_accept_event& operator=(const network_accept_event& other)
		{
			accept = other.accept;
			addr = other.addr;
			return *this;
		}

		network_accept_event(network_io_context* ptr)
			:accept(ptr->target_socket)
			, addr(ptr->addr)
		{}
#ifdef WIN32
		SOCKET accept;
#else
		int accept;
#endif // WIN32
		ipv4_addr addr;
	};
	
	class _NetworkIOService
	{
	public:
#ifdef WIN32
		using iocp_t = stdx::iocp<network_io_context>;
		using socket_t = SOCKET;
		using file_handle_t = HANDLE;
#else
		using socket_t = int;
		using file_handle_t = int;
#endif
		_NetworkIOService();

		delete_copy(_NetworkIOService);

		~_NetworkIOService();

#ifdef WIN32
		SOCKET create_socket(const int& addr_family, const int& sock_type, const int& protocol);
		SOCKET create_wsasocket(const int& addr_family, const int& sock_type, const int& protocol);
#else
		int create_socket(const int& addr_family, const int& sock_type, const int& protocol);
#endif

		//发送数据
		void send(socket_t sock, const char* data, const stdx::socket_size_t& size, std::function<void(network_send_event, std::exception_ptr)> callback);

		void send_file(socket_t sock, file_handle_t file_with_cache, std::function<void(std::exception_ptr)> callback);

		//接收数据
		void recv(socket_t sock, const  stdx::socket_size_t& size, std::function<void(network_recv_event, std::exception_ptr)> callback);

		void connect(socket_t sock, stdx::ipv4_addr& addr);

		socket_t accept(socket_t sock, ipv4_addr& addr);

		socket_t accept(socket_t sock);

		void listen(socket_t sock, int backlog);

		void bind(socket_t sock, ipv4_addr& addr);

		void send_to(socket_t sock, const ipv4_addr& addr, const char* data, const socket_size_t& size, std::function<void(stdx::network_send_event, std::exception_ptr)> callback);

		void recv_from(socket_t sock, const socket_size_t& size, std::function<void(network_recv_event, std::exception_ptr)> callback);

		void close(socket_t sock);

		ipv4_addr get_local_addr(socket_t sock) const;

		ipv4_addr get_remote_addr(socket_t sock) const;
#ifdef WIN32

		static void _GetAcceptEx(SOCKET s, LPFN_ACCEPTEX *ptr)
		{
			GUID id = WSAID_ACCEPTEX;
			DWORD buf;
			if (WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &id, sizeof(id), ptr, sizeof(LPFN_ACCEPTEX), &buf, NULL, NULL)==SOCKET_ERROR)
			{
				_ThrowWSAError
			}
		}

		static void _GetAcceptExSockaddr(SOCKET s, LPFN_GETACCEPTEXSOCKADDRS *ptr)
		{
			GUID id = WSAID_GETACCEPTEXSOCKADDRS;
			DWORD buf;
			if (WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &id, sizeof(id), ptr, sizeof(LPFN_GETACCEPTEXSOCKADDRS), &buf, NULL, NULL)==SOCKET_ERROR)
			{
				_ThrowWSAError
			}
		}

		void init_accept_ex(SOCKET s);
#endif
		void accept_ex(socket_t sock, std::function<void(network_accept_event, std::exception_ptr)> callback);
#ifdef WIN32
	public:
		static LPFN_ACCEPTEX m_accept_ex;
		static LPFN_GETACCEPTEXSOCKADDRS m_get_addr_ex;
		static std::once_flag m_once_flag;
#endif
	private:
#ifdef WIN32
		iocp_t m_iocp;
		static DWORD recv_flag;
#else
		stdx::reactor m_reactor;
#endif
		std::shared_ptr<bool> m_alive;
		void init_threadpoll() noexcept;
	};

	class network_io_service
	{
#ifdef WIN32
		using iocp_t = _NetworkIOService::iocp_t;
#endif // WIN32
		using socket_t = _NetworkIOService::socket_t;
		using file_handle_t = _NetworkIOService::file_handle_t;
		using impl_t = std::shared_ptr<_NetworkIOService>;
	public:
		network_io_service()
			:m_impl(std::make_shared<_NetworkIOService>())
		{}

		network_io_service(const network_io_service& other)
			:m_impl(other.m_impl)
		{}

		network_io_service(network_io_service&& other) noexcept
			:m_impl(std::move(other.m_impl))
		{}

		network_io_service& operator=(const network_io_service& other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		~network_io_service() = default;

		socket_t create_socket(const int& addr_family, const int& sock_type, const int& protocol)
		{
#ifdef WIN32
			return m_impl->create_wsasocket(addr_family, sock_type, protocol);
#else
			return m_impl->create_socket(addr_family, sock_type, protocol);
#endif // WIN32
		}

		void send(socket_t sock, const char* data, const socket_size_t& size, std::function<void(network_send_event, std::exception_ptr)>&& callback)
		{
			m_impl->send(sock, data, size, std::move(callback));
		}

		void send_file(socket_t sock, file_handle_t file_with_cache, std::function<void(std::exception_ptr)>&& callback)
		{
			m_impl->send_file(sock, file_with_cache, callback);
		}

		void recv(socket_t sock, const socket_size_t& size, std::function<void(network_recv_event, std::exception_ptr)>&& callback)
		{
			m_impl->recv(sock, size, callback);
		}

		void connect(socket_t sock, stdx::ipv4_addr& addr)
		{
			m_impl->connect(sock, addr);
		}

		void accept_ex(socket_t sock, std::function<void(network_accept_event, std::exception_ptr)> &&callback)
		{
			return m_impl->accept_ex(sock,callback);
		}


		socket_t accept(socket_t sock, ipv4_addr& addr)
		{
			return m_impl->accept(sock, addr);
		}

		socket_t accept(socket_t sock)
		{
			return m_impl->accept(sock);
		}

		void listen(socket_t sock, int backlog)
		{
			m_impl->listen(sock, backlog);
		}

		void bind(socket_t sock, ipv4_addr& addr)
		{
			m_impl->bind(sock, addr);
		}

		void send_to(socket_t sock, const ipv4_addr& addr, const char* data, const socket_size_t& size, std::function<void(stdx::network_send_event, std::exception_ptr)>&& callback)
		{
			m_impl->send_to(sock, addr, data, size, std::move(callback));
		}

		void recv_from(socket_t sock, const socket_size_t& size, std::function<void(network_recv_event, std::exception_ptr)>&& callback)
		{
			m_impl->recv_from(sock, size, callback);
		}

		void close(socket_t sock)
		{
			m_impl->close(sock);
		}

		ipv4_addr get_local_addr(socket_t sock) const
		{
			return m_impl->get_local_addr(sock);
		}

		ipv4_addr get_remote_addr(socket_t sock) const
		{
			return m_impl->get_remote_addr(sock);
		}
		operator bool() const
		{
			return (bool)m_impl;
		}

		bool operator==(const network_io_service& other) const
		{
			return m_impl == other.m_impl;
		}
	private:
		impl_t m_impl;
	};

	class _Socket
	{
		using io_service_t = network_io_service;
		using socket_t = _NetworkIOService::socket_t;
		using file_handle_t = _NetworkIOService::file_handle_t;
	public:
		explicit _Socket(const io_service_t& io_service, socket_t s);

		explicit _Socket(const io_service_t& io_service);

		delete_copy(_Socket);

		~_Socket();

		void init(const int& addr_family, const int& sock_type, const int& protocol)
		{
			m_handle = m_io_service.create_socket(addr_family, sock_type, protocol);
		}


		stdx::task<stdx::network_send_event> send(const char* data, const socket_size_t& size);

		stdx::task<void> send_file(file_handle_t file_handle);


		stdx::task<stdx::network_send_event> send_to(const ipv4_addr& addr, const char* data, const socket_size_t& size);


		stdx::task<stdx::network_recv_event> recv(const socket_size_t& size);


		stdx::task<stdx::network_recv_event> recv_from(const socket_size_t& size);

		void bind(ipv4_addr& addr)
		{
			m_io_service.bind(m_handle, addr);
		}

		void listen(int backlog)
		{
			m_io_service.listen(m_handle, backlog);
		}

		socket_t accept(ipv4_addr& addr)
		{
			return m_io_service.accept(m_handle, addr);
		}

		socket_t accept()
		{
			return m_io_service.accept(m_handle);
		}

		stdx::task<stdx::network_accept_event> accept_ex();

		void close();

		void connect(ipv4_addr& addr)
		{
			m_io_service.connect(m_handle, addr);
		}

		io_service_t io_service() const
		{
			return m_io_service;
		}

		ipv4_addr local_addr() const
		{
			return m_io_service.get_local_addr(m_handle);
		}

		ipv4_addr remote_addr() const
		{
			return m_io_service.get_remote_addr(m_handle);
		}

		//直到call返回true停止
		void recv_until(const socket_size_t& size, std::function<bool(stdx::task_result<stdx::network_recv_event>)> call);

		void recv_until_error(const socket_size_t& size, std::function<void(stdx::network_recv_event)> call, std::function<void(std::exception_ptr)> err_handler);

		io_service_t get_io_service() const;
	private:
		io_service_t m_io_service;
		socket_t m_handle;
	};

	struct network_connected_event;

	class socket
	{
		using impl_t = std::shared_ptr<_Socket>;
		using self_t = socket;
		using socket_t = _NetworkIOService::socket_t;
		using file_handle_t = _NetworkIOService::file_handle_t;
		using io_service_t = network_io_service;
	public:

		socket()
			:m_impl(nullptr)
		{}

		socket(const io_service_t& io_service)
			:m_impl(std::make_shared<_Socket>(io_service))
		{}

		socket(const self_t& other)
			:m_impl(other.m_impl)
		{}
		socket(self_t&& other) noexcept
			:m_impl(std::move(other.m_impl))
		{}
		~socket()
		{}
		self_t& operator=(const self_t& other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		void init(const int& addr_family, const int& sock_type, const int& protocol)
		{
			return m_impl->init(addr_family, sock_type, protocol);
		}

		void bind(ipv4_addr& addr)
		{
			m_impl->bind(addr);
		}

		void listen(int backlog)
		{
			m_impl->listen(backlog);
		}

		self_t accept(ipv4_addr& addr)
		{
			socket_t s = m_impl->accept(addr);
			return socket(m_impl->io_service(), s);
		}

		self_t accept()
		{
			socket_t s = m_impl->accept();
			return socket(m_impl->io_service(), s);
		}

		stdx::task<network_connected_event> accept_ex();

		void close()
		{
			m_impl->close();
		}

		void connect(ipv4_addr& addr)
		{
			m_impl->connect(addr);
		}

		ipv4_addr local_addr() const
		{
			return m_impl->local_addr();
		}

		ipv4_addr remote_addr() const
		{
			return m_impl->remote_addr();
		}

		stdx::task<network_send_event> send(const char* data, const socket_size_t& size)
		{
			return m_impl->send(data, size);
		}

		stdx::task<void> send_file(file_handle_t file_with_cache)
		{
			return m_impl->send_file(file_with_cache);
		}

		stdx::task<network_send_event> send_to(const ipv4_addr& addr, const char* data, const socket_size_t& size)
		{
			return m_impl->send_to(addr, data, size);
		}

		stdx::task<network_recv_event> recv(const socket_size_t& size)
		{
			return m_impl->recv(size);
		}

		stdx::task<network_recv_event> recv_from(const socket_size_t& size)
		{
			return m_impl->recv_from(size);
		}

		//直到call返回true停止
		void recv_until(const socket_size_t& size, std::function<bool(stdx::task_result<stdx::network_recv_event>)>  call)
		{
			return m_impl->recv_until(size, call);
		}

		void recv_until_error(const socket_size_t& size, std::function<void(stdx::network_recv_event)> call, std::function<void(std::exception_ptr)> err_handler)
		{
			return m_impl->recv_until_error(size, call, err_handler);
		}

		void accept_until(std::function<bool(stdx::task_result<stdx::network_connected_event>)> call);

		void accept_until_error(std::function<void(stdx::network_connected_event)> call,std::function<void(std::exception_ptr)> err_handler);

		bool operator==(const stdx::socket& other) const
		{
			return m_impl == other.m_impl;
		}

		operator bool() const
		{
			return (bool)m_impl;
		}

	private:
		impl_t m_impl;

		socket(const io_service_t& io_service, socket_t s)
			:m_impl(std::make_shared<_Socket>(io_service, s))
		{}
	};

	struct network_connected_event
	{

		network_connected_event() = default;

		network_connected_event(stdx::socket _connection, const stdx::ipv4_addr& _addr)
			:connection(_connection)
			, addr(_addr)
		{}

		~network_connected_event() = default;

		network_connected_event(const network_connected_event& other)
			:connection(other.connection)
			, addr(other.addr)
		{}

		network_connected_event(network_connected_event&& other) noexcept
			:connection(other.connection)
			, addr(other.addr)
		{}

		network_connected_event& operator=(const network_connected_event& other)
		{
			connection = other.connection;
			addr = other.addr;
			return *this;
		}

		network_connected_event& operator=(network_connected_event&& other) noexcept
		{
			connection = other.connection;
			addr = other.addr;
			return *this;
		}

		stdx::socket connection;
		ipv4_addr addr;
	};

	extern stdx::socket open_socket(const stdx::network_io_service& io_service, const int& addr_family, const int& sock_type, const int& protocol);
	extern stdx::socket open_socket(const stdx::network_io_service& io_service, const stdx::addr_family& addr_family, const stdx::socket_type& sock_type, const stdx::protocol& protocol);
	extern stdx::socket open_tcpsocket(const stdx::network_io_service& io_service);
	extern stdx::socket open_udpsocket(const stdx::network_io_service& io_service);
	extern stdx::socket connect_to(const stdx::network_io_service &io_service,stdx::ipv4_addr &addr);
	extern stdx::socket connect_to(const stdx::network_io_service& io_service, stdx::ipv4_addr &&addr);
	extern stdx::socket listen_for(const stdx::network_io_service& io_service, stdx::ipv4_addr &addr);
	extern stdx::socket listen_for(const stdx::network_io_service& io_service, stdx::ipv4_addr &&addr);
#endif // _STDX_HAS_SOCKET
}

#ifdef WIN32
#undef _ThrowWinError
#undef _ThrowWSAError
#endif

#ifdef LINUX
#undef _ThrowLinuxError
#endif // LINUX