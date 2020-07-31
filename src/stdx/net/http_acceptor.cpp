#include <stdx/net/http_acceptor.h>
#include <stdx/net/http_connection.h>


stdx::basic_http_acceptor::basic_http_acceptor(stdx::socket sock, size_t max_size)
	: base_t(sock)
	, m_max_size(max_size)
{}

typename stdx::basic_http_acceptor::connection_t stdx::basic_http_acceptor::make_connection(stdx::socket sock)
{
	return stdx::make_http_connection(sock,m_max_size);
}

extern stdx::http_acceptor stdx::make_http_acceptor(network_io_service io_service, ipv4_addr addr, size_t max_size)
{
	stdx::socket sock = stdx::open_tcpsocket(io_service);
	sock.bind(addr);
	sock.listen(65535);
	return stdx::make_acceptor<stdx::basic_http_acceptor>(sock,max_size);
}