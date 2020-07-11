#include "client_test.h"

int client_test(int argc, char** argv)
{
	stdx::network_io_service io_service;
	stdx::socket sock = stdx::open_tcpsocket(io_service);
	stdx::ipv4_addr addr(U("127.0.0.1"), 8080);
	sock.connect(addr).then([](stdx::task_result<void> r)
		{

			try
			{
				stdx::printf(U("Complete!\n"));
				r.get();
				stdx::printf(U("OK!\n"));
			}
			catch (...)
			{
				stdx::perrorf(U("Error!\n"));
			}
		}).get();
		return 0;
}