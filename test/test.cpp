#include <iostream>
#include <stdx/file.h>
#include <stdx/string.h>
#include <stdx/net/http.h>
#include <stdx/traits/convertable.h>
#include <stdx/net/socket.h>
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
		stdx::perrorf(U("{0}\n"), e.what());
		return -1;
	}
	std::cout << "listen: http://0.0.0.0:8080" << std::endl;
	s.listen(65535);
	while (true)
	{
		auto c = s.accept();
		auto t = c.recv(1024).then([c](stdx::network_recv_event &e) mutable
		{
			std::string request(e.buffer,e.size);
			stdx::printf(U("Receive Data:\n{0}\n"),request);
			try
			{
				stdx::http_request_header rq_header = stdx::http_request_header::from_string(stdx::string::from_u8_string(request));
				stdx::printf(U("HTTP-Version:{0}\nUrl:{1}\nMethod:{2}"), stdx::http_version_string(rq_header.version()), rq_header.request_url(), stdx::http_method_string(rq_header.method()));
			}
			catch (const std::exception&err)
			{
				stdx::perrorf(U("{0}\n"), err.what());
			}
			stdx::http_response_header header(200);
			header.cookies().push_back(stdx::http_cookie(U("test"), U("test")));
			header.add_header(U("Content-Type"), U("text/html"));
			std::string str = header.to_string().to_u8_string();
			std::string body = "<html><body><h1>Hello World</h1></body></html>";
			str.append("\r\n");
			str.append(body);
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
#pragma endregion
#endif
	return 0;
}