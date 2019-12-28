#include <iostream>
#include <stdx/file.h>
#include <stdx/net/socket.h>
#include <sstream>
#include <stdx/string.h>
#include <stdx/logger.h>
int main(int argc, char **argv)
{
	#define ENABLE_WEB
#ifdef ENABLE_WEB
#pragma region web_test
	stdx::network_io_service service;
	stdx::socket s = stdx::open_socket(service, stdx::addr_family::ip, stdx::socket_type::stream, stdx::protocol::tcp);
	try
	{
		std::cout <<typename_of(service)<< "\n";
		stdx::network_addr addr("0.0.0.0", 8080);
		s.bind(addr);
	}
	catch (std::exception &e)
	{
		std::cerr << e.what();
		return -1;
	}
	std::cout << "listen: http://0.0.0.0:8080" << std::endl;
	std::cout << "access: http://s-hk-bgp.sfclub.cc:8080" <<std::endl;
	s.listen(65535);
	//stdx::file_io_service file_io_service;
	while (true)
	{
		auto c = s.accept();
		c.recv_from(1024).then([c](stdx::network_recv_event &e) mutable
		{
			std::cout << "from: " << e.addr.ip() << ":" <<e.addr.port() <<std::endl;
			std::cout << "recv:" << std::endl
					<< e.buffer <<std::endl;
			std::string str = "HTTP/1.1 200 OK\r\nContent-Type:text/html";
			std::string body = "<html><body><h1>Hello World</h1></body></html>";
			//str.append(std::to_string(body.size()));
			str.append("\r\n");
			str.append("\r\n");
			str.append(body);
			std::cout << str <<std::endl;
			c.send(str.c_str(), str.size()).then([c](stdx::task_result<stdx::network_send_event> &r) mutable
			{
				try
				{
					auto e = r.get();
					std::cout << "send: " <<e.size <<" bytes" << std::endl;
				}
				catch (const std::exception&)
				{
					
				}
			});
		});
	}
#pragma endregion
#endif 
//#define ENABLE_FILE
#ifdef ENABLE_FILE
	stdx::file_io_service service;
	stdx::file file(service, "./a.txt");
	stdx::file_stream stream = file.open_stream(stdx::file_access_type::all, stdx::file_open_type::create);
	stream.write("Hello World", 512)
		.then([stream](stdx::file_write_event ev) mutable
	{
		std::cout << "Total Writed: " << ev.size << " Bytes" << std::endl;
		return;
	}).wait();

	std::cin.get();
#endif // ENABLE_FILE
	return 0;
}