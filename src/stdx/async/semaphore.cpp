#include <stdx/async/semaphore.h>

void stdx::_Semaphore::wait()
{
	std::unique_lock<std::mutex> lock(*mutex);
	cv->wait(lock, [this]() mutable
	{
		int value = notify_count->load();
		if (value == 0)
		{
			return false;
		}
		int new_value = value - 1;
		while (true)
		{
			if (value == 0)
			{
				return false;
			}
			bool exchange = notify_count->compare_exchange_strong(value, new_value);
			if ((!exchange) && (!value))
			{
				return false;
			}
			else if (exchange)
			{
				return true;
			}
		}
	});
}

void stdx::_Semaphore::notify()
{
	notify_count->fetch_add(1);
	this->cv->notify_one();
}

void stdx::_Semaphore::notify_all()
{
	cv->notify_all();
	notify_count->store(0);
}