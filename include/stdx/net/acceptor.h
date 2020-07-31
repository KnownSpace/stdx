#pragma once
#include <stdx/env.h>
#include <stdx/net/connection.h>

namespace stdx
{
	template<typename _Input, typename _Output = _Input>
	INTERFACE_CLASS basic_acceptor
	{
		using input_t = _Input;

		using output_t = _Output;

		using connection_t = stdx::connection<input_t,output_t>;

		virtual stdx::task<connection_t> accept() = 0;

		virtual void accept_until(stdx::cancel_token token, std::function<void(connection_t)> fn, std::function<void(std::exception_ptr)> err_handler) = 0;

		INTERFACE_CLASS_HELPER(basic_acceptor);
	};

	template<typename _Input, typename _Output = _Input>
	class acceptor
	{
		using impl_t = std::shared_ptr<stdx::basic_acceptor<_Input, _Output>>;

		using self_t = stdx::acceptor<_Input, _Output>;

		using input_t = _Input;

		using output_t = _Output;

		using connection_t = stdx::connection<input_t, output_t>;
	public:
		acceptor()
			:m_impl(nullptr)
		{}

		acceptor(impl_t impl)
			:m_impl(std::move(impl))
		{}

		acceptor(const self_t& other)
			:m_impl(other.m_impl)
		{}

		acceptor(self_t&& other) noexcept
			:m_impl(std::move(other.m_impl))
		{}

		~acceptor() = default;

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

		bool operator==(const self_t& other) const
		{
			return m_impl == other.m_impl;
		}

		operator bool() const
		{
			return (bool)m_impl;
		}

		stdx::task<connection_t> accept()
		{
			return m_impl->accept();
		}

		void accept_until(stdx::cancel_token token, std::function<void(connection_t)> fn, std::function<void(std::exception_ptr)> err_handler)
		{
			return m_impl->accept_until(token, fn, err_handler);
		}
	private:
		impl_t m_impl;
	};

	template<typename _Impl,typename ..._Args,typename _Input = typename _Impl::input_t,typename _Output = typename _Impl::output_t>
	inline stdx::acceptor<_Input, _Output> make_acceptor(_Args &&...args)
	{
		std::shared_ptr<stdx::basic_acceptor<_Input,_Output>> impl = std::make_shared<_Impl>(args...);
		return stdx::acceptor<_Input,_Output>(impl);
	}
}