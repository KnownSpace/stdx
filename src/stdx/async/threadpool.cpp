#include <stdx/async/threadpool.h>
#include <stdx/finally.h>
#include <stdx/datetime.h>

stdx::threadpool::impl_t stdx::threadpool::m_impl;

//构造函数
stdx::_Threadpool::_Threadpool() noexcept
	: m_alive(std::make_shared<bool>(true))
	, m_task_queue(std::make_shared<std::queue<runable>>())
	, m_cv(std::make_shared<std::condition_variable>())
	, m_mutex(std::make_shared<std::mutex>())
{
	//初始化线程池
	init_threads();
}

//析构函数

stdx::_Threadpool::~_Threadpool() noexcept
{
	//终止时设置状态
	*m_alive = false;
	std::unique_lock<std::mutex> lock(*m_mutex);
	m_cv->notify_all();
}

void stdx::_Threadpool::join_as_worker()
{
	std::unique_lock<std::mutex> lock(*m_mutex);
	lock.unlock();
	auto handle = [](std::shared_ptr<std::queue<runable>> tasks,std::shared_ptr<std::condition_variable> cond, std::shared_ptr<std::mutex> mutex, std::shared_ptr<bool> alive)
	{
		while (*alive)
		{
			std::unique_lock<std::mutex> __lock(*mutex);
			while (tasks->empty() && *alive)
			{
				cond->wait(__lock);
			}
			if (!(tasks->empty()))
			{
				//如果任务列表不为空
				//执行任务
				try
				{
					runable t = std::move(tasks->front());
					//从queue中pop
					tasks->pop();
					__lock.unlock();
					if (t)
					{
						t();
					}
				}
				catch (const std::exception& err)
				{
					//忽略出现的错误
#ifdef DEBUG
					::fprintf(stderr, "[Threadpool]执行任务的过程中出错,%s\n", err.what());
#endif
				}
				catch (...)
				{
				}
			}
		}
	};
	handle(m_task_queue,m_cv,m_mutex,m_alive);
}

//添加线程
void stdx::_Threadpool::add_thread() noexcept
{
	auto handle = [](std::shared_ptr<std::queue<runable>> tasks, std::shared_ptr<std::condition_variable> cond, std::shared_ptr<std::mutex> mutex,std::shared_ptr<bool> alive)
	{
		//如果存活
		while (*alive)
		{
			std::unique_lock<std::mutex> __lock(*mutex);
			//等待通知
			while (tasks->empty() && *alive)
			{
				cond->wait(__lock);
			}
			if (!(tasks->empty()))
			{
				//如果任务列表不为空
				//执行任务
				try
				{
					runable t = std::move(tasks->front());
					//从queue中pop
					tasks->pop();
					__lock.unlock();
					if (t)
					{
						t();
					}
				}
				catch (const std::exception& err)
				{
					//忽略出现的错误
#ifdef DEBUG
					::fprintf(stderr, "[Threadpool]执行任务的过程中出错,%s\n", err.what());
#endif
				}
				catch (...)
				{
				}
			}
		}
	};
	//创建线程
	std::thread t(handle, m_task_queue, m_cv, m_mutex, m_alive);
	//分离线程
	t.detach();
}

//初始化线程池
void stdx::_Threadpool::init_threads() noexcept
{
	uint32_t threads_number = cpu_cores() * 2;
	for (size_t i = 0; i < threads_number; i++)
	{
		add_thread();
	}
}

void stdx::threadpool::loop_do(stdx::cancel_token token, std::function<void()> call)
{
	stdx::threadpool::run([](stdx::cancel_token token, std::function<void()> call)
		{
			if (!token.is_cancel())
			{
				call();
				stdx::threadpool::loop_do(token, call);
			}
		}, token, call);
}

void stdx::threadpool::lazy_do(uint64_t lazy_ms, std::function<void()> call, uint64_t target_tick)
{
	if (lazy_ms == 0)
	{
		call();
		return;
	}
	if (lazy_ms < STDX_LAZY_MAX_TIME)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(lazy_ms));
		call();
	}
	else
	{
		if (target_tick == 0)
		{
			target_tick = stdx::get_tick_count() + lazy_ms;
		}
		uint64_t now = stdx::get_tick_count();
		if (now >= target_tick)
		{
			call();
			return;
		}
		lazy_ms -= (now - target_tick);
		stdx::threadpool::run([target_tick,lazy_ms,call]() 
		{
				stdx::threadpool::lazy_do(lazy_ms, call, target_tick);
		});
	}
}

void stdx::threadpool::lazy_loop_do(stdx::cancel_token token, uint64_t lazy_ms, std::function<void()> call)
{
	stdx::threadpool::lazy_do(lazy_ms, [call,token,lazy_ms]() 
	{
		if (!token.is_cancel())
		{
			call();
			lazy_loop_do(token, lazy_ms, call);
		}
	}, 0);
}