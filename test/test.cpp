#include <iostream>
#include <stdx/file.h>
#include <stdx/net/http.h>
#include <stdx/net/socket.h>
#include <list>
#include <stdx/logger.h>
#include <stdx/net/http_connection.h>
#include <stdx/debug.h>

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

void handle_client_file(stdx::network_connected_event ev, const stdx::file_io_service& io_service)
{
	stdx::socket c(ev.connection);
	auto t = c.recv(4096).then([io_service, c](stdx::task_result<stdx::network_recv_event> r) mutable
		{
			try
			{
				auto e = r.get();
				std::string tmp(e.buffer, e.size);
				stdx::http_request&& request = stdx::http_request::from_bytes(tmp);
				if (request.request_header().request_url() == U("/"))
				{
					request.request_header().request_url().append(U("index.html"));
				}
				if (request.request_header().request_url().end_with(U("/")))
				{
					stdx::http_response response(404);
					response.response_header().add_header(U("Content-Type"), U("text/html"));
					stdx::string body = U("<html><body><h1>Not Found</h1></body></html>");
					response.response_body().push(body);
					return stdx::complete_task(response);
				}
				stdx::string path = U(".");
				path.append(request.request_header().request_url());
				stdx::file file(io_service, path);
				if (file.exist())
				{
					auto stream = file.open_stream(stdx::file_access_type::read, stdx::file_open_type::open);
					return stream.read_to_end(0).then([stream](stdx::file_read_event ev) mutable
						{
							stdx::http_response response(200);
							//response.response_header().add_header(U("Content-Type"), U("text/html"));
							response.response_body().push((const unsigned char*)(const char*)ev.buffer, ev.buffer.size());
							stream.close();
							return response;
						});
				}
				else
				{
					stdx::http_response response(404);
					response.response_header().add_header(U("Content-Type"), U("text/html"));
					stdx::string body = U("<html><body><h1>Not Found</h1></body></html>");
					response.response_body().push(body);
					return stdx::complete_task(response);
				}
			}
			catch (const std::exception& err)
			{
				stdx::perrorf(U("请求处理出错:{0}\n"), err.what());
				stdx::http_response response(502);
				response.response_header().add_header(U("Content-Type"), U("text/html"));
				stdx::string body = U("<html><body><h1>Server Unavailable</h1></body></html>");
				response.response_body().push(body);
				return stdx::complete_task(response);
			}
		})
		.then([c](stdx::http_response res) mutable
			{
				auto&& bytes = res.to_bytes();
				return c.send((const char*)bytes.data(), bytes.size());
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

void handle_request(stdx::http_connection conn, stdx::http_request request,stdx::file_io_service &io_service)
{
	stdx::debug_tracker tracker;
	bool keep = request.request_header().is_keepalive();
	if (request.request_header().request_url() == U("/"))
	{
		request.request_header().request_url().append(U("index.html"));
	}
	if (request.request_header().request_url().end_with(U("/")))
	{
		stdx::http_response response(404);
		if (keep)
		{
			response.response_header().add_header(U("Connection"), U("keep-alive"));
		}
		else
		{
			response.response_header().add_header(U("Connection"), U("close"));
		}
		response.response_header().add_header(U("Content-Type"), U("text/html"));
		stdx::string body = U("<html><body><h1>Not Found</h1></body></html>");
		response.response_body().push(body);
		conn.write(response)
			.then([conn,keep](size_t ev) mutable
			{
				if (!keep)
				{
					conn.close();
				}
			});
		return;
	}
	stdx::string path = U(".");
	path.append(request.request_header().request_url());
	stdx::file file(io_service, path);
	if (file.exist())
	{
		auto stream = file.open_stream(stdx::file_access_type::read, stdx::file_open_type::open);
		stream.read_to_end(0).then([stream,conn,keep,tracker](stdx::file_read_event ev) mutable
			{
				stream.close();
				stdx::http_response response(200);
				if (keep)
				{
					response.response_header().add_header(U("Connection"), U("keep-alive"));
				}
				else
				{
					response.response_header().add_header(U("Connection"), U("close"));
				}
				response.response_body().push(ev.buffer, ev.buffer.size());
				ev.buffer.free();
				return conn.write(response)
					.then([conn, keep,tracker](stdx::task_result<size_t> ev) mutable
						{
							if (!keep)
							{
								conn.close();
							}
						});
			});
	}
	else
	{
		stdx::http_response response(404);
		if (keep)
		{
			response.response_header().add_header(U("Connection"), U("keep-alive"));
		}
		else
		{
			response.response_header().add_header(U("Connection"), U("close"));
		}
		response.response_header().add_header(U("Content-Type"), U("text/html"));
		stdx::string body = U("<html><body><h1>Not Found</h1></body></html>");
		response.response_body().push(body);
		conn.write(response).then([conn, keep](size_t ev) mutable
			{
				if (!keep)
				{
					conn.close();
				}
			});
	}
}

int main(int argc, char** argv)
{
#define ENABLE_WEB
#ifdef ENABLE_WEB
#pragma region web_test
	stdx::file_io_service file_io_service;
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
	s.accept_until_error([file_io_service](stdx::network_connected_event ev)  mutable
		{
			//handle_client(ev,doc_content);
			handle_client_file(ev, file_io_service);
			/*stdx::http_connection conn = stdx::open_http_connection(ev.connection);
			conn.open();
			conn.read_until_error([conn,file_io_service](stdx::http_request req) mutable
			{
				handle_request(conn,req,file_io_service);
			}, [](std::exception_ptr err) 
			{
				try
				{
					if (err)
					{
						std::rethrow_exception(err);
					}
				}
				catch (const std::exception& err)
				{
					stdx::perrorf(U("Handle Error:{0}"), err.what());
				}
			});*/
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
	stdx::threadpool::join_as_worker();
#pragma endregion
#endif
		return 0;
}