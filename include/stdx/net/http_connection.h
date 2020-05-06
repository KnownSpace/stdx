#pragma once
#include <stdx/net/http.h>
#include <stdx/net/connection.h>

namespace stdx
{
	using http_connection = stdx::connection<stdx::http_request,stdx::http_response>;

	using http_client = stdx::connection<stdx::http_response,stdx::http_request>;

	enum class http_parser_state
	{
		pending,
		wait_header,
		wait_body,
		finish
	};

	struct _HTTPRequestParser
	{
	public:
		_HTTPRequestParser(size_t max_length);

		~_HTTPRequestParser() = default;
		
		stdx::http_parser_state state() const
		{
			return m_state;
		}

		void push(std::string &&str);

		stdx::http_request pop();

		size_t size()
		{
			return m_reqs.size();
		}
	private:
		std::list<stdx::http_request> m_reqs;
		std::string m_body_buf;
		std::string m_header_buf;
		std::shared_ptr<stdx::http_request_header> m_header;
		stdx::http_parser_state m_state;
		size_t m_max_length;

		void _PublishHeader(std::string&& str);

		void _PublishRequest(std::shared_ptr<stdx::http_request_header> header);

		void _PublishRequest(std::shared_ptr<stdx::http_request_header> header,stdx::http_form_ptr form);

		void _PushPending(std::string &&str);

		void _PushWaitHeader(std::string &&str);

		void _PushWaitBody(std::string &&str);

		uint64_t _GetContentLength();

		void _Reset();
	};

	class _HttpConnection:public stdx::basic_connection<stdx::http_request,stdx::http_response>
	{
		using base_t = stdx::basic_connection<stdx::http_request, stdx::http_response>;
	public:
		_HttpConnection(const stdx::socket& sock,size_t max_length);

		~_HttpConnection();

		void open()
		{
			_StartListen();
		}

		void close()
		{
			m_socket.close();
			m_is_connected = false;
		}

		stdx::task<stdx::http_request> read();

		stdx::task<size_t> write(const stdx::http_response& package);

		void read_until(std::function<bool(stdx::task_result<stdx::http_request>)>&& fn)
		{
			_ReadUntil(std::move(fn));
		}

		stdx::task<size_t> write(const char* buf, size_t size);

		stdx::task<void> write_file(stdx::file_handle file);

		bool is_connected() const
		{
			return m_is_connected;
		}
	private:
		std::list<stdx::task_completion_event<stdx::http_request>> m_waits;
		std::list<stdx::task_completion_event<stdx::http_request>> m_reqs;
		stdx::_HTTPRequestParser m_parser;
		stdx::socket m_socket;
		std::atomic_bool m_is_connected;
		stdx::spin_lock m_lock;

		void _ReadUntil(std::function<bool(stdx::task_result<stdx::http_request>)> fn);

		stdx::task_completion_event<stdx::http_request> _GetCE();

		void _PushRequest(stdx::http_request&& req);

		void _StartListen();
	};

	extern stdx::http_connection open_http_connection(const stdx::socket &sock,size_t max_length = 8*1024*1024);
}