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
}