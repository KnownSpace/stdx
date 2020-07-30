#include "task_test.h"
#include <stdx/io.h>

int task_test(int argc, char** argv)
{
	{
		auto t = stdx::async([]()
			{
				stdx::printf(U("Hello World\n"));
				return 1;
			})
			.then([](stdx::task_result<int> r) {
				int i = r.get();
				stdx::printf(U("Return value is {0}\n"), i);
				throw std::exception();
				})
				.then([](stdx::task_result<void> r) {
					try
					{
						r.get();
					}
					catch (const std::exception& e)
					{
						stdx::perrorf(U("Error message is {0}\n"), e.what());
					}
				});
		NO_USED(t);
	}
	{
		stdx::task_completion_event<void> ce;
		auto t = ce.get_task().then([]() 
		{
			stdx::printf(U("Complete\n"));
		});
		ce.set_value();
		ce.run();
		NO_USED(t);
	}
	{
		stdx::task_completion_event<void> ce;
		auto t = ce.get_task().then([](stdx::task_result<void> r) {
			try
			{
				r.get();
			}
			catch (const std::exception &e)
			{
				stdx::perrorf(U("Error message is {0}\n"), e.what());
			}
		});
		ce.set_exception(std::make_exception_ptr(std::exception()));
		ce.run();
		NO_USED(t);
	}
	stdx::threadpool.join_as_worker();
	return 0;
}