#pragma once
#include <stdx/net/connection.h>

namespace stdx
{
	template<typename _Input, typename _Output = _Input>
	class basic_socket_connection :public stdx::basic_connection<_Input, _Output>
	{
	public:
		basic_socket_connection(const stdx::socket& sock)
			:m_socket(sock)
		{}

		virtual ~basic_socket_connection() = default;

		virtual stdx::task<size_t> write(const char* buf, size_t size) override
		{
			stdx::uint64_union u;
			u.value = size;
			if (u.height != 0)
			{
				return stdx::error_task<size_t>(std::make_exception_ptr(std::invalid_argument("buffer size too big")));
			}
			return m_socket.send(buf, u.low)
				.then([](stdx::task_result<stdx::network_send_event> r)
					{
						auto ev = r.get();
						return ev.size;
					});
		}

		virtual stdx::task<void> write_file(stdx::file_handle file) override
		{
			return m_socket.send_file(file);
		}

		virtual void close() override
		{
			m_socket.close();
		}
	protected:
		stdx::socket m_socket;
	};
}