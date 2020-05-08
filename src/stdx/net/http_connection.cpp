#include <stdx/net/http_connection.h>

stdx::basic_http_connection::basic_http_connection(const stdx::socket& sock, uint64_t max_size)
	:base_t(sock)
	,m_parser(max_size)
{}

bool stdx::basic_http_connection::_CheckParser(stdx::http_request_parser &parser, stdx::task_completion_event<stdx::http_request> &ce)
{
	if (parser.finish_count())
	{
		ce.set_value(parser.pop());
		ce.run_on_this_thread();
		return true;
	}
	else if (parser.error())
	{
		ce.set_exception(std::make_exception_ptr(stdx::parse_error("parser fault")));
		ce.run_on_this_thread();
		return true;
	}
	return false;
}

stdx::task<stdx::http_request> stdx::basic_http_connection::read()
{
	stdx::task_completion_event<stdx::http_request> ce;
	if (_CheckParser(m_parser, ce))
	{
		return ce.get_task();
	}
	auto parser = m_parser;
	m_socket.recv_until(4096,[parser,ce](stdx::task_result<stdx::network_recv_event> r) mutable
	{
		try
		{
			auto ev = r.get();
			parser.push(ev.buffer, ev.size);
			if (_CheckParser(parser, ce))
			{
				return true;
			}
			return false;
		}
		catch (const std::exception&)
		{
			ce.set_exception(std::current_exception());
			ce.run_on_this_thread();
			return true;
		}
	});
	return ce.get_task();
}

stdx::task<size_t> stdx::basic_http_connection::write(const stdx::http_response& package)
{
	auto &&vec = package.to_bytes();
	return base_t::write((const char *)vec.data(), vec.size());
}

//void stdx::basic_http_connection::read_until(std::function<bool(stdx::task_result<stdx::http_request>)> fn)
//{
//	if (!m_parser.error())
//	{
//		while (m_parser.finish_count())
//		{
//			if (fn(stdx::make_task_result(std::move(m_parser.pop()))))
//			{
//				return;
//			}
//		}
//		auto parser = m_parser;
//		m_socket.recv_until(4096, [parser,fn](stdx::task_result<stdx::network_recv_event> r) mutable
//			{
//				try
//				{
//					auto ev = r.get();
//					parser.push(ev.buffer, ev.size);
//					if (!parser.error())
//					{
//						while (parser.finish_count())
//						{
//							if (fn(stdx::make_task_result(std::move(parser.pop()))))
//							{
//								return true;
//							}
//						}
//						return false;
//					}
//					return fn(stdx::make_error_result<stdx::http_request>(std::make_exception_ptr(stdx::parse_error("parser fault"))));
//				}
//				catch (const std::exception&)
//				{
//					return fn(stdx::make_error_result<stdx::http_request>(std::current_exception()));;
//				}
//			});
//	}
//	else
//	{
//		if (!fn(stdx::make_error_result<stdx::http_request>(std::make_exception_ptr(stdx::parse_error("parser fault")))))
//		{
//			read_until(fn);
//		}
//	}
//}

stdx::http_connection stdx::make_http_connection(const stdx::socket& sock, uint64_t max_size)
{
	return stdx::make_connection<stdx::basic_http_connection>(sock,max_size);
}