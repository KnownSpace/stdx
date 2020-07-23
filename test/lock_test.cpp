#include "lock_test.h"

int lock_test(int argc, char** argv)
{
	{
		stdx::rw_flag flag;
		auto t1 = flag.lock_write().then([flag]() mutable
			{
				::printf("Write\n");
				std::this_thread::sleep_for(std::chrono::seconds(2));
				flag.unlock();
			});
		auto t2 = flag.lock_read().then([flag]() mutable
			{
				::printf("Read 1\n");
				std::this_thread::sleep_for(std::chrono::seconds(10));
				::printf("Read 1 Done\n");
				flag.unlock();
			});
		auto t3 = flag.lock_read().then([flag]() mutable
			{
				::printf("Read 2\n");
				flag.unlock();
			});
	}
	{
		stdx::shared_flag flag(2);
		auto t1 = flag.lock().then([flag]() mutable 
			{
				::printf("Shared 1\n");
				std::this_thread::sleep_for(std::chrono::seconds(2));
				flag.unlock();
			});
		auto t2 = flag.lock().then([flag]() mutable
			{
				::printf("Shared 2\n");
				std::this_thread::sleep_for(std::chrono::seconds(5));
				flag.unlock();
			});
		auto t3 = flag.lock().then([flag]() mutable
			{
				::printf("Shared 3\n");
				std::this_thread::sleep_for(std::chrono::seconds(2));
				flag.unlock();
			});
	}
	stdx::threadpool.join_as_worker();
	return 0;
}