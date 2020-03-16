#include <stdx/net/http.h>

stdx::http_cookie::http_cookie()
	:m_name()
	,m_value()
	,m_enable_max_age(false)
	,m_max_age(0)
	,m_path()
	,m_secure()
	,m_http_only()
	,m_domain()
	,m_enable_expires(false)
	,m_expires()
{}

stdx::http_cookie::http_cookie(const stdx::string& name, const stdx::string& value)
	:m_name(name)
	,m_value(value)
	,m_enable_max_age(false)
	,m_max_age(0)
	,m_path()
	,m_secure(false)
	,m_http_only(false)
	,m_domain()
	, m_enable_expires(false)
	,m_expires()
{
	if (m_name.begin_with(U("__Secure-")))
	{
		m_secure = true;
	}
	else if (m_name.begin_with(U("__Host-")))
	{
		m_path = U("/");
	}
}

stdx::http_cookie::http_cookie(const stdx::http_cookie& other)
	:m_name(other.m_name)
	,m_value(other.m_value)
	,m_enable_max_age(other.m_enable_max_age)
	,m_max_age(other.m_max_age)
	,m_path(other.m_path)
	,m_secure(other.m_secure)
	,m_http_only(other.m_http_only)
	,m_domain(other.m_domain)
	,m_enable_expires(other.m_enable_expires)
	,m_expires(other.m_expires)
{}

stdx::http_cookie::http_cookie(stdx::http_cookie&& other) noexcept
	:m_name(other.m_name)
	,m_value(other.m_value)
	,m_enable_max_age(other.m_enable_max_age)
	,m_max_age(other.m_max_age)
	,m_path(other.m_path)
	,m_secure(other.m_secure)
	,m_http_only(other.m_http_only)
	,m_domain(other.m_domain)
	,m_enable_expires(other.m_enable_expires)
	,m_expires(other.m_expires)
{}

stdx::http_cookie& stdx::http_cookie::operator=(const stdx::http_cookie& other)
{
	m_name = other.m_name;
	m_value = other.m_value;
	m_enable_max_age = other.m_enable_max_age;
	m_max_age = other.m_max_age;
	m_path = other.m_path;
	m_secure = other.m_secure;
	m_http_only = other.m_http_only;
	m_domain = other.m_domain;
	m_enable_expires = other.m_enable_expires;
	m_expires = other.m_expires;
	return *this;
}

stdx::http_cookie& stdx::http_cookie::operator=(stdx::http_cookie&& other) noexcept
{
	m_name = other.m_name;
	m_value = other.m_value;
	m_path = other.m_path;
	m_secure = other.m_secure;
	m_http_only = other.m_http_only;
	m_domain = other.m_domain;
	m_enable_expires = other.m_enable_expires;
	m_expires = other.m_expires;
	return *this;
}

stdx::string stdx::http_cookie::to_cookie_string() const
{
	if (m_name.empty())
	{
		throw std::logic_error("you should set coookie name first");
	}
	stdx::string str(U("Cookie: "));
	str.append(m_name);
	if (!m_value.empty())
	{
		str.push_back(U('='));
		str.append(m_value);
	}
	return str;
}

stdx::string stdx::http_cookie::to_cookie_string_without_header() const
{
	if (m_name.empty())
	{
		throw std::logic_error("you should set coookie name first");
	}
	stdx::string str;
	str.append(m_name);
	if (!m_value.empty())
	{
		str.push_back(U('='));
		str.append(m_value);
	}
	return str;
}

stdx::string stdx::http_cookie::to_set_cookie_string() const
{
	if (m_name.empty())
	{
		throw std::logic_error("you should set coookie namefirst");
	}
	stdx::string str(U("Set-Cookie: "));
	str.append(m_name);
	if (!m_value.empty())
	{
		str.push_back(U('='));
		str.append(m_value);
	}
	if (m_enable_max_age)
	{
		str.append(U("; "));
		str.append(U("Max-Age="));
		str.append(stdx::to_string(m_max_age));
	}
	else
	{
		if (m_enable_expires)
		{
			str.append(U("; "));
			str.append(U("Expires="));
			stdx::string tmp(stdx::to_day_name(m_expires.week_day()));
			tmp.append(U(", "));
			str.append(tmp);
			tmp = m_expires.to_string(U("%day {0} %year %hour:%min:%sec GMT"));
			stdx::format_string(tmp, stdx::to_month_name(m_expires.month()));
			str.append(str);
		}
	}
	if (!m_path.empty())
	{
		str.append(U("; "));
		str.append(U("Path="));
		str.append(m_path);
	}
	if (!m_domain.empty())
	{
		str.append(U("; "));
		str.append(U("Domain="));
		str.append(m_domain);
	}
	if (m_secure)
	{
		str.append(U("; "));
		str.append(U("Secure"));
	}
	if (m_http_only)
	{
		str.append(U("; "));
		str.append(U("HttpOnly"));
	}
	return str;
}

stdx::string stdx::http_cookie::to_set_cookie_string_without_header() const
{
	if (m_name.empty())
	{
		throw std::logic_error("you should set coookie name first");
	}
	stdx::string str;
	str.append(m_name);
	if (!m_value.empty())
	{
		str.push_back(U('='));
		str.append(m_value);
	}
	if (m_enable_max_age)
	{
		str.append(U("; "));
		str.append(U("Max-Age="));
		str.append(stdx::to_string(m_max_age));
	}
	if (!m_path.empty())
	{
		str.append(U("; "));
		str.append(U("Path="));
		str.append(m_path);
	}
	if (!m_domain.empty())
	{
		str.append(U("; "));
		str.append(U("Domain="));
		str.append(m_domain);
	}
	if (m_secure)
	{
		str.append(U("; "));
		str.append(U("Secure"));
	}
	if (m_http_only)
	{
		str.append(U("; "));
		str.append(U("HttpOnly"));
	}
	return str;
}

stdx::string& stdx::http_cookie::name()
{
	return m_name;
}

const stdx::string& stdx::http_cookie::name() const
{
	return m_name;
}

stdx::string& stdx::http_cookie::value()
{
	return m_value;
}

const stdx::string& stdx::http_cookie::value() const
{
	return m_value;
}

bool& stdx::http_cookie::enable_max_age()
{
	return m_enable_max_age;
}

const bool &stdx::http_cookie::enable_max_age() const
{
	return m_enable_max_age;
}

typename stdx::http_cookie::max_age_t& stdx::http_cookie::max_age()
{
	return m_max_age;
}

const typename stdx::http_cookie::max_age_t& stdx::http_cookie::max_age() const
{
	return m_max_age;
}

stdx::string& stdx::http_cookie::path()
{
	return m_path;
}

const stdx::string& stdx::http_cookie::path() const
{
	return m_path;
}

stdx::http_cookie& stdx::http_cookie::set_secure(bool secure)
{
	m_secure = secure;
	return *this;
}

bool stdx::http_cookie::secure() const
{
	return m_secure;
}

stdx::http_cookie& stdx::http_cookie::set_http_only(bool http_only)
{
	m_http_only = http_only;
	return *this;
}

bool stdx::http_cookie::http_only() const
{
	return m_http_only;
}

stdx::string& stdx::http_cookie::domain()
{
	return m_domain;
}

const stdx::string& stdx::http_cookie::domain() const
{
	return m_domain;
}

bool stdx::http_cookie::enable_expires() const
{
	if (m_enable_max_age)
	{
		return false;
	}
	return m_enable_expires;
}

void stdx::http_cookie::set_enable_expires(bool value)
{
	if (!m_enable_max_age)
	{
		m_enable_expires = value;
	}
	else
	{
		m_enable_expires = false;
	}
}

stdx::datetime& stdx::http_cookie::expires()
{
	return m_expires;
}

const stdx::datetime& stdx::http_cookie::expires() const
{
	return m_expires;
}


stdx::http_cache_control::http_cache_control()
	:m_max_age(0)
	,m_max_stale(0)
	,m_min_fresh(0)
	,m_s_max_age(0)
	,m_type(stdx::http_cache_control_type::no_store)
{}

stdx::http_cache_control::http_cache_control(const stdx::http_cache_control& other)
	:m_max_age(other.m_max_age)
	,m_max_stale(other.m_max_stale)
	,m_min_fresh(other.m_min_fresh)
	,m_s_max_age(other.m_s_max_age)
	,m_type(other.m_type)
{}

stdx::http_cache_control::http_cache_control(stdx::http_cache_control&& other) noexcept
	:m_max_age(other.m_max_age)
	,m_max_stale(other.m_max_stale)
	,m_min_fresh(other.m_min_fresh)
	,m_s_max_age(other.m_s_max_age)
	,m_type(other.m_type)
{}

stdx::http_cache_control& stdx::http_cache_control::operator=(const stdx::http_cache_control& other)
{
	m_max_age = other.m_max_age;
	m_max_stale = other.m_max_stale;
	m_min_fresh = other.m_min_fresh;
	m_s_max_age = other.m_s_max_age;
	m_type = other.m_type;
	return *this;
}

stdx::http_cache_control& stdx::http_cache_control::operator=(stdx::http_cache_control&& other) noexcept
{
	m_max_age = other.m_max_age;
	m_max_stale = other.m_max_stale;
	m_min_fresh = other.m_min_fresh;
	m_s_max_age = other.m_s_max_age;
	m_type = other.m_type;
	return *this;
}

stdx::string stdx::http_cache_control::to_string() const
{
	stdx::string str(U("Cache-control: "));
	str.append(type_to_string(m_type));
	if (m_max_age != 0)
	{
		str.append(U(", "));
		str.append(U("max-age="));
		str.append(stdx::to_string(m_max_age));
	}
	if (m_max_stale != 0)
	{
		str.append(U(", "));
		str.append(U("max-stale="));
		str.append(stdx::to_string(m_max_stale));
	}
	if (m_min_fresh != 0)
	{
		str.append(U(", "));
		str.append(U("min-fresh="));
		str.append(stdx::to_string(m_min_fresh));
	}
	if (m_s_max_age != 0)
	{
		str.append(U(", "));
		str.append(U("s-maxage="));
		str.append(stdx::to_string(m_s_max_age));
	}
	return str;
}

stdx::string stdx::http_cache_control::to_string_without_header() const
{
	stdx::string str;
	str.append(type_to_string(m_type));
	if (m_max_age != 0)
	{
		str.append(U(", "));
		str.append(U("max-age="));
		str.append(stdx::to_string(m_max_age));
	}
	if (m_max_stale != 0)
	{
		str.append(U(", "));
		str.append(U("max-stale="));
		str.append(stdx::to_string(m_max_stale));
	}
	if (m_min_fresh != 0)
	{
		str.append(U(", "));
		str.append(U("min-fresh="));
		str.append(stdx::to_string(m_min_fresh));
	}
	if (m_s_max_age != 0)
	{
		str.append(U(", "));
		str.append(U("s-maxage="));
		str.append(stdx::to_string(m_s_max_age));
	}
	return str;
}

stdx::string stdx::http_cache_control::type_to_string(stdx::http_cache_control_type type)
{
	switch (type)
	{
	case stdx::http_cache_control_type::no_cache:
		return U("no-cache");
	case stdx::http_cache_control_type::no_store:
		return U("no-store");
	case stdx::http_cache_control_type::only_if_cache:
		return U("only-if-cache");
	case stdx::http_cache_control_type::_public:
		return U("public");
	case stdx::http_cache_control_type::_private:
		return U("private");
	case stdx::http_cache_control_type::no_transform:
		return U("no-transform");
	case stdx::http_cache_control_type::must_revalidate:
		return U("must-revalidate");
	case stdx::http_cache_control_type::proxy_revalidate:
		return U("proxy-revalidate");
	default:
		throw std::invalid_argument("unkonw cache type");
	}
}

bool stdx::http_cache_control::enable_max_age() const
{
	return (m_max_age == 0) && (!enable_s_max_age());
}

typename stdx::http_cache_control::max_age_t& stdx::http_cache_control::max_age()
{
	return m_max_age;
}

const typename stdx::http_cache_control::max_age_t stdx::http_cache_control::max_age() const
{
	return m_max_age;
}

bool stdx::http_cache_control::enable_max_stale() const
{
	return m_max_stale == 0;
}

typename stdx::http_cache_control::max_stale_t& stdx::http_cache_control::max_stale()
{
	return m_max_stale;
}

const typename stdx::http_cache_control::max_stale_t& stdx::http_cache_control::max_stale() const
{
	return m_max_stale;
}

bool stdx::http_cache_control::enable_min_fresh() const
{
	return m_min_fresh == 0;
}

typename stdx::http_cache_control::min_fresh_t& stdx::http_cache_control::min_fresh()
{
	return m_min_fresh;
}

const typename stdx::http_cache_control::min_fresh_t& stdx::http_cache_control::min_fresh() const
{
	return m_min_fresh;
}

bool stdx::http_cache_control::enable_s_max_age() const
{
	return m_s_max_age == 0;
}

typename stdx::http_cache_control::max_age_t& stdx::http_cache_control::s_max_age()
{
	return m_s_max_age;
}

const typename stdx::http_cache_control::max_age_t& stdx::http_cache_control::s_max_age() const
{
	return m_s_max_age;
}

stdx::http_cache_control_type stdx::http_cache_control::cache_type() const
{
	return m_type;
}

void stdx::http_cache_control::set_cache_type(stdx::http_cache_control_type type)
{
	m_type = type;
}

stdx::http_header::http_header()
	:m_version(stdx::http_version::http_1_1)
	,m_headers()
{}

stdx::http_header::http_header(stdx::http_version version)
	: m_version(version)
	, m_headers()
{}

stdx::http_header::http_header(const stdx::http_header& other)
	:m_version(other.m_version)
	,m_headers(other.m_headers)
{}

stdx::http_header::http_header(stdx::http_header&& other) noexcept
	:m_version(other.m_version)
	,m_headers(other.m_headers)
{}

stdx::http_header& stdx::http_header::operator=(const stdx::http_header& other)
{
	m_version = other.m_version;
	m_headers = other.m_headers;
	return *this;
}

stdx::http_header& stdx::http_header::operator=(stdx::http_header&& other) noexcept
{
	m_version = other.m_version;
	m_headers = other.m_headers;
	return *this;
}

stdx::string stdx::http_header::to_string() const
{
	stdx::string str;
	for (auto begin = m_headers.begin(),end = m_headers.end();begin != end;begin++)
	{
		str.append(begin->first);
		str.append(U(" :"));
		str.append(begin->second);
		str.append(U("\r\n"));
	}
	return str;
}

const stdx::http_version& stdx::http_header::version() const
{
	return m_version;
}

stdx::http_version& stdx::http_header::version()
{
	return m_version;
}

stdx::http_header& stdx::http_header::add_header(stdx::string&& name,stdx::string&& value)
{
	if (name.empty())
	{
		throw std::invalid_argument("name could not be empty");
	}
	if (value.empty())
	{
		throw std::invalid_argument("value could not be empty");
	}
	if (name.back() == U(' '))
	{
		name.erase_back();
	}
	if (value.front() == U(' '))
	{
		value.erase_front();
	}
	m_headers.emplace(name,value);
	return *this;
}

stdx::http_header& stdx::http_header::add_header(const stdx::string& name, const stdx::string& value)
{
	if (name.empty())
	{
		throw std::invalid_argument("name could not be empty");
	}
	if (value.empty())
	{
		throw std::invalid_argument("value could not be empty");
	}
	if (name.back() == U(' ') || value.front() == U(' '))
	{
		stdx::string t1(name),t2(value);
		return add_header(std::move(t1),std::move(t2));
	}
	m_headers.emplace(name, value);
	return *this;
}

stdx::http_header& stdx::http_header::remove_header(stdx::string&& name)
{
	if (name.empty())
	{
		throw std::invalid_argument("name could not be empty");
	}
	if (name.back() == U(' '))
	{
		name.erase_back();
	}
	m_headers.erase(name);
	return *this;
}

stdx::http_header& stdx::http_header::remove_header(const stdx::string& name)
{
	if (name.empty())
	{
		throw std::invalid_argument("name could not be empty");
	}
	if (name.back() == U(' '))
	{
		stdx::string tmp(name);
		return remove_header(std::move(tmp));
	}
	m_headers.erase(name);
	return *this;
}

stdx::http_header& stdx::http_header::add_header(const stdx::string& raw_string)
{
	if (raw_string.empty())
	{
		throw std::invalid_argument("raw string could not be empty");
	}
	std::list<stdx::string> &&list = raw_string.split(U(":"));
	if (list.size() < 2)
	{
		throw std::invalid_argument("invalid raw string");
	}
	add_header(list.front(), list.back());
	return *this;
}

const stdx::string& stdx::http_header::operator[](const stdx::string& name) const
{
	if (name.back() == U(' '))
	{
		stdx::string tmp(name);
		return this->operator[](std::move(tmp));
	}
	return m_headers.at(name);
}

stdx::string& stdx::http_header::operator[](const stdx::string& name)
{
	if (name.back() == U(' '))
	{
		stdx::string tmp(name);
		return this->operator[](std::move(tmp));
	}
	return m_headers.at(name);
}

const stdx::string& stdx::http_header::operator[](stdx::string&& name) const
{
	if (name.back() == U(' '))
	{
		name.erase_back();
	}
	return m_headers.at(name);
}

stdx::string& stdx::http_header::operator[](stdx::string&& name)
{
	if (name.back() == U(' '))
	{
		name.erase_back();
	}
	return m_headers.at(name);
}

bool stdx::http_header::exist(const stdx::string& name) const
{
	if (name.back() == U(' '))
	{
		stdx::string tmp(name);
		return exist(std::move(tmp));
	}
	return m_headers.find(name) != m_headers.end();
}

bool stdx::http_header::exist(stdx::string&& name) const
{
	if (name.back() == U(' '))
	{
		name.erase_back();
	}
	return m_headers.find(name) != m_headers.end();
}

void stdx::http_header::clear()
{
	m_headers.clear();
}

size_t stdx::http_header::size() const
{
	return m_headers.size();
}

typename stdx::http_header::iterator_t stdx::http_header::begin()
{
	return m_headers.begin();
}

typename stdx::http_header::const_iterator_t stdx::http_header::cbegin() const
{
	return m_headers.cbegin();
}

typename stdx::http_header::iterator_t stdx::http_header::end()
{
	return m_headers.end();
}

typename stdx::http_header::const_iterator_t stdx::http_header::cend()
{
	return m_headers.cend();
}

stdx::string stdx::http_version_string(stdx::http_version version)
{
	switch (version)
	{
	case stdx::http_version::http_1_0:
		return U("HTTP/1.0");
	case stdx::http_version::http_1_1:
		return U("HTTP/1.1");
	case stdx::http_version::http_2_0:
		return U("HTTP/2.0");
	default:
		return U("HTTP/1.1");
	}
}

stdx::http_version stdx::make_http_version_by_string(const stdx::string& str)
{
	if (str == U("HTTP/1.0"))
	{
		return stdx::http_version::http_1_0;
	}
	if (str == U("HTTP/1.1"))
	{
		return stdx::http_version::http_1_1;
	}
	if (str == U("HTTP/2.0"))
	{
		return stdx::http_version::http_2_0;
	}
	throw std::invalid_argument("invaild http version string");
}

stdx::string stdx::http_status_message(stdx::http_status_code_t code)
{
	switch (code)
	{
	case 100:
		return U("Continue");
	case 200:
		return U("OK");
	case 101:
		return U("Switching Protocol");
	case 103:
		return U("Early Hints");
	case 201:
		return U("Created");
	case 202:
		return U("Accept");
	case 203:
		return U("Non-Authoritative Information");
	case 204:
		return U("No Content");
	case 205:
		return U("Reset Content");
	case 206:
		return U("Partial Content");
	case 300:
		return U("Multiple Choices");
	case 301:
		return U("Moved Permanently");
	case 302:
		return U("Found");
	case 303:
		return U("See Other");
	case 304:
		return U("Not Modified");
	case 307:
		return U("Temporary Redirect");
	case 308:
		return U("Permanent Redirect");
	case 400:
		return U("Bad Request");
	case 401:
		return U("Unauthorized");
	case 402:
		return U("Payment Required");
	case 403:
		return U("Forbidden");
	case 404:
		return U("Not Found");
	case 405:
		return U("Method Not Allowed");
	case 406:
		return U("Not Acceptable");
	case 407:
		return U("Proxy Authentication Required");
	case 408:
		return U("Request Timeout");
	case 409:
		return U("Conflict");
	case 410:
		return U("Gone");
	case 411:
		return U("Length Required");
	case 412:
		return U("Precondition Failed");
	case 413:
		return U("Payload Too Large");
	case 414:
		return U("URI Too Long");
	case 415:
		return U("Unsupported Media Type");
	case 416:
		return U("Range Not Satisfiable");
	case 417:
		return U("Expectation Failed");
	case 418:
		return U("I'm a teapot");
	case 422:
		return U("Unprocessable Entity");
	case 425:
		return U("Too Early");
	case 426:
		return U("Upgrade Required");
	case 428:
		return U("Precondition Required");
	case 429:
		return U("Too Many Requests");
	case 431:
		return U("Request Header Fields Too Large");
	case 451:
		return U("Unavailable For Legal Reasons");
	case 500:
		return U("Internal Server Error");
	case 501:
		return U("Not Implemented");
	case 502:
		return U("Bad Gateway");
	case 503:
		return U("Service Unavailable");
	case 504:
		return U("Gateway Timeout");
	case 505:
		return U("HTTP Version Not Supported");
	case 506:
		return U("Variant Also Negotiates");
	case 507:
		return U("Insufficient Storage");
	case 508:
		return U("Loop Detected");
	case 510:
		return U("Not Extended");
	case 511:
		return U("Network Authentication Required");
	default:
		return U("Unkown Status Code Message");
	}
}

stdx::string stdx::http_method_string(stdx::http_method method)
{
	switch (method)
	{
	case stdx::http_method::get:
		return U("GET");
	case stdx::http_method::post:
		return U("POST");
	case stdx::http_method::put:
		return U("PUT");
	case stdx::http_method::del:
		return U("DELETE");
	case stdx::http_method::options:
		return U("OPTIONS");
	case stdx::http_method::head:
		return U("HEAD");
	case stdx::http_method::trace:
		return U("TRACE");
	case stdx::http_method::connect:
		return U("CONNECT");
	case stdx::http_method::patch:
		return U("PATCH");
	default:
		return U("GET");
	}
}

stdx::http_method stdx::make_http_method_by_string(const stdx::string& str)
{
	if (str == U("GET"))
	{
		return stdx::http_method::get;
	}
	if (str == U("POST"))
	{
		return stdx::http_method::post;
	}
	if (str == U("PUT"))
	{
		return stdx::http_method::put;
	}
	if (str == U("DELETE"))
	{
		return stdx::http_method::del;
	}
	if (str == U("OPTIONS"))
	{
		return stdx::http_method::options;
	}
	if (str == U("HEAD"))
	{
		return stdx::http_method::head;
	}
	if (str == U("TRACE"))
	{
		return stdx::http_method::trace;
	}
	if (str == U("CONNECT"))
	{
		return stdx::http_method::connect;
	}
	if (str == U("PATCH"))
	{
		return stdx::http_method::patch;
	}
	throw std::invalid_argument("invalid http method string");
}

std::list<stdx::http_cookie> stdx::make_cookies_by_cookie_header(const stdx::string& header)
{
	if (header.empty())
	{
		throw std::invalid_argument("invalid cookie string");
	}
	size_t pos = header.find(U(':'));
	if (pos == stdx::string::npos)
	{
		throw std::invalid_argument("invalid cookie string");
	}
	std::list<stdx::http_cookie> list;
	stdx::string&& cookies_string = header.substr(pos+1, header.size() - pos-1);
	std::list<stdx::string>&& cookie_list = cookies_string.split(U(";"));
	for (auto begin = cookie_list.begin(), end = cookie_list.end(); begin != end; begin++)
	{
		if (begin->front() == U(' '))
		{
			begin->erase_front();
		}
		pos = begin->find(U('='));
		if (pos == stdx::string::npos)
		{
			stdx::http_cookie cookie(*begin, U(""));
			list.push_back(std::move(cookie));
		}
		else
		{
			stdx::string&& name = begin->substr(0, pos);
			stdx::string&& value = begin->substr(pos + 1, begin->size() - pos - 1);
			stdx::http_cookie cookie(name,value);
			list.push_back(std::move(cookie));
		}
	}
	return list;
}

stdx::http_cookie stdx::make_cookie_by_set_cookie_header(const stdx::string& header)
{
	if (header.empty())
	{
		throw std::invalid_argument("invalid set-cookie string");
	}
	size_t pos = header.find(U(":"));
	if (pos == stdx::string::npos)
	{
		throw std::invalid_argument("invalid set-cookie string");
	}
	stdx::http_cookie cookie;
	stdx::string set_cookie_string = header.substr(pos + 1, header.size() - pos - 1);
	std::list<stdx::string>&& list = set_cookie_string.split(U(";"));
	for (auto begin=list.begin(),end=list.end();begin!=end;begin++)
	{
		if (begin->front() == U(' '))
		{
			begin->erase_front();
		}
		pos = begin->find(U('='));
		if (pos == stdx::string::npos)
		{
			if (*begin == U("HttpOnly"))
			{
				cookie.set_http_only(true);
			}
			if (*begin == U("Secure"))
			{
				cookie.set_secure(true);
			}
		}
		else
		{
			stdx::string&& name = begin->substr(0, pos);
			stdx::string&& value = begin->substr(pos + 1, begin->size() - pos - 1);
			if (name == U("Expires"))
			{
				pos = value.find(U(','));
				if (pos == stdx::string::npos)
				{
					throw std::invalid_argument("invalid set-cookie string in expires field");
				}
				stdx::string&& date_str = value.substr(pos+1,value.size()-pos-1);
				if (date_str.front() == U(' '))
				{
					date_str.erase_front();
				}
				std::list<stdx::string>&& date_list = date_str.split(U(" "));
				if (date_list.size() < 4)
				{
					throw std::invalid_argument("invalid set-cookie string in expires field");
				}
				auto date_begin = date_list.begin();
				stdx::uint32_union u;
				u.value	= date_begin->to_uint32();
				cookie.expires().day(u.low);
				date_begin++;
				cookie.expires().month(stdx::month_name_to_time_int(*date_begin));
				date_begin++;
				u.value = date_begin->to_uint32();
				cookie.expires().year(u.low);
				date_begin++;
				date_list = date_begin->split(U(":"));
				if (date_list.size() != 3)
				{
					throw std::invalid_argument("invalid set-cookie string in expires field");
				}
				date_begin = date_list.begin();
				u.value = date_begin->to_uint32();
				cookie.expires().hour(u.low);
				date_begin++;
				u.value = date_begin->to_uint32();
				cookie.expires().minute(u.low);
				date_begin++;
				u.value = date_begin->to_uint32();
				cookie.expires().second(u.low);
				cookie.set_enable_expires(true);
			}
			else if (name == U("Max-Age"))
			{
				cookie.enable_max_age() = true;
				cookie.max_age() = value.to_uint64();
			}
			else if (name == U("Domain"))
			{
				cookie.domain() = value;
			}
			else if (name == U("Path"))
			{
				cookie.path() = value;
			}
			else
			{
				cookie.name() = name;
				cookie.value() = value;
			}
		}
	}
	return cookie;
}

stdx::http_request_header::http_request_header()
	:stdx::http_header()
	,m_method(stdx::http_method::get)
	,m_request_url(U("/"))
	,m_cookies()
{}

stdx::http_request_header::http_request_header(stdx::http_version version)
	:stdx::http_header(version)
	,m_method(stdx::http_method::get)
	,m_request_url(U("/"))
	,m_cookies()
{}

stdx::http_request_header::http_request_header(stdx::http_method method)
	:stdx::http_header()
	, m_method(method)
	, m_request_url(U("/"))
	, m_cookies()
{}

stdx::http_request_header::http_request_header(stdx::http_version version, stdx::http_method method)
	:stdx::http_header(version)
	, m_method(method)
	, m_request_url(U("/"))
	, m_cookies()
{}

stdx::http_request_header::http_request_header(stdx::http_method method, const stdx::string& request_url)
	:stdx::http_header()
	, m_method(method)
	, m_request_url(request_url)
	, m_cookies()
{}

stdx::http_request_header::http_request_header(stdx::http_version version, stdx::http_method method, const stdx::string& request_url)
	:stdx::http_header(version)
	, m_method(method)
	, m_request_url(request_url)
	, m_cookies()
{}

stdx::http_request_header::http_request_header(const stdx::http_request_header& other)
	:stdx::http_header(other)
	,m_method(other.m_method)
	,m_request_url(other.m_request_url)
	,m_cookies(other.m_cookies)
{}

stdx::http_request_header::http_request_header(stdx::http_request_header&& other) noexcept
	:stdx::http_header(other)
	, m_method(other.m_method)
	, m_request_url(other.m_request_url)
	, m_cookies(other.m_cookies)
{}

stdx::http_request_header& stdx::http_request_header::operator=(const stdx::http_request_header& other)
{
	http_header::operator=(other);
	m_method = other.m_method;
	m_request_url = other.m_request_url;
	m_cookies = other.m_cookies;
	return *this;
}

stdx::http_request_header& stdx::http_request_header::operator=(stdx::http_request_header&& other) noexcept
{
	http_header::operator=(other);
	m_method = other.m_method;
	m_request_url = other.m_request_url;
	m_cookies = other.m_cookies;
	return *this;
}

stdx::http_method& stdx::http_request_header::method()
{
	return m_method;
}

const stdx::http_method& stdx::http_request_header::method() const
{
	return m_method;
}

stdx::string& stdx::http_request_header::request_url()
{
	return m_request_url;
}

const stdx::string& stdx::http_request_header::request_url() const
{
	return m_request_url;
}

std::list<stdx::http_cookie>& stdx::http_request_header::cookies()
{
	return m_cookies;
}

const std::list<stdx::http_cookie>& stdx::http_request_header::cookies() const
{
	return m_cookies;
}

stdx::string stdx::http_request_header::to_string() const
{
	//请求行
	stdx::string str(stdx::http_method_string(m_method));
	str.push_back(U(' '));
	stdx::string tmp(m_request_url);
	tmp.url_encode();
	str.append(tmp);
	str.push_back(U(' '));
	str.append(stdx::http_version_string(version()));
	//其他头部
	str.append(http_header::to_string());
	//Cookie
	if (!m_cookies.empty())
	{
		str.append(U("Cookie: "));
		for (auto begin = m_cookies.begin(), end = m_cookies.end(); begin != end; begin++)
		{
			str.append(begin->to_cookie_string_without_header());
			str.append(U("; "));
		}
	}
	return str;
}

stdx::http_request_header stdx::http_request_header::from_string(const stdx::string& str)
{
	if (str.empty())
	{
		throw std::invalid_argument("invalid request header string");
	}
	stdx::http_request_header header;
	std::list<stdx::string> &&list = str.split(U("\r\n"));
	if (list.empty())
	{
		throw std::invalid_argument("can not split string by CRLF");
	}
	//请求行
	stdx::string& first_line = list.front();
	std::list<stdx::string>&& request_info = first_line.split(U(" "));
	if (request_info.size() < 3)
	{
		throw std::invalid_argument("invalid request header string");
	}
	{
		auto begin = request_info.begin();
		header.method() = stdx::make_http_method_by_string(*begin);
		begin++;
		begin->url_decode();
		header.request_url() = *begin;
		begin++;
		header.version() = stdx::make_http_version_by_string(*begin);
	}
	//其他头部和Cookie
	if (list.size() > 1)
	{
		auto begin = list.begin(), end = list.end();
		begin++;
		while (begin != end)
		{
			if (!begin->empty())
			{
				if (begin->begin_with(U("Cookie")))
				{
					header.cookies() = std::move(stdx::make_cookies_by_cookie_header(*begin));
				}
				else
				{
					header.add_header(*begin);
				}
			}
			begin++;
		}
	}
	return header;
}

stdx::http_response_header::http_response_header()
	:stdx::http_header()
	,m_status_code(200)
	,m_set_cookies()
{}

stdx::http_response_header::http_response_header(stdx::http_version version)
	:stdx::http_header(version)
	,m_status_code(200)
	,m_set_cookies()
{}

stdx::http_response_header::http_response_header(stdx::http_status_code_t code)
	:stdx::http_header()
	,m_status_code(code)
	,m_set_cookies()
{}

stdx::http_response_header::http_response_header(stdx::http_version version, stdx::http_status_code_t code)
	:stdx::http_header(version)
	,m_status_code(code)
	,m_set_cookies()
{}

stdx::http_response_header::http_response_header(const stdx::http_response_header& other)
	:stdx::http_header(other)
	,m_status_code(other.m_status_code)
	,m_set_cookies(other.m_set_cookies)
{}

stdx::http_response_header::http_response_header(stdx::http_response_header&& other) noexcept
	:stdx::http_header(other)
	, m_status_code(other.m_status_code)
	, m_set_cookies(other.m_set_cookies)
{}

stdx::http_response_header& stdx::http_response_header::operator=(const stdx::http_response_header& other)
{
	http_header::operator=(other);
	m_status_code = other.m_status_code;
	m_set_cookies = other.m_set_cookies;
	return *this;
}

stdx::http_response_header& stdx::http_response_header::operator=(stdx::http_response_header&& other) noexcept
{
	http_header::operator=(other);
	m_status_code = other.m_status_code;
	m_set_cookies = other.m_set_cookies;
	return *this;
}

stdx::http_status_code_t& stdx::http_response_header::status_code()
{
	return m_status_code;
}

const stdx::http_status_code_t& stdx::http_response_header::status_code() const
{
	return m_status_code;
}

std::list<stdx::http_cookie>& stdx::http_response_header::cookies()
{
	return m_set_cookies;
}

const std::list<stdx::http_cookie>& stdx::http_response_header::cookies() const
{
	return m_set_cookies;
}

stdx::string stdx::http_response_header::to_string() const
{
	//状态行
	stdx::string str(stdx::http_version_string(version()));
	str.push_back(U(' '));
	str.append(stdx::to_string(m_status_code));
	str.push_back(U(' '));
	str.append(stdx::http_status_message(m_status_code));
	str.append(U("\r\n"));
	//其他头部
	str.append(http_header::to_string());
	//Set-Cookie
	for (auto begin = m_set_cookies.begin(), end = m_set_cookies.end(); begin != end; begin++)
	{
		str.append(begin->to_set_cookie_string());
		str.append(U("\r\n"));
	}
	return str;
}

stdx::http_response_header stdx::http_response_header::from_string(const stdx::string& str)
{
	if (str.empty())
	{
		throw std::invalid_argument("invalid response header string");
	}
	stdx::http_response_header header;
	std::list<stdx::string>&& list = str.split(U("\r\n"));
	if (list.empty())
	{
		throw std::invalid_argument("can not split string by CRLF");
	}
	//状态行
	stdx::string& first_line = list.front();
	std::list<stdx::string>&& status_info = first_line.split(U(" "));
	if (status_info.size() < 2)
	{
		throw std::invalid_argument("invalid response header string");
	}
	{
		auto begin = status_info.begin();
		header.version() = stdx::make_http_version_by_string(*begin);
		begin++;
		header.status_code() = begin->to_uint32();
		begin++;
	}
	//其他头部和Cookie
	if (list.size() > 1)
	{
		auto begin = list.begin(),end = list.end();
		begin++;
		while (begin!=end)
		{
			if (begin->begin_with(U("Set-Cookie")))
			{
				header.cookies().push_back(std::move(stdx::make_cookie_by_set_cookie_header(*begin)));
			}
			else
			{
				header.add_header(*begin);
			}
			begin++;
		}
	}
	return header;
}