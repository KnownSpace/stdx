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
	stdx::file_io_service file_io_service;
	stdx::file doc(file_io_service,U("./index.html"));
	if (!doc.exist())
	{
		stdx::perrorf(U("Error:File {0} does not exist"),doc.path());
		return -1;
	}
	std::string doc_content;
	auto stream = doc.open_stream(stdx::file_access_type::read, stdx::file_open_type::open);
	stream.read_to_end(0).then([&doc_content](stdx::task_result<stdx::file_read_event> r) mutable
	{
		try
		{
			auto e = r.get();
			doc_content = std::string(e.buffer, e.buffer.size());
		}
		catch (const std::exception &err)
		{
			stdx::perrorf(U("Error:{0}"),err.what());
			std::terminate();
		}
	}).wait();
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
		auto t = c.recv(1024).then([c,doc_content](stdx::network_recv_event &e) mutable
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
				else if (rq_header.request_url() == U("/test"))
				{
					stdx::http_response_header header(200);
					header.add_header(U("Content-Type"), U("text/html"));
					std::string body = stdx::ansi_to_utf8(doc_content);
					header.add_header(U("Content-Length"), stdx::to_string(body.size()));
					str = header.to_string().to_u8_string();
					str.append("\r\n");
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
					std::string body = stdx::ansi_to_utf8(doc_content);
					header.add_header(U("Content-Length"), stdx::to_string(body.size()));
					str = header.to_string().to_u8_string();
					str.append("\r\n");
					str.append(body);
				}
				return str;
			}
			catch (const std::exception&err)
			{
				stdx::perrorf(U("Error:{0}\n"), err.what());
			}
			return std::string();
		}).then([c](std::string str) mutable
		{
			stdx::uint64_union u;
			u.value = str.size();
			auto t = c.send(str.c_str(), u.low).then([c](stdx::task_result<stdx::network_send_event>& r) mutable
			{
				try
				{
					auto e = r.get();
					stdx::printf(U("Send: {0} Bytes\n"), e.size);
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