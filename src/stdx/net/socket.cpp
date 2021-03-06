﻿#include <stdx/net/socket.h>
#include <stdx/finally.h>

#ifdef WIN32
#include <Mstcpip.h>
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
LPFN_CONNECTEX stdx::_NetworkIOService::m_connect_ex = nullptr;
#endif

#ifdef WIN32

void stdx::_NetworkIOService::_GetAcceptEx(SOCKET s, LPFN_ACCEPTEX* ptr)
{
	GUID id = WSAID_ACCEPTEX;
	DWORD buf;
	if (WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &id, sizeof(id), ptr, sizeof(LPFN_ACCEPTEX), &buf, NULL, NULL) == SOCKET_ERROR)
	{
		_ThrowWSAError
	}
}

void stdx::_NetworkIOService::_GetAcceptExSockaddr(SOCKET s, LPFN_GETACCEPTEXSOCKADDRS* ptr)
{
	GUID id = WSAID_GETACCEPTEXSOCKADDRS;
	DWORD buf;
	if (WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &id, sizeof(id), ptr, sizeof(LPFN_GETACCEPTEXSOCKADDRS), &buf, NULL, NULL) == SOCKET_ERROR)
	{
		_ThrowWSAError
	}
}

void stdx::_NetworkIOService::_GetConnectEx(socket_t s, LPFN_CONNECTEX* ptr)
{
	GUID id = WSAID_CONNECTEX;
	DWORD buf;
	if (WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &id, sizeof(id), ptr, sizeof(LPFN_CONNECTEX), &buf, NULL, NULL) == SOCKET_ERROR)
	{
		_ThrowWSAError
	}
}

void stdx::_NetworkIOService::_InitExFn(SOCKET s)
{
	std::call_once(m_once_flag, [s]() mutable
	{
#ifdef DEBUG
		::printf("[Network IO Service]Initzating Ex Function\n");
#endif
		_GetAcceptEx(s, &m_accept_ex);
		_GetAcceptExSockaddr(s, &m_get_addr_ex);
		_GetConnectEx(s, &m_connect_ex);
	});
}

void stdx::_NetworkIOService::_SetLoopBackFastPath(socket_t s)
{
	BOOL value = TRUE;
	DWORD ret = 1;
	int r = ::WSAIoctl(s, SIO_LOOPBACK_FAST_PATH, &value, sizeof(BOOL),NULL, 0,&ret, NULL, NULL);
	DBG_VAR(r);
	return;
}

#endif // WIN32

typename stdx::_NetworkIOService::socket_t stdx::_NetworkIOService::create_socket(const int& addr_family, const int& sock_type, const int& protocol)
{
#ifdef LINUX
	socket_t sock = ::socket(addr_family, sock_type|SOCK_CLOEXEC, protocol);
#else
	socket_t sock = ::socket(addr_family, sock_type, protocol);
#endif
#ifdef WIN32
	if (sock == INVALID_SOCKET)
	{
		_ThrowWSAError
	}
	_SetLoopBackFastPath(sock);
	stdx::threadpool.get_poller().bind((HANDLE)sock);
#else
	if (sock == -1)
	{
		_ThrowLinuxError
	}
	_SetReuseAddr(sock);
	_SetNonBlocking(sock);
	stdx::threadpool.get_poller().bind(sock);
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
	_SetLoopBackFastPath(sock);
	stdx::threadpool.get_poller().bind((HANDLE)sock);
	return sock;
}
#endif

void stdx::_NetworkIOService::send(socket_t sock, stdx::buffer buf, const socket_size_t& size, std::function<void(network_send_event, std::exception_ptr)> callback)
{
#ifdef WIN32
	auto* context_ptr = new network_io_context;
	if (context_ptr == nullptr)
	{
		callback(stdx::network_send_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	context_ptr->this_socket = sock;
	context_ptr->buf = buf;
	context_ptr->buffer.buf = buf;
	context_ptr->buffer.len = size;
	std::function <void(network_io_context*, std::exception_ptr)> call  = [callback](network_io_context* context_ptr, std::exception_ptr error)
	{
		if (error)
		{
			delete context_ptr;
			callback(network_send_event(), error);
			return;
		}
		network_send_event context(context_ptr);
		stdx::finally fin([context_ptr]()
		{
			delete context_ptr;
		});
		callback(context, nullptr);
	};
	context_ptr->callback = call;
	prepare_callback(context_ptr);
	if (WSASend(sock, &(context_ptr->buffer), 1, &(context_ptr->size), NULL, &(context_ptr->m_ol), NULL) == SOCKET_ERROR)
	{
		try
		{
			_ThrowWSAError
		}
		catch (const std::exception&)
		{
			delete context_ptr;
			callback(stdx::network_send_event(), std::current_exception());
			return;
		}
	}
#else
	stdx::network_io_context *context = new stdx::network_io_context;
	if (context == nullptr)
	{
		callback(stdx::network_send_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	context->buf = buf;
	context->send_offset = 0;
	context->this_socket = sock;
	context->code = stdx::network_io_context_code::send;
	context->err_code = 0;
	context->send_size = size;
	auto call = [callback](network_io_context *context_ptr, std::exception_ptr error)
	{
		if (error)
		{
			delete context_ptr;
			callback(network_send_event(), error);
			return;
		}
		network_send_event context(context_ptr);
		stdx::finally fin([context_ptr]()
		{
			delete context_ptr;
		});
		callback(context, nullptr);
	};
	context->callback = call;
	prepare_callback(context);
	try
	{
		stdx::threadpool.get_poller().post(context);
	}
	catch (const std::exception&)
	{
		delete context;
		callback(stdx::network_send_event(), std::current_exception());
	}
#endif
}
void stdx::_NetworkIOService::send_file(socket_t sock, file_handle_t file_with_cache, std::function<void(std::exception_ptr)> callback)
{
#ifdef WIN32
	stdx::network_io_context* context_ptr = new network_io_context;
	context_ptr->buffer.buf = NULL;
	context_ptr->buffer.len = 0;
	auto call =  [callback](network_io_context* context_ptr, std::exception_ptr error)
	{
		stdx::finally fin([context_ptr]()
		{
				delete context_ptr;
		});
		callback(error);
	};
	context_ptr->callback = call;
	prepare_callback(context_ptr);
	if (!(::TransmitFile(sock, file_with_cache, 0, 0, &context_ptr->m_ol, NULL, 0)))
	{
		try
		{
			_ThrowWSAError
		}
		catch (const std::exception&)
		{
			delete context_ptr;
			callback(std::current_exception());
			return;
		}
	}
#else
	stdx::network_io_context* context = new stdx::network_io_context;
	if (context == nullptr)
	{
		callback(std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	context->target_socket = file_with_cache;
	context->send_offset = 0;
	context->this_socket = sock;
	context->code = stdx::network_io_context_code::send;
	context->err_code = 0;
	auto call =  [callback](network_io_context* context_ptr, std::exception_ptr error)
	{
		if (error)
		{
			delete context_ptr;
			callback(error);
			return;
		}
		stdx::finally fin([context_ptr]()
		{
			delete context_ptr;
		});
		callback(nullptr);
	};
	context->callback = call;
	prepare_callback(context);
	try
	{
		stdx::threadpool.get_poller().post(context);
	}
	catch (const std::exception&)
	{
		delete context;
		callback(std::current_exception());
	}
#endif
}

void stdx::_NetworkIOService::recv(socket_t sock,stdx::buffer buf, std::function<void(network_recv_event, std::exception_ptr)> callback)
{
#ifdef WIN32
	auto* context_ptr = new network_io_context;
	if (context_ptr == nullptr)
	{
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	context_ptr->this_socket = sock;
	context_ptr->buf = buf;
	context_ptr->buffer.buf = buf;
	context_ptr->buffer.len = static_cast<ULONG>(buf.size());
	auto call = [callback](network_io_context* context_ptr, std::exception_ptr error)
	{
		if (error)
		{
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
	prepare_callback(context_ptr);
	if (WSARecv(sock, &(context_ptr->buffer), 1, &(context_ptr->size), &(_NetworkIOService::recv_flag), &(context_ptr->m_ol), NULL) == SOCKET_ERROR)
	{
		try
		{
			_ThrowWSAError
		}
		catch (const std::exception&)
		{
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
	context->code = stdx::network_io_context_code::recv;
	context->this_socket = sock;
	context->size = buf.size();
	context->buf = buf;
	std::function<void(stdx::network_io_context*, std::exception_ptr)> call = [callback](stdx::network_io_context* context, std::exception_ptr err) mutable
	{
		if (err)
		{
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
	prepare_callback(context);
	try
	{
		stdx::threadpool.get_poller().post(context);
	}
	catch (const std::exception&)
	{
		delete context;
		callback(stdx::network_recv_event(), std::current_exception());
	}
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


void stdx::_NetworkIOService::send_to(socket_t sock, const ipv4_addr& addr, stdx::buffer buf, const socket_size_t& size, std::function<void(stdx::network_send_event, std::exception_ptr)> callback)
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
	context_ptr->buf = buf;
	context_ptr->buffer.buf = buf;
	context_ptr->buffer.len = size;
	auto call = [callback](network_io_context* context_ptr, std::exception_ptr error)
	{
		if (error)
		{
			delete context_ptr;
			callback(network_send_event(), error);
			return;
		}
		network_send_event context(context_ptr);
		stdx::finally fin([context_ptr]()
			{
				delete context_ptr;
			});
		callback(context, nullptr);
	};
	context_ptr->callback = call;
	prepare_callback(context_ptr);
	if (WSASendTo(sock, &(context_ptr->buffer), 1, &(context_ptr->size), NULL, (context_ptr->addr), ipv4_addr::addr_len, &(context_ptr->m_ol), NULL) == SOCKET_ERROR)
	{
		try
		{
			_ThrowWSAError
		}
		catch (const std::exception&)
		{
			delete context_ptr;
			callback(stdx::network_send_event(), std::current_exception());
			return;
		}
	}
#else
	stdx::network_io_context* context = new stdx::network_io_context;
	if (context == nullptr)
	{
		callback(stdx::network_send_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	context->buf = buf;
	context->send_size = size;
	context->send_offset = 0;
	context->this_socket = sock;
	context->code = stdx::network_io_context_code::sendto;
	context->err_code = 0;
	context->addr = addr;
	auto call =  [callback](network_io_context* context_ptr, std::exception_ptr error)
	{
		if (error)
		{
			delete context_ptr;
			callback(network_send_event(), error);
			return;
		}
		network_send_event context(context_ptr);
		stdx::finally fin([context_ptr]()
			{
				delete context_ptr;
			});
		callback(context, nullptr);
	};
	context->callback = call;
	prepare_callback(context);
	try
	{
		stdx::threadpool.get_poller().post(context);
	}
	catch (const std::exception&)
	{
		delete context;
		callback(stdx::network_send_event(), std::current_exception());
	}
#endif

}

void stdx::_NetworkIOService::recv_from(socket_t sock, stdx::buffer buf, std::function<void(network_recv_event, std::exception_ptr)> callback)
{
#ifdef WIN32
	auto* context_ptr = new network_io_context;
	if (context_ptr == nullptr)
	{
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	context_ptr->this_socket = sock;
	context_ptr->buf = buf;
	context_ptr->buffer.buf = buf;
	context_ptr->buffer.len = static_cast<ULONG>(buf.size());
	SOCKADDR_IN* addr = (SOCKADDR_IN*)stdx::malloc(sizeof(SOCKADDR_IN));
	if (addr == nullptr)
	{
		delete context_ptr;
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	memset(addr, 0, sizeof(SOCKADDR_IN));
	int* addr_size = (int*)stdx::malloc(sizeof(int));
	if (addr_size == nullptr)
	{
		delete context_ptr;
		stdx::free(addr);
		callback(stdx::network_recv_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	*addr_size = sizeof(SOCKADDR_IN);
	context_ptr->callback = [callback, addr, addr_size](network_io_context* context_ptr, std::exception_ptr error)
	{
		if (error)
		{
			stdx::free(addr);
			stdx::free(addr_size);
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
	prepare_callback(context_ptr);
	if (WSARecvFrom(sock, &(context_ptr->buffer), 1, &(context_ptr->size), &(_NetworkIOService::recv_flag), (SOCKADDR*)addr, addr_size, &(context_ptr->m_ol), NULL) == SOCKET_ERROR)
	{
		try
		{
			_ThrowWSAError
		}
		catch (const std::exception&)
		{
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
	context->size = buf.size();
	context->buf = buf;
	std::function<void(stdx::network_io_context*, std::exception_ptr)> call =  [callback](stdx::network_io_context* context, std::exception_ptr err) mutable
	{
		if (err)
		{
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
	prepare_callback(context);
	try
	{
		stdx::threadpool.get_poller().post(context);
	}
	catch (const std::exception &err)
	{
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
	stdx::threadpool.get_poller().unbind(sock, [](int fd)
	{
			::close(fd);
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

void stdx::_NetworkIOService::accept_ex(socket_t sock, std::function<void(network_accept_event, std::exception_ptr)> callback)
{
#ifdef WIN32
	try
	{
		_InitExFn(sock);
	}
	catch (const std::exception&)
	{
		callback(stdx::network_accept_event(),std::current_exception());
		return;
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
	context->callback = [callback](stdx::network_io_context* context_ptr, std::exception_ptr error)
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
		prepare_callback(context);
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
	std::function<void(stdx::network_io_context*, std::exception_ptr)> call = [callback](stdx::network_io_context* context_ptr, std::exception_ptr err) mutable
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
	prepare_callback(context);
	try
	{
		stdx::threadpool.get_poller().post(context);
	}
	catch (const std::exception&)
	{
		delete context;
		callback(stdx::network_accept_event(), std::current_exception());
	}
#endif
}

void stdx::_NetworkIOService::connect_ex(socket_t sock,stdx::ipv4_addr addr, std::function<void(std::exception_ptr)> callback)
{
#ifdef WIN32
	try
	{
		_InitExFn(sock);
	}
	catch (const std::exception&)
	{
		callback(std::current_exception());
		return;
	}
	network_io_context *context_ptr = new network_io_context;
	if (context_ptr == nullptr)
	{
		callback(std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	context_ptr->callback = [callback](stdx::network_io_context* context, std::exception_ptr err)
	{
		stdx::finally fin([context]()
			{
				delete context;
			});
		callback(err);
	};
	prepare_callback(context_ptr);
	try
	{
		BOOL r =  m_connect_ex(sock, addr, addr.addr_len, NULL, 0, NULL,&(context_ptr->m_ol));
		if (r == FALSE)
		{
			_ThrowWSAError
		}
	}
	catch (const std::exception &)
	{
		delete context_ptr;
		callback(std::current_exception());
	}
#else
	
	int r = ::connect(sock, addr, addr.addr_len);
	if (r == 0)
	{
		callback(nullptr);
		return;
	}
	if (errno != EINPROGRESS)
	{
		try
		{
			_ThrowLinuxError
		}
		catch (const std::exception&)
		{
			callback(std::current_exception());
		}
		return;
	}
	network_io_context* context_ptr = new network_io_context;
	if (context_ptr == nullptr)
	{
		callback(std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	context_ptr->code = stdx::network_io_context_code::connect;
	context_ptr->this_socket = sock;
	context_ptr->callback = [callback](stdx::network_io_context* context, std::exception_ptr err)
	{
		stdx::finally fin([context]()
			{
				delete context;
			});
		callback(err);
	};
	prepare_callback(context_ptr);
	try
	{
		stdx::threadpool.get_poller().post(context_ptr);
	}
	catch (const std::exception&)
	{
		delete context_ptr;
		callback(std::current_exception());
	}
#endif
}

const uint32_t stdx::_NetworkIOService::loop_num = GET_CPU_CORES();

void stdx::_NetworkIOService::set_keepalive(socket_t sock, bool opt)
{
#ifdef WIN32
	tcp_keepalive keepin;
	tcp_keepalive keepout;
	keepin.keepaliveinterval = 75000;
	keepin.keepalivetime = 7200*1000;
	keepin.onoff = opt ? 1:0;
	DWORD ret = 0;
	::WSAIoctl(sock, SIO_KEEPALIVE_VALS, &keepin, sizeof(keepin), &keepout, sizeof(keepout), &ret, NULL, NULL);
#else
	int val = opt ? 1 : 0;
	int r = ::setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));
	if (r < 0)
	{
		_ThrowLinuxError
	}
#endif
}

void stdx::_NetworkIOService::prepare_callback(stdx::network_io_context* context)
{
#ifdef WIN32
	context->execute = [](stdx::stand_context *cont) 
	{
		stdx::network_io_context* context = (stdx::network_io_context*)cont;
		if (!context)
		{
			return;
		}
		std::exception_ptr error(nullptr);
		try
		{
			DWORD flag = 0;
			if (!WSAGetOverlappedResult(context->this_socket, &(context->m_ol), &(context->size), true, &flag))
			{
				_ThrowWSAError
			}
		}
		catch (const std::exception&)
		{
			error = std::current_exception();
		}
		auto call = context->callback;
		if (!call)
		{
			delete context;
			return;
		}
		try
		{
			call(context, error);
		}
		catch (const std::exception& ex)
		{
			DBG_VAR(ex);
#ifdef DEBUG
			::printf("[NetworkIOService]Callback error: %s\n", ex.what());
#endif
		}
	};
#else
	context->events = _GetEvents(context);
	context->key = context->this_socket;
	context->io_operation = [](stdx::stand_context* cont) 
	{
		stdx::network_io_context* context = (stdx::network_io_context*)cont;
		if (!context)
		{
			return true;
		}
		return _IOOperate(context);
	};
	context->execute = [](stdx::stand_context *cont) 
	{
		stdx::network_io_context* context = (stdx::network_io_context*)cont;
		if (!context)
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
		else if (context->code == stdx::network_io_context_code::accept || context->code == stdx::network_io_context_code::accept_ipv6)
		{
			stdx::threadpool.get_poller().bind(context->target_socket);
		}
		try
		{
			call(context, err);
		}
		catch (const std::exception& ex)
		{
			DBG_VAR(ex);
#ifdef DEBUG
			::printf("[NetworkIOService]Callback error: %s\n", ex.what());
#endif
		}
	};
#endif
}

#ifdef LINUX

void stdx::_NetworkIOService::_SetNonBlocking(socket_t sock)
{
	int flags = fcntl(sock, F_GETFL, 0);
	flags |= SOCK_NONBLOCK;
	fcntl(sock, F_SETFL, flags);
}

void stdx::_NetworkIOService::_SetReuseAddr(socket_t sock)
{
	int opt = 1;
	::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

bool stdx::_NetworkIOService::_IOOperate(stdx::network_io_context* context)
{
	ssize_t r = 0;
	if (context->code == stdx::network_io_context_code::recv)
	{
		r = ::recv(context->this_socket,(char *)context->buf, context->size, MSG_NOSIGNAL | MSG_DONTWAIT);
	}
	else if (context->code == stdx::network_io_context_code::recvfrom)
	{
		sockaddr_in addr;
		socklen_t addr_size = sizeof(sockaddr_in);
		r = ::recvfrom(context->this_socket, (char *)context->buf, context->size, MSG_NOSIGNAL | MSG_DONTWAIT, (sockaddr*)&addr, &addr_size);
		context->addr = stdx::ipv4_addr(addr);
}
	else if (context->code == stdx::network_io_context_code::accept)
	{
		sockaddr_in addr;
		socklen_t addr_size = sizeof(sockaddr_in);
		context->target_socket = ::accept4(context->this_socket, (sockaddr*)&addr, &addr_size,SOCK_NONBLOCK|SOCK_CLOEXEC);
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			return false;
		}
		//else if(errno == EMFILE)
		//{
		//	//Too may files open
		//	::close(stdx::_NetworkIOService::m_null_fd);
		//	stdx::_NetworkIOService::m_null_fd = ::accept4(context->this_socket, (sockaddr*)&addr, &addr_size, SOCK_NONBLOCK | SOCK_CLOEXEC);
		//	::close(stdx::_NetworkIOService::m_null_fd);
		//	stdx::_NetworkIOService::m_null_fd = stdx::_NetworkIOService::open_null_fd();
		//	context->target_socket = -1;
		//	r = -1;
		//}
		else
		{
			if (context->target_socket != -1)
			{
				context->addr = stdx::ipv4_addr(addr);
				_SetNonBlocking(context->target_socket);
				r = 1;
			}
			else
			{
				r = -1;
			}
		}
	}
	else if (context->code == stdx::network_io_context_code::send)
	{
		char* buf = context->buf;
		buf += context->send_offset;
		size_t buf_size = context->send_size;
		buf_size -= context->send_offset;
		r = ::send(context->this_socket, buf, buf_size, MSG_NOSIGNAL | MSG_DONTWAIT);
		if (r != context->send_size && r > 0)
		{
			context->send_offset += r;
			if (context->send_offset != context->send_size)
			{
				return false;
			}
		}
	}
	else if (context->code == stdx::network_io_context_code::sendto)
	{
		char* buf = context->buf;
		buf += context->send_offset;
		size_t buf_size = context->send_size;
		buf_size -= context->send_offset;
		r = ::sendto(context->this_socket,buf,buf_size, MSG_NOSIGNAL | MSG_DONTWAIT,(sockaddr*)context->addr,stdx::ipv4_addr::addr_len);
		if (r != context->send_size && r > 0)
		{
			context->send_offset += r;
			if (context->send_offset != context->send_size)
			{
				return false;
			}
		}
	}
	else if (context->code == stdx::network_io_context_code::sendfile)
	{
		off_t off = context->send_offset;
		r = ::sendfile(context->this_socket, context->target_socket, &off, context->send_size);
		context->send_offset = off;
	}
	else if (context->code == stdx::network_io_context_code::connect)
	{
		r = 1;
	}
	if (r < 0)
	{
		if (errno == EWOULDBLOCK || errno == EAGAIN)
		{
			return false;
		}
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
	constexpr uint32_t check_in = stdx::network_io_context_code::accept | 
		stdx::network_io_context_code::accept_ipv6 | 
		stdx::network_io_context_code::recv | 
		stdx::network_io_context_code::recvfrom | 
		stdx::network_io_context_code::recvfrom_ipv6;
	if (context->code & check_in)
	{
		return stdx::epoll_events::in;
	}
	constexpr uint32_t check_out = stdx::network_io_context_code::send |
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

stdx::task<stdx::network_send_event> stdx::_Socket::send(stdx::buffer buf, const socket_size_t& size)
{
	if (!m_io_service)
	{
		throw std::logic_error("this io service has been free");
	}
	stdx::task_completion_event<stdx::network_send_event> ce;
	m_io_service.send(m_handle, buf, size, [ce](stdx::network_send_event context, std::exception_ptr error) mutable
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

stdx::task<stdx::network_send_event> stdx::_Socket::send_to(const ipv4_addr& addr, stdx::buffer buf, const socket_size_t& size)
{
	if (!m_io_service)
	{
		throw std::logic_error("this io service has been free");
	}
	stdx::task_completion_event<stdx::network_send_event> ce;
	m_io_service.send_to(m_handle, addr, buf, size, [ce](stdx::network_send_event context, std::exception_ptr error) mutable
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

stdx::task<stdx::network_recv_event> stdx::_Socket::recv(stdx::buffer buf)
{
	if (!m_io_service)
	{
		throw std::logic_error("this io service has been free");
	}
	stdx::task_completion_event<stdx::network_recv_event> ce;
	m_io_service.recv(m_handle, buf, [ce](stdx::network_recv_event context, std::exception_ptr error) mutable
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

stdx::task<stdx::network_accept_event> stdx::_Socket::accept()
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

stdx::task<stdx::network_recv_event> stdx::_Socket::recv_from(stdx::buffer buf)
{
	if (!m_io_service)
	{
		throw std::logic_error("this io service has been free");
	}
	stdx::task_completion_event<stdx::network_recv_event> ce;
	m_io_service.recv_from(m_handle, buf, [ce](stdx::network_recv_event context, std::exception_ptr error) mutable
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

void stdx::_Socket::recv_until(stdx::buffer buf, stdx::cancel_token token, std::function<void(stdx::network_recv_event)> fn, std::function<void(std::exception_ptr)> err_handler)
{
	if (token.is_cancel())
	{
		return;
	}
	m_io_service.recv(m_handle,buf, [token,fn,err_handler,this,buf](stdx::network_recv_event ev,std::exception_ptr err) mutable
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
				err_handler(std::current_exception());
			}
			if (!token.is_cancel())
			{
				recv_until(buf, token, fn, err_handler);
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

stdx::task<void> stdx::_Socket::connect(ipv4_addr& addr)
{
	stdx::task_completion_event<void> ce;
	m_io_service.connect_ex(m_handle, addr, [ce](std::exception_ptr err) mutable
	{
		if (err)
		{
			ce.set_exception(err);
		}
		else
		{
			ce.set_value();
		}
		ce.run_on_this_thread();
	});
	auto task = ce.get_task();
	return task;
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

void stdx::_Socket::set_keepalive(bool opt)
{
	m_io_service.set_keepalive(m_handle,opt);
}

stdx::task<stdx::network_connected_event> stdx::socket::accept()
{
	io_service_t io_service = m_impl->get_io_service();
	return m_impl->accept().then([io_service](stdx::task_result<stdx::network_accept_event> r) mutable
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
#endif

#ifdef WIN32
#undef _ThrowWinError
#undef _ThrowWSAError
#endif

#ifdef LINUX
#undef _ThrowLinuxError
#endif // LINUX