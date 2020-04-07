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
#endif // STDX_NOT_LIMITED_CPU_USING
}

//构造函数

stdx::_Threadpool::_Threadpool() noexcept
	:m_alive_count(std::make_shared<uint32_t>(0))
	,m_free_count(std::make_shared<uint32_t>(0))
	, m_count_lock()
	, m_queue_lock()
	, m_alive(std::make_shared<bool>(true))
	, m_task_queue(std::make_shared<std::queue<runable_ptr>>())
	, m_barrier()
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

uint32_t stdx::_Threadpool::expand_number_of_threads()
{
#ifndef STDX_NOT_LIMITED_CPU_USING
	return suggested_threads_number();
#endif
	uint32_t cores = cpu_cores();
	cores += 1;
	cores *= 2;
	if (cores < 20)
	{
		return cores;
	}
	return 20;
}

bool stdx::_Threadpool::need_expand() const
{
	uint32_t limited = cpu_cores()*10;
	if (*m_alive_count > limited)
	{
		return false;
	}
	if (*m_free_count < m_task_queue->size())
	{
		return true;
	}
	return false;
}

void stdx::_Threadpool::expand(uint32_t number_of_threads)
{
	for (size_t i = 0; i < number_of_threads; i++)
	{
		add_thread();
	}
}

void stdx::_Threadpool::join_handle()
{
	std::unique_lock<stdx::spin_lock> lock(m_count_lock);
	*m_alive_count += 1;
	lock.unlock();
	auto handle = [](std::shared_ptr<std::queue<runable_ptr>> tasks, stdx::semaphore semaphore, std::shared_ptr<uint32_t> count, stdx::spin_lock count_lock, stdx::spin_lock queue_lock, std::shared_ptr<bool> alive, std::shared_ptr<uint32_t> alive_count)
	{

		stdx::finally fin([count_lock, alive_count]() mutable
		{
			std::unique_lock<stdx::spin_lock> lock(count_lock);
			*alive_count -= 1;
		});

		//如果存活
		while (*alive)
		{
			//等待通知
			if (!semaphore.wait_for(std::chrono::minutes(10)))
			{
				//如果10分钟后未通知
				//退出线程
				std::unique_lock<stdx::spin_lock> lock(count_lock);
#ifdef DEBUG
				::printf("[Threadpool]线程池等待任务超时,清除线程\n");
#endif
				* count = *count - 1;
				return;
			}
			if (!(tasks->empty()))
			{
#ifdef DEBUG
				::printf("[Threadpool]当前线程池空闲线程数:%u\n", *count);
#endif
				//如果任务列表不为空
				//减去一个计数
				std::unique_lock<stdx::spin_lock> lock(count_lock);
				*count = *count - 1;
#ifdef DEBUG
				::printf("[Threadpool]当前线程池空闲线程数:%u\n", *count);
#endif
				lock.unlock();
				//执行任务
				try
				{
					std::unique_lock<stdx::spin_lock> _lock(queue_lock);
#ifdef DEBUG
					::printf("[Threadpool]线程池正在获取任务\n");
#endif
					runable_ptr t = std::move(tasks->front());
					//从queue中pop
					tasks->pop();
					_lock.unlock();
#ifdef DEBUG
					::printf("[Threadpool]线程池获取任务成功\n");
#endif
					if (t)
					{
						t->run();
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
#ifdef DEBUG
				::printf("[Threadpool]当前剩余未处理任务数:%zu\n", tasks->size());
#endif
				//完成或终止后
				//添加计数
				lock.lock();
				*count = *count + 1;
				lock.unlock();
#ifdef DEBUG
				::printf("[Threadpool]当前线程池空闲线程数:%u\n", *count);
#endif
			}
			else
			{
#ifdef DEBUG
				::printf("[Threadpool]当前线程池空闲线程数:%u\n", *count);
#endif
				continue;
			}
		}
	};
	handle(m_task_queue, m_barrier, m_free_count, m_count_lock, m_queue_lock, m_alive,m_alive_count);
}

//添加线程
void stdx::_Threadpool::add_thread() noexcept
{
#ifdef DEBUG
	printf("[Threadpool]正在创建新线程\n");
#endif
	std::unique_lock<stdx::spin_lock> lock(m_count_lock);
	*m_alive_count += 1;
	lock.unlock();
	//创建线程
	std::thread t([](std::shared_ptr<std::queue<runable_ptr>> tasks, stdx::semaphore semaphore, std::shared_ptr<uint32_t> count, stdx::spin_lock count_lock,stdx::spin_lock queue_lock, std::shared_ptr<bool> alive,std::shared_ptr<uint32_t> alive_count)
	{
		stdx::finally fin([count_lock,alive_count]() mutable
		{
				std::unique_lock<stdx::spin_lock> lock(count_lock);
				*alive_count -= 1;
		});
		//如果存活
		while (*alive)
		{
			//等待通知
			if (!semaphore.wait_for(std::chrono::minutes(10)))
			{
				//如果10分钟后未通知
				//退出线程
				std::unique_lock<stdx::spin_lock> lock(count_lock);
#ifdef DEBUG
				::printf("[Threadpool]线程池等待任务超时,清除线程\n");
#endif
				*count = *count - 1;
				return;
			}
			if (!(tasks->empty()))
			{
#ifdef DEBUG
				::printf("[Threadpool]当前线程池空闲线程数:%u\n", *count);
#endif
				//如果任务列表不为空
				//减去一个计数
				std::unique_lock<stdx::spin_lock> lock(count_lock);
				* count = *count - 1;
#ifdef DEBUG
				::printf("[Threadpool]当前线程池空闲线程数:%u\n", *count);
#endif
				lock.unlock();
				//执行任务
				try
				{
					std::unique_lock<stdx::spin_lock> _lock(queue_lock);
#ifdef DEBUG
					::printf("[Threadpool]线程池正在获取任务\n");
#endif
					runable_ptr t = std::move(tasks->front());
					//从queue中pop
					tasks->pop();
					_lock.unlock();
#ifdef DEBUG
					::printf("[Threadpool]线程池获取任务成功\n");
#endif
					if (t)
					{
						t->run();
					}
				}
				catch (const std::exception &err)
				{
					//忽略出现的错误
#ifdef DEBUG
					::fprintf(stderr, "[Threadpool]执行任务的过程中出错,%s\n",err.what());
#endif
				}
				catch (...)
				{
				}
#ifdef DEBUG
				::printf("[Threadpool]当前剩余未处理任务数:%zu\n",tasks->size());
#endif
				//完成或终止后
				//添加计数
				lock.lock();
				*count = *count + 1;
				lock.unlock();
#ifdef DEBUG
				::printf("[Threadpool]当前线程池空闲线程数:%u\n", *count);
#endif
			}
			else
			{
#ifdef DEBUG
				::printf("[Threadpool]当前线程池空闲线程数:%u\n", *count);
#endif
				continue;
			}
		}
	}, m_task_queue, m_barrier, m_free_count, m_count_lock,m_queue_lock, m_alive,m_alive_count);
	//分离线程
	t.detach();
}

//初始化线程池

void stdx::_Threadpool::init_threads() noexcept
{
#ifdef DEBUG
	printf("[Threadpool]正在初始化线程池\n");
#endif // DEBUG
	uint32_t threads_number = suggested_threads_number();
	*m_free_count += threads_number;
	expand(threads_number);
#ifdef DEBUG
	printf("[Threadpool]初始化完成,共创建%u条线程\n",threads_number);
#endif // DEBUG
}