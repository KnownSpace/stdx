#pragma once
#include <stdx/state_machine.h>
#include <stdx/net/http.h>
#include <stdx/buffer.h>

namespace stdx
{
	enum class http_parser_state
	{
		pending,
		wait_header,
		wait_body,
		finish,
		error
	};

	struct http_request_parser_model
	{
		stdx::http_parser_state state;
		std::list<stdx::http_request> requests;
		std::string header_buffer;
		std::string body_buffer;
		std::string arg;
		std::shared_ptr<stdx::http_request_header> header;
		uint64_t max_size = 8*1024*1024;
	};

	using http_request_parser_state_machine = stdx::state_machine<stdx::http_request_parser_model>;

	class _HttpRequestParserState:public stdx::basic_state_machine<stdx::http_request_parser_model>
	{
	protected:
		using model_t = stdx::http_request_parser_model;
		using model_ptr_t = std::shared_ptr<model_t>;
	private:
		using base_t = stdx::basic_state_machine<stdx::http_request_parser_model>;
	public:
		_HttpRequestParserState(const model_ptr_t model);

		virtual ~_HttpRequestParserState() = default;

		virtual model_t& state() override;

		virtual const model_t& state() const override;

		virtual stdx::http_request_parser_state_machine reset() override;

		virtual bool is_end() const override;

		virtual bool movable() const override;
	protected:
		model_ptr_t m_model;

		void _CheckArg() const;

		uint64_t _GetBodySize() const;

		stdx::http_form_ptr _MakeForm();

		std::shared_ptr<stdx::http_request_header> _MakeHeader();

		void _FinishParse(const stdx::http_form_ptr &form);

		void _FinishParse();
	};

	class _HttpRequestParserPendingState:public stdx::_HttpRequestParserState
	{
		using base_t = stdx::_HttpRequestParserState;
	public:
		_HttpRequestParserPendingState(const model_ptr_t model);
		~_HttpRequestParserPendingState() = default;

		virtual stdx::http_request_parser_state_machine move_next() override;
	private:

	};

	class _HttpRequestParserWaitHeaderState:public stdx::_HttpRequestParserState
	{
		using base_t = stdx::_HttpRequestParserState;
	public:
		_HttpRequestParserWaitHeaderState(const model_ptr_t model);
		~_HttpRequestParserWaitHeaderState() = default;

		virtual stdx::http_request_parser_state_machine move_next() override;
	private:

	};

	class _HttpRequestParserWaitBodyState:public stdx::_HttpRequestParserState
	{
		using base_t = stdx::_HttpRequestParserState;
	public:
		_HttpRequestParserWaitBodyState(const model_ptr_t model);
		~_HttpRequestParserWaitBodyState() = default;

		virtual stdx::http_request_parser_state_machine move_next() override;
	private:

	};

	class _HttpRequestParserFinishState:public stdx::_HttpRequestParserState
	{
		using base_t = stdx::_HttpRequestParserState;
	public:
		_HttpRequestParserFinishState(const model_ptr_t model);
		~_HttpRequestParserFinishState() = default;

		virtual stdx::http_request_parser_state_machine move_next() override;

		virtual bool is_end() const override;
	private:

	};

	class _HttpRequestParserErrorState :public stdx::_HttpRequestParserState
	{
		using base_t = stdx::_HttpRequestParserState;
	public:
		_HttpRequestParserErrorState(const model_ptr_t model);
		~_HttpRequestParserErrorState() = default;

		virtual stdx::http_request_parser_state_machine move_next() override;

		virtual bool is_end() const override;
	private:

	};

	class http_request_parser
	{
		using self_t = stdx::http_request_parser;
	public:
		http_request_parser(uint64_t max_size);

		http_request_parser(const self_t &other);

		http_request_parser(self_t&& other) noexcept;

		~http_request_parser() = default;

		self_t& operator=(const self_t& other);

		self_t& operator=(self_t&& other) noexcept;

		void push(stdx::buffer buf,size_t size);

		size_t finish_count() const;

		bool error() const;

		stdx::http_request pop();

		bool operator==(const self_t& other);

		operator bool() const;
	private:
		std::shared_ptr<stdx::http_request_parser_model> m_model;
		stdx::http_request_parser_state_machine m_state_machine;
	};

	class parse_error:public std::logic_error
	{
		using base_t = std::logic_error;
		using self_t = stdx::parse_error;
	public:
		parse_error(const char* what)
			:base_t(what)
		{}

		parse_error(const std::string &what)
			:base_t(what)
		{}

		parse_error(const stdx::string &what)
			:base_t(what.to_native_string())
		{}

		~parse_error() = default;

	private:

	};
}