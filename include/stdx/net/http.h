#pragma once
#include <stdx/env.h>
#include <stdx/string.h>
#include <stdx/datetime.h>
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

	extern stdx::string http_version_string(stdx::http_version version);

	extern stdx::string http_method_string(stdx::http_method method);

	enum class http_cache_control_type
	{
		/*response or request*/
		no_cache,
		no_store,

		/*request only*/
		only_if_cache,

		/*response only*/
		_public,
		_private,
		no_transform,
		must_revalidate,
		proxy_revalidate
	};

	using http_status_code = uint16_t;

	extern stdx::string http_status_message(stdx::http_status_code code);

	using http_max_age_t = uint64_t;

	struct http_cookie
	{
	public:
		using max_age_t = http_max_age_t;
	protected:
		stdx::string m_name;
		stdx::string m_value;
		bool m_enable_max_age;
		max_age_t m_max_age;
		stdx::string m_path;
		bool m_secure;
		bool m_http_only;
		stdx::string m_domain;
	public:
		http_cookie();

		http_cookie(const stdx::string& name, const stdx::string& value);

		http_cookie(const stdx::http_cookie& other);

		http_cookie(stdx::http_cookie&& other) noexcept;

		~http_cookie()=default;

		stdx::http_cookie& operator=(const stdx::http_cookie& other);

		stdx::http_cookie& operator=(stdx::http_cookie &&other) noexcept;

		stdx::string to_cookie_string() const;
		stdx::string to_cookie_string_without_header() const;
		stdx::string to_set_cookie_string() const;
		stdx::string to_set_cookie_string_without_header() const;

		stdx::string &name();
		const stdx::string& name() const;

		stdx::string& value();
		const stdx::string &value() const;

		bool enable_max_age() const;
		stdx::http_cookie::max_age_t &max_age();
		const stdx::http_cookie::max_age_t &max_age() const;

		stdx::string& path();
		const stdx::string& path()const;

		stdx::http_cookie &set_secure(bool secure);
		bool secure() const;

		stdx::http_cookie &set_http_only(bool http_only);
		bool http_only() const;

		stdx::string &domain();
		const stdx::string &domain() const;
	};

	struct http_cache_control
	{
	public:
		using max_age_t = http_max_age_t;
		using max_stale_t = max_age_t;
		using min_fresh_t = max_age_t;
	private:
		max_age_t m_max_age;
		max_stale_t m_max_stale;
		min_fresh_t m_min_fresh;
		max_age_t m_s_max_age;
		stdx::http_cache_control_type m_type;
	public:
		http_cache_control();

		http_cache_control(const stdx::http_cache_control &other);

		http_cache_control(stdx::http_cache_control&& other) noexcept;

		stdx::http_cache_control& operator=(const stdx::http_cache_control& other);

		stdx::http_cache_control& operator=(stdx::http_cache_control&& other) noexcept;

		~http_cache_control() = default;

		static stdx::string type_to_string(stdx::http_cache_control_type type);
		stdx::string to_string() const;
		stdx::string to_string_without_header() const;

		bool enable_max_age() const;
		max_age_t &max_age();
		const max_age_t max_age() const;

		bool enable_max_stale() const;
		max_stale_t &max_stale();
		const max_stale_t &max_stale() const;

		bool enable_min_fresh() const;
		min_fresh_t& min_fresh();
		const min_fresh_t& min_fresh() const;

		bool enable_s_max_age() const;
		max_age_t& s_max_age();
		const max_age_t &s_max_age() const;

		stdx::http_cache_control_type cache_type() const;
		void set_cache_type(stdx::http_cache_control_type type);
	};

	struct http_header
	{
	public:
		http_header();

		http_header(stdx::http_version version);

		http_header(const stdx::http_header& other);

		http_header(stdx::http_header&& other) noexcept;

		virtual ~http_header() = default;

		stdx::http_header& operator=(const stdx::http_header& other);

		stdx::http_header& operator=(stdx::http_header&& other) noexcept;

		virtual stdx::string to_string() const;

		const stdx::http_version &version() const;

		stdx::http_version& version();

		stdx::http_header& add_header(const stdx::string& name, const stdx::string& value);

		stdx::http_header& add_header(stdx::string&& name, stdx::string&& value);

		stdx::http_header& remove_header(const stdx::string &name);

		stdx::http_header& remove_header(stdx::string&& name);

		stdx::http_header& add_header(const stdx::string& raw_string);

		stdx::string& operator[](const stdx::string& name);
		const stdx::string& operator[](const stdx::string& name) const;
		stdx::string& operator[](stdx::string&& name);
		const stdx::string& operator[](stdx::string&& name) const;

		bool exist(const stdx::string& name) const;
		bool exist(stdx::string &&name) const;

		void clear();

		size_t size() const;
	private:
		stdx::http_version m_version;
		std::unordered_map<stdx::string, stdx::string> m_headers;
	};

	struct http_request_header:public stdx::http_header
	{
	public:

		virtual stdx::string to_string() const override;
	private:
		stdx::http_method m_method;
		stdx::string m_request_url;
		std::list<stdx::http_cookie> m_cookies;
	};

	struct http_response_header:public stdx::http_header
	{
	public:

		virtual stdx::string to_string() const override;
	private:
		stdx::http_status_code m_status_code;
		std::list<stdx::http_cookie> m_set_cookies;
	};
}