#pragma once
#include <stdx/net/socket_connection.h>
#include <stdx/net/http_parser.h>

namespace stdx
{
	using http_connection = stdx::connection<stdx::http_request,stdx::http_response>;

	using http_client = stdx::connection<stdx::http_response, stdx::http_request>;

	class basic_http_connection:public stdx::basic_socket_connection<stdx::http_request,stdx::http_response>
	{
		using base_t = stdx::basic_socket_connection<stdx::http_request, stdx::http_response>;
	public:
		basic_http_connection(const stdx::socket &sock,uint64_t max_size);

		virtual ~basic_http_connection() = default;

		virtual stdx::task<stdx::http_request> read() override;

		virtual stdx::task<size_t> write(const stdx::http_response& package) override;

		virtual void read_until(stdx::cancel_token token, std::function<void(input_t)> fn, std::function<void(std::exception_ptr)> err_handler) override;
	protected:
		stdx::http_request_parser m_parser;
	private:
		void _Read(std::function<void(stdx::http_request,std::exception_ptr)> callback);
	};

	extern stdx::http_connection make_http_connection(const stdx::socket &sock,uint64_t max_size);
}