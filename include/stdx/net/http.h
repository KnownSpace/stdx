#pragma once
#include <stdx/env.h>
#include <stdx/string.h>
#include <unordered_map>

namespace stdx
{
	enum class http_method
	{
		get,		//GET
		post,		//POST
		put,		//PUT
		del,		//DELETE
		options,	//OPTIONS
		head,		//HEAD
		trace,		//TRACE
		connect,	//CONNECT
		patch		//PATCH
	};
	
	enum class http_version
	{
		http_1_0,	//http 1.0
		http_1_1,	//http 1.1
		http_2_0	//http 2.0
	};

	enum class http_head_type
	{
		request,	//请求头部
		response	//响应头部
	};

	enum class http_cache_control_type
	{
		/*default*/
		none,

		/*response or request*/
		no_cache,
		no_soore,
		max_age,

		/*request only*/
		max_stale,
		min_fresh,
		only_if_cache,

		/*response only*/
		_public,
		_private,
		no_transform,
		must_revalidate,
		proxy_revalidate
	};
	
	struct http_cache_control
	{
		http_cache_control_type type;
		stdx::string value;

		http_cache_control()
			:type(stdx::http_cache_control_type::none)
			,value()
		{}
		http_cache_control(const http_cache_control &other)
			:type(other.type)
			,value(other.value)
		{}
		~http_cache_control() = default;

		stdx::http_cache_control& operator=(const stdx::http_cache_control& other)
		{
			type = other.type;
			value = other.value;
			return *this;
		}
	};

	enum class http_connection_type
	{
		close,
		keep_alive
	};

	class _HttpHead
	{
	public:
		_HttpHead(stdx::http_head_type type)
			:m_type(type)
			,m_version(stdx::http_version::http_1_1)
			,m_cache_control()
		{}

		_HttpHead(stdx::http_head_type type, stdx::http_version version)
			:m_type(type)
			, m_version(version)
			, m_cache_control()
		{}

		virtual	~_HttpHead() = default;
		
		virtual stdx::string to_string() const = 0;

		stdx::http_version version() const;
		
		bool have_cache_control() const;
		stdx::http_cache_control &cache_control();
		const stdx::http_cache_control &cache_control() const;
	private:
		stdx::http_head_type m_type;
	protected:
		stdx::http_version m_version;
		stdx::http_cache_control m_cache_control;
	};

	struct http_authorization
	{
		stdx::string type;
		stdx::string value;

		http_authorization()
			:type()
			,value()
		{}
		http_authorization(const stdx::string &_type,const stdx::string &val)
			:type(_type)
			,value(val)
		{}
		http_authorization(const http_authorization &other)
			:type(other.type)
			,value(other.value)
		{}
		~http_authorization() = default;
		http_authorization& operator=(const http_authorization& other)
		{
			type = other.type;
			value = other.value;
			return *this;
		}
		bool vaild() const
		{
			return (!type.empty());
		}
	};

	struct http_cookie 
	{
		stdx::string name;
		stdx::string value;
		bool http_only;
		stdx::string domain;
		stdx::string expires;
		stdx::string path;
		bool secure;

		http_cookie()
			:name()
			,value()
			,http_only(false)
			,domain()
			,expires()
			,path()
			,secure(false)
		{}

		http_cookie(const stdx::string& n, const stdx::string &val)
			:name(n)
			,value(val)
			,http_only(false)
			,domain()
			,expires()
			,path()
			,secure(false)
		{}

		http_cookie(const http_cookie &other)
			:name(other.name)
			,value(other.value)
			,http_only(other.http_only)
			,domain(other.domain)
			,expires(other.expires)
			,path(other.path)
			,secure(other.secure)
		{}

		~http_cookie() = default;

		http_cookie& operator=(const http_cookie& other)
		{
			name = other.name;
			value = other.value;
			http_only = other.http_only;
			domain = other.domain;
			expires = other.expires;
			path = other.path;
			secure = other.secure;
			return *this;
		}
	};

	class _HttpRequestHead:public _HttpHead
	{
	public:
		_HttpRequestHead();
		_HttpRequestHead(stdx::http_version version);
		_HttpRequestHead(stdx::http_method method);
		_HttpRequestHead(stdx::http_version version,stdx::http_method method);
		virtual ~_HttpRequestHead()=default;

		stdx::string to_string() const override;

		stdx::http_method method() const;
		void method(stdx::http_method method);

		const stdx::string &url() const;
		void url(const stdx::string &url);

		bool have_accept() const;
		std::list<stdx::string> &accept();
		const std::list<stdx::string> &accept() const;

		bool hava_accept_charset() const;
		std::list<stdx::string> &accept_charset();
		const std::list<stdx::string> &accept_charset() const;

		bool hava_accept_language() const;
		std::list<stdx::string> &accept_language();
		const std::list<stdx::string> &accept_language() const;

		bool enable_connection_type() const;
		stdx::http_connection_type connection_type() const;

		bool using_content_length() const;
		uint64_t content_length() const;
		void content_length(const uint64_t &length);

		bool have_cookie() const;
		std::list<stdx::http_cookie> &cookies();
		const std::list<stdx::http_cookie> &cookies() const;

		bool have_host() const;
		const stdx::string &host() const;
		void host(const stdx::string &host);

		bool have_user_agent() const;
		std::list<stdx::string> &user_agent();
		const std::list<stdx::string> &user_agent() const;

		bool have_authorization() const;
		stdx::http_authorization &authorization();
		const stdx::http_authorization &authorization() const;

		std::unordered_map<stdx::string, stdx::string> &other_head();
		const std::unordered_map<stdx::string, stdx::string> &other_head() const;
	private:
		stdx::http_method m_method;
		stdx::string m_url;
		std::list<stdx::string> m_accept;
		std::list<stdx::string> m_accept_charset;
		std::list<stdx::string> m_accept_lanuage;
		stdx::http_connection_type m_connection;
		uint64_t m_content_length;
		std::list<stdx::http_cookie> m_cookies;
		stdx::string m_host;
		std::list<stdx::string> m_user_agent;
		stdx::http_authorization m_authorization;
		std::unordered_map<stdx::string,stdx::string> m_other_head;
	};
}