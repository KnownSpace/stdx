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
	//unlocker
	{
		stdx::task_flag flag;
		auto t1 = flag.lock().then([flag]() mutable 
		{
			::printf("Flag 1\n");
			stdx::unlocker<stdx::task_flag> unlocker(flag);
			std::this_thread::sleep_for(std::chrono::seconds(2));
			::printf("Flag 1 Done\n");
		});
		auto t2 = flag.lock().then([flag]() mutable 
		{
				::printf("Flag 2\n");
				stdx::unlocker<stdx::task_flag> unlocker(flag);
		});
	}
	//combine tasks
	{
		auto t1 = stdx::async([]() 
		{
				return 1;
		});
		auto t2 = stdx::async([]() 
		{
				return stdx::ignore;
		});
		auto t3 = stdx::async([]() 
		{
				return 'A';
		});
		auto t4 = stdx::async([]() 
		{
				::printf("Void task\n");
		});
		auto t5 = stdx::combine_tasks(t1, t2,t3,stdx::as_ignore_task(t4));
		t5.then([](std::tuple<int,stdx::ignore_t,char,stdx::ignore_t> r) 
		{
			int a, b = 0;
			char c;
			std::tie(a,stdx::ignore,c,stdx::ignore) = std::move(r);
			::printf("Combine Result: %d %d %c\n",a,b,c);
		});
	}
	stdx::threadpool.join_as_worker();
	return 0;
}