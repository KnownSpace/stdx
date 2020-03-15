#include <iostream>
#include <stdx/file.h>
#include <stdx/string.h>
#include <stdx/net/http.h>
#include <stdx/traits/convertable.h>
int main(int argc, char **argv)
{
	//#define ENABLE_WEB
#ifdef ENABLE_WEB
#pragma region web_test
	stdx::network_io_service service;
	stdx::socket s = stdx::open_socket(service, stdx::addr_family::ip, stdx::socket_type::stream, stdx::protocol::tcp);
	try
	{
		stdx::ipv4_addr addr(U("127.0.0.1"), 8080);
		s.bind(addr);
	}
	catch (std::exception &e)
	{
		perrorf("%s\n", e.what());
		return -1;
	}
	std::cout << "listen: http://0.0.0.0:8080" << std::endl;
	s.listen(65535);
	while (true)
	{
		auto c = s.accept();
		auto t = c.recv_from(1024).then([c](stdx::network_recv_event &e) mutable
		{
			stdx::string ip = e.addr.ip();
			//printf("%"PRISTR"\n", U("recv"));
			//printf("from %"PRISTR":%"PRIu16"",ip.c_str(),e.addr.port());
			stdx::cout() << U("from: ") << ip << U(":") << e.addr.port() << std::endl;
			std::cout << "recv:" << std::endl
				<< e.buffer << std::endl;
			std::string str = "HTTP/1.1 200 OK\r\nContent-Type:text/html";
			std::string body = "<html><body><h1>Hello World</h1></body></html>";
			str.append("\r\n");
			str.append("\r\n");
			str.append(body);
			std::cout << str << std::endl;
			stdx::uint64_union u;
			u.value = str.size();
			auto t = c.send(str.c_str(), u.low).then([c](stdx::task_result<stdx::network_send_event> &r) mutable
			{
				try
				{
					auto e = r.get();
					std::cout << "send: " << e.size << " bytes" << std::endl;
				}
				catch (const std::exception&)
				{

				}
			});
		});
	}
	std::cin.get();
#pragma endregion
#endif
	return 0;
}