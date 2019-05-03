﻿#include <stdx/io.h>
#include <stdx/net/socket.h>
#include <stdx/sys/window.h>
#include <iostream>
int main()
{
	stdx::network_io_service service;
	stdx::socket s(service, stdx::addr_family::ip, stdx::socket_type::stream, stdx::protocol::tcp);
	try 
	{
		stdx::network_addr addr("0.0.0.0", 8080);
		s.bind(addr);
	}
	catch (std::exception &e)
	{
		std::cerr << e.what();
		return -1;
	}
	s.listen(1024);
	while (true)
	{
		auto c = s.accept();
		c.recv(1024).then([c](stdx::network_recv_event e) mutable
		{
			std::string data = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n<html><body><h1>OK!</h1></body></html>";
			return c.send(data.c_str(), data.size()).then([c](stdx::network_send_event)mutable
			{
				c.close();
			});
		});
	}
	return 0;
}