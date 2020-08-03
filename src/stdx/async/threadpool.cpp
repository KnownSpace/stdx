#include <stdx/async/threadpool.h>
#include <stdx/finally.h>
#include <stdx/datetime.h>
#include <stdx/io.h>

stdx::thread_pool stdx::threadpool = stdx::make_io_thread_pool(GET_CPU_CORES()*2+2);

//构造函数
stdx::_McmpThreadPool::_McmpThreadPool(uint32_t num_threads) noexcept
	: m_alive(std::make_shared<bool>(true))
	, m_task_queue(std::make_shared<std::queue<runable>>())
	, m_cv(std::make_shared<std::condition_variable>())
	, m_mutex(std::make_shared<std::mutex>())
{
	//初始化线程池
	init_threads(num_threads);
}

//析构函数

stdx::_McmpThreadPool::~_McmpThreadPool() noexcept
{
	//终止时设置状态
	*m_alive = false;
	std::unique_lock<std::mutex> lock(*m_mutex);
	m_cv->notify_all();
}

void stdx::_McmpThreadPool::join_as_worker()
{
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
					DBG_VAR(err);
					//忽略出现的错误
#ifdef DEBUG
					::fprintf(stderr, "[Threadpool]Run task fail: %s\n", err.what());
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
void stdx::_McmpThreadPool::add_thread() noexcept
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
					DBG_VAR(err);
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
void stdx::_McmpThreadPool::init_threads(uint32_t num_threads) noexcept
{
	for (size_t i = 0; i < num_threads; i++)
	{
		add_thread();
	}
}

stdx::_RoundRobinThreadPool::_RoundRobinThreadPool(uint32_t num_threads)
	:m_lock()
	,m_index(0)
	,m_enable(std::make_shared<std::atomic_bool>(true))
	,m_workers()
	,m_joiners()
	,m_size(num_threads)
{
	for (uint32_t i = 0; i < num_threads; i++)
	{
		m_workers.emplace_back(new stdx::worker_thread());
	}
}

stdx::_RoundRobinThreadPool::~_RoundRobinThreadPool()
{
	*m_enable = false;
	for (auto begin = m_workers.begin(),end = m_workers.end();begin != end;++begin)
	{
		delete (*begin);
	}
	std::unique_lock<stdx::spin_lock> lock(m_lock);
	for (auto begin = m_joiners.begin(),end = m_joiners.end();begin != end;++begin)
	{
		(*begin)->push([]() {});
	}
}

size_t stdx::_RoundRobinThreadPool::_GetIndex()
{
	return m_index.fetch_add(1);
}

void stdx::_RoundRobinThreadPool::run(std::function<void()>&& task)
{
	size_t index = _GetIndex();
	index %= m_size;
	if (index >= m_workers.size())
	{
		std::unique_lock<stdx::spin_lock> lock(m_lock);
		index %= m_joiners.size();
		m_joiners[index]->push(std::move(task));
		return;
	}
	m_workers[index]->push(std::move(task));
	return;
}

void stdx::_RoundRobinThreadPool::join_as_worker()
{
	std::shared_ptr<stdx::worker_context> context = std::make_shared<stdx::worker_context>();
	if (!context)
	{
		throw std::bad_alloc();
	}
	{
		std::unique_lock<stdx::spin_lock> lock(m_lock);
		m_joiners.push_back(context);
		m_size += 1;
	}
	while (*m_enable)
	{
		std::function<void()> &&task = context->pop();
		task();
	}
}

void stdx::thread_pool::loop_do(stdx::cancel_token token, std::function<void()> call)
{
	run([this](stdx::cancel_token token, std::function<void()> call)
		{
			if (!token.is_cancel())
			{
				call();
				loop_do(token, call);
			}
		}, token, call);
}

void stdx::thread_pool::lazy_do(uint64_t lazy_ms, std::function<void()> call, uint64_t target_tick)
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
		run([target_tick,lazy_ms,call,this]() 
		{
				lazy_do(lazy_ms, call, target_tick);
		});
	}
}

void stdx::thread_pool::lazy_loop_do(stdx::cancel_token token, uint64_t lazy_ms, std::function<void()> call)
{
	stdx::thread_pool::lazy_do(lazy_ms, [call,token,lazy_ms,this]() 
	{
		if (!token.is_cancel())
		{
			call();
			lazy_loop_do(token, lazy_ms, call);
		}
	}, 0);
}

stdx::thread_pool stdx::make_mcmp_thread_pool(uint32_t size)
{
	return stdx::make_thread_pool<stdx::_McmpThreadPool>(size);
}

stdx::thread_pool stdx::make_round_robin_thread_pool(uint32_t size)
{
	return stdx::make_thread_pool<stdx::_RoundRobinThreadPool>(size);
}

extern stdx::thread_pool stdx::make_io_thread_pool(uint32_t size)
{
	return stdx::make_thread_pool<stdx::_IoThreadPool>(size);
}

stdx::_IoThreadPool::_IoThreadPool(uint32_t num_threads)
#ifdef WIN32
	:m_poller(stdx::make_iocp_poller<stdx::stand_context>())
#else
	:m_poller(stdx::make_epoll_multipoller<stdx::stand_context>(stdx::implicit_cast<size_t>(num_threads)))
#endif
	,m_token()
	,m_threads()
#ifndef WIN32
	, m_lock()
	, m_tasks()
#endif
{
	for (uint32_t i =0;i < num_threads;++i)
	{
		m_threads.push_back(std::make_shared<std::thread>([this,i]() {
			while (!m_token.is_cancel())
			{
#ifndef WIN32
				while (_HandleTasks())
				{}
#endif
				try
				{
#ifdef WIN32
					stdx::stand_context* context = m_poller.get();
#else
					stdx::stand_context* context = m_poller.get_at(i);
#endif
					if (context)
					{
						try
						{
							context->execute(context);
						}
						catch (const std::exception& e)
						{
							DBG_VAR(e);
#ifdef DEBUG
							::printf("[Thread Pool]Error: %s\n", e.what());
#endif
						}
					}
				}
				catch (const std::exception &e)
				{
					DBG_VAR(e);
#ifdef DEBUG
					::printf("[Thread Pool]Get task error: %s\n",e.what());
#endif
				}
			}
		}));
	}
}

stdx::_IoThreadPool::~_IoThreadPool()
{
	m_token.cancel();
	_Join();
}

void stdx::_IoThreadPool::run(std::function<void()>&& task)
{
	_Run(std::move(task));
}

void stdx::_IoThreadPool::join_as_worker()
{
#ifdef WIN32
	while (!m_token.is_cancel())
	{
		stdx::stand_context* context = m_poller.get();
		context->execute(context);
	}
#else
	_Join();
#endif
}

void stdx::_IoThreadPool::_Join()
{
	for (auto begin = m_threads.begin(), end = m_threads.end(); begin != end; ++begin)
	{
		(*begin)->join();
	}
}

void stdx::_IoThreadPool::_Run(std::function<void()> task)
{
#ifdef WIN32
	stdx::stand_context* context = new stdx::stand_context();
	if (context == nullptr)
	{
		throw std::bad_alloc();
	}
	context->execute = [task](stdx::stand_context* context) mutable
	{
		try
		{
			task();
		}
		catch (const std::exception& e)
		{
			::printf("[Thread Pool]Error: %s\n", e.what());
		}
		delete context;
	};
	::memset(&(context->m_ol), 0, sizeof(OVERLAPPED));
	m_poller.post(context);
#else
	bool need_notice;
	{
		std::unique_lock<stdx::spin_lock> lock(m_lock);
		need_notice = m_tasks.empty();
		m_tasks.push_back(std::move(task));
	}
	if (need_notice)
	{
		m_poller.notice();
	}
#endif
}

#ifndef WIN32
bool stdx::_IoThreadPool::_HandleTasks()
{
	std::unique_lock<stdx::spin_lock> lock(m_lock);
	if (m_tasks.empty())
	{
		return false;
	}
	std::function<void()> task = std::move(m_tasks.front());
	m_tasks.pop_front();
	lock.unlock();
	try
	{
		task();
	}
	catch (const std::exception &e)
	{
		DBG_VAR(e);
#ifdef DEBUG
		::printf("[Thread Pool]Error: %s\n",e.what());
#endif
	}
	return true;
}
#endif