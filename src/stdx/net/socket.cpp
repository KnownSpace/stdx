#include <stdx/net/socket.h>

#ifdef WIN32
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

int stdx::forward_protocol(const stdx::protocol & protocol)
{
	return static_cast<int>(protocol);
}

int stdx::forward_socket_type(const stdx::socket_type & sock_type)
{
	return static_cast<int>(sock_type);
}

int stdx::forward_addr_family(const stdx::addr_family & addr_family)
{
	return static_cast<int>(addr_family);
}

stdx::_WSAStarter stdx::_wsastarter;

stdx::_NetworkIOService::_NetworkIOService()
	:m_iocp()
	, m_alive(std::make_shared<bool>(true))
{
	init_threadpoll();
}

stdx::_NetworkIOService::~_NetworkIOService()
{
	*m_alive = false;
	for (size_t i = 0,size = suggested_threads_number(); i < size; i++)
	{
		m_iocp.post(0, nullptr, nullptr);
	}
}

SOCKET stdx::_NetworkIOService::create_socket(const int &addr_family, const int &sock_type, const int &protocol)
{
	SOCKET sock = ::socket(addr_family, sock_type, protocol);
	if (sock == INVALID_SOCKET)
	{
		_ThrowWSAError
	}
	m_iocp.bind(sock);
	return sock;
}
SOCKET stdx::_NetworkIOService::create_wsasocket(const int &addr_family, const int &sock_type, const int &protocol)
{
	SOCKET sock = WSASocketW(addr_family, sock_type, protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (sock == INVALID_SOCKET)
	{
		_ThrowWSAError
	}
	m_iocp.bind(sock);
	return sock;
}

void stdx::_NetworkIOService::send(SOCKET sock, const char* data, const DWORD & size, std::function<void(network_send_event, std::exception_ptr)> callback)
{
	auto *context_ptr = new network_io_context;
	if (context_ptr == nullptr)
	{
		callback(stdx::network_send_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	context_ptr->this_socket = sock;
	char *buffer = (char*) ::calloc(sizeof(char), size);
	if (buffer == nullptr)
	{
		delete context_ptr;
		callback(stdx::network_send_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	memcpy(buffer, data, size);
	context_ptr->buffer.buf = buffer;
	context_ptr->buffer.len = size;
	auto *call = new std::function <void(network_io_context*, std::exception_ptr)>;
	if (call == nullptr)
	{
		free(buffer);
		delete context_ptr;
		callback(stdx::network_send_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	*call = [callback](network_io_context *context_ptr, std::exception_ptr error)
	{
		if (error)
		{
			::free(context_ptr->buffer.buf);
			delete context_ptr;
			callback(network_send_event(), error);
			return;
		}
		network_send_event context(context_ptr);
		::free(context_ptr->buffer.buf);
		delete context_ptr;
		callback(context, nullptr);
	};
	context_ptr->callback = call;
	if (WSASend(sock, &(context_ptr->buffer), 1, &(context_ptr->size), NULL, &(context_ptr->m_ol), NULL) == SOCKET_ERROR)
	{
		try
		{
			_ThrowWSAError
		}
		catch (const std::exception&)
		{
#ifdef DEBUG
			printf("[Network IO Service]IO操作投递失败\n");
#endif // DEBUG
			free(context_ptr->buffer.buf);
			delete call;
			delete context_ptr;
			callback(stdx::network_send_event(), std::current_exception());
			return;
		}
	}
#ifdef DEBUG
	printf("[Network IO Service]IO操作已投递\n");
#endif // DEBUG
}

void stdx::_NetworkIOService::send_file(SOCKET sock, HANDLE file_with_cache,std::function<void(std::exception_ptr)> callback)
{
	stdx::network_io_context *context_ptr = new network_io_context;
	context_ptr->buffer.buf = NULL;
	context_ptr->buffer.len = 0;
	auto *call = new std::function <void(network_io_context*, std::exception_ptr)>;
	if (call == nullptr)
	{
		callback(std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	*call = [callback](network_io_context *context_ptr, std::exception_ptr error)
	{
		delete context_ptr;
		callback(error);
	};
	context_ptr->callback = call;
	if (!(::TransmitFile(sock, file_with_cache, 0, 0, &context_ptr->m_ol, NULL, 0)))
	{
		try
		{
			_ThrowWSAError
		}
		catch (const std::exception&)
		{
			delete call;
			delete context_ptr;
			callback(std::current_exception());
			return;
		}
	}
}


void stdx::_NetworkIOService::recv(SOCKET sock, const DWORD &size, std::function<void(network_recv_event, std::exception_ptr)> callback)
{
	auto *context_ptr = new network_io_context;
	if (context_ptr == nullptr)
	{
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	context_ptr->this_socket = sock;
	char *buf = (char*) ::calloc(sizeof(char), size);
	if (buf == nullptr)
	{
		delete context_ptr;
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	memset(buf, 0, size);
	context_ptr->buffer.buf = buf;
	context_ptr->buffer.len = size;
	auto *call = new std::function <void(network_io_context*, std::exception_ptr)>;
	if (call == nullptr)
	{
		free(buf);
		delete context_ptr;
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	*call = [callback](network_io_context *context_ptr, std::exception_ptr error)
	{
		if (error)
		{
			::free(context_ptr->buffer.buf);
			delete context_ptr;
			callback(network_recv_event(), error);
			return;
		}
		if (context_ptr->size < 1)
		{
			try
			{
				std::string _ERROR_STR("windows WSA error:");
				_ERROR_STR.append(std::to_string(WSAEDISCON));
				throw std::system_error(std::error_code(WSAEDISCON, std::system_category()), _ERROR_STR.c_str());
			}
			catch (const std::exception&)
			{
				free(context_ptr->buffer.buf);
				delete context_ptr;
				callback(network_recv_event(), std::current_exception());
				return;
			}
		}
		network_recv_event context(context_ptr);
		delete context_ptr;
		callback(context, std::exception_ptr(nullptr));
	};
	context_ptr->callback = call;
	if (WSARecv(sock, &(context_ptr->buffer), 1, &(context_ptr->size), &(_NetworkIOService::recv_flag), &(context_ptr->m_ol), NULL) == SOCKET_ERROR)
	{
		try
		{
			_ThrowWSAError
		}
		catch (const std::exception&)
		{
#ifdef DEBUG
			printf("[Network IO Service]IO操作投递失败\n");
#endif // DEBUG
			delete call;
			free(context_ptr->buffer.buf);
			delete context_ptr;
			callback(stdx::network_recv_event(), std::current_exception());
			return;
		}
	}
#ifdef DEBUG
	printf("[Network IO Service]IO操作已投递\n");
#endif // DEBUG
}

void stdx::_NetworkIOService::connect(SOCKET sock, stdx::ipv4_addr &addr)
{
	if (WSAConnect(sock, addr, ipv4_addr::addr_len, NULL, NULL, NULL, NULL) == SOCKET_ERROR)
	{
		_ThrowWSAError
	}
}
SOCKET stdx::_NetworkIOService::accept(SOCKET sock)
{
	SOCKET s = WSAAccept(sock, NULL, 0, NULL, NULL);
	if (s == INVALID_SOCKET)
	{
		_ThrowWSAError
	}
	m_iocp.bind(s);
	return s;
}
SOCKET stdx::_NetworkIOService::accept(SOCKET sock, ipv4_addr & addr)
{
	int size = ipv4_addr::addr_len;
	SOCKET s = WSAAccept(sock, addr, &size, NULL, NULL);
	if (s == INVALID_SOCKET)
	{
		_ThrowWSAError
	}
	m_iocp.bind(s);
	return s;
}

void stdx::_NetworkIOService::listen(SOCKET sock, int backlog)
{
	if (::listen(sock, backlog) == SOCKET_ERROR)
	{
		_ThrowWSAError
	}
}

void stdx::_NetworkIOService::bind(SOCKET sock, ipv4_addr & addr)
{
	if (::bind(sock, addr, ipv4_addr::addr_len) == SOCKET_ERROR)
	{
		_ThrowWSAError
	}
}

void stdx::_NetworkIOService::send_to(SOCKET sock, const ipv4_addr & addr, const char * data, const DWORD &size, std::function<void(stdx::network_send_event, std::exception_ptr)> callback)
{
	stdx::network_io_context *context_ptr = new stdx::network_io_context;
	if (context_ptr == nullptr)
	{
		callback(stdx::network_send_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	context_ptr->addr = addr;
	context_ptr->this_socket = sock;
	char *buf = (char*)::calloc(sizeof(char), size);
	if (buf == nullptr)
	{
		delete context_ptr;
		callback(stdx::network_send_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	memcpy(buf, data, size);
	context_ptr->buffer.buf = buf;
	context_ptr->buffer.len = size;
	auto *call = new std::function <void(network_io_context*, std::exception_ptr)>;
	if (call == nullptr)
	{
		free(buf);
		delete context_ptr;
		callback(stdx::network_send_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	*call = [callback](network_io_context *context_ptr, std::exception_ptr error)
	{
		if (error)
		{
			free(context_ptr->buffer.buf);
			delete context_ptr;
			callback(network_send_event(), error);
			return;
		}
		network_send_event context(context_ptr);
		free(context_ptr->buffer.buf);
		delete context_ptr;
		callback(context, nullptr);
	};
	context_ptr->callback = call;
	if (WSASendTo(sock, &(context_ptr->buffer), 1, &(context_ptr->size), NULL, (context_ptr->addr), ipv4_addr::addr_len, &(context_ptr->m_ol), NULL) == SOCKET_ERROR)
	{
		try
		{
			_ThrowWSAError
		}
		catch (const std::exception&)
		{
			free(context_ptr->buffer.buf);
			delete call;
			delete context_ptr;
			callback(stdx::network_send_event(), std::current_exception());
			return;
		}
	}
}

void stdx::_NetworkIOService::recv_from(SOCKET sock, const DWORD &size, std::function<void(network_recv_event, std::exception_ptr)> callback)
{
	auto *context_ptr = new network_io_context;
	if (context_ptr == nullptr)
	{
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	context_ptr->this_socket = sock;
	char *buf = (char*) ::calloc(sizeof(char), size);
	if (buf == nullptr)
	{
		delete context_ptr;
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	memset(buf, 0, size);
	context_ptr->buffer.buf = buf;
	context_ptr->buffer.len = size;
	auto *call = new std::function <void(network_io_context*, std::exception_ptr)>;
	SOCKADDR_IN *addr = (SOCKADDR_IN*)::malloc(sizeof(SOCKADDR_IN));
	if (addr == nullptr)
	{
		delete context_ptr;
		free(buf);
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	memset(addr, 0,sizeof(SOCKADDR_IN));
	int *addr_size = (int*)::malloc(sizeof(int));
	if (addr_size == nullptr)
	{
		delete context_ptr;
		free(addr);
		free(buf);
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	*addr_size = sizeof(SOCKADDR_IN);
	*call = [callback,addr,addr_size](network_io_context *context_ptr, std::exception_ptr error)
	{
		if (error)
		{
			::free(addr);
			::free(addr_size);
			::free(context_ptr->buffer.buf);
			delete context_ptr;
			callback(network_recv_event(), error);
			return;
		}
		if (context_ptr->size < 1)
		{
			try
			{
				std::string _ERROR_STR("windows WSA error:");
				_ERROR_STR.append(std::to_string(WSAEDISCON));
				throw std::system_error(std::error_code(WSAEDISCON, std::system_category()), _ERROR_STR.c_str());
			}
			catch (const std::exception&)
			{
				::free(addr);
				::free(addr_size);
				::free(context_ptr->buffer.buf);
				delete context_ptr;
				try
				{
					callback(network_recv_event(), std::current_exception());
				}
				catch (const std::exception&)
				{

				}
			}
			return;
		}
		context_ptr->addr = *addr;
		::free(addr);
		::free(addr_size);
		network_recv_event context(context_ptr);
		delete context_ptr;
		callback(context, std::exception_ptr(nullptr));
	};
	context_ptr->callback = call;
	if (WSARecvFrom(sock, &(context_ptr->buffer),1, &(context_ptr->size), &(_NetworkIOService::recv_flag),(SOCKADDR*)addr, addr_size, &(context_ptr->m_ol), NULL) == SOCKET_ERROR)
	{
		try
		{
			_ThrowWSAError
		}
		catch (const std::exception&)
		{
#ifdef DEBUG
			printf("[Network IO Service]IO操作投递失败\n");
#endif // DEBUG
			delete call;
			free(addr);
			free(addr_size);
			::free(context_ptr->buffer.buf);
			delete context_ptr;
			callback(stdx::network_recv_event(), std::current_exception());
			return;
		}
	}
#ifdef DEBUG
	printf("[Network IO Service]IO操作已投递\n");
#endif // DEBUG
}

void stdx::_NetworkIOService::close(SOCKET sock)
{
	if (::closesocket(sock) == SOCKET_ERROR)
	{
		_ThrowWSAError
	}
}

 stdx::ipv4_addr stdx::_NetworkIOService::get_local_addr(SOCKET sock) const
{
	ipv4_addr addr;
	int len = ipv4_addr::addr_len;
	if (getsockname(sock, addr, &len) == SOCKET_ERROR)
	{
		_ThrowWSAError
	}
	return addr;
}

 stdx::ipv4_addr stdx::_NetworkIOService::get_remote_addr(SOCKET sock) const
 {
	 ipv4_addr addr;
	 int len = ipv4_addr::addr_len;
	 if (getpeername(sock, addr, &len) == SOCKET_ERROR)
	 {
		 _ThrowWSAError
	 }
	 return addr;
 }

 //static LPFN_ACCEPTEX accept_ex;
 //static LPFN_GETACCEPTEXSOCKADDRS get_addr_ex;

 void stdx::_NetworkIOService::init_threadpoll() noexcept
 {
#ifdef DEBUG
	 printf("[Network IO Service]正在初始化IO服务\n");
#endif // DEBUG
	 for (size_t i = 0, cores = stdx::suggested_threads_number(); i < cores; i++)
	 {
		 stdx::threadpool::run([](iocp_t iocp, std::shared_ptr<bool> alive)
		 {
			 while (*alive)
			 {
				 auto *context_ptr = iocp.get();
#ifdef DEBUG
				 printf("[IOCP]IO操作完成\n");
#endif // DEBUG
				 if (context_ptr == nullptr)
				 {
					 continue;
				 }
				 std::exception_ptr error(nullptr);
				 try
				 {
					 DWORD flag = 0;
					 if (!WSAGetOverlappedResult(context_ptr->this_socket, &(context_ptr->m_ol), &(context_ptr->size), true, &flag))
					 {
						 _ThrowWSAError
					 }
				 }
				 catch (const std::exception&)
				 {
					 error = std::current_exception();
				 }
				 auto *call = context_ptr->callback;
				 try
				 {
					 (*call)(context_ptr, error);
				 }
				 catch (const std::exception&)
				 {
				 }
				 delete call;
			 }
		 }, m_iocp, m_alive);
	 }
 }
 DWORD stdx::_NetworkIOService::recv_flag = 0;

 stdx::_Socket::_Socket(const io_service_t & io_service, SOCKET s)
	 :m_io_service(io_service)
	 , m_handle(s)
 {}

 stdx::_Socket::_Socket(const io_service_t & io_service)
	 : m_io_service(io_service)
	 , m_handle(INVALID_SOCKET)
 {}

 stdx::_Socket::~_Socket()
 {
	 close();
 }

 stdx::task<stdx::network_send_event> stdx::_Socket::send(const char * data, const DWORD &size)
 {
	 if (!m_io_service)
	 {
		 throw std::logic_error("this io service has been free");
	 }
	 stdx::task_complete_event<stdx::network_send_event> ce;
	 m_io_service.send(m_handle, data, size, [ce](stdx::network_send_event context, std::exception_ptr error) mutable
	 {
		 if (error)
		 {
			 ce.set_exception(error);
		 }
		 else
		 {
			 ce.set_value(context);
		 }
		 ce.run_on_this_thread();
	 });
	 return ce.get_task();
 }

 stdx::task<void> stdx::_Socket::send_file(HANDLE file_handle)
 {
	 if (!m_io_service)
	 {
		 throw std::logic_error("this io service has been free");
	 }
	 stdx::task_complete_event<void> ce;
	 m_io_service.send_file(m_handle, file_handle, [ce](std::exception_ptr error) mutable
	 {
		 if (error)
		 {
			 ce.set_exception(error);
		 }
		 else
		 {
			 ce.set_value();
		 }
		 ce.run_on_this_thread();
	 });
	 return ce.get_task();
 }

 stdx::task<stdx::network_send_event> stdx::_Socket::send_to(const ipv4_addr & addr, const char * data, const DWORD & size)
 {
	 if (!m_io_service)
	 {
		 throw std::logic_error("this io service has been free");
	 }
	 stdx::task_complete_event<stdx::network_send_event> ce;
	 m_io_service.send_to(m_handle, addr, data, size, [ce](stdx::network_send_event context, std::exception_ptr error) mutable
	 {
		 if (error)
		 {
			 ce.set_exception(error);
		 }
		 else
		 {
			 ce.set_value(context);
		 }
		 ce.run_on_this_thread();
	 });
	 return ce.get_task();
 }

 stdx::task<stdx::network_recv_event> stdx::_Socket::recv(const DWORD& size)
 {
	 if (!m_io_service)
	 {
		 throw std::logic_error("this io service has been free");
	 }
	 stdx::task_complete_event<stdx::network_recv_event> ce;
	 m_io_service.recv(m_handle, size, [ce](stdx::network_recv_event context, std::exception_ptr error) mutable
	 {
		 if (error)
		 {
			 ce.set_exception(error);
		 }
		 else
		 {
			 ce.set_value(context);
		 }
		 ce.run_on_this_thread();
	 });
	 return ce.get_task();
 }

 stdx::task<stdx::network_recv_event> stdx::_Socket::recv_from(const DWORD &size)
 {
	 if (!m_io_service)
	 {
		 throw std::logic_error("this io service has been free");
	 }
	 stdx::task_complete_event<stdx::network_recv_event> ce;
	 m_io_service.recv_from(m_handle, size, [ce](stdx::network_recv_event context, std::exception_ptr error) mutable
	 {
		 if (error)
		 {
			 ce.set_exception(error);
		 }
		 else
		 {
			 ce.set_value(context);
		 }
		 ce.run_on_this_thread();
	 });
	 return ce.get_task();
 }

 void stdx::_Socket::recv_utill(const DWORD& size, std::function<bool(stdx::task_result<stdx::network_recv_event>)> call)
 {
	 auto x = this->recv(size).then([this, size, call](stdx::task_result<network_recv_event> r) mutable
		 {
			 if (!call(r))
			 {
				 recv_utill(size, call);
			 }
		 });
 }

 void stdx::_Socket::recv_utill_error(const DWORD& size, std::function<void(stdx::network_recv_event)> call, std::function<void(std::exception_ptr)> err_handler)
 {
	 return this->recv_utill(size, [call, err_handler](stdx::task_result<network_recv_event>& r) mutable
		 {
			 try
			 {
				 stdx::network_recv_event e = r.get();
				 call(e);
			 }
			 catch (const std::exception&)
			 {
				 err_handler(std::current_exception());
				 return false;
			 }
			 return true;
		 });
 }

 void stdx::_Socket::close()
 {
	 if (m_handle != INVALID_SOCKET)
	 {
		 m_io_service.close(m_handle);
		 m_handle = INVALID_SOCKET;
	 }
 }
 stdx::socket stdx::open_socket(const stdx::network_io_service & io_service, const int & addr_family, const int & sock_type, const int & protocol)
 {
	 stdx::socket sock(io_service);
	 sock.init(addr_family, sock_type, protocol);
	 return sock;
 }

 stdx::socket stdx::open_socket(const stdx::network_io_service & io_service, const stdx::addr_family & addr_family, const stdx::socket_type & sock_type, const stdx::protocol & protocol)
 {
	 return stdx::open_socket(io_service,stdx::forward_addr_family(addr_family),stdx::forward_socket_type(sock_type),stdx::forward_protocol(protocol));
 }

 stdx::socket stdx::open_tcpsocket(const stdx::network_io_service &io_service)
 {
	 return stdx::open_socket(io_service, stdx::addr_family::ip, stdx::socket_type::stream, stdx::protocol::tcp);
 }

 stdx::socket stdx::open_udpsocket(const stdx::network_io_service &io_service)
 {
	 return stdx::open_socket(io_service, stdx::addr_family::ip, stdx::socket_type::dgram, stdx::protocol::udp);
 }
#undef _ThrowWinError
#undef _ThrowWSAError
#endif

#ifdef LINUX
#define _ThrowLinuxError auto _ERROR_CODE = errno;\
						 throw std::system_error(std::error_code(_ERROR_CODE,std::system_category()),strerror(_ERROR_CODE)); \


 int stdx::forward_protocol(const stdx::protocol & protocol)
 {
	 return static_cast<int>(protocol);
 }

 int stdx::forward_socket_type(const stdx::socket_type & sock_type)
 {
	 return static_cast<int>(sock_type);
 }

 int stdx::forward_addr_family(const stdx::addr_family & addr_family)
 {
	 return static_cast<int>(addr_family);
 }

 int stdx::_NetworkIOService::create_socket(const int &addr_family, const int &sock_type, const int &protocol)
 {

	 int sock = ::socket(addr_family, sock_type, protocol);
	 if (sock == -1)
	 {
		 _ThrowLinuxError
	 }
	 if (protocol == stdx::forward_protocol(stdx::protocol::tcp))
	 {
		/* int flag = fcntl(sock, F_GETFL, 0);
		 fcntl(sock, F_SETFL, flag | O_NONBLOCK);*/
	 }
	 return sock;
 }

 void stdx::_NetworkIOService::connect(int sock, stdx::ipv4_addr &addr)
 {
	 if (::connect(sock, addr, ipv4_addr::addr_len) == -1)
	 {
		 _ThrowLinuxError
	 }
 }

 stdx::ipv4_addr stdx::_NetworkIOService::get_local_addr(int sock) const
 {
	 ipv4_addr addr;
	 socklen_t len = ipv4_addr::addr_len;
	 if (getsockname(sock, addr, &len) == -1)
	 {
		 _ThrowLinuxError
	 }
	 return addr;
 }

 stdx::ipv4_addr stdx::_NetworkIOService::get_remote_addr(int sock)const
 {
	 ipv4_addr addr;
	 socklen_t len = ipv4_addr::addr_len;
	 if (getpeername(sock, addr, &len) == -1)
	 {
		 _ThrowLinuxError
	 }
	 return addr;
 }

 int stdx::_NetworkIOService::accept(int sock, ipv4_addr &addr)
 {
	 socklen_t len = ipv4_addr::addr_len;
	 int new_sock = ::accept(sock, (sockaddr*)addr, &len);
	 if (new_sock == -1)
	 {
		 _ThrowLinuxError
	 }
	 m_reactor.bind(new_sock);
	 return new_sock;
 }
 
 int stdx::_NetworkIOService::accept(int sock)
 {
	 socklen_t len = ipv4_addr::addr_len;
	 ipv4_addr addr;
	 int new_sock = ::accept(sock, (sockaddr*)addr, &len);
	 if (new_sock == -1)
	 {
		 _ThrowLinuxError
	 }
	 m_reactor.bind(new_sock);
	 return new_sock;
 }

 void stdx::_NetworkIOService::listen(int sock,int backlog)
 {
	 if (::listen(sock, backlog) == -1)
	 {
		 _ThrowLinuxError
	 }
 }

 void stdx::_NetworkIOService::bind(int sock, stdx::ipv4_addr &addr)
 {
	 if (::bind(sock, addr, ipv4_addr::addr_len) == -1)
	 {
		 _ThrowLinuxError
	 }
 }

 void stdx::_NetworkIOService::send(int sock, const char* data, size_t size, std::function<void(network_send_event, std::exception_ptr)> callback)
 {
	 char* buf = (char*) ::calloc(size,sizeof(char));
	 if (buf == nullptr)
	 {
		 callback(stdx::network_send_event(), std::make_exception_ptr(std::bad_alloc()));
		 return;
	 }
	 memcpy(buf,data,size);
	 stdx::threadpool::run([sock,buf,size,callback]() 
	 {
		 ssize_t r = 0;
		 std::exception_ptr err(nullptr);
		 try
		 {
			 r = ::send(sock, buf, size, MSG_NOSIGNAL);
			 if (r < 1)
			 {
				 _ThrowLinuxError
			 }
		 }
		 catch (const std::exception&)
		 {
			 err = std::current_exception();
		 }
		 ::free(buf);
		 if (err)
		 {
			 callback(network_send_event(), err);
			 return;
		 }
		 network_send_event ev;
		 ev.sock = sock;
		 ev.size = r;
		 callback(ev, err);
	 });
 }

 void stdx::_NetworkIOService::send_file(int sock, int file_handle,std::function<void(std::exception_ptr)> callback)
 {
	 stdx::threadpool::run([sock,file_handle,callback]()
	 {
		 struct stat stat_buf;
		 fstat(file_handle, &stat_buf);
		 int r = ::sendfile(sock, file_handle, 0,stat_buf.st_size);
		 std::exception_ptr err(nullptr);
		 try
		 {
			 if (r < 1)
			 {
				 _ThrowLinuxError
			 }
		 }
		 catch (const std::exception&)
		 {
			 err = std::current_exception();
		 }
		 callback(err);
	 });
 }

 void stdx::_NetworkIOService::recv(int sock, const size_t &size, std::function<void(network_recv_event,std::exception_ptr)> callback)
 {
	 epoll_event ev;
	 ev.events = stdx::epoll_events::in;
	 stdx::network_io_context *context = new stdx::network_io_context;
	 if (context == nullptr)
	 {
		 callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
	 }
	 context->code = stdx::network_io_context_code::recv;
	 context->this_socket = sock;
	 context->size = size;
	 char *buf = (char*)calloc(size, sizeof(char));
	 if (buf == nullptr)
	 {
		 delete context;
		 callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		 return;
	 }
	 context->buffer = buf;
	 context->buffer_size = size;
	 std::function<void(stdx::network_io_context*,std::exception_ptr)> *call = new std::function<void(stdx::network_io_context*, std::exception_ptr)>;
	 if (call == nullptr)
	 {
		 free(buf);
		 delete context;
		 callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		 return;
	 }
	 *call = [callback](stdx::network_io_context* context, std::exception_ptr err) mutable
	 {
		 if (err)
		 {
			 free(context->buffer);
			 delete context;
			 callback(stdx::network_recv_event(), err);
			 return;
		 }
		 stdx::network_recv_event ev(context);
		 delete context;
		 callback(ev, err);
	 };
	 context->callback = call;
	 ev.data.ptr = context;
	 m_reactor.push(sock,ev);
#ifdef DEBUG
	 printf("[Network IO Service]IO操作已投递\n");
#endif // DEBUG
 }

 void stdx::_NetworkIOService::send_to(int sock, const ipv4_addr &addr, const char *data,size_t size, std::function<void(stdx::network_send_event, std::exception_ptr)> callback)
 {
	 char* buf = (char*) ::malloc(size);
	 if (buf == nullptr)
	 {
		 callback(stdx::network_send_event(), std::make_exception_ptr(std::bad_alloc()));
		 return;
	 }
	 memcpy(buf,data,size);
	 stdx::threadpool::run([sock,addr,buf,size,callback]() 
	 {
		 socklen_t len = stdx::ipv4_addr::addr_len;
		 ssize_t r = 0;
		 std::exception_ptr err(nullptr);
		 try
		 {
			 r = ::sendto(sock, buf, size,MSG_NOSIGNAL, (const sockaddr*)addr, len);
			 if (r < 1)
			 {
				 _ThrowLinuxError
			 }
		 }
		 catch (const std::exception&)
		 {
			 err = std::current_exception();
		 }
		 ::free(buf);
		 if (err)
		 {
			 callback(network_send_event(), err);
			 return;
		 }
		 network_send_event ev;
		 ev.sock = sock;
		 ev.size = r;
		 callback(ev, err);
	 });
 }

 void stdx::_NetworkIOService::recv_from(int sock, const size_t &size, std::function<void(network_recv_event, std::exception_ptr)> callback)
 {
	 epoll_event ev;
	 ev.events = stdx::epoll_events::in;
	 stdx::network_io_context *context = new stdx::network_io_context;
	 if (context == nullptr)
	 {
		 callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		 return;
	 }
	 context->code = stdx::network_io_context_code::recvfrom;
	 context->this_socket = sock;
	 context->size = size;
	 char *buf = (char*)calloc(size, sizeof(char));
	 if (buf == nullptr)
	 {
		 delete context;
		 callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		 return;
	 }
	 context->buffer = buf;
	 context->buffer_size = size;
	 std::function<void(stdx::network_io_context*, std::exception_ptr)> *call = new std::function<void(stdx::network_io_context*, std::exception_ptr)>;
	 if (call == nullptr)
	 {
		 free(buf);
		 delete context;
		 callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		 return;
	 }
	 *call = [callback](stdx::network_io_context* context, std::exception_ptr err) mutable
	 {
		 if (err)
		 {
			 free(context->buffer);
			 delete context;
			 callback(stdx::network_recv_event(), err);
			 return;
		 }
		 stdx::network_recv_event ev(context);
		 delete context;
		 callback(ev, err);
	 };
	 context->callback = call;
	 ev.data.ptr = context;
	 m_reactor.push(sock, ev);
#ifdef DEBUG
	 printf("[Network IO Service]IO操作已投递\n");
#endif // DEBUG
 }

 void stdx::_NetworkIOService::close(int sock)
 {
	 ::close(sock);
	 m_reactor.unbind(sock); //It is OK!
 }

 
 void stdx::_NetworkIOService::init_threadpoll() noexcept
 {
#ifdef DEBUG
	 printf("[Network IO Service]正在初始化IO服务\n");
#endif // DEBUG
	 for (size_t i = 0,cores = stdx::suggested_threads_number(); i < cores; i++)
	 {
		 stdx::threadpool::run([](stdx::reactor reactor,std::shared_ptr<bool> alive) 
		 {
			 while (*alive)
			 {
				 try
				 {
					 reactor.get<stdx::network_io_context_finder>([](epoll_event *ev_ptr)
					 {
#ifdef DEBUG
						 printf("[Epoll]检测到IO请求\n");
#endif // DEBUG
						 stdx::network_io_context *context = (stdx::network_io_context *)ev_ptr->data.ptr;
						 ssize_t r = 0;
						 if (context->code == stdx::network_io_context_code::recv)
						 {
							 r = ::recv(context->this_socket, context->buffer, context->size, MSG_NOSIGNAL);
						 }
						 else if (context->code == stdx::network_io_context_code::recvfrom)
						 {
							 sockaddr_in addr;
							 socklen_t addr_size = sizeof(sockaddr_in);
							 r = ::recvfrom(context->this_socket, context->buffer, context->size, MSG_NOSIGNAL, (sockaddr*)&addr, &addr_size);
							 context->addr = stdx::ipv4_addr(addr);
						 }
						 auto *callback = context->callback;
						 std::exception_ptr err(nullptr);
						 try
						 {
							 if (r < 1)
							 {
								 _ThrowLinuxError
							 }
						 }
						 catch (const std::exception&)
						 {
							 err = std::current_exception();
						 }
						 context->size = (size_t)r;
						 try
						 {
							 (*callback)(context, err);
						 }
						 catch (const std::exception&)
						 {

						 }
						 delete callback;
					 });
				 }
				 catch (const std::exception&)
				 {
				 }
			 }
		 },m_reactor,m_alive);
	 }
 }

 int stdx::network_io_context_finder::find(epoll_event *ev)
 {
	 stdx::network_io_context *cxt = (stdx::network_io_context*)ev->data.ptr;
	 return cxt->this_socket;
 }

 stdx::_Socket::_Socket(const io_service_t & io_service, int s)
	 :m_io_service(io_service)
	 , m_handle(s)
 {}

 stdx::_Socket::_Socket(const io_service_t & io_service)
	 : m_io_service(io_service)
	 , m_handle(-1)
 {}

 stdx::_Socket::~_Socket()
 {
	 if (m_handle != -1)
	 {
		 m_io_service.close(m_handle);
		 m_handle = -1;
	 }
 }

 stdx::task<stdx::network_send_event> stdx::_Socket::send(const char * data, const size_t & size)
 {
	 if (!m_io_service)
	 {
		 throw std::logic_error("this io service has been free");
	 }
	 stdx::task_complete_event<stdx::network_send_event> ce;
	 m_io_service.send(m_handle, data, size, [ce](stdx::network_send_event context, std::exception_ptr error) mutable
	 {
		 if (error)
		 {
			 ce.set_exception(error);
		 }
		 else
		 {
			 ce.set_value(context);
		 }
		 ce.run_on_this_thread();
	 });
	 return ce.get_task();
 }

 stdx::task<void> stdx::_Socket::send_file(int file_handle)
 {
	 if (!m_io_service)
	 {
		 throw std::logic_error("this io service has been free");
	 }
	 stdx::task_complete_event<void> ce;
	 m_io_service.send_file(m_handle,file_handle,[ce](std::exception_ptr error) mutable
	 {
		 if (error)
		 {
			 ce.set_exception(error);
		 }
		 else
		 {
			 ce.set_value();
		 }
		 ce.run_on_this_thread();
	 });
	 return ce.get_task();
 }

 stdx::task<stdx::network_send_event> stdx::_Socket::send_to(const ipv4_addr & addr, const char * data, const size_t & size)
 {
	 if (!m_io_service)
	 {
		 throw std::logic_error("this io service has been free");
	 }
	 stdx::task_complete_event<stdx::network_send_event> ce;
	 m_io_service.send_to(m_handle, addr, data, size, [ce](stdx::network_send_event context, std::exception_ptr error) mutable
	 {
		 if (error)
		 {
			 ce.set_exception(error);
		 }
		 else
		 {
			 ce.set_value(context);
		 }
		 ce.run_on_this_thread();
	 });
	 return ce.get_task();
 }

 stdx::task<stdx::network_recv_event> stdx::_Socket::recv(const size_t & size)
 {
	 if (!m_io_service)
	 {
		 throw std::logic_error("this io service has been free");
	 }
	 stdx::task_complete_event<stdx::network_recv_event> ce;
	 m_io_service.recv(m_handle, size, [ce](stdx::network_recv_event context, std::exception_ptr error) mutable
	 {
		 if (error)
		 {
			 ce.set_exception(error);
		 }
		 else
		 {
			 ce.set_value(context);
		 }
		 ce.run_on_this_thread();
	 });
	 return ce.get_task();
 }

 stdx::task<stdx::network_recv_event> stdx::_Socket::recv_from( const size_t & size)
 {
	 if (!m_io_service)
	 {
		 throw std::logic_error("this io service has been free");
	 }
	 stdx::task_complete_event<stdx::network_recv_event> ce;
	 m_io_service.recv_from(m_handle,size, [ce](stdx::network_recv_event context, std::exception_ptr error) mutable
	 {
		 if (error)
		 {
			 ce.set_exception(error);
		 }
		 else
		 {
			 ce.set_value(context);
		 }
		 ce.run_on_this_thread();
	 });
	 return ce.get_task();
 }

 void stdx::_Socket::recv_utill(const DWORD& size, std::function<bool(stdx::task_result<stdx::network_recv_event>)> call)
 {
	 auto x = this->recv(size).then([this, size, call](stdx::task_result<network_recv_event> r) mutable
		 {
			 if (!call(r))
			 {
				 recv_utill(size, call);
			 }
		 });
 }

 void stdx::_Socket::recv_utill_error(const DWORD& size, std::function<void(stdx::network_recv_event)> call, std::function<void(std::exception_ptr)> err_handler)
 {
	 return this->recv_utill(size, [call, err_handler](stdx::task_result<network_recv_event>& r) mutable
		 {
			 try
			 {
				 stdx::network_recv_event e = r.get();
				 call(e);
			 }
			 catch (const std::exception&)
			 {
				 err_handler(std::current_exception());
				 return false;
			 }
			 return true;
		 });
 }

 void stdx::_Socket::close()
 {
	 if (m_handle != -1)
	 {
		 m_io_service.close(m_handle);
		 m_handle = -1;
	 }
 }

 stdx::socket stdx::open_socket(const stdx::network_io_service & io_service, const int & addr_family, const int & sock_type, const int & protocol)
 {
	 stdx::socket sock(io_service);
	 sock.init(addr_family, sock_type, protocol);
	 return sock;
 }

 stdx::socket stdx::open_socket(const stdx::network_io_service & io_service, const stdx::addr_family & addr_family, const stdx::socket_type & sock_type, const stdx::protocol & protocol)
 {
	 return stdx::open_socket(io_service, stdx::forward_addr_family(addr_family), stdx::forward_socket_type(sock_type), stdx::forward_protocol(protocol));
 }

 stdx::socket stdx::open_tcpsocket(const stdx::network_io_service &io_service)
 {
	 return stdx::open_socket(io_service, stdx::addr_family::ip, stdx::socket_type::stream, stdx::protocol::tcp);
 }

 stdx::socket stdx::open_udpsocket(const stdx::network_io_service &io_service)
 {
	 return stdx::open_socket(io_service, stdx::addr_family::ip, stdx::socket_type::dgram, stdx::protocol::udp);
 }

#undef _ThrowLinuxError
#endif // LINUX
