#include <iostream>
#include <stdx/file.h>
#include <stdx/string.h>
#include <stdx/net/http.h>
#include <stdx/traits/convertable.h>
#include <stdx/net/socket.h>
#include <list>
#include <stdx/object_pool.h>
#include <stdx/factory.h>

void handle_client(stdx::network_connected_event ev,std::string &doc_content)
{
	stdx::socket c(ev.connection);
	auto t = c.recv(4096).then([doc_content, c](stdx::task_result<stdx::network_recv_event> r) mutable
		{
			try
			{
				auto e = r.get();
				std::string tmp(e.buffer, e.size);
				stdx::http_request&& request = stdx::http_request::from_bytes(tmp);
				if (request.request_header().request_url() == U("/"))
				{
					stdx::http_response response(200);
					response.response_header().add_header(U("Content-Type"), U("text/html"));
					std::string body = stdx::ansi_to_utf8(doc_content);
					const unsigned long long int& tmp = body.size();
					response.response_header().add_header(U("Content-Length"), stdx::to_string(tmp));
					response.response_body().push((const unsigned char*)body.c_str(), body.size());
					return response;
				}
				else
				{
					stdx::http_response response(404);
					response.response_header().add_header(U("Content-Type"), U("text/html"));
					stdx::string body = U("<html><body><h1>Not Found</h1></body></html>");
					response.response_body().push(body);
					return response;
				}
			}
			catch (const std::exception& err)
			{
				stdx::perrorf(U("请求处理出错:{0}\n"), err.what());
				stdx::http_response response(502);
				response.response_header().add_header(U("Content-Type"), U("text/html"));
				stdx::string body = U("<html><body><h1>Server Unavailable</h1></body></html>");
				response.response_body().push(body);
				return response;
			}
		}).then([c](stdx::http_response res) mutable
			{
				std::vector<unsigned char>&& bytes = res.to_bytes();
				stdx::uint64_union u;
				u.value = bytes.size();
				return c.send((const char*)bytes.data(), u.low);
			})
			.then([c](stdx::task_result<stdx::network_send_event> r) mutable
			{
				try
				{
						c.close();
					auto e = r.get();
				}
				catch (const std::exception& err)
				{
					stdx::perrorf(U("发送失败:{0}\n"), err.what());
				}
			});
}

int main(int argc, char** argv)
{
#define ENABLE_WEB
#ifdef ENABLE_WEB
#pragma region web_test
	stdx::file_io_service file_io_service;
	stdx::file doc(file_io_service, U("./index.html"));
	if (!doc.exist())
	{
		stdx::perrorf(U("Error:File {0} does not exist"), doc.path());
		return -1;
	}
	std::string doc_content;
	auto stream = doc.open_stream(stdx::file_access_type::read, stdx::file_open_type::open);
	stream.read_to_end(0).then([&doc_content](stdx::task_result<stdx::file_read_event> r) mutable
		{
			try
			{
				stdx::printf(U("读取文件完成\n"));
				auto&& e = r.get();
				doc_content = std::string(e.buffer, e.buffer.size());
			}
			catch (const std::exception& err)
			{
				stdx::perrorf(U("Error:{0}"), err.what());
				std::terminate();
			}
		}).wait();
		stdx::network_io_service service;
		stdx::socket s = stdx::open_socket(service, stdx::addr_family::ip, stdx::socket_type::stream, stdx::protocol::tcp);
		try
		{
			stdx::ipv4_addr addr(U("0.0.0.0"), 8080);
			if (argc != 1)
			{
				addr.port(std::stoul(argv[1]));
			}
			s.bind(addr);
			s.listen(65535);
			stdx::printf(U("Listen: {0}:{1}\n"), addr.ip(), addr.port());
		}
		catch (std::exception& e)
		{
			stdx::perrorf(U("Error:{0}\n"), e.what());
			return -1;
		}
		stdx::spin_lock lock;
		uint32_t num = 0;
		s.accept_until_error([doc_content](stdx::network_connected_event ev)  mutable
			{
				handle_client(ev,doc_content);
			},
			[](std::exception_ptr error)
			{
				stdx::printf(U("监听关闭\n"));
				try
				{
					if (error)
					{
						std::rethrow_exception(error);
					}
				}
				catch (const std::exception& err)
				{
					stdx::perrorf(U("Accept Error:{0}"), err.what());
				}
			});
		stdx::threadpool::join_handle();
		s.close();
#pragma endregion
#endif
		return 0;
}