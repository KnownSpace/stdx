#pragma once
#include <stdx/net/socket.h>
#include <stdx/file.h>

namespace stdx
{
	//connection interface
	template<typename _Input,typename _Output = _Input>
	INTERFACE_CLASS basic_connection
	{
		using input_t = _Input;

		using output_t = _Output;

		virtual void open()
		{}

		virtual void close() = 0;

		virtual stdx::task<input_t> read() = 0;

		virtual void read_until(stdx::cancel_token token, std::function<void(input_t)> fn,std::function<void(std::exception_ptr)> err_handler) = 0;

		virtual stdx::task<size_t> write(const output_t &package) = 0;

		virtual stdx::task<size_t> write(stdx::buffer buf, size_t size) = 0;

		virtual stdx::task<void> write_file(stdx::file_handle file) = 0;

		INTERFACE_CLASS_HELPER(basic_connection);
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
			:m_impl(std::move(other.m_impl))
		{}

		~connection() = default;

		self_t& operator=(const self_t& other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		self_t& operator=(self_t&& other) noexcept
		{
			m_impl = std::move(other.m_impl);
			return *this;
		}

		void open()
		{
			return m_impl->open();
		}

		void close()
		{
			return m_impl->close();
		}

		stdx::task<_Input> read()
		{
			return m_impl->read();
		}

		void read_until(stdx::cancel_token token, std::function<void(_Input)> fn, std::function<void(std::exception_ptr)> err_handler)
		{
			return m_impl->read_until(token, fn, err_handler);
		}

		stdx::task<size_t> write(const _Output& package)
		{
			return m_impl->write(package);
		}

		stdx::task<size_t> write(stdx::buffer buf, size_t size)
		{
			return m_impl->write(buf, size);
		}

		stdx::task<void> write_file(stdx::file_handle file)
		{
			return m_impl->write_file(file);
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
}