#include <iostream>
#include <stdx/file.h>
#include <stdx/net/socket.h>
#include <sstream>
#include <stdx/string.h>
#include <stdx/logger.h>
#include <list>
#include <stdx/big_int.h>
#include <stdx/algorithm.h>
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
//#define ENABLE_FILE
#ifdef ENABLE_FILE
	stdx::file_io_service service;
	stdx::file file(service, U("./a.txt"));
	if (file.exist())
	{
		file.remove();
	}
	stdx::file_stream stream = file.open_stream(stdx::file_access_type::all, stdx::file_open_type::create);
	stdx::realpath(file.path()).then([](stdx::task_result<stdx::string> r)
	{
			try
			{
				auto path = r.get();
#ifdef WIN32
				printf("full path is %ls\n", path.c_str());
#else
				printf("full path is %s\n", path.c_str());
#endif
			}
			catch (const std::system_error & err)
			{
				std::cerr << err.code().message() << "\n";
				throw;
			}
	}).wait();
	auto t = stream.write(U("Hello World"), 0)
		.then([stream](stdx::task_result<stdx::file_write_event> r) mutable
	{
		stdx::cout() << U("写入完成\n");
		try
		{
			auto ev = r.get();
			
			stdx::cout() << U("Total Writed: ") << ev.size << U(" Bytes") << std::endl;
		}
		catch (const std::system_error &err)
		{
			stdx::string str = stdx::string::from_native_string(err.code().message());
			stdx::cerr() << str << U("\n");
			throw;
		}
		return stream.read_to_end(0);
	}).then([stream](stdx::task_result<stdx::file_read_event> r) mutable
	{
			stream.close();
			try
			{
				auto ev = r.get();
				stdx::string str = stdx::string::from_buffer(ev.buffer);
				stdx::cout() << U("content:") << str << U("\n");

			}
			catch (const std::system_error &err)
			{
				stdx::string str = stdx::string::from_native_string(err.code().message());
				stdx::cerr() << str << U("\n");
				throw;
			}
	});
	t.wait();
	int i = stdx::cancel_token_value::none;
	file.copy_to(U("./b.txt"), &i, [](uint64_t total_size,uint64_t tran_size) 
	{
		stdx::cout() << U("copy:") << tran_size << U(" bytes\n");
	}, [](uint64_t total_size, uint64_t tran_size) 
	{
		std::cout << U("cancel\n");
	});
	std::cin.get();
#endif // ENABLE_FILE
	stdx::string a, b;
	stdx::cin() >> a >> b;
	stdx::_BigInt bi1 = stdx::_BigInt::from_hex_string(a), bi2 = stdx::_BigInt::from_hex_string(b);
	bi1 *= bi2;
	stdx::cout() << bi1.to_hex_string()
			<<std::endl;
	return 0;
}