#include <stdx/async/threadpool.h>

const stdx::threadpool::impl_t stdx::threadpool::m_impl = std::make_shared <stdx::_Threadpool>();

//构造函数

stdx::_Threadpool::_Threadpool() noexcept
	:m_free_count(std::make_shared<uint_32>())
	, m_count_lock()
	, m_alive(std::make_shared<bool>(true))
	, m_task_queue(std::make_shared<std::queue<runable_ptr>>())
	, m_barrier()
	, m_lock()
{
	//初始化线程池
	init_threads();
}

//析构函数

stdx::_Threadpool::~_Threadpool() noexcept
{
	//终止时设置状态
	*m_alive = false;
}

//添加线程

void stdx::_Threadpool::add_thread() noexcept
{
	//创建线程
	std::thread t([](std::shared_ptr<std::queue<runable_ptr>> tasks, stdx::semaphore semaphore, stdx::spin_lock lock, std::shared_ptr<uint_32> count, stdx::spin_lock count_lock, std::shared_ptr<bool> alive)
	{
		//如果存活
		while (*alive)
		{
			//等待通知
			if (!semaphore.wait_for(std::chrono::minutes(10)))
			{
				//如果10分钟后未通知
				//退出线程
				count_lock.lock();
				*count -= 1;
				count_lock.unlock();
				return;
			}
			if (!(tasks->empty()))
			{
				//如果任务列表不为空
				//减去一个计数
				*count -= 1;
				//进入自旋锁
				lock.lock();
				if (tasks->empty())
				{
					*count += 1;
					lock.unlock();
					continue;
				}
				//获取任务
				runable_ptr t(std::move(tasks->front()));
				//从queue中pop
				tasks->pop();
				//解锁
				lock.unlock();
				//执行任务
				try
				{
					if (t)
					{
						t->run();
					}
				}
				catch (...)
				{
					//忽略出现的错误
				}
				//完成或终止后
				//添加计数
				count_lock.lock();
				*count += 1;
				count_lock.unlock();
			}
			else
			{
				continue;
			}
		}
	}, m_task_queue, m_barrier, m_lock, m_free_count, m_count_lock, m_alive);
	//分离线程
	t.detach();
}

//初始化线程池

void stdx::_Threadpool::init_threads() noexcept
{
	unsigned int cores = (cpu_cores()) << 2;
	*m_free_count += cores;
	for (unsigned int i = 0; i < cores; i++)
	{
		add_thread();
	}
}
