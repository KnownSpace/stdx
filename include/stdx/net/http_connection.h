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

		//virtual void read_until(std::function<bool(stdx::task_result<stdx::http_request>)> fn);
	protected:
		stdx::http_request_parser m_parser;
	private:
		static bool _CheckParser(stdx::http_request_parser &parser,stdx::task_completion_event<stdx::http_request> &ce);
	};

	extern stdx::http_connection make_http_connection(const stdx::socket &sock,uint64_t max_size);
}