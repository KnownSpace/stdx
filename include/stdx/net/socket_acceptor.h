#pragma once
#include <stdx/net/acceptor.h>
#include <stdx/net/socket_connection.h>

namespace stdx
{
	template<typename _Input, typename _Output = _Input>
	class basic_socket_acceptor :public stdx::basic_acceptor<_Input, _Output>
	{
		using base_t = stdx::basic_acceptor<_Input, _Output>;
	public:

		using connection_t = typename base_t::connection_t;

		basic_socket_acceptor(stdx::socket sock)
			:base_t()
			,m_sock(sock)
#ifndef WIN32
			, m_nullfd(::open("/dev/null", O_CLOEXEC))
#endif
		{}

		virtual ~basic_socket_acceptor()
		{
#ifndef WIN32
			if (m_nullfd != -1)
			{
				::close(m_nullfd);
			}
#endif
		}

		virtual stdx::task<connection_t> accept() override
		{
			return m_sock.accept().then([this](stdx::task_result<stdx::network_connected_event> r)
				{
					stdx::network_connected_event ev = r.get();
					return make_connection(ev.connection);
				});
		}

		virtual void accept_until(stdx::cancel_token token, std::function<void(connection_t)> fn, std::function<void(std::exception_ptr)> err_handler) override
		{
#ifdef WIN32
			m_sock.accept_until(token, [this, fn, err_handler](stdx::network_connected_event ev) mutable
				{
					try
					{
						auto conn = make_connection(ev.connection);
						fn(conn);
					}
					catch (const std::exception&)
					{
						err_handler(std::current_exception());
					}
				}, err_handler);
#else
			m_sock.accept_until(token, [fn,err_handler,this](stdx::network_connected_event ev) mutable
				{
					try
					{
						auto conn = make_connection(ev.connection);
						fn(conn);
					}
					catch (const std::exception&)
					{
						err_handler(std::current_exception());
					}
				}, [this, err_handler](std::exception_ptr err) mutable
				{
					try
					{
						if (err)
						{
							std::rethrow_exception(err);
						}
					}
					catch (const std::system_error& e)
					{
						if (e.code().value() == EMFILE)
						{
							::close(m_nullfd);
							m_nullfd = ::accept(m_sock.native_handle(), nullptr, nullptr);
							::close(m_nullfd);
							m_nullfd = ::open("/dev/null", O_CLOEXEC);
						}
						err_handler(std::current_exception());
					}
					catch (const std::exception& e)
					{
						err_handler(std::current_exception());
					}
				});
#endif
		}

	protected:
		virtual connection_t make_connection(stdx::socket sock) = 0;

	private:
		stdx::socket m_sock;
#ifndef WIN32
		int m_nullfd;
#endif
	};
}