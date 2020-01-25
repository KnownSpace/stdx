#include <stdx/async/threadpool.h>

const stdx::threadpool::impl_t stdx::threadpool::m_impl = std::make_shared <stdx::_Threadpool>();

uint_32 stdx::suggested_threads_number()
{
	uint_32 cores = cpu_cores();
	if (cores < 8)
	{
		return cores*2;
	}
	else
	{
		return 8;
	}
}

//���캯��

stdx::_Threadpool::_Threadpool() noexcept
	:m_free_count(std::make_shared<uint_32>())
	, m_count_lock()
	, m_alive(std::make_shared<bool>(true))
	, m_task_queue(std::make_shared<std::queue<runable_ptr>>())
	, m_barrier()
	, m_lock()
{
	//��ʼ���̳߳�
	init_threads();
}

//��������

stdx::_Threadpool::~_Threadpool() noexcept
{
	//��ֹʱ����״̬
	*m_alive = false;
#ifdef DEBUG
	printf("[Threadpool]�̳߳���������\n");
#endif // DEBUG
}

//����߳�

void stdx::_Threadpool::add_thread() noexcept
{
	//�����߳�
	std::thread t([](std::shared_ptr<std::queue<runable_ptr>> tasks, stdx::semaphore semaphore, stdx::spin_lock lock, std::shared_ptr<uint_32> count, stdx::spin_lock count_lock, std::shared_ptr<bool> alive)
	{
		//������
		while (*alive)
		{
			//�ȴ�֪ͨ
			if (!semaphore.wait_for(std::chrono::minutes(10)))
			{
				//���10���Ӻ�δ֪ͨ
				//�˳��߳�
#ifdef DEBUG
				printf("[Threadpool]�̳߳صȴ�����ʱ,����߳�\n");
#endif // DEBUG
				count_lock.lock();
				*count -= 1;
				count_lock.unlock();
				return;
			}
			if (!(tasks->empty()))
			{
#ifdef DEBUG
				printf("[Threadpool]��ǰ�̳߳ؿ����߳���:%d\n", *count);
#endif // DEBUG
				//��������б�Ϊ��
				//��ȥһ������
				*count -= 1;
				//����������
				lock.lock();
				if (tasks->empty())
				{
					*count += 1;
					lock.unlock();
					continue;
				}
#ifdef DEBUG
				printf("[Threadpool]�̳߳��ѽ��ձ�Ͷ�ݵ�����\n");
#endif // DEBUG
				//��ȡ����
				runable_ptr t(tasks->front());
				//��queue��pop
				tasks->pop();
				//����
				lock.unlock();
				//ִ������
				try
				{
					if (t)
					{
						t->run();
					}
				}
				catch (...)
				{
					//���Գ��ֵĴ���
				}
#ifdef DEBUG
				printf("[Threadpool]��ǰʣ��δ����������:%lld\n",tasks->size());
#endif // DEBUG
				//��ɻ���ֹ��
				//��Ӽ���
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
	//�����߳�
	t.detach();
}

//��ʼ���̳߳�

void stdx::_Threadpool::init_threads() noexcept
{
#ifdef DEBUG
	printf("[Threadpool]���ڳ�ʼ���̳߳�\n");
#endif // DEBUG
	unsigned int threads_number = suggested_threads_number();
	*m_free_count += threads_number;
	for (unsigned int i = 0; i < threads_number; i++)
	{
		add_thread();
	}
}