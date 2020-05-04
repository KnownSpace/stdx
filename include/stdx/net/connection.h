#pragma once
#include <stdx/net/socket.h>

namespace stdx
{
	//connection interface
	template<typename _Input,typename _Output = _Input>
	interface_class basic_connection
	{
		using input_t = _Input;

		using output_t = _Output;

		virtual void open(const stdx::string& ip,uint64_t port) = 0;

		virtual void close() = 0;

		virtual stdx::task<input_t> read() = 0;

		virtual void read_until(std::function<bool(stdx::task_result<input_t>)>&& fn) = 0;

		virtual void read_until_error(std::function<void(input_t)>&& fn, std::function<void(std::exception_ptr)>&& on_error)
		{
			return this->read_until([fn, on_error](stdx::task_result<input_t> r)
			{
				try
				{
					auto ev = r.get();
					fn(ev);
				}
				catch (const std::exception&)
				{
					on_error(std::current_exception());
					return false;
				}
			});
		}

		virtual stdx::task<void> write(const output_t &package) = 0;

		virtual bool is_connected() const = 0;

		virtual ~basic_connection() = default;
	};

	//connection
	template<typename _Input, typename _Output = _Input>
	class connection
	{
		using impl_t = std::shared_ptr<stdx::basic_connection<_Input, _Output>>;
		using self_t = stdx::connection<_Input,_Output>;
	public:
		explicit connection()
			:m_impl(nullptr)
		{}

		connection(const impl_t &impl)
			:m_impl(impl)
		{}

		connection(const self_t &other)
			:m_impl(other.m_impl)
		{}

		connection(self_t &&other) noexcept
			:m_impl(other.m_impl)
		{}

		~connection() = default;

		self_t& operator=(const self_t& other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		self_t& operator=(self_t&& other) noexcept
		{
			m_impl = other.m_impl;
			return *this;
		}

		void open(const stdx::string& ip, uint64_t port)
		{
			return m_impl->open(ip, port);
		}

		void close()
		{
			return close();
		}

		stdx::task<_Input> read()
		{
			return m_impl->read();
		}

		void read_until(std::function<bool(stdx::task_result<_Input>)>&& fn)
		{
			return m_impl->read_until(std::move(fn));
		}

		void read_until_error(std::function<void(_Input)>&& fn, std::function<void(std::exception_ptr)>&& on_error)
		{
			return m_impl->read_until_error(std::move(fn), std::move(on_error));
		}

		stdx::task<void> write(const _Output& package)
		{
			return m_impl->write(package);
		}

		bool is_connected() const
		{
			return m_impl->is_connected();
		}

		bool operator==(const self_t& other) const
		{
			return m_impl == other.m_impl;
		}

		operator bool() const
		{
			return (bool)m_impl;
		}
	private:
		impl_t m_impl;
	};

	//maker function
	template<typename _Impl,typename ..._Args,typename _Input = typename _Impl::input_t,typename _Output = typename _Impl::output_t>
	inline stdx::connection<_Input,_Output> make_connection(_Args &&...args)
	{
		auto impl_ptr = std::make_shared<_Impl>(args...);
		return stdx::connection<_Input, _Output>(impl_ptr);
	}

	//listener interface
	template<typename _Input, typename _Output = _Input>
	interface_class basic_listener
	{
		using input_t = _Input;

		using output_t = _Output;

		using conn_t = stdx::connection<input_t,output_t>;

		virtual void open(uint64_t port) = 0;

		virtual void open(const stdx::string &ip,uint64_t port) = 0;

		virtual stdx::task<conn_t> accept() = 0;

		virtual void accept_until(std::function<bool(stdx::task_result<conn_t>)> &&fn) = 0;

		virtual void accept_until_error(std::function<void(conn_t)>&& fn, std::function<void(std::exception_ptr)>&& on_error)
		{
			return this->accept_until([fn, on_error](stdx::task_result<conn_t> r)
			{
				try
				{
					conn_t conn = r.get();
					fn(conn);
				}
				catch (const std::exception&)
				{
					on_error(std::current_exception());
					return false;
				}
			});
		}

		virtual void close() = 0;

		virtual bool is_running() const = 0;

		virtual ~basic_listener() = default;
	};

	//listener
	template<typename _Input, typename _Output = _Input>
	class listener
	{
		using impl_t = std::shared_ptr<stdx::basic_listener>;
		using self_t = stdx::listener<_Input,_Output>;
		using conn_t = stdx::connection<_Input,_Output>;
	public:

		explicit listener()
			:m_impl(nullptr)
		{}

		listener(const impl_t &impl)
			:m_impl(impl)
		{}

		listener(const self_t &other)
			:m_impl(other.m_impl)
		{}

		listener(self_t &&other) noexcept
			:m_impl(other.m_impl)
		{}

		~listener() = default;

		self_t& operator=(const self_t& other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		self_t& operator=(self_t&& other) noexcept
		{
			m_impl = other.m_impl;
			return *this;
		}

		void open(uint64_t port)
		{
			return m_impl->open(port);
		}

		void open(const stdx::string& ip, uint64_t port)
		{
			return m_impl->open(ip, port);
		}

		stdx::task<conn_t> accept()
		{
			return m_impl->accept();
		}

		void accept_until(std::function<bool(stdx::task_result<conn_t>)>&& fn)
		{
			return m_impl->accept_until(std::move(fn));
		}

		void accept_until_error(std::function<void(conn_t)>&& fn, std::function<void(std::exception_ptr)>&& on_error)
		{
			return m_impl->accept_until_error(std::move(fn), std::move(on_error));
		}

		void close()
		{
			return m_impl->close();
		}

		bool is_running() const
		{
			return m_impl->is_running();
		}

		bool operator==(const self_t& other) const
		{
			return m_impl == other.m_impl;
		}

		operator bool() const
		{
			return (bool)m_impl;
		}
	private:
		impl_t m_impl;
	};

	//maker function
	template<typename _Impl, typename ..._Args, typename _Input = typename _Impl::input_t,typename _Output = typename _Impl::output_t>
	inline stdx::listener<_Input,_Output> make_listener(_Args &&...args)
	{
		auto impl_ptr = std::make_shared<_Impl>(args...);
		return stdx::listener<_Input, _Output>(impl_ptr);
	}

	//server interface
	template<typename _Input, typename _Output = _Input>
	interface_class basic_server
	{
		using input_t = _Input;

		using output_t = _Output;

		using conn_t = stdx::connection<input_t,output_t>;

		using on_recv_t = std::function<void(conn_t,input_t)>;

		using on_conn_t = std::function<void(conn_t)>;

		using on_client_error_t = std::function<void(conn_t,std::exception_ptr)>;

		using on_server_error_t = std::function<void(std::exception_ptr)>;

		virtual void open(uint64_t port) = 0;

		virtual void open(const stdx::string& ip,uint64_t port) = 0;

		virtual void close() = 0;

		virtual void set_on_recv(on_recv_t &&handle) = 0;

		virtual void set_on_conn(on_conn_t &&handle) = 0;

		virtual void set_on_client_error(on_client_error_t &&handle) = 0;

		virtual void set_on_server_error(on_server_error_t &&handle) = 0;

		virtual void raise_recv(conn_t conn,input_t package) = 0;

		virtual void raise_conn(conn_t conn) = 0;

		virtual void raise_client_error(conn_t conn,std::exception err) = 0;

		virtual void raise_server_error(std::exception_ptr err) = 0;

		virtual bool is_running() const = 0;

		virtual ~basic_server() = default;
	};

	//server
	template<typename _Input, typename _Output = _Input>
	class server
	{
		using impl_t = std::shared_ptr<stdx::basic_server<_Input, _Output>>;

		using self_t = stdx::server<_Input,_Output>;

		using conn_t = stdx::connection<_Input,_Output>;

		using on_recv_t = std::function<void(conn_t, _Input)>;

		using on_conn_t = std::function<void(conn_t)>;

		using on_client_error_t = std::function<void(conn_t, std::exception_ptr)>;

		using on_server_error_t = std::function<void(std::exception_ptr)>;

	public:

		explicit server()
			:m_impl(nullptr)
		{}

		server(const impl_t &impl)
			:m_impl(impl)
		{}

		server(const self_t &other)
			:m_impl(other.m_impl)
		{}

		server(self_t&& other) noexcept
			:m_impl(other.m_impl)
		{}

		~server() = default;

		self_t& operator=(const self_t& other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		self_t& operator=(self_t&& other) noexcept
		{
			m_impl = other.m_impl;
			return *this;
		}

		void open(uint64_t port)
		{
			return m_impl->open(port);
		}

		void open(const stdx::string& ip, uint64_t port)
		{
			return m_impl->open(ip,port);
		}

		void close()
		{
			return m_impl->close();
		}

		self_t &set_on_recv(on_recv_t&& handle)
		{
			m_impl->set_on_recv(std::move(handle));
			return *this;
		}

		self_t& set_on_conn(on_conn_t&& handle)
		{
			m_impl->set_on_conn(std::move(handle));
			return *this;
		}

		self_t &set_on_client_error(on_client_error_t&& handle)
		{
			m_impl->set_on_client_error(std::move(handle));
			return *this;
		}

		self_t &set_on_server_error(on_server_error_t&& handle)
		{
			m_impl->set_on_server_error(std::move(handle));
			return *this;
		}

		void  raise_recv(conn_t conn,_Input package)
		{
			return m_impl->raise_recv(conn, package);
		}

		void raise_conn(conn_t conn)
		{
			return m_impl->raise_conn(conn);
		}

		void raise_client_error(conn_t conn, std::exception err)
		{
			return m_impl->raise_client_error(conn, err);
		}

		void raise_server_error(std::exception_ptr err)
		{
			return m_impl->raise_server_error(err);
		}

		bool is_running() const
		{
			return m_impl->is_running();
		}

		bool operator==(const self_t& other) const
		{
			m_impl = other.m_impl;
			return *this;
		}

		operator bool() const
		{
			return (bool)m_impl;
		}
	private:
		impl_t m_impl;
	};

	//maker function
	template<typename _Impl, typename ..._Args, typename _Input = typename _Impl::input_t,typename _Output = typename _Impl::output_t>
	inline stdx::server<_Input,_Output> make_server(_Args&&...args)
	{
		auto impl_ptr = std::make_shared<_Impl>(args...);
		return stdx::server<_Input, _Output>(impl_ptr);
	}
}