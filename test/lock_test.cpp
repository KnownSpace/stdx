#include "lock_test.h"

int lock_test(int argc, char** argv)
{
	constexpr size_t test_count = 20;
	//rw flag
	{
		stdx::rw_flag flag;
		std::shared_ptr<std::vector<int>> list = std::make_shared<std::vector<int>>();
		for (size_t i = 0; i < test_count; i++)
		{
			auto x = flag.lock_write().then([i,flag,list]() mutable {
				stdx::unlocker<stdx::rw_flag> lock(flag);
				list->push_back(i);
			});
			NO_USED(x);
		}
		while (list->size() != test_count)
		{
			std::this_thread::yield();
		}
		for (size_t i = 0; i < test_count; i++)
		{
			auto x = flag.lock_read().then([i,flag,list]() mutable {
				stdx::unlocker<stdx::rw_flag> lock(flag);
				::printf("list[%zu] = %zu\n",i,list->at(i));
			});
			NO_USED(x);
		}
	}
	//shared flag
	{
		stdx::shared_flag flag(3);
		for (size_t i = 0; i < test_count; i++)
		{
			auto x = flag.lock().then([flag,i]()mutable {
				stdx::unlocker<stdx::shared_flag> lock(flag);
				::printf("Shared Test %zu\n",i);
			});
			NO_USED(x);
		}
	}
	//unique flag
	{
		stdx::unique_flag flag;
		for (size_t i = 0; i < test_count;++i)
		{
			auto x = flag.lock().then([flag,i]() mutable
			{
				stdx::unlocker<stdx::unique_flag> unlocker(flag);
				::printf("Unique Test %zu\n", i);
			});
		}
	}
	stdx::threadpool.join_as_worker();
	return 0;
}