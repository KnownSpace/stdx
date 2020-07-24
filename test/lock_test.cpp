#include "lock_test.h"

int lock_test(int argc, char** argv)
{
	//rw flag
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
				::printf("Read 2 Done\n");
			});
		auto t4 = flag.lock_read().then([flag]() mutable 
		{
			::printf("Read 3 -> Write\n");
			return flag.relock_to_write();
		})
		.then([flag]() mutable 
		{
			::printf("Write 2 Done\n");
			flag.unlock();
		});
	}
	//shared flag
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
	//notice flag
	{
		stdx::notice_flag flag(3);
		auto t1 = stdx::async([flag]() mutable 
		{
			::printf("Notice 1\n");
			flag.notice();
		});
		auto t2 = stdx::async([flag]() mutable
		{
			::printf("Notice 2\n");
			flag.notice();
		});
		auto t3 = flag.get_task().then([flag]() mutable 
		{
			::printf("Be noticed\n");
			flag.reset();
		});
		auto t4 = stdx::async([flag]() mutable
		{
			::printf("Notice 3\n");
			flag.notice();
		});
	}
	stdx::threadpool.join_as_worker();
	return 0;
}