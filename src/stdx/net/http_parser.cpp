#include <stdx/net/http_parser.h>

stdx::_HttpRequestParserState::_HttpRequestParserState(const model_ptr_t model)
	:base_t()
	,m_model(model)
{}

typename stdx::_HttpRequestParserState::model_t &stdx::_HttpRequestParserState::state()
{
	return *m_model;
}

const typename stdx::_HttpRequestParserState::model_t& stdx::_HttpRequestParserState::state() const
{
	return *m_model;
}

stdx::http_request_parser_state_machine stdx::_HttpRequestParserState::reset()
{
	m_model->arg.clear();
	m_model->requests.clear();
	m_model->header_buffer.clear();
	m_model->body_buffer.clear();
	m_model->header.reset();
	m_model->state = stdx::http_parser_state::pending;
	auto t = stdx::make_state_machine<stdx::_HttpRequestParserPendingState>(m_model);
	return t;
}

void stdx::_HttpRequestParserState::_CheckArg() const
{
	if (m_model->arg.empty())
	{
		throw std::invalid_argument("argument could not be empty");
	}
}

bool stdx::_HttpRequestParserState::is_end() const
{
	return false;
}

bool stdx::_HttpRequestParserState::movable() const
{
	if (!m_model)
	{
		return false;
	}
	return !m_model->arg.empty();
}

uint64_t stdx::_HttpRequestParserState::_GetBodySize() const
{
	if (m_model->header)
	{
		if (m_model->header->exist(U("Content-Length")))
		{
			return (*(m_model->header))[U("Content-Length")].to_uint64();
		}
	}
	return 0;
}

std::shared_ptr<stdx::http_request_header> stdx::_HttpRequestParserState::_MakeHeader()
{
	auto header = std::make_shared<stdx::http_request_header>(stdx::http_request_header::from_string(stdx::string::from_u8_string(m_model->header_buffer)));
	m_model->header_buffer.clear();
	return header;
}

stdx::http_form_ptr stdx::_HttpRequestParserState::_MakeForm()
{
	stdx::http_form_type type = stdx::http_form_type::urlencoded;
	stdx::string boundary(U(""));
	if (m_model->header->exist(U("Content-Type")))
	{
		type = stdx::get_http_form_type_and_boundary((*(m_model->header))[U("Content-Length")], boundary);
	}
	auto form = stdx::make_http_form(type, m_model->body_buffer, boundary);
	m_model->body_buffer.clear();
	return form;
}

void stdx::_HttpRequestParserState::_FinishParse(const stdx::http_form_ptr& form)
{
	stdx::http_request req(m_model->header, form);
	m_model->requests.push_back(req);
	m_model->header.reset();
}

void stdx::_HttpRequestParserState::_FinishParse()
{
	stdx::http_request req(m_model->header);
	m_model->requests.push_back(req);
	m_model->header.reset();
}

stdx::_HttpRequestParserPendingState::_HttpRequestParserPendingState(const model_ptr_t model)
	:base_t(model)
{
	model->state = stdx::http_parser_state::pending;
}

stdx::http_request_parser_state_machine stdx::_HttpRequestParserPendingState::move_next()
{
	_CheckArg();
	return stdx::make_state_machine<stdx::_HttpRequestParserWaitHeaderState>(m_model);
}

stdx::_HttpRequestParserWaitHeaderState::_HttpRequestParserWaitHeaderState(const model_ptr_t model)
	:base_t(model)
{
	model->state = stdx::http_parser_state::wait_header;
}

stdx::http_request_parser_state_machine stdx::_HttpRequestParserWaitHeaderState::move_next()
{
	_CheckArg();
	uint64_t size = m_model->header_buffer.size() + m_model->arg.size();
	if (size > m_model->max_size)
	{
		m_model->arg.clear();
		auto t = stdx::make_state_machine<stdx::_HttpRequestParserErrorState>(m_model);
		return t;
	}
	size_t pos = m_model->arg.rfind("\r\n\r\n");
	if (pos == std::string::npos)
	{
		m_model->header_buffer.append(m_model->arg);
		m_model->arg.clear();
		auto t = stdx::make_state_machine<stdx::_HttpRequestParserWaitHeaderState>(m_model);
		return t;
	}
	size_t end = m_model->arg.size();
	if ((pos+4)==end)
	{
		m_model->header_buffer.append(m_model->arg);
		m_model->arg.clear();
		try
		{
			m_model->header = _MakeHeader();
			if (m_model->header->method() == stdx::http_method::get)
			{
				_FinishParse();
				auto t = stdx::make_state_machine<stdx::_HttpRequestParserFinishState>(m_model);
				return t;
			}
			else
			{
				uint64_t size = _GetBodySize();
				if (size > m_model->max_size)
				{
					auto t = stdx::make_state_machine<stdx::_HttpRequestParserErrorState>(m_model);
					return t;
				}
				else
				{
					auto t = stdx::make_state_machine<stdx::_HttpRequestParserWaitBodyState>(m_model);
					return t;
				}
			}
		}
		catch (const std::invalid_argument&)
		{
			auto t = stdx::make_state_machine<stdx::_HttpRequestParserErrorState>(m_model);
			return t;
		}
		catch (const std::exception&)
		{
			throw;
		}
	}
	else
	{
		m_model->header_buffer.append(m_model->arg.c_str(),pos);
		m_model->header = _MakeHeader();
		try
		{
			uint64_t size = _GetBodySize();
			if (size > m_model->max_size)
			{
				m_model->arg.clear();
				auto t = stdx::make_state_machine<stdx::_HttpRequestParserErrorState>(m_model);
				return t;
			}
		}
		catch (const std::invalid_argument&)
		{
			m_model->arg.clear();
			auto t = stdx::make_state_machine<stdx::_HttpRequestParserErrorState>(m_model);
			return t;
		}
		catch (const std::exception&)
		{
			m_model->arg.clear();
			throw;
		}
		m_model->arg.erase(0, pos+4);
		auto t = stdx::make_state_machine<stdx::_HttpRequestParserWaitBodyState>(m_model);
		return t;
	}
}

stdx::_HttpRequestParserWaitBodyState::_HttpRequestParserWaitBodyState(const model_ptr_t model)
	:base_t(model)
{
	m_model->state = stdx::http_parser_state::wait_body;
}

stdx::http_request_parser_state_machine stdx::_HttpRequestParserWaitBodyState::move_next()
{
	_CheckArg();
	uint64_t size = m_model->arg.size() + m_model->body_buffer.size();
	if (size > m_model->max_size)
	{
		m_model->arg.clear();
		auto t = stdx::make_state_machine<stdx::_HttpRequestParserErrorState>(m_model);
		return t;
	}
	uint64_t body_size = _GetBodySize();
	if (size == body_size)
	{
		m_model->body_buffer.append(m_model->arg);
		m_model->arg.clear();
		try
		{
			auto form = _MakeForm();
			_FinishParse(form);
			auto t = stdx::make_state_machine<stdx::_HttpRequestParserFinishState>(m_model);
			return t;
		}
		catch (const std::invalid_argument&)
		{
			auto t = stdx::make_state_machine<stdx::_HttpRequestParserErrorState>(m_model);
			return t;
		}
		catch (const std::exception &)
		{
			m_model->body_buffer.clear();
			throw;
		}
	}
	else if (size > body_size)
	{
		m_model->arg.clear();
		auto t = stdx::make_state_machine<stdx::_HttpRequestParserErrorState>(m_model);
		return t;
	}
	m_model->body_buffer.append(m_model->arg);
	m_model->arg.clear();
	auto t = stdx::make_state_machine<stdx::_HttpRequestParserWaitBodyState>(m_model);
	return t;
}

stdx::_HttpRequestParserFinishState::_HttpRequestParserFinishState(const model_ptr_t model)
	:base_t(model)
{
	model->state = stdx::http_parser_state::finish;
}

stdx::http_request_parser_state_machine stdx::_HttpRequestParserFinishState::move_next()
{
	return reset();
}

stdx::_HttpRequestParserErrorState::_HttpRequestParserErrorState(const model_ptr_t model)
	:base_t(model)
{
	model->state = stdx::http_parser_state::error;
}

stdx::http_request_parser_state_machine stdx::_HttpRequestParserErrorState::move_next()
{
	return reset();
}

bool stdx::_HttpRequestParserFinishState::is_end() const
{
	return true;
}

bool stdx::_HttpRequestParserErrorState::is_end() const
{
	return true;
}

stdx::http_request_parser::http_request_parser(uint64_t max_size)
	:m_model(std::make_shared<stdx::http_request_parser_model>())
	,m_state_machine(stdx::make_state_machine<stdx::_HttpRequestParserPendingState>(m_model))
{
	m_model->max_size = max_size;
}

stdx::http_request_parser::http_request_parser(const self_t& other)
	:m_model(other.m_model)
	,m_state_machine(other.m_state_machine)
{}

stdx::http_request_parser::http_request_parser(self_t&& other) noexcept
	:m_model(std::move(other.m_model))
	,m_state_machine(std::move(other.m_state_machine))
{}

typename stdx::http_request_parser::self_t& stdx::http_request_parser::operator=(const self_t& other)
{
	stdx::http_request_parser tmp(other);
	stdx::copy_by_move(*this, std::move(tmp));
	return *this;
}

typename stdx::http_request_parser::self_t& stdx::http_request_parser::operator=(self_t&& other) noexcept
{
	m_model = std::move(other.m_model);
	m_state_machine = std::move(other.m_state_machine);
	return *this;
}

void stdx::http_request_parser::push(stdx::buffer buf, size_t size)
{
	if (!buf.size() || (!buf.check()))
	{
		return;
	}
	m_model->arg.append(buf,size);
	while (m_state_machine.movable())
	{
		m_state_machine = m_state_machine.move_next();
	}
}

size_t stdx::http_request_parser::finish_count() const
{
	return m_model->requests.size();
}

bool stdx::http_request_parser::error() const
{
	return m_model->state == stdx::http_parser_state::error;
}

bool stdx::http_request_parser::operator==(const self_t& other)
{
	return m_state_machine == other.m_state_machine && m_model == other.m_model;
}

stdx::http_request_parser::operator bool() const
{
	return m_state_machine && m_model;
}

stdx::http_request stdx::http_request_parser::pop()
{
	if (!m_model)
	{
		throw std::bad_alloc();
	}
	auto req = m_model->requests.front();
	m_model->requests.pop_front();
	return req;
}