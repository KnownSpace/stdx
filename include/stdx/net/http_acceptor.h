#pragma once
#include <stdx/net/socket_acceptor.h>
#include <stdx/net/http.h>

namespace stdx
{
	using http_acceptor = stdx::acceptor<stdx::http_request, stdx::http_response>;

	class basic_http_acceptor:public stdx::basic_socket_acceptor<stdx::http_request, stdx::http_response>
	{
		using base_t = stdx::basic_socket_acceptor<stdx::http_request, stdx::http_response>;
		using connection_t = typename base_t::connection_t;
	public:
		basic_http_acceptor(stdx::socket sock,size_t max_size);

		virtual ~basic_http_acceptor() =default;
	protected:
		virtual connection_t make_connection(stdx::socket sock) override;
	private:
		size_t m_max_size;
	};

	extern stdx::http_acceptor make_http_acceptor(stdx::network_io_service io_service, stdx::ipv4_addr addr,size_t max_size);
}