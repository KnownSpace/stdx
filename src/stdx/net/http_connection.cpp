#include <stdx/net/http_connection.h>

stdx::_HTTPRequestParser::_HTTPRequestParser(size_t max_length)
	:m_reqs()
	,m_body_buf()
	,m_header_buf()
	,m_header(nullptr)
	,m_state()
	,m_max_length(max_length)
{}

void stdx::_HTTPRequestParser::_PublishRequest(std::shared_ptr<stdx::http_request_header> header)
{
	m_reqs.push_back(stdx::http_request(header));
	m_state = stdx::http_parser_state::finish;
}

void stdx::_HTTPRequestParser::_PublishRequest(std::shared_ptr<stdx::http_request_header> header,stdx::http_form_ptr form)
{
	m_reqs.push_back(stdx::http_request(header,form));
	m_state = stdx::http_parser_state::finish;
}

void stdx::_HTTPRequestParser::_PublishHeader(std::string&& str)
{
	try
	{
		m_header = std::make_shared<stdx::http_request_header>(stdx::http_request_header::from_string(stdx::string::from_u8_string(str)));
	}
	catch (const std::exception&)
	{
		_Reset();
		return;
	}
	if (m_header->method() == stdx::http_method::get)
	{
		_PublishRequest(m_header);
		m_header.reset();
		return;
	}
	else
	{
		if (m_header->exist(U("Content-Length")))
		{
			size_t size = _GetContentLength();
			if (size > m_max_length)
			{
				_Reset();
				return;
			}
			if (size == 0)
			{
				_PublishRequest(m_header);
				m_header.reset();
				return;
			}
			else
			{
				m_state = stdx::http_parser_state::wait_body;
				return;
			}
		}
		else
		{
			_PublishRequest(m_header);
			m_header.reset();
			return;
		}
	}
}

uint64_t stdx::_HTTPRequestParser::_GetContentLength()
{
	if (!m_header->exist(U("Content-Length")))
	{
		return 0;
	}
	return (*m_header)[U("Content-Length")].to_uint64();
}

void stdx::_HTTPRequestParser::_Reset()
{
	m_header.reset();
	m_header_buf.clear();
	m_body_buf.clear();
	m_state = stdx::http_parser_state::pending;
}

void stdx::_HTTPRequestParser::_PushPending(std::string&& str)
{
	size_t pos = str.rfind("\r\n\r\n");
	if (pos == std::string::npos)
	{
		m_header_buf = std::move(str);
		m_state = stdx::http_parser_state::wait_header;
		return;
	}
	else
	{
		size_t end = str.size();
		if ((pos + 4) == (end - 1))
		{
			_PublishHeader(std::move(str));
			return;
		}
		else
		{
			std::string&& header_str = str.substr(0, pos);
			_PublishHeader(std::move(header_str));
			if (m_state == stdx::http_parser_state::wait_body)
			{
				uint64_t length = _GetContentLength();
				if (length > m_max_length)
				{
					_Reset();
					return;
				}
				std::string&& body_str = str.substr(pos + 4, str.size() - pos - 4);
				m_body_buf = std::move(body_str);
				if (m_body_buf.size() == length)
				{
					stdx::string boundary(U(""));
					stdx::http_form_type type = stdx::http_form_type::urlencoded;
					if (m_header->exist(U("Content-Type")))
					{
						stdx::get_http_form_type_and_boundary((*m_header)[U("Content-Type")], boundary);
					}
					try
					{
						stdx::http_form_ptr form = stdx::make_http_form(type, m_body_buf, boundary);
						_PublishRequest(m_header, form);
					}
					catch (const std::exception&)
					{
						_Reset();
					}
				}
				else
				{
					m_state = stdx::http_parser_state::wait_body;
				}
			}
			return;
		}
	}
}

void stdx::_HTTPRequestParser::_PushWaitHeader(std::string&& str)
{
	size_t pos = str.rfind("\r\n\r\n");
	if (pos == std::string::npos)
	{
		m_header_buf.append(str);
		if (m_header_buf.size() > m_max_length)
		{
			_Reset();
		}
		return;
	}
	else
	{
		size_t end = str.size();
		if ((pos + 4) == (end - 1))
		{
			m_header_buf.append(str);
			str = std::move(m_header_buf);
			_PublishHeader(std::move(str));
			return;
		}
		else
		{
			m_header_buf.append(str.substr(0, pos));
			_PublishHeader(std::move(m_header_buf));
			m_header_buf.clear();
			if (m_state == stdx::http_parser_state::wait_body)
			{
				size_t length = _GetContentLength();
				if (length > m_max_length)
				{
					_Reset();
					return;
				}
				std::string&& body_str = str.substr(pos + 4, str.size() - pos - 4);
				m_body_buf.append(body_str);
				if (m_body_buf.size() == length)
				{
					stdx::string boundary(U(""));
					stdx::http_form_type type = stdx::http_form_type::urlencoded;
					if (m_header->exist(U("Content-Type")))
					{
						stdx::get_http_form_type_and_boundary((*m_header)[U("Content-Type")], boundary);
					}
					try
					{
						stdx::http_form_ptr form = stdx::make_http_form(type, m_body_buf, boundary);
						_PublishRequest(m_header, form);
					}
					catch (const std::exception&)
					{
						_Reset();
					}
				}
			}
		}
	}
}

void stdx::_HTTPRequestParser::_PushWaitBody(std::string&& str)
{
	m_body_buf.append(str);
	size_t length = _GetContentLength();
	if (length > m_max_length)
	{
		_Reset();
		return;
	}
	if (length == m_body_buf.size())
	{
		stdx::string boundary(U(""));
		stdx::http_form_type type = stdx::http_form_type::urlencoded;
		if (m_header->exist(U("Content-Type")))
		{
			stdx::get_http_form_type_and_boundary((*m_header)[U("Content-Type")], boundary);
		}
		try
		{
			stdx::http_form_ptr form = stdx::make_http_form(type, m_body_buf, boundary);
			_PublishRequest(m_header, form);
		}
		catch (const std::exception&)
		{
			_Reset();
		}
	}
}

void stdx::_HTTPRequestParser::push(std::string &&str)
{
	if (str.empty())
	{
		return;
	}
	if (m_state == stdx::http_parser_state::pending)
	{
		_PushPending(std::move(str));
	}
	else if (m_state == stdx::http_parser_state::wait_header)
	{
		_PushWaitHeader(std::move(str));
	}
	else if (m_state == stdx::http_parser_state::wait_body)
	{
		_PushWaitBody(std::move(str));
	}
}

stdx::http_request stdx::_HTTPRequestParser::pop()
{
	stdx::http_request req = m_reqs.front();
	m_reqs.pop_front();
	if (m_reqs.empty())
	{
		m_state = stdx::http_parser_state::pending;
	}
	return req;
}

stdx::_HttpConnection::_HttpConnection(const stdx::socket& sock,size_t max_length)
	:base_t()
	, m_waits()
	,m_reqs()
	, m_parser(max_length)
	, m_socket(sock)
	, m_is_connected(true)
	, m_lock()
{}

stdx::_HttpConnection::~_HttpConnection()
{
	close();
}
stdx::task<stdx::http_request> stdx::_HttpConnection::read()
{
	std::unique_lock<stdx::spin_lock> lock(m_lock);
	if (m_reqs.empty())
	{
		stdx::task_completion_event<stdx::http_request> ce;
		m_waits.push_back(ce);
		return ce.get_task();
	}
	else
	{
		stdx::task_completion_event<stdx::http_request> ce(m_reqs.front());
		m_reqs.pop_front();
		return ce.get_task();
	}
}

stdx::task<size_t> stdx::_HttpConnection::write(const stdx::http_response& package)
{
	std::vector<unsigned char>&& bytes = package.to_bytes();
	return m_socket.send((const char*)bytes.data(), bytes.size())
		.then([](stdx::task_result<stdx::network_send_event> r)
			{
				auto ev = r.get();
				return ev.size;
			});
}

void stdx::_HttpConnection::_ReadUntil(std::function<bool(stdx::task_result<stdx::http_request>)> fn)
{
	read().then([fn, this](stdx::task_result<stdx::http_request> r) mutable
		{
			if (!fn(r) && m_is_connected)
			{
				_ReadUntil(fn);
			}
		});
}

stdx::task_completion_event<stdx::http_request> stdx::_HttpConnection::_GetCE()
{
	std::unique_lock<stdx::spin_lock> lock(m_lock);
	if (m_waits.empty())
	{
		stdx::task_completion_event<stdx::http_request> ce;
		m_reqs.push_back(ce);
		lock.unlock();
		return ce;
	}
	else
	{
		stdx::task_completion_event<stdx::http_request> ce = m_waits.front();
		m_waits.pop_front();
		return ce;
	}
}

void stdx::_HttpConnection::_PushRequest(stdx::http_request&& req)
{
	stdx::task_completion_event<stdx::http_request> ce = _GetCE();
	ce.set_value(req);
	ce.run_on_this_thread();
	return;
}

void stdx::_HttpConnection::_StartListen()
{
	//m_socket.recv_until(4096, [this](stdx::task_result<stdx::network_recv_event> r) mutable
	//	{
	//		if (!m_is_connected)
	//		{
	//			return true;
	//		}
	//		try
	//		{
	//			auto ev = r.get();
	//			std::string str(ev.buffer, ev.size);
	//			ev.buffer.free();
	//			m_parser.push(std::move(str));
	//			while (m_parser.size())
	//			{
	//				auto req = m_parser.pop();
	//				_PushRequest(std::move(req));
	//			}
	//			return false;
	//		}
	//		catch (const std::system_error& err)
	//		{
	//			m_is_connected = false;
	//			return true;
	//		}
	//		catch (const std::exception& err)
	//		{
	//			m_is_connected = false;
	//			return true;
	//		}
	//	});
}

stdx::task<size_t> stdx::_HttpConnection::write(const char* buf, size_t size)
{
	return m_socket.send(buf,size)
		.then([](stdx::task_result<stdx::network_send_event> r) 
		{
			auto ev = r.get();
			return ev.size;
		});
}

stdx::task<void> stdx::_HttpConnection::write_file(stdx::file_handle file)
{
	return m_socket.send_file(file);
}

stdx::http_connection stdx::open_http_connection(const stdx::socket& sock, size_t max_length)
{
	return stdx::make_connection<stdx::_HttpConnection>(sock,max_length);
}