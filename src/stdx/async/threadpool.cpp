#include <stdx/async/threadpool.h>
#include <stdx/finally.h>

stdx::threadpool::impl_t stdx::threadpool::m_impl;
uint32_t stdx::suggested_threads_number()
{
	uint32_t cores = cpu_cores();
#ifdef STDX_NOT_LIMITED_CPU_USING
	return cores * 2 + 2;
#else
	if (cores < 3)
	{
		return cores;
	}
	if (cores < 9)
	{
		return cores * 2 + 2;
	}
	else
	{
		return 20;
	}
#endif
}

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
#ifdef DEBUG
	printf("[Threadpool]线程池正在销毁\n");
#endif // DEBUG
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
			else
			{
				continue;
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
			else
			{
				continue;
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
#ifdef DEBUG
	printf("[Threadpool]正在初始化线程池\n");
#endif // DEBUG
	uint32_t threads_number = suggested_threads_number() * 2;
	for (size_t i = 0; i < threads_number; i++)
	{
		add_thread();
	}
#ifdef DEBUG
	printf("[Threadpool]初始化完成,共创建%u条线程\n",threads_number);
#endif // DEBUG
}