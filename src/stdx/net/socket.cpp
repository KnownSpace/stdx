#include <stdx/net/socket.h>
#include <stdx/finally.h>
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

#ifdef _STDX_HAS_SOCKET
int stdx::forward_protocol(const stdx::protocol& protocol)
{
	return static_cast<int>(protocol);
}

int stdx::forward_socket_type(const stdx::socket_type& sock_type)
{
	return static_cast<int>(sock_type);
}

int stdx::forward_addr_family(const stdx::addr_family& addr_family)
{
	return static_cast<int>(addr_family);
}
#ifdef WIN32
stdx::_WSAStarter stdx::_wsastarter;
#endif

#ifdef WIN32
LPFN_ACCEPTEX stdx::_NetworkIOService::m_accept_ex = nullptr;
LPFN_GETACCEPTEXSOCKADDRS stdx::_NetworkIOService::m_get_addr_ex = nullptr;
std::once_flag stdx::_NetworkIOService::m_once_flag;
#endif

#ifdef LINUX
void clean(epoll_event* ptr);
#endif // LINUX


std::once_flag stdx::_NetworkIOService::_once_flag;

std::shared_ptr<stdx::_NetworkIOService> stdx::_NetworkIOService::_instance(nullptr);

std::shared_ptr<stdx::_NetworkIOService> stdx::_NetworkIOService::get_instance()
{
	std::call_once(stdx::_NetworkIOService::_once_flag, []() 
	{
		stdx::_NetworkIOService::_instance = std::make_shared<stdx::_NetworkIOService>();
	});
	return stdx::_NetworkIOService::_instance;
}

stdx::_NetworkIOService::_NetworkIOService()
#ifdef WIN32
	:m_poller(stdx::make_iocp_poller<stdx::network_io_context>())
#else
	: m_poller(stdx::make_epoll_proactor_poller<stdx::network_io_context>([](stdx::network_io_context *context) 
		{
			_Clean(context);
		}, [](stdx::network_io_context* context) 
		{
			return _IOOperate(context);
		},[](stdx::network_io_context* context) 
		{
			return _GetFd(context);
		}, [](stdx::network_io_context* context) 
		{
			return _GetEvents(context);
		}))
#endif
	, m_token()
{
	init_threadpoll();
}

stdx::_NetworkIOService::~_NetworkIOService()
{
	m_token.cancel();
#ifdef WIN32
	for (size_t i = 0, size = cpu_cores(); i < size; i++)
	{
		m_poller.post(nullptr);
	}
#endif
}

#ifdef WIN32
void stdx::_NetworkIOService::init_accept_ex(SOCKET s)
{
	std::call_once(m_once_flag, [s]() mutable
	{
#ifdef DEBUG
		::printf("[Network IO Service]初始化AcceptEx\n");
#endif // DEBUG
		_GetAcceptEx(s, &m_accept_ex);
		_GetAcceptExSockaddr(s, &m_get_addr_ex);

	});
}
#endif // WIN32

typename stdx::_NetworkIOService::socket_t stdx::_NetworkIOService::create_socket(const int& addr_family, const int& sock_type, const int& protocol)
{
	socket_t sock = ::socket(addr_family, sock_type, protocol);
#ifdef WIN32
	if (sock == INVALID_SOCKET)
	{
		_ThrowWSAError
	}
	m_poller.bind((HANDLE)sock);
#else
	if (sock == -1)
	{
		_ThrowLinuxError
	}
	m_poller.bind(sock);
#endif
	return sock;
}

#ifdef WIN32
SOCKET stdx::_NetworkIOService::create_wsasocket(const int& addr_family, const int& sock_type, const int& protocol)
{
	SOCKET sock = WSASocketW(addr_family, sock_type, protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (sock == INVALID_SOCKET)
	{
		_ThrowWSAError
	}
	m_poller.bind((HANDLE)sock);
	return sock;
}
#endif

#ifdef LINUX
void _Send(int sock,char* buf,size_t size,size_t offset,std::function<void(stdx::network_send_event,std::exception_ptr)> callback)
{
	ssize_t r = 0;
	std::exception_ptr err(nullptr);
	try
	{
		r = ::send(sock, buf+offset, size-offset, MSG_NOSIGNAL|MSG_DONTWAIT);
		if (r == 0)
		{
			throw std::system_error(std::error_code(ECONNRESET, std::system_category()));
		}
		else if (r < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				stdx::threadpool.run([sock,buf,size,offset,callback]() 
				{
						_Send(sock, buf, size, offset, callback);
				});
				return;
			}
			_ThrowLinuxError
		}
		offset += r;
		if (offset != size)
		{
			stdx::threadpool.run([sock, buf, size, offset, callback]()
			{
				_Send(sock, buf, size, offset, callback);
			});
			return;
		}
	}
	catch (const std::exception&)
	{
		err = std::current_exception();
	}
	stdx::free(buf);
	if (err)
	{
		callback(stdx::network_send_event(), err);
		return;
	}
	stdx::network_send_event ev;
	ev.sock = sock;
	ev.size = r;
	callback(ev, err);
}
#endif // LINUX

void stdx::_NetworkIOService::send(socket_t sock, const char* data, const socket_size_t& size, std::function<void(network_send_event, std::exception_ptr)> callback)
{
#ifdef WIN32
	auto* context_ptr = new network_io_context;
	if (context_ptr == nullptr)
	{
		callback(stdx::network_send_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	context_ptr->this_socket = sock;
	char* buffer = (char*) stdx::calloc(sizeof(char), size);
	if (buffer == nullptr)
	{
		delete context_ptr;
		callback(stdx::network_send_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	memcpy(buffer, data, size);
	context_ptr->buffer.buf = buffer;
	context_ptr->buffer.len = size;
	auto* call = new std::function <void(network_io_context*, std::exception_ptr)>;
	if (call == nullptr)
	{
		free(buffer);
		delete context_ptr;
		callback(stdx::network_send_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	*call = [callback](network_io_context* context_ptr, std::exception_ptr error)
	{
		if (error)
		{
			stdx::free(context_ptr->buffer.buf);
			delete context_ptr;
			callback(network_send_event(), error);
			return;
		}
		network_send_event context(context_ptr);
		stdx::free(context_ptr->buffer.buf);
		stdx::finally fin([context_ptr]()
		{
			delete context_ptr;
		});
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
			::printf("[Network IO Service]IO操作投递失败\n");
#endif // DEBUG
			free(context_ptr->buffer.buf);
			delete call;
			delete context_ptr;
			callback(stdx::network_send_event(), std::current_exception());
			return;
		}
	}
#ifdef DEBUG
	::printf("[Network IO Service]IO操作已投递\n");
#endif // DEBUG
#else
	char* buf = (char*) stdx::calloc(size, sizeof(char));
	if (buf == nullptr)
	{
		callback(stdx::network_send_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	memcpy(buf, data, size);
	stdx::threadpool.run([sock, buf, size, callback]()
	{
		_Send(sock, buf, size,0, callback);
	});
#endif
}
void stdx::_NetworkIOService::send_file(socket_t sock, file_handle_t file_with_cache, std::function<void(std::exception_ptr)> callback)
{
#ifdef WIN32
	stdx::network_io_context* context_ptr = new network_io_context;
	context_ptr->buffer.buf = NULL;
	context_ptr->buffer.len = 0;
	auto* call = new std::function <void(network_io_context*, std::exception_ptr)>;
	if (call == nullptr)
	{
		callback(std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	*call = [callback](network_io_context* context_ptr, std::exception_ptr error)
	{
		stdx::finally fin([context_ptr]()
		{
				delete context_ptr;
		});
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
#else
	stdx::threadpool.run([sock, file_with_cache, callback]()
		{
			struct stat stat_buf;
			fstat(file_with_cache, &stat_buf);
			int r = ::sendfile(sock, file_with_cache, 0, stat_buf.st_size);
			std::exception_ptr err(nullptr);
			try
			{
				if (r == 0)
				{
					throw std::system_error(std::error_code(ECONNRESET, std::system_category()));
				}
				else if (r < 0)
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
#endif
}

void stdx::_NetworkIOService::recv(socket_t sock, const socket_size_t& size, std::function<void(network_recv_event, std::exception_ptr)> callback)
{
#ifdef WIN32
	auto* context_ptr = new network_io_context;
	if (context_ptr == nullptr)
	{
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	context_ptr->this_socket = sock;
	char* buf = (char*) stdx::calloc(sizeof(char), size);
	if (buf == nullptr)
	{
		delete context_ptr;
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	memset(buf, 0, size);
	context_ptr->buffer.buf = buf;
	context_ptr->buffer.len = size;
	auto* call = new std::function <void(network_io_context*, std::exception_ptr)>;
	if (call == nullptr)
	{
		free(buf);
		delete context_ptr;
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	*call = [callback](network_io_context* context_ptr, std::exception_ptr error)
	{
		if (error)
		{
			stdx::free(context_ptr->buffer.buf);
			delete context_ptr;
			callback(network_recv_event(), error);
			return;
		}
		if (context_ptr->size < 1)
		{
			try
			{
				throw std::system_error(std::error_code(WSAEDISCON, std::system_category()));
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
		stdx::finally fin([context_ptr]() 
		{
			delete context_ptr;
		});
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
			::printf("[Network IO Service]IO操作投递失败\n");
#endif // DEBUG
			delete call;
			free(context_ptr->buffer.buf);
			delete context_ptr;
			callback(stdx::network_recv_event(), std::current_exception());
			return;
		}
	}
#ifdef DEBUG
	::printf("[Network IO Service]IO操作已投递\n");
#endif // DEBUG
#else
	stdx::network_io_context* context = new stdx::network_io_context;
	if (context == nullptr)
	{
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
	}
	context->code = stdx::network_io_context_code::recv;
	context->this_socket = sock;
	context->size = size;
	char* buf = (char*)stdx::calloc(size, sizeof(char));
	if (buf == nullptr)
	{
		delete context;
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	context->buffer = buf;
	context->buffer_size = size;
	std::function<void(stdx::network_io_context*, std::exception_ptr)>* call = new std::function<void(stdx::network_io_context*, std::exception_ptr)>;
	if (call == nullptr)
	{
		stdx::free(buf);
		delete context;
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	*call = [callback](stdx::network_io_context* context, std::exception_ptr err) mutable
	{
		if (err)
		{
			stdx::free(context->buffer);
			delete context;
			callback(stdx::network_recv_event(), err);
			return;
		}
		stdx::network_recv_event ev(context);
		stdx::finally fin([context]()
			{
				delete context;
			});
		callback(ev, err);
	};
	context->callback = call;
	try
	{
		m_poller.post(context);
	}
	catch (const std::exception&)
	{
#ifdef DEBUG
		::printf("[Network IO Service]IO操作投递失败\n");
#endif // DEBUG
		delete call;
		stdx::free(context->buffer);
		delete context;
		callback(stdx::network_recv_event(), std::current_exception());
	}
#endif
}

void stdx::_NetworkIOService::connect(socket_t sock,  stdx::ipv4_addr& addr)
{
#ifdef WIN32
	if (WSAConnect(sock, addr, ipv4_addr::addr_len, NULL, NULL, NULL, NULL) == SOCKET_ERROR)
	{
		_ThrowWSAError
	}
#else
	if (::connect(sock, addr, ipv4_addr::addr_len) == -1)
	{
		_ThrowLinuxError
	}
#endif 
}

typename stdx::_NetworkIOService::socket_t stdx::_NetworkIOService::accept(socket_t sock)
{
#ifdef WIN32
	SOCKET s = WSAAccept(sock, NULL, 0, NULL, NULL);
	if (s == INVALID_SOCKET)
	{
		_ThrowWSAError
	}
	m_poller.bind((HANDLE)s);
	return s;
#else
	socklen_t len = ipv4_addr::addr_len;
	ipv4_addr addr;
	int new_sock = ::accept(sock, (sockaddr*)addr, &len);
	if (new_sock == -1)
	{
		_ThrowLinuxError
	}
	m_poller.bind(new_sock);
	return new_sock;
#endif
}

typename stdx::_NetworkIOService::socket_t stdx::_NetworkIOService::accept(socket_t sock, ipv4_addr& addr)
{
#ifdef WIN32
	int size = ipv4_addr::addr_len;
	SOCKET s = WSAAccept(sock, addr, &size, NULL, NULL);
	if (s == INVALID_SOCKET)
	{
		_ThrowWSAError
	}
	m_poller.bind((HANDLE)s);
	return s;
#else
	socklen_t len = ipv4_addr::addr_len;
	int new_sock = ::accept(sock, (sockaddr*)addr, &len);
	if (new_sock == -1)
	{
		_ThrowLinuxError
	}
	m_poller.bind(new_sock);
	return new_sock;
#endif
}

void stdx::_NetworkIOService::listen(socket_t sock, int backlog)
{
#ifdef WIN32
	if (::listen(sock, backlog) == SOCKET_ERROR)
	{
		_ThrowWSAError
	}
#else
	if (::listen(sock, backlog) == -1)
	{
		_ThrowLinuxError
	}
#endif
}

void stdx::_NetworkIOService::bind(socket_t sock, ipv4_addr& addr)
{
#ifdef WIN32
	if (::bind(sock, addr, ipv4_addr::addr_len) == SOCKET_ERROR)
	{
		_ThrowWSAError
	}
#else
	if (::bind(sock, addr, ipv4_addr::addr_len) == -1)
	{
		_ThrowLinuxError
	}
#endif
}

#ifdef LINUX
void _SendTo(int sock, stdx::ipv4_addr addr, char* buf, size_t size, size_t offset, std::function<void(stdx::network_send_event, std::exception_ptr)> callback)
{
	socklen_t len = stdx::ipv4_addr::addr_len;
	ssize_t r = 0;
	std::exception_ptr err(nullptr);
	try
	{
		stdx::ipv4_addr &_addr = addr;
		r = ::sendto(sock, buf+offset, size-offset, MSG_NOSIGNAL|MSG_DONTWAIT, _addr, len);
		if (r == 0)
		{
			throw std::system_error(std::error_code(ECONNRESET, std::system_category()));
		}
		else if (r < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				stdx::threadpool.run([sock,addr, buf, size, offset, callback]()
					{
						_SendTo(sock,addr, buf, size, offset, callback);
					});
				return;
			}
			_ThrowLinuxError
		}
		offset += r;
		if (offset != size)
		{
			stdx::threadpool.run([sock, addr, buf, size, offset, callback]()
				{
					_SendTo(sock, addr, buf, size, offset, callback);
				});
			return;
		}
	}
	catch (const std::exception&)
	{
		err = std::current_exception();
	}
	stdx::free(buf);
	if (err)
	{
		callback(stdx::network_send_event(), err);
		return;
	}
	stdx::network_send_event ev;
	ev.sock = sock;
	ev.size = r;
	callback(ev, err);
}
#endif // LINUX


void stdx::_NetworkIOService::send_to(socket_t sock, const ipv4_addr& addr, const char* data, const socket_size_t& size, std::function<void(stdx::network_send_event, std::exception_ptr)> callback)
{
#ifdef WIN32
	stdx::network_io_context* context_ptr = new stdx::network_io_context;
	if (context_ptr == nullptr)
	{
		callback(stdx::network_send_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	context_ptr->addr = addr;
	context_ptr->this_socket = sock;
	char* buf = (char*)stdx::calloc(sizeof(char), size);
	if (buf == nullptr)
	{
		delete context_ptr;
		callback(stdx::network_send_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	memcpy(buf, data, size);
	context_ptr->buffer.buf = buf;
	context_ptr->buffer.len = size;
	auto* call = new std::function <void(network_io_context*, std::exception_ptr)>;
	if (call == nullptr)
	{
		free(buf);
		delete context_ptr;
		callback(stdx::network_send_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	*call = [callback](network_io_context* context_ptr, std::exception_ptr error)
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
		stdx::finally fin([context_ptr]()
			{
				delete context_ptr;
			});
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
#else
	char* buf = (char*) stdx::calloc(size,sizeof(char));
	if (buf == nullptr)
	{
		callback(stdx::network_send_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	memcpy(buf, data, size);
	stdx::threadpool.run([addr,sock, buf, size, callback]()
	{
		_SendTo(sock,addr, buf, size, 0, callback);
	});
#endif

}

void stdx::_NetworkIOService::recv_from(socket_t sock, const socket_size_t& size, std::function<void(network_recv_event, std::exception_ptr)> callback)
{
#ifdef WIN32
	auto* context_ptr = new network_io_context;
	if (context_ptr == nullptr)
	{
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	context_ptr->this_socket = sock;
	char* buf = (char*) stdx::calloc(sizeof(char), size);
	if (buf == nullptr)
	{
		delete context_ptr;
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	memset(buf, 0, size);
	context_ptr->buffer.buf = buf;
	context_ptr->buffer.len = size;
	auto* call = new std::function <void(network_io_context*, std::exception_ptr)>;
	SOCKADDR_IN* addr = (SOCKADDR_IN*)stdx::malloc(sizeof(SOCKADDR_IN));
	if (addr == nullptr)
	{
		delete context_ptr;
		stdx::free(buf);
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	memset(addr, 0, sizeof(SOCKADDR_IN));
	int* addr_size = (int*)stdx::malloc(sizeof(int));
	if (addr_size == nullptr)
	{
		delete context_ptr;
		stdx::free(addr);
		stdx::free(buf);
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	*addr_size = sizeof(SOCKADDR_IN);
	*call = [callback, addr, addr_size](network_io_context* context_ptr, std::exception_ptr error)
	{
		if (error)
		{
			stdx::free(addr);
			stdx::free(addr_size);
			stdx::free(context_ptr->buffer.buf);
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
				stdx::free(addr);
				stdx::free(addr_size);
				stdx::free(context_ptr->buffer.buf);
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
		stdx::free(addr);
		stdx::free(addr_size);
		network_recv_event context(context_ptr);
		stdx::finally fin([context_ptr]()
			{
				delete context_ptr;
			});
		callback(context, std::exception_ptr(nullptr));
	};
	context_ptr->callback = call;
	if (WSARecvFrom(sock, &(context_ptr->buffer), 1, &(context_ptr->size), &(_NetworkIOService::recv_flag), (SOCKADDR*)addr, addr_size, &(context_ptr->m_ol), NULL) == SOCKET_ERROR)
	{
		try
		{
			_ThrowWSAError
		}
		catch (const std::exception&)
		{
			delete call;
			stdx::free(addr);
			stdx::free(addr_size);
			stdx::free(context_ptr->buffer.buf);
			delete context_ptr;
			callback(stdx::network_recv_event(), std::current_exception());
			return;
		}
	}
#else
	stdx::network_io_context* context = new stdx::network_io_context;
	if (context == nullptr)
	{
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	context->code = stdx::network_io_context_code::recvfrom;
	context->this_socket = sock;
	context->size = size;
	char* buf = (char*)stdx::calloc(size, sizeof(char));
	if (buf == nullptr)
	{
		delete context;
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	context->buffer = buf;
	context->buffer_size = size;
	std::function<void(stdx::network_io_context*, std::exception_ptr)>* call = new std::function<void(stdx::network_io_context*, std::exception_ptr)>;
	if (call == nullptr)
	{
		stdx::free(buf);
		delete context;
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	*call = [callback](stdx::network_io_context* context, std::exception_ptr err) mutable
	{
		if (err)
		{
			stdx::free(context->buffer);
			delete context;
			callback(stdx::network_recv_event(), err);
			return;
		}
		stdx::network_recv_event ev(context);
		stdx::finally fin([context]()
			{
				delete context;
			});
		callback(ev, err);
	};
	context->callback = call;
	try
	{
		m_poller.post(context);
	}
	catch (const std::exception &err)
	{
		delete call;
		stdx::free(context->buffer);
		delete context;
		callback(stdx::network_recv_event(), std::current_exception());
	}
#endif

}

void stdx::_NetworkIOService::close(socket_t sock)
{
#ifdef WIN32
	if (::closesocket(sock) == SOCKET_ERROR)
	{
		_ThrowWSAError
	}
#else
	m_poller.unbind(sock, [](socket_t sock) 
	{
			::close(sock);
	});
#endif
}

stdx::ipv4_addr stdx::_NetworkIOService::get_local_addr(socket_t sock) const
{
	ipv4_addr addr;
#ifdef WIN32
	int len = ipv4_addr::addr_len;
#else
	socklen_t len = ipv4_addr::addr_len;
#endif
	if (getsockname(sock, addr, &len) == -1)
	{
#ifdef WIN32
		_ThrowWSAError
#else
		_ThrowLinuxError
#endif

	}
	return addr;
}

stdx::ipv4_addr stdx::_NetworkIOService::get_remote_addr(socket_t sock) const
{
#ifdef WIN32
	ipv4_addr addr;
	int len = ipv4_addr::addr_len;
	if (getpeername(sock, addr, &len) == SOCKET_ERROR)
	{
		_ThrowWSAError
	}
	return addr;
#else
	ipv4_addr addr;
	socklen_t len = ipv4_addr::addr_len;
	if (getpeername(sock, addr, &len) == -1)
	{
		_ThrowLinuxError
	}
	return addr;
#endif
}

#ifdef LINUX
int stdx::network_io_context_finder::find(epoll_event* ev)
{
	stdx::network_io_context* cxt = (stdx::network_io_context*)ev->data.ptr;
	return cxt->this_socket;
}
#endif // WIN32

void stdx::_NetworkIOService::accept_ex(socket_t sock, std::function<void(network_accept_event, std::exception_ptr)> callback)
{
#ifdef WIN32
	try
	{
		init_accept_ex(sock);
	}
	catch (const std::exception&)
	{
	}
	stdx::network_io_context *context = new stdx::network_io_context;
	if (context == nullptr)
	{
		callback(stdx::network_accept_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	context->buffer.len = 128;
	context->buffer.buf = (char*)stdx::calloc(128,sizeof(char));
	if (context->buffer.buf == nullptr)
	{
		delete context;
		callback(stdx::network_accept_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	memset(context->buffer.buf, 0, context->buffer.len);
	context->callback = new std::function<void(stdx::network_io_context*,std::exception_ptr)>;
	if (context->callback == nullptr)
	{
		stdx::free(context->buffer.buf);
		delete context;
		callback(stdx::network_accept_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	*(context->callback) = [callback](stdx::network_io_context* context_ptr, std::exception_ptr error)
	{
		if (error)
		{
			stdx::free(context_ptr->buffer.buf);
			delete context_ptr;
			callback(stdx::network_accept_event(), error);
			return;
		}
		int local_len, remote_len;
		stdx::ipv4_addr local, remote;
		sockaddr* local_ptr = local, * remote_ptr = remote;
		stdx::_NetworkIOService::m_get_addr_ex(context_ptr->buffer.buf, 0, stdx::ipv4_addr::addr_len + 16, stdx::ipv4_addr::addr_len + 16, &local_ptr, &local_len, &remote_ptr, &remote_len);
		stdx::network_accept_event ev;
		ev.accept = context_ptr->target_socket;
		ev.addr = remote;
		stdx::free(context_ptr->buffer.buf);
		stdx::finally fin([context_ptr]()
			{
				delete context_ptr;
			});
		callback(ev, nullptr);
	};
	socket_t new_sock = INVALID_SOCKET;
	try
	{
		new_sock = create_wsasocket(stdx::forward_addr_family(stdx::addr_family::ip), stdx::forward_socket_type(stdx::socket_type::stream), stdx::forward_protocol(stdx::protocol::ip));
		context->target_socket = new_sock;
		context->this_socket = sock;
		BOOL r = m_accept_ex(sock, context->target_socket, context->buffer.buf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &(context->buffer.len), &(context->m_ol));
		if (r == FALSE)
		{
			_ThrowWSAError
		}
	}
	catch (const std::exception&)
	{
		if (new_sock != INVALID_SOCKET)
		{
			close(new_sock);
		}
		delete context->callback;
		stdx::free(context->buffer.buf);
		delete context;
		callback(stdx::network_accept_event(), std::current_exception());
	}
#else
	stdx::network_io_context* context = new stdx::network_io_context;
	if (context == nullptr)
	{
		callback(stdx::network_accept_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	context->code = stdx::network_io_context_code::accept;
	context->this_socket = sock;
	std::function<void(stdx::network_io_context*, std::exception_ptr)>* call = new std::function<void(stdx::network_io_context*, std::exception_ptr)>;
	if (call == nullptr)
	{
		delete context;
		callback(stdx::network_accept_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	*call = [callback](stdx::network_io_context* context_ptr, std::exception_ptr err) mutable
	{
		if (err)
		{
			delete context_ptr;
			callback(stdx::network_accept_event(), err);
			return;
		}
		stdx::network_accept_event ev;
		ev.accept = context_ptr->target_socket;
		ev.addr = context_ptr->addr;
		delete context_ptr;
		callback(ev, err);
	};
	context->callback = call;
	try
	{
		m_poller.post(context);
	}
	catch (const std::exception&)
	{
		delete call;
		delete context;
		callback(stdx::network_accept_event(), std::current_exception());
	}
#endif
}

void stdx::_NetworkIOService::init_threadpoll() noexcept
{
#ifdef WIN32
	for (uint32_t i = 0, cores = cpu_cores(); i < cores; i++)
	{
		stdx::threadpool.loop_run(m_token,[](stdx::io_poller<stdx::network_io_context> poller)
			{
				stdx::network_io_context* context_ptr = nullptr;
				try
				{
					context_ptr = poller.get(STDX_LAZY_MAX_TIME);
				}
				catch (const std::exception&)
				{
				}
				if (context_ptr == nullptr)
				{
					return;
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
				auto* call = context_ptr->callback;
				if (call == nullptr)
				{
					delete context_ptr;
					return;
				}
				stdx::threadpool.run([call,context_ptr,error]() 
				{
						stdx::finally fin([call]()
							{
								if (call)
								{
									delete call;
								}
							});
						try
						{
							(*call)(context_ptr, error);
						}
						catch (const std::exception&)
						{
						}
				});
			}, m_poller);
	}
#else
	for (uint32_t i = 0, cores = cpu_cores(); i < cores; i++)
	{
//		stdx::threadpool.loop_run(m_token,[](stdx::reactor reactor)
//			{
//				try
//				{
//					reactor.get([reactor](epoll_event* ev_ptr) mutable
//						{
//							stdx::network_io_context* context = (stdx::network_io_context*)ev_ptr->data.ptr;
//							if (context == nullptr)
//							{
//								return;
//							}
//							if (ev_ptr->events & stdx::epoll_events::hup)
//							{
//								_Clean(ev_ptr);
//								reactor.loop(context->this_socket);
//								return;
//							}
//							else if (ev_ptr->events & stdx::epoll_events::err)
//							{
//								_Clean(ev_ptr);
//								reactor.loop(context->this_socket);
//								return;
//							}
//							ssize_t r = 0;
//							if (context->code == stdx::network_io_context_code::recv)
//							{
//								r = ::recv(context->this_socket, context->buffer, context->size, MSG_NOSIGNAL | MSG_DONTWAIT);
//							}
//							else if (context->code == stdx::network_io_context_code::recvfrom)
//							{
//								sockaddr_in addr;
//								socklen_t addr_size = sizeof(sockaddr_in);
//								r = ::recvfrom(context->this_socket, context->buffer, context->size, MSG_NOSIGNAL | MSG_DONTWAIT, (sockaddr*)&addr, &addr_size);
//								context->addr = stdx::ipv4_addr(addr);
//							}
//							else if (context->code == stdx::network_io_context_code::accept)
//							{
//								sockaddr_in addr;
//								socklen_t addr_size = sizeof(sockaddr_in);
//								context->target_socket = ::accept4(context->this_socket, (sockaddr*)&addr, &addr_size, SOCK_NONBLOCK);
//								context->addr = stdx::ipv4_addr(addr);
//								if (context->target_socket == -1)
//								{
//									epoll_event ev = *ev_ptr;
//									reactor.push(context->this_socket, ev);
//									reactor.loop(context->this_socket);
//									return;
//								}
//								else
//								{
//									reactor.bind(context->target_socket);
//									r = 1;
//								}
//							}
//							auto* callback = context->callback;
//							std::exception_ptr err(nullptr);
//							try
//							{
//								if (r == 0)
//								{
//									throw std::system_error(std::error_code(ECONNRESET, std::system_category()));
//								}
//								else if (r < 0)
//								{
//									if (errno == EAGAIN || errno == EWOULDBLOCK)
//									{
//										reactor.push(context->this_socket, *ev_ptr);
//										reactor.loop(context->this_socket);
//										return;
//									}
//									_ThrowLinuxError
//								}
//								context->size = (size_t)r;
//							}
//							catch (const std::exception&)
//							{
//								err = std::current_exception();
//#ifdef DEBUG
//								::printf("[Epoll]IO操作出错\n");
//#endif // DEBUG
//							}
//							reactor.loop(context->this_socket);
//							if (context->callback == nullptr)
//							{
//								_Clean(ev_ptr);
//								return;
//							}
//							stdx::finally fin([callback]()
//								{
//									delete callback;
//								});
//							try
//							{
//								(*callback)(context, err);
//							}
//							catch (const std::exception&)
//							{
//							}
//						}, STDX_LAZY_MAX_TIME);
//				}
//				catch (const std::exception&)
//				{
//				}
//			}, m_reactor);
		
		stdx::threadpool.loop_run(m_token,[](stdx::io_poller<stdx::network_io_context> poller)
			{
				auto context = poller.get(STDX_LAZY_MAX_TIME);
				if (context == nullptr)
				{
					return;
				}
				auto call = context->callback;
				if (call == nullptr)
				{
					delete context;
					return;
				}
				std::exception_ptr err(nullptr);
				if (context->err_code != 0)
				{
					err = std::make_exception_ptr(std::system_error(std::error_code(context->err_code, std::system_category())));
				}
				stdx::threadpool.run([call,err,context]() 
				{
						stdx::finally fin([call]() 
						{
								delete call;
						});
						(*call)(context, err);
				});
			},m_poller);
	}
#endif
}

#ifdef LINUX
void stdx::_NetworkIOService::_Clean(stdx::network_io_context* context)
{
	auto* callback = context->callback;
	if (callback != nullptr)
	{
		try
		{
			(*callback)(context, std::make_exception_ptr(std::system_error(std::error_code(ECONNABORTED, std::system_category()))));
		}
		catch (const std::exception& err)
		{
#ifdef DEBUG
			::printf("[Epoll]执行Callback出错%s\n", err.what());
#endif // DEBUG
		}
		delete callback;
	}
	else
	{
		delete context;
	}
}

int stdx::_NetworkIOService::_GetFd(stdx::network_io_context* context)
{
	return context->this_socket;
}

bool stdx::_NetworkIOService::_IOOperate(stdx::network_io_context* context)
{
	ssize_t r = 0;
	if (context->code == stdx::network_io_context_code::recv)
	{
		r = ::recv(context->this_socket, context->buffer, context->size, MSG_NOSIGNAL | MSG_DONTWAIT);
	}
	else if (context->code == stdx::network_io_context_code::recvfrom)
	{
		sockaddr_in addr;
		socklen_t addr_size = sizeof(sockaddr_in);
		r = ::recvfrom(context->this_socket, context->buffer, context->size, MSG_NOSIGNAL | MSG_DONTWAIT, (sockaddr*)&addr, &addr_size);
		context->addr = stdx::ipv4_addr(addr);
}
	else if (context->code == stdx::network_io_context_code::accept)
	{
		sockaddr_in addr;
		socklen_t addr_size = sizeof(sockaddr_in);
		context->target_socket = ::accept4(context->this_socket, (sockaddr*)&addr, &addr_size, SOCK_NONBLOCK);
		context->addr = stdx::ipv4_addr(addr);
		if (context->target_socket == -1)
		{
			return false;
		}
		else
		{
			r = 1;
		}
	}
	if (r < 0)
	{
		context->err_code = errno;
	}
	else if(r == 0)
	{
		context->err_code = ECONNRESET;
	}
	else
	{
		context->err_code = 0;
	}
	context->size = (size_t)r;
	return true;
}

uint32_t stdx::_NetworkIOService::_GetEvents(stdx::network_io_context* context)
{
	uint32_t check_in = stdx::network_io_context_code::accept | 
		stdx::network_io_context_code::accept_ipv6 | 
		stdx::network_io_context_code::recv | 
		stdx::network_io_context_code::recvfrom | 
		stdx::network_io_context_code::recvfrom_ipv6;
	if (context->code & check_in)
	{
		return stdx::epoll_events::in;
	}
	uint32_t check_out = stdx::network_io_context_code::send |
		stdx::network_io_context_code::sendfile |
		stdx::network_io_context_code::sendto |
		stdx::network_io_context_code::sendto_ipv6 |
		stdx::network_io_context_code::connect;
	if (context->code & check_out)
	{
		return stdx::epoll_events::out;
	}
	return 0;
}
#endif // LINUX


#ifdef WIN32
DWORD stdx::_NetworkIOService::recv_flag = 0;
#endif // WIN32

stdx::_Socket::_Socket(const io_service_t& io_service, socket_t s)
	:m_io_service(io_service)
	, m_handle(s)
{}

stdx::_Socket::_Socket(const io_service_t& io_service)
	: m_io_service(io_service)
#ifdef WIN32
	, m_handle(INVALID_SOCKET)
#else
	, m_handle(-1)
#endif
{}

stdx::_Socket::~_Socket()
{
	close();
}

stdx::task<stdx::network_send_event> stdx::_Socket::send(const char* data, const socket_size_t& size)
{
	if (!m_io_service)
	{
		throw std::logic_error("this io service has been free");
	}
	stdx::task_completion_event<stdx::network_send_event> ce;
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
	auto t = ce.get_task();
	return t;
}

stdx::task<void> stdx::_Socket::send_file(file_handle_t file_handle)
{
	if (!m_io_service)
	{
		throw std::logic_error("this io service has been free");
	}
	stdx::task_completion_event<void> ce;
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
	auto t = ce.get_task();
	return t;
}

stdx::task<stdx::network_send_event> stdx::_Socket::send_to(const ipv4_addr& addr, const char* data, const socket_size_t& size)
{
	if (!m_io_service)
	{
		throw std::logic_error("this io service has been free");
	}
	stdx::task_completion_event<stdx::network_send_event> ce;
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
	auto t = ce.get_task();
	return t;
}

stdx::task<stdx::network_recv_event> stdx::_Socket::recv(const socket_size_t& size)
{
	if (!m_io_service)
	{
		throw std::logic_error("this io service has been free");
	}
	stdx::task_completion_event<stdx::network_recv_event> ce;
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
	auto t = ce.get_task();
	return t;
}

stdx::task<stdx::network_accept_event> stdx::_Socket::accept_ex()
{
	if (!m_io_service)
	{
		throw std::logic_error("this io service has been free");
	}
	stdx::task_completion_event<stdx::network_accept_event> ce;
	m_io_service.accept_ex(m_handle, [ce](stdx::network_accept_event context,std::exception_ptr error) mutable 
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
	auto t = ce.get_task();
	return t;
}

stdx::task<stdx::network_recv_event> stdx::_Socket::recv_from(const socket_size_t& size)
{
	if (!m_io_service)
	{
		throw std::logic_error("this io service has been free");
	}
	stdx::task_completion_event<stdx::network_recv_event> ce;
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
	auto t = ce.get_task();
	return t;
}

void stdx::_Socket::recv_until(socket_size_t size, stdx::cancel_token token, std::function<void(stdx::network_recv_event)> fn, std::function<void(std::exception_ptr)> err_handler)
{
	if (token.is_cancel())
	{
		return;
	}
	m_io_service.recv(m_handle, size, [token,fn,err_handler,this,size](stdx::network_recv_event ev,std::exception_ptr err) mutable
	{
			try
			{
				if (err)
				{
					err_handler(err);
				}
				else
				{
					fn(ev);
				}
			}
			catch (const std::exception&)
			{
			}
			if (!token.is_cancel())
			{
				recv_until(size, token, fn, err_handler);
			}
	});
}

void stdx::_Socket::close()
{
#ifdef WIN32
	socket_t sock = m_handle.exchange(INVALID_SOCKET);
	if (sock != INVALID_SOCKET)
	{
		m_io_service.close(sock);
	}
#else
	socket_t sock = m_handle.exchange(-1);
	if (sock != -1)
	{
		m_io_service.close(sock);
	}
#endif
}

typename stdx::_Socket::io_service_t stdx::_Socket::get_io_service() const
{
	return m_io_service;
}

void stdx::_Socket::accept_until(stdx::cancel_token token, std::function<void(stdx::network_accept_event)> fn, std::function<void(std::exception_ptr)> err_handler)
{
	if (token.is_cancel())
	{
		return;
	}
	m_io_service.accept_ex(m_handle, [fn,err_handler,token,this](stdx::network_accept_event ev,std::exception_ptr err) mutable
	{
			try
			{
				if (err)
				{
					err_handler(err);
				}
				else
				{
					fn(ev);
				}
			}
			catch (const std::exception&)
			{
			}
			if (!token.is_cancel())
			{
				accept_until(token, fn, err_handler);
			}
	});
}

stdx::task<stdx::network_connected_event> stdx::socket::accept_ex()
{
	io_service_t io_service = m_impl->get_io_service();
	return m_impl->accept_ex().then([io_service](stdx::task_result<stdx::network_accept_event> r) mutable
	{
			try
			{
				auto &&ev = r.get();
				stdx::socket _sock(io_service,ev.accept);
				stdx::network_connected_event context(_sock,ev.addr);
				return context;
			}
			catch (const std::exception&)
			{
				throw;
			}
	});
}



void stdx::socket::accept_until(stdx::cancel_token token, std::function<void(stdx::network_connected_event)> fn, std::function<void(std::exception_ptr)> err_handler)
{
	m_impl->accept_until(token, [fn,this](stdx::network_accept_event ev) mutable 
	{
		stdx::socket _sock(m_impl->get_io_service(), ev.accept);
		stdx::network_connected_event context(_sock, ev.addr);
		fn(context);
	}, err_handler);
}

stdx::socket stdx::open_socket(const stdx::network_io_service& io_service, const int& addr_family, const int& sock_type, const int& protocol)
{
	stdx::socket sock(io_service);
	sock.init(addr_family, sock_type, protocol);
	return sock;
}

stdx::socket stdx::open_socket(const stdx::network_io_service& io_service, const stdx::addr_family& addr_family, const stdx::socket_type& sock_type, const stdx::protocol& protocol)
{
	return stdx::open_socket(io_service, stdx::forward_addr_family(addr_family), stdx::forward_socket_type(sock_type), stdx::forward_protocol(protocol));
}

stdx::socket stdx::open_tcpsocket(const stdx::network_io_service& io_service)
{
	return stdx::open_socket(io_service, stdx::addr_family::ip, stdx::socket_type::stream, stdx::protocol::tcp);
}

stdx::socket stdx::open_udpsocket(const stdx::network_io_service& io_service)
{
	return stdx::open_socket(io_service, stdx::addr_family::ip, stdx::socket_type::dgram, stdx::protocol::udp);
}

stdx::socket stdx::connect_to(const stdx::network_io_service& io_service,stdx::ipv4_addr& addr)
{
	stdx::socket sock = stdx::open_tcpsocket(io_service);
	sock.connect(addr);
	return sock;
}

stdx::socket stdx::connect_to(const stdx::network_io_service& io_service, stdx::ipv4_addr&& addr)
{
	stdx::socket sock = stdx::open_tcpsocket(io_service);
	sock.connect(addr);
	return sock;
}

stdx::socket stdx::listen_for(const stdx::network_io_service& io_service, stdx::ipv4_addr& addr)
{
	stdx::socket sock = stdx::open_tcpsocket(io_service);
	sock.bind(addr);
	sock.listen(65535);
	return sock;
}

stdx::socket stdx::listen_for(const stdx::network_io_service& io_service, stdx::ipv4_addr&& addr)
{
	stdx::socket sock = stdx::open_tcpsocket(io_service);
	sock.bind(addr);
	sock.listen(65535);
	return sock;
}
#endif

#ifdef WIN32
#undef _ThrowWinError
#undef _ThrowWSAError
#endif

#ifdef LINUX
#undef _ThrowLinuxError
#endif // LINUX