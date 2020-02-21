#include <stdx/net/http.h>

stdx::http_version stdx::_HttpHead::version() const
{
	return m_version;
}

bool stdx::_HttpHead::have_cache_control() const
{
	return (m_cache_control.type == stdx::http_cache_control_type::none);
}

stdx::http_cache_control &stdx::_HttpHead::cache_control()
{
	return m_cache_control;
}

const stdx::http_cache_control &stdx::_HttpHead::cache_control() const
{
	return m_cache_control;
}

stdx::_HttpRequestHead::_HttpRequestHead()
	:stdx::_HttpHead(stdx::http_head_type::request)
	, m_method(stdx::http_method::get)
	, m_url()
	, m_accept()
	, m_accept_charset()
	, m_accept_lanuage()
	, m_connection(stdx::http_connection_type::keep_alive)
	, m_content_length(0)
	, m_cookies()
	, m_host()
	, m_user_agent()
	, m_authorization()
	, m_other_head()
{}

stdx::_HttpRequestHead::_HttpRequestHead(stdx::http_version version)
	:stdx::_HttpHead(stdx::http_head_type::request,version)
	, m_method(stdx::http_method::get)
	, m_url()
	, m_accept()
	, m_accept_charset()
	, m_accept_lanuage()
	, m_connection(stdx::http_connection_type::keep_alive)
	, m_content_length(0)
	, m_cookies()
	, m_host()
	, m_user_agent()
	, m_authorization()
	, m_other_head()
{}

stdx::_HttpRequestHead::_HttpRequestHead(stdx::http_method method)
	:stdx::_HttpHead(stdx::http_head_type::request)
	, m_method(method)
	, m_url()
	, m_accept()
	, m_accept_charset()
	, m_accept_lanuage()
	, m_connection(stdx::http_connection_type::keep_alive)
	, m_content_length(0)
	, m_cookies()
	, m_host()
	, m_user_agent()
	, m_authorization()
	, m_other_head()
{}

stdx::_HttpRequestHead::_HttpRequestHead(stdx::http_version version, stdx::http_method method)
	:stdx::_HttpHead(stdx::http_head_type::request,version)
	, m_method(method)
	, m_url()
	, m_accept()
	, m_accept_charset()
	, m_accept_lanuage()
	, m_connection(stdx::http_connection_type::keep_alive)
	, m_content_length(0)
	, m_cookies()
	, m_host()
	, m_user_agent()
	, m_authorization()
	, m_other_head()
{}

stdx::string stdx::_HttpRequestHead::to_string() const
{
	return stdx::string();
}

stdx::http_method stdx::_HttpRequestHead::method() const
{
	return m_method;
}

void stdx::_HttpRequestHead::method(stdx::http_method method)
{
	m_method = method;
}

const stdx::string &stdx::_HttpRequestHead::url() const
{
	return m_url;
}

void stdx::_HttpRequestHead::url(const stdx::string& url)
{
	m_url = url;
}

bool stdx::_HttpRequestHead::have_accept() const
{
	return (!m_accept.empty());
}

std::list<stdx::string>& stdx::_HttpRequestHead::accept()
{
	return m_accept;
}

const std::list<stdx::string>& stdx::_HttpRequestHead::accept() const
{
	return m_accept;
}

bool stdx::_HttpRequestHead::hava_accept_charset() const
{
	return (!m_accept.empty());
}

std::list<stdx::string>& stdx::_HttpRequestHead::accept_charset()
{
	return m_accept_charset;
}

const std::list<stdx::string>& stdx::_HttpRequestHead::accept_charset() const
{
	return m_accept_charset;
}

bool stdx::_HttpRequestHead::hava_accept_language() const
{
	return (!m_accept_lanuage.empty());
}

std::list<stdx::string>& stdx::_HttpRequestHead::accept_language()
{
	return m_accept_lanuage;
}

const std::list<stdx::string>& stdx::_HttpRequestHead::accept_language() const
{
	return m_accept_lanuage;
}

bool stdx::_HttpRequestHead::enable_connection_type() const
{
	return (m_version == stdx::http_version::http_1_1)||(m_version == stdx::http_version::http_2_0);
}

stdx::http_connection_type stdx::_HttpRequestHead::connection_type() const
{
	return m_connection;
}

bool stdx::_HttpRequestHead::using_content_length() const
{
	return (m_content_length != 0);
}

uint64_t stdx::_HttpRequestHead::content_length() const
{
	return m_content_length;
}

void stdx::_HttpRequestHead::content_length(const uint64_t& length)
{
	m_content_length = length;
}

bool stdx::_HttpRequestHead::have_cookie() const
{
	return (!m_cookies.empty());
}

std::list<stdx::http_cookie>& stdx::_HttpRequestHead::cookies()
{
	return m_cookies;
}

const std::list<stdx::http_cookie>& stdx::_HttpRequestHead::cookies() const
{
	return m_cookies;
}

bool stdx::_HttpRequestHead::have_host() const
{
	return (!m_host.empty());
}

const stdx::string &stdx::_HttpRequestHead::host() const
{
	return m_host;
}

void stdx::_HttpRequestHead::host(const stdx::string& host)
{
	m_host = host;
}

bool stdx::_HttpRequestHead::have_user_agent() const
{
	return (!m_user_agent.empty());
}

std::list<stdx::string>& stdx::_HttpRequestHead::user_agent()
{
	return m_user_agent;
}

const std::list<stdx::string>& stdx::_HttpRequestHead::user_agent() const
{
	return m_user_agent;
}

bool stdx::_HttpRequestHead::have_authorization() const
{
	return false;
}

stdx::http_authorization& stdx::_HttpRequestHead::authorization()
{
	return m_authorization;
}

const stdx::http_authorization &stdx::_HttpRequestHead::authorization() const
{
	return m_authorization;
}

std::unordered_map<stdx::string, stdx::string>& stdx::_HttpRequestHead::other_head()
{
	return m_other_head;
}

const std::unordered_map<stdx::string, stdx::string>& stdx::_HttpRequestHead::other_head() const
{
	return m_other_head;
}