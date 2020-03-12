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

bool stdx::http_cookie::enable_max_age() const
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

stdx::http_header::http_header(const stdx::http_header& other)
	:m_headers(other.m_headers)
{}

stdx::http_header::http_header(stdx::http_header&& other) noexcept
	:m_headers(other.m_headers)
{}

stdx::http_header& stdx::http_header::operator=(const stdx::http_header& other)
{
	m_headers = other.m_headers;
	return *this;
}

stdx::http_header& stdx::http_header::operator=(stdx::http_header&& other)
{
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

stdx::http_header& stdx::http_header::add_header(stdx::string&& name, const stdx::string& value)
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
		name.erase(name.size()-1);
	}
	m_headers[name] = value;
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
	if (name.back() == U(' '))
	{
		stdx::string tmp(name);
		return add_header(std::move(tmp), value);
	}
	m_headers[name] = value;
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
		name.earse(name.size() - 1);
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