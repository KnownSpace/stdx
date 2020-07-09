#include "web_test.h"

void add_keepalive(stdx::http_response& res, bool keep)
{
	if (keep)
	{
		res.response_header().add_header(U("Connection"), U("keep-alive"));
	}
}

bool handle_request(stdx::http_connection conn, stdx::http_request req, stdx::file_io_service& ios)
{
	if (req.request_header().request_url() == U("/"))
	{
		req.request_header().request_url().append(U("index.html"));
	}
	bool keep = req.request_header().is_keepalive();
	stdx::string path(U("."));
	path.append(req.request_header().request_url());
	stdx::file file(ios, path);
	if (!path.end_with(U('/')) && file.exist())
	{
		auto stream = file.open_stream(stdx::file_access_type::read, stdx::file_open_type::open);
		auto  x = stream.read_to_end(0)
			.then([stream, conn, keep](stdx::task_result<stdx::file_read_event> r) mutable
				{
					stream.close();
					try
					{
						auto ev = r.get();
						stdx::http_response response(200);
						//response.response_header().add_header(U("Content-Type"), U("text/html"));
						response.response_body().push((const unsigned char*)(const char*)ev.buffer, ev.buffer.size());
						add_keepalive(response, keep);
						auto t = conn.write(response)
							.then([conn, keep](stdx::task_result<size_t> r)mutable
								{
									if (!keep)
									{
										conn.close();
									}
								});
					}
					catch (const std::exception& err)
					{
						stdx::perrorf(U("请求处理出错:{0}\n"), err.what());
						stdx::http_response response(502);
						response.response_header().add_header(U("Content-Type"), U("text/html"));
						stdx::string body = U("<html><body><h1>Server Unavailable</h1></body></html>");
						response.response_body().push(body);
						add_keepalive(response, keep);
						auto t = conn.write(response)
							.then([conn, keep](stdx::task_result<size_t> r)mutable
								{
									if (!keep)
									{
										conn.close();
									}
								});
					}
				});
		NO_USED(x);
	}
	else
	{
		stdx::http_response response(404);
		response.response_header().add_header(U("Content-Type"), U("text/html"));
		stdx::string body = U("<html><body><h1>Not Found</h1></body></html>");
		response.response_body().push(body);
		add_keepalive(response, keep);
		auto t = conn.write(response)
			.then([conn, keep](stdx::task_result<size_t> r)mutable
				{
					if (!keep)
					{
						conn.close();
					}
				});
	}
	return keep;
}

bool handle_request_hello(stdx::http_connection conn, stdx::http_request req)
{

	bool keep = req.request_header().is_keepalive();
	stdx::http_response response(200);
	add_keepalive(response, keep);
	response.response_body().push(U("Hello World"));
	auto t = conn.write(response)
		.then([conn, keep](stdx::task_result<size_t> r)mutable
			{
				if (!keep)
				{
					conn.close();
				}
			});
	return keep;
}



bool handle_request_counter(stdx::http_connection conn, stdx::http_request req,std::shared_ptr<std::atomic_size_t> counter)
{

	bool keep = req.request_header().is_keepalive();
	stdx::http_response response(200);
	add_keepalive(response, keep);
	response.response_body().push(U("Hello World"));
	auto t = conn.write(response)
		.then([conn, keep,counter](stdx::task_result<size_t> r)mutable
			{
				if (!keep)
				{
					conn.close();
				}
				size_t num =  counter->fetch_add(1);
				::printf("%zu\n",num);
			});
	return keep;
}

int web_test(int argc, char** argv)
{
	stdx::file_io_service file_io_service;
	//build io service
	stdx::network_io_service service;
	//open socket
	stdx::socket s = stdx::open_socket(service, stdx::addr_family::ip, stdx::socket_type::stream, stdx::protocol::tcp);
	try
	{
		stdx::ipv4_addr addr(U("0.0.0.0"), 8080);
		if (argc != 1)
		{
			stdx::string port_str = stdx::string::from_native_string(argv[1]);
			stdx::uint32_union port_union;
			port_union.value = port_str.to_uint32();
			addr.port(port_union.low);
		}
		//bind address
		s.bind(addr);
		//listen
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
	stdx::logger logger = stdx::make_default_logger();
	stdx::cancel_token accept_token;
	std::shared_ptr<std::atomic_size_t> counter = std::make_shared<std::atomic_size_t>(0);
	//持续accept
	s.accept_until(accept_token, [file_io_service, logger,counter](stdx::network_connected_event ev)  mutable
		{
			//获取于客户端的连接
			stdx::http_connection conn = stdx::make_http_connection(ev.connection, 8 * 1024 * 1024);
			stdx::cancel_token token;
			//持续read
			conn.read_until(token, [token, conn, file_io_service,counter](stdx::http_request req) mutable
				{
					//处理请求
					if (!handle_request_hello(conn, req))
					{
						token.cancel();
					}
				}, 
				//错误处理相关
				[token, logger, conn](std::exception_ptr err) mutable
				{
					conn.close();
					try
					{
						if (err)
						{
							std::rethrow_exception(err);
						}
					}
					catch (const std::exception& e)
					{
						NO_USED(e);
						token.cancel();
					}
				});
		},
		//错误处理相关
		[accept_token](std::exception_ptr error) mutable
		{
			//stdx::printf(U("Stop Accepting\n"));
			//accept_token.cancel();
			try
			{
				if (error)
				{
					std::rethrow_exception(error);
				}
			}
			catch (const std::exception& err)
			{
				stdx::perrorf(U("Accept Error:{0}\n"), err.what());
			}
		});
	stdx::threadpool.join_as_worker();
	return 0;
}