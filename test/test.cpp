#include <iostream>
#include <stdx/file.h>
#include <stdx/string.h>
#include <stdx/net/http.h>
#include <stdx/traits/convertable.h>
#include <stdx/net/socket.h>
int main(int argc, char **argv)
{
#define ENABLE_WEB
#ifdef ENABLE_WEB
#pragma region web_test
	stdx::network_io_service service;
	stdx::socket s = stdx::open_socket(service, stdx::addr_family::ip, stdx::socket_type::stream, stdx::protocol::tcp);
	try
	{
		stdx::ipv4_addr addr(U("127.0.0.1"), 8080);
		s.bind(addr);
		stdx::printf(U("Listen: {0}:{1}\n"),addr.ip(),addr.port());
	}
	catch (std::exception &e)
	{
		stdx::perrorf(U("Error:{0}\n"), e.what());
		return -1;
	}
	s.listen(65535);
	while (true)
	{
		auto c = s.accept();
		auto t = c.recv(1024).then([c](stdx::network_recv_event &e) mutable
		{
			std::string request(e.buffer,e.size);
			try
			{
				stdx::http_request_header rq_header = stdx::http_request_header::from_string(stdx::string::from_u8_string(request));
				stdx::printf(U("HTTP-Version:{0}\nUrl:{1}\nMethod:{2}\n"),stdx::http_version_string(rq_header.version()),rq_header.request_url(),stdx::http_method_string(rq_header.method()));
				stdx::printf(U("Headers:\n"));
				for (auto begin = rq_header.begin(),end=rq_header.end();begin!=end;begin++)
				{
					stdx::printf(U("	{0}:{1}\n"),begin->first,begin->second);
				}
				if (!rq_header.cookies().empty())
				{
					stdx::printf(U("Cookies:\n"));
					for (auto begin = rq_header.cookies().begin(),end=rq_header.cookies().end();begin!=end;begin++)
					{
						stdx::printf(U("	{0}:{1}\n"),begin->name(),begin->value());
					}
				}
				std::string str;
				if (rq_header.request_url() != U("/"))
				{
					stdx::http_response_header header(404);
					header.add_header(U("Content-Type"), U("text/html"));
					str = header.to_string().to_u8_string();
					str.append("\r\n");
					std::string body = "<html><body><h1>Not Found</h1></body></html>";
					str.append(body);
				}
				else
				{
					stdx::http_response_header header(200);
					if (rq_header.cookies().empty())
					{
						stdx::http_cookie cookie(U("test"), U("test"));
						cookie.expires() = stdx::datetime::now_utc();
						stdx::time_span span;
						span.minute = 5;
						cookie.expires() += span;
						cookie.set_enable_expires(true);
						header.cookies().push_back(cookie);
					}
					header.add_header(U("Content-Type"), U("text/html"));
					std::string body = "<html><body><h1>Hello World</h1></body></html>";
					header.add_header(U("Content-Length"), stdx::to_string(body.size()));
					str = header.to_string().to_u8_string();
					str.append("\r\n");
					str.append(body);
				}
				stdx::uint64_union u;
				u.value = str.size();
				auto t = c.send(str.c_str(), u.low).then([c](stdx::task_result<stdx::network_send_event>& r) mutable
				{
					try
					{
						auto e = r.get();
						stdx::printf(U("Send: {0} Bytes\n"),e.size);
					}
					catch (const std::exception&)
					{

					}
				});
			}
			catch (const std::exception&err)
			{
				stdx::perrorf(U("Error:{0}\n"), err.what());
			}
		});
	}
#pragma endregion
#endif
	return 0;
}