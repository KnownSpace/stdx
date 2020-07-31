#include "file_test.h"

int file_test(int argc, char** argv)
{
	stdx::file_io_service io_service;
	stdx::file_stream fs = stdx::open_file_stream(io_service, U("./test.txt"), stdx::file_access_type::all, stdx::file_open_type::open);
	fs.read_to_end(0).then([](stdx::task_result<stdx::file_read_event> r) {
		try
		{
			stdx::file_read_event ev = r.get();
			stdx::printf(U("{0}\n"),ev.buffer.size());
		}
		catch (const std::exception &e)
		{
			stdx::perrorf(U("{0}\n"), e.what());
		}
	});
	stdx::threadpool.join_as_worker();
	return 0;
}