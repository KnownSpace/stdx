#pragma once
#include <stdx/env.h>
#include <stdx/string.h>
#include <stdx/datetime.h>
#include <unordered_map>
#include <stdx/nullable.h>

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

	extern stdx::http_version make_http_version_by_string(const stdx::string &str);

	extern stdx::string http_method_string(stdx::http_method method);

	extern stdx::http_method make_http_method_by_string(const stdx::string& str);

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

	using http_status_code_t = uint32_t;

	extern stdx::string http_status_message(stdx::http_status_code_t code);

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
		bool m_enable_expires;
		stdx::datetime m_expires;
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

		const bool &enable_max_age() const;
		bool& enable_max_age();
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

		bool enable_expires() const;
		void set_enable_expires(bool value);
		stdx::datetime& expires();
		const stdx::datetime& expires() const;
	};

	extern std::list<stdx::http_cookie> make_cookies_by_cookie_header(const stdx::string &header);

	extern stdx::http_cookie make_cookie_by_set_cookie_header(const stdx::string &header);

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
		using map_t = std::unordered_map<stdx::string, stdx::string>;
		using iterator_t = typename map_t::iterator;
		using const_iterator_t = typename map_t::const_iterator;
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

		iterator_t begin();

		const_iterator_t cbegin() const;

		iterator_t end();

		const_iterator_t cend();
	private:
		stdx::http_version m_version;
		map_t m_headers;
	};

	struct http_request_header:public stdx::http_header
	{
	public:
		http_request_header();

		http_request_header(stdx::http_version version);

		http_request_header(stdx::http_method method);

		http_request_header(stdx::http_version version, stdx::http_method method);

		http_request_header(stdx::http_method method, const stdx::string& request_url);

		http_request_header(stdx::http_version version, stdx::http_method method, const stdx::string& request_url);

		http_request_header(const stdx::http_request_header& other);

		http_request_header(stdx::http_request_header&& other) noexcept;

		virtual ~http_request_header() = default;

		stdx::http_request_header& operator=(const stdx::http_request_header& other);

		stdx::http_request_header& operator=(stdx::http_request_header&& other) noexcept;

		virtual stdx::string to_string() const override;

		stdx::http_method& method();
		const stdx::http_method& method() const;

		stdx::string& request_url();
		const stdx::string& request_url() const;

		std::list<stdx::http_cookie>& cookies();
		const std::list<stdx::http_cookie>& cookies()const;

		static stdx::http_request_header from_string(const stdx::string& str);
	private:
		stdx::http_method m_method;
		stdx::string m_request_url;
		std::list<stdx::http_cookie> m_cookies;
	};

	struct http_response_header:public stdx::http_header
	{
	public:
		http_response_header();

		http_response_header(stdx::http_version version);

		http_response_header(stdx::http_status_code_t code);

		http_response_header(stdx::http_version version, stdx::http_status_code_t code);

		http_response_header(const stdx::http_response_header& other);

		http_response_header(stdx::http_response_header&& other) noexcept;

		virtual ~http_response_header() = default;

		stdx::http_response_header& operator=(const stdx::http_response_header& other);

		stdx::http_response_header& operator=(stdx::http_response_header&& other) noexcept;

		virtual stdx::string to_string() const override;

		stdx::http_status_code_t& status_code();
		const stdx::http_status_code_t& status_code() const;

		std::list<stdx::http_cookie>& cookies();
		const std::list<stdx::http_cookie>& cookies() const;

		static stdx::http_response_header from_string(const stdx::string &str);
	private:
		stdx::http_status_code_t m_status_code;
		std::list<stdx::http_cookie> m_set_cookies;
	};

	using http_header_ptr = std::shared_ptr<stdx::http_header>;

	interface_class http_body
	{
	public:
		using byte_t = unsigned char;
		virtual ~http_body() = default;
		virtual std::vector<byte_t> to_bytes() const = 0;
		virtual bool empty() const = 0;
	};

	struct http_parameter
	{
		using byte_t = unsigned char;
		using vector_t = std::vector<byte_t>;
		using map_t = std::unordered_map<stdx::string,vector_t>;
	public:
		http_parameter() = default;

		http_parameter(const vector_t& vector);

		http_parameter(const map_t& map);

		http_parameter(const stdx::http_parameter& other);

		http_parameter(stdx::http_parameter&& other) noexcept;

		~http_parameter() = default;

		stdx::http_parameter& operator=(const stdx::http_parameter& other);

		stdx::http_parameter& operator=(stdx::http_parameter&& other) noexcept;
		
		bool is_val() const;

		vector_t& val();
		const vector_t& val() const;

		void set_val();

		void set_val(const vector_t& val);

		int32_t val_as_int32() const;
		int64_t val_as_int64() const;

		uint32_t val_as_uint32() const;
		uint64_t val_as_uint64() const;

		double val_as_double() const;
		long double val_as_ldouble() const;

		stdx::string val_as_string() const;

		bool is_map() const;

		map_t &valmap();
		const map_t& valmap() const;

		void set_map();

		void set_map(const map_t &map);

		vector_t& subval(const stdx::string &name);
		const vector_t& subval(const stdx::string &name) const;

		bool exist_subval(const stdx::string& name) const;

		int32_t subval_as_int32(const stdx::string& name) const;
		int64_t subval_as_int64(const stdx::string& name) const;

		uint32_t subval_as_uint32(const stdx::string& name) const;
		uint64_t subval_as_uint64(const stdx::string& name) const;

		double subval_as_double(const stdx::string& name) const;
		long double subval_as_ldouble(const stdx::string& name) const;

		stdx::string subval_as_string(const stdx::string& name) const;

		bool exist_map_body() const
		{
			return exist_subval(U("Body"));
		}

		vector_t& map_body()
		{
			return subval(U("Body"));
		}

		const vector_t& map_body() const
		{
			return subval(U("Body"));
		}

		void set_map_body(const vector_t &body)
		{
			if (!exist_map_body())
			{
				m_map.val().emplace(U("Body"), body);
			}
			map_body() = body;
		}

		void set_map_body()
		{
			vector_t body;
			set_map_body(body);
		}

		stdx::string map_body_as_string()
		{
			return subval_as_string(U("Body"));
		}

		int32_t map_body_as_int32() const
		{
			return subval_as_int32(U("Body"));
		}

		int64_t map_body_as_int64() const
		{
			return subval_as_int64(U("Body"));
		}

		uint32_t map_body_as_uint32() const
		{
			return subval_as_uint32(U("Body"));
		}

		uint64_t map_body_as_uint64() const
		{
			return subval_as_uint64(U("Body"));
		}

		double map_body_as_double() const
		{
			return subval_as_double(U("Body"));
		}

		long double map_body_as_ldouble() const
		{
			return subval_as_ldouble(U("Body"));
		}

		bool exist_map_content_type() const
		{
			return exist_subval(U("Content-Type"));
		}

		stdx::string map_content_type() const
		{
			return subval_as_string(U("Content-Type"));
		}

		void set_map_content_type(const stdx::string& content_type)
		{
			if (content_type.empty())
			{
				throw std::invalid_argument("invalid content-type");
			}
			std::string&& tmp = content_type.to_u8_string();
			std::vector<byte_t> vector(tmp.begin(),tmp.end());
			if (exist_map_content_type())
			{
				subval(U("Content-Type")) = vector;
			}
			else
			{
				m_map.val().emplace(U("Content-Type"),std::move(vector));
			}
		}
		
		bool exist_map_encoding() const
		{
			return exist_subval(U("Content-Transfer-Encoding"));
		}

		stdx::string map_encoding() const
		{
			return subval_as_string(U("Content-Transfer-Encoding"));
		}

		void set_map_encoding(const stdx::string& encoding)
		{
			if (encoding.empty())
			{
				throw std::invalid_argument("invalid encoding");
			}
			std::string&& tmp = encoding.to_u8_string();
			std::vector<byte_t> vector(tmp.begin(), tmp.end());
			if (exist_map_content_type())
			{
				subval(U("Content-Transfer-Encoding")) = vector;
			}
			else
			{
				m_map.val().emplace(U("Content-Transfer-Encoding"), std::move(vector));
			}
		}
	private:
		stdx::nullable<vector_t> m_vector;
		stdx::nullable<map_t> m_map;
	};

	enum class http_form_type
	{
		urlencoded,
		multipart,
		text
	};

	extern stdx::string http_form_type_string(stdx::http_form_type type);

	extern stdx::http_form_type make_http_form_type_by_string(const stdx::string &type);

	interface_class http_form:public http_body
	{
	public:

		using arg_t = http_parameter;

		using collection_t = std::unordered_map<stdx::string,arg_t>;

		using iterator_t = typename collection_t::iterator;

		using const_iterator_t = typename collection_t::const_iterator;

		virtual ~http_form() =default;

		virtual arg_t &get(const stdx::string &name) = 0;

		virtual const arg_t& get(const stdx::string& name) const = 0;

		virtual void add(const stdx::string& name, const arg_t& value) = 0;

		virtual void del(const stdx::string& name) = 0;

		arg_t& operator[](const stdx::string& name)
		{
			return get(name);
		}

		const arg_t& operator[](const stdx::string& name) const
		{
			return get(name);
		}

		virtual iterator_t begin() =0;
		virtual const_iterator_t cbegin() const = 0;

		virtual iterator_t end() = 0;
		virtual const_iterator_t cend() const = 0;

		virtual bool exist(const stdx::string& name) const = 0;

		virtual stdx::http_form_type form_type() const = 0;

		virtual void clear() = 0;
	};

	struct http_urlencoded_form:public stdx::http_form
	{
		using self_t = stdx::http_urlencoded_form;
	public:
		http_urlencoded_form();

		http_urlencoded_form(const self_t& other);

		http_urlencoded_form(self_t&& other) noexcept;

		~http_urlencoded_form() = default;

		self_t& operator=(const self_t& other);
		self_t& operator=(self_t&& other) noexcept;

		virtual arg_t& get(const stdx::string& name) override;
		virtual const arg_t& get(const stdx::string& name) const override;

		virtual void add(const stdx::string& name, const arg_t& value) override;

		virtual void del(const stdx::string& name) override;

		virtual iterator_t begin() override;
		virtual const_iterator_t cbegin() const override;

		virtual iterator_t end() override;
		virtual const_iterator_t cend() const override;

		virtual bool exist(const stdx::string& name) const override;

		virtual std::vector<byte_t> to_bytes() const override;

		virtual stdx::http_form_type form_type() const override
		{
			return stdx::http_form_type::urlencoded;
		}

		virtual bool empty() const override
		{
			return m_collection.empty();
		}

		virtual void clear() override
		{
			m_collection.clear();
		}

	private:
		collection_t m_collection;
	};

	struct http_multipart_form:public http_form
	{
		using self_t = stdx::http_multipart_form;
	public:
		http_multipart_form() = default;

		http_multipart_form(const stdx::string &boundary);

		http_multipart_form(const self_t& other);

		http_multipart_form(self_t&& other) noexcept;

		~http_multipart_form() = default;

		self_t& operator=(const self_t& other);

		self_t& operator=(self_t&& other) noexcept;

		virtual arg_t& get(const stdx::string& name) override;
		virtual const arg_t& get(const stdx::string& name) const override;

		virtual void add(const stdx::string& name, const arg_t& value) override;

		virtual void del(const stdx::string& name) override;

		virtual iterator_t begin() override;
		virtual const_iterator_t cbegin() const override;

		virtual iterator_t end() override;
		virtual const_iterator_t cend() const override;

		virtual bool exist(const stdx::string& name) const override;

		virtual std::vector<byte_t> to_bytes() const override;

		stdx::string& boundary();
		const stdx::string& boundary() const;

		virtual stdx::http_form_type form_type() const override
		{
			return stdx::http_form_type::multipart;
		}

		virtual bool empty() const override
		{
			return m_collection.empty();
		}

		virtual void clear() override
		{
			m_collection.clear();
		}

	private:
		stdx::string m_boundary;
		collection_t m_collection;
	};

	struct http_text_form:public http_form
	{
		using self_t = stdx::http_text_form;
	public:

		http_text_form();

		http_text_form(const self_t &other);

		http_text_form(self_t&& other) noexcept;

		~http_text_form() = default;

		self_t& operator=(const self_t& other);

		self_t& operator=(self_t&& other) noexcept;

		virtual arg_t& get(const stdx::string& name) override;
		virtual const arg_t& get(const stdx::string& name) const override;

		virtual void add(const stdx::string& name, const arg_t& value) override;

		virtual void del(const stdx::string& name) override;

		virtual iterator_t begin() override;
		virtual const_iterator_t cbegin() const override;

		virtual iterator_t end() override;
		virtual const_iterator_t cend() const override;

		virtual bool exist(const stdx::string& name) const override;

		virtual std::vector<byte_t> to_bytes() const override;

		virtual stdx::http_form_type form_type() const override
		{
			return stdx::http_form_type::text;
		}

		virtual bool empty() const override
		{
			return m_collection.empty() || m_collection.at(U("value")).val().empty();
		}

		virtual void clear() override
		{
			if (!m_collection.empty())
			{
				m_collection.at(U("value")).val().clear();
			}
		}
	private:
		collection_t m_collection;
	};

	using http_form_ptr = std::shared_ptr<stdx::http_form>;

	//enum class http_response_body_type
	//{

	//};

	struct http_response_body:public stdx::http_body
	{
	public:
		virtual ~http_response_body() = default;

		virtual std::vector<byte_t> data() const = 0;

		virtual stdx::string data_as_string() const = 0;

		virtual void push(const byte_t *buffer,size_t count) = 0;

		virtual void push(const std::vector<byte_t>& buffer) = 0;

		virtual void pop();
	};

	using http_response_body_ptr = std::shared_ptr<stdx::http_response_body>;

	struct http_identity_body :public stdx::http_response_body
	{
		using self_t = stdx::http_identity_body;
	public:
		http_identity_body();

		http_identity_body(const std::vector<byte_t>& data);

		http_identity_body(const std::initializer_list<std::vector<byte_t>>& data);

		http_identity_body(const self_t& other);

		http_identity_body(self_t&& other) noexcept;

		~http_identity_body() = default;

		self_t& operator=(const self_t& other);

		self_t& operator=(self_t&& other) noexcept;

		virtual std::vector<byte_t> to_bytes() const override;

		virtual bool empty() const override;

		virtual std::vector<byte_t> data() const override;

		virtual stdx::string data_as_string() const override;

		virtual void push(const byte_t* buffer, size_t count) override;

		virtual void push(const std::vector<byte_t>& buffer) override;

		virtual void pop() override;

		stdx::string body_type() const
		{
			return U("identity");
		}
	private:
		std::list<std::vector<byte_t>> m_data;
	};

	struct http_chunk_body :public stdx::http_response_body
	{
		using self_t = stdx::http_chunk_body;
	public:
		http_chunk_body();

		http_chunk_body(const std::vector<byte_t>& data);

		http_chunk_body(const std::initializer_list<std::vector<byte_t>>& data);

		http_chunk_body(const stdx::string &trailer);

		http_chunk_body(const std::vector<byte_t> &data,const stdx::string& trailer);

		http_chunk_body(const std::initializer_list<std::vector<byte_t>>& data, const stdx::string& trailer);

		http_chunk_body(const self_t& other);

		http_chunk_body(self_t&& other) noexcept;

		~http_chunk_body() = default;

		self_t& operator=(const self_t& other);

		self_t& operator=(self_t&& other) noexcept;

		virtual std::vector<byte_t> to_bytes() const override;

		virtual bool empty() const override;

		virtual std::vector<byte_t> data() const override;

		virtual stdx::string data_as_string() const override;

		virtual void push(const byte_t* buffer, size_t count) override;

		virtual void push(const std::vector<byte_t>& buffer) override;

		virtual void pop() override;

		stdx::string& trailer();

		const stdx::string& trailer() const;

		stdx::string body_type() const
		{
			return U("chunk");
		}
	private:
		std::list<std::vector<byte_t>> m_data;
		stdx::string m_trailer;
	};

	class http_msg
	{
	public:
		using byte_t = unsigned char;

		virtual ~http_msg() = default;

		virtual std::vector<byte_t> to_bytes() const;

		virtual stdx::http_header& header() = 0;

		virtual const stdx::http_header& header() const = 0;

		virtual stdx::http_body& body() = 0;

		virtual const stdx::http_body& body() const = 0;
	};

	using http_msg_ptr = std::shared_ptr<stdx::http_msg>;

	class http_request:public http_msg
	{
		using header_t = std::shared_ptr<stdx::http_request_header>;
		using body_t = std::shared_ptr<stdx::http_form>;
	public:
		http_request();

		http_request(const stdx::http_request& other);

		template<typename _Form,class = typename std::enable_if<stdx::is_base_on<_Form,stdx::http_form>::value>::type>
		http_request()
			:m_header(std::make_shared<stdx::http_request_header>())
			,m_form(std::make_shared<_Form>())
		{}

		http_request(const stdx::http_form_ptr& form);

		http_request(const header_t& header);

		http_request(const header_t& header, const stdx::http_form_ptr& form);

		template<typename _Form, class = typename std::enable_if<stdx::is_base_on<_Form, stdx::http_form>::value>::type>
		http_request(const header_t& header)
			: m_header(header)
			, m_form(std::make_shared<_Form>())
		{}

		template<typename _Form, class = typename std::enable_if<stdx::is_base_on<_Form, stdx::http_form>::value>::type>
		http_request(const header_t& header,const std::shared_ptr<_Form> &form)
			: m_header(header)
			, m_form(form)
		{}

		http_request(stdx::string url);

		http_request(stdx::http_method method, stdx::string url);

		template<typename _Form, class = typename std::enable_if<stdx::is_base_on<_Form, stdx::http_form>::value>::type>
		http_request(stdx::http_method method, stdx::string url, const std::shared_ptr<_Form>& form)
			:m_header(std::make_shared<stdx::http_request_header>(method,url))
			,m_form(form)
		{}

		template<typename _Form, class = typename std::enable_if<stdx::is_base_on<_Form, stdx::http_form>::value>::type>
		http_request(stdx::http_method method, stdx::string url)
			:m_header(std::make_shared<stdx::http_request_header>(method, url))
			,m_form(std::make_shared<_Form>())
		{}

		~http_request() = default;

		stdx::http_request& operator=(const stdx::http_request& other);

		bool operator==(const stdx::http_request& other) const;

		bool operator!=(const stdx::http_request& other) const
		{
			return !this->operator==(other);
		}

		stdx::http_request_header& request_header();

		const stdx::http_request_header& request_header() const;

		virtual stdx::http_header& header() override
		{
			return request_header();
		}

		virtual const stdx::http_header& header() const override
		{
			return request_header();
		}

		stdx::http_form& form();

		const stdx::http_form& form() const;

		virtual stdx::http_body& body() override
		{
			return form();
		}

		virtual const stdx::http_body& body() const override
		{
			return form();
		}

		std::vector<byte_t> to_bytes();

		virtual std::vector<byte_t> to_bytes() const override;

		static stdx::http_request from_bytes(const std::vector<unsigned char> &bytes);
	private:
		header_t m_header;
		body_t m_form;
	};

	extern stdx::http_urlencoded_form make_http_urlencoded_form(const std::vector<unsigned char>& bytes);

	extern stdx::http_multipart_form make_http_multipart_form(const std::vector<unsigned char>& bytes,const stdx::string &boundary);

	extern stdx::http_text_form make_http_text_form(const std::vector<unsigned char>& bytes);

	extern stdx::http_form_ptr make_http_form(stdx::http_form_type type,const std::vector<unsigned char> &bytes,const stdx::string &boundary);

	extern stdx::http_form_type get_http_form_type_and_boundary(const stdx::string &content_type,stdx::string &boundary);

	class http_response:public stdx::http_msg
	{
		using header_t = std::shared_ptr<stdx::http_response_header>;
		using body_t = stdx::http_response_body_ptr;
	public:
		http_response();

		http_response(const stdx::http_response& other);

		template<typename _Body, class = typename std::enable_if<stdx::is_base_on<_Body, stdx::http_response_body>::value>::type>
		http_response()
			:m_header(std::make_shared<stdx::http_request_header>())
			,m_body(std::make_shared<_Body>())
		{}

		http_response(const body_t& body);

		http_response(const header_t& header);

		http_response(const header_t& header, const body_t& body);

		template<typename _Body, class = typename std::enable_if<stdx::is_base_on<_Body, stdx::http_response_body>::value>::type>
		http_response(const header_t &header)
			:m_header(header)
			,m_body(std::make_shared<_Body>())
		{}

		template<typename _Body, class = typename std::enable_if<stdx::is_base_on<_Body, stdx::http_response_body>::value>::type>
		http_response(const header_t& header,const std::shared_ptr<_Body> &body)
			: m_header(header)
			, m_body(body)
		{}

		http_response(stdx::http_status_code_t status_code);

		template<typename _Body, class = typename std::enable_if<stdx::is_base_on<_Body, stdx::http_response_body>::value>::type>
		http_response(stdx::http_status_code_t status_code)
			:m_header(std::make_shared<stdx::http_response_header>(status_code))
			,m_body(std::make_shared<_Body>())
		{}

		template<typename _Body, class = typename std::enable_if<stdx::is_base_on<_Body, stdx::http_response_body>::value>::type>
		http_response(stdx::http_status_code_t status_code,const std::shared_ptr<_Body> &body)
			:m_header(std::make_shared<stdx::http_response_header>(status_code))
			,m_body(body)
		{}

		~http_response() = default;

		stdx::http_response& operator=(const stdx::http_response& other);

		bool operator==(const stdx::http_response& other) const;

		bool operator!=(const stdx::http_response& other) const;

		stdx::http_response& response_header();

		const stdx::http_response& response_header() const;

		stdx::http_response_body& response_body();

		const stdx::http_response_body& response_body() const;

		virtual std::vector<byte_t> to_bytes() const override;

		virtual stdx::http_header& header() override;

		virtual const stdx::http_header& header() const override;

		virtual stdx::http_body& body() override;

		virtual const stdx::http_body& body() const override;
	private:
		header_t m_header;
		body_t m_body;
	};
}