#pragma once
#include <list>
#include <stdx/net/socket.h>

namespace stdx
{
	struct client_connect_event
	{
		stdx::socket client;	//�ͻ���
		bool allow;				//�Ƿ��������
	};

	enum class parser_process
	{
		wait,		//ָʾδ���
		complete,	//ָʾ�����
		overload	//ָʾ�������ݹ���
	};

	struct parser_model
	{
		parser_process process;	//��������
		stdx::buffer buffer;	//������
		size_t pos;				//�ָ��
	};

	struct package_arrivals_event
	{
		stdx::socket client;	//�ͻ���
		stdx::buffer buffer;	//�����İ�������
	};
	
	//�������Ӵ�����
	interface_class accept_handler
	{
	public:
		virtual ~accept_handler() = default;
		virtual void handle(stdx::client_connect_event &) = 0;
	};

	//���ݽ�����
	interface_class parser
	{
	public:
		virtual ~parser() = default;
		virtual void handle(stdx::parser_model &)=0;
	};

	//���ݴ�����
	interface_class package_handler
	{
	public:
		virtual ~package_handler() = default;
		virtual void handle(stdx::package_arrivals_event &) = 0;
	};

	using accept_handler_ptr = std::shared_ptr<accept_handler>;

	using parser_ptr = std::shared_ptr<parser_ptr>;

	using package_handler_ptr = std::shared_ptr<package_handler>;

	struct client_model
	{
		stdx::socket sock;	//�ͻ���
		parser_ptr parser;	//������
	};

	interface_class client_builder
	{
	public:
		virtual ~client_builder() = default;
		virtual client_model build(stdx::socket) = 0;
	};

	using client_builder_ptr = std::shared_ptr<client_builder>;

	class _TcpServer
	{
	public:
		_TcpServer(stdx::network_io_service io_service)
			:m_server(stdx::open_tcpsocket(io_service))
			,m_accept_handler(nullptr)
			,m_client_builder(nullptr)
			,m_package_handler(nullptr)
		{}
		delete_copy(_TcpServer);
		~_TcpServer()=default;

		void bind(const uint_16 &port);

		void bind(cstring ip, const uint_16 &port);

		void set_accept_handler(accept_handler_ptr ptr);

		void set_client_builder(client_builder_ptr ptr);

		void set_package_handler(package_handler_ptr ptr);

		void run();

	private:
		stdx::socket m_server;
		accept_handler_ptr m_accept_handler;
		client_builder_ptr m_client_builder;
		package_handler_ptr m_package_handler;
	};
}