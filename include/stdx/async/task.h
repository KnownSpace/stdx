#pragma once
#include <stdx/async/threadpool.h>
#include <stdx/async/spin_lock.h>
#include <memory>
#include <future>
#include <stdx/traits/value_type.h>
#include <stdx/function.h>
#include <stdx/env.h>


namespace stdx
{
#pragma region SomeDataStruct
	//Task状态
	enum class task_state
	{
		//就绪
		ready = 0,
		//完成
		complete = 1,
		//运行中
		running = 2,
		//错误
		error = 3
	};

	template<typename _T>
	using promise_ptr = std::shared_ptr<std::promise<_T>>;

	using state_ptr = std::shared_ptr<task_state>;

	template<typename _T>
	using shared_future_ptr = std::shared_ptr<std::shared_future<_T>>;

	template<typename _T>
	promise_ptr<_T> make_promise_ptr()
	{
		return std::make_shared<std::promise<_T>>();
	}

	//task_result模板
	template<typename _T>
	class task_result
	{
	public:
		task_result() = default;
		task_result(std::shared_future<_T> future)
			:m_future(future)
		{}

		~task_result() = default;
		task_result(const task_result<_T>& other)
			:m_future(other.m_future)
		{}

		task_result(task_result<_T>&& other) noexcept
			:m_future(std::move(other.m_future))
		{}

		task_result<_T>& operator=(const task_result<_T>& other)
		{
			m_future = other.m_future;
			return *this;
		}

		const _T& get() const
		{

			return m_future.get();
		}
	private:
		std::shared_future<_T> m_future;
	};

	template<>
	class task_result<void>
	{
	public:
		task_result() = default;
		task_result(std::shared_future<void> future)
			:m_future(future)
		{}

		~task_result() = default;
		task_result(const task_result<void>& other)
			:m_future(other.m_future)
		{}

		task_result(task_result<void>&& other) noexcept
			:m_future(std::move(other.m_future))
		{}

		task_result<void>& operator=(const task_result<void>& other)
		{
			m_future = other.m_future;
			return *this;
		}

		void get() const
		{
			m_future.get();
		}
	private:
		std::shared_future<void> m_future;
	};

	template<typename _T>
	class task_result<_T&>
	{
	public:
		task_result() = default;
		task_result(std::shared_future<_T> future)
			:m_future(future)
		{}

		~task_result() = default;
		task_result(const task_result<_T>& other)
			:m_future(other.m_future)
		{}

		task_result(task_result<_T>&& other)
			:m_future(std::move(other.m_future))
		{}

		task_result<_T>& operator=(const task_result<_T>& other)
		{
			m_future = other.m_future;
			return *this;
		}

		_T& get() const
		{

			return m_future.get();
		}
	private:
		std::shared_future<_T> m_future;
	};
#pragma endregion


	template<typename R>
	class _Task;

	template<typename _T>
	struct is_task
	{
		enum
		{
			value = 0
		};
	};

	template<typename _T>
	struct _GetTaskResult
	{
		class err_result;
		using type = err_result;
	};

	template<typename _T>
	using get_task_result = typename _GetTaskResult<_T>::type;

	//Task模板
	template<typename _R>
	class task
	{
		using impl_t = std::shared_ptr<stdx::_Task<_R>>;
	public:
		task()
			:m_impl(nullptr)
		{}

		task(const task<_R>& other)
			: m_impl(other.m_impl)
		{}

		task(task<_R>&& other) noexcept
			: m_impl(std::move(other.m_impl))
		{}

		template<typename _Fn, typename ..._Args
			//checkers
			, class = typename std::enable_if<stdx::is_callable<_Fn>::value
			&&
			(std::is_convertible<typename stdx::function_info<_Fn>::result, _R>::value || std::is_same<typename stdx::value_type<typename stdx::function_info<_Fn>::result>, _R>::value)
			>::type
		>
			task(_Fn&& fn, _Args&&...args)
			:m_impl(std::make_shared<_Task<_R>>(std::move(fn), args...))
		{
		}

		explicit task(impl_t impl)
			:m_impl(impl)
		{}

		task<_R>& operator=(const task<_R>& other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		task<_R>& operator=(task<_R>&& other) noexcept
		{
			m_impl = std::move(other.m_impl);
			return *this;
		}

		operator impl_t()
		{
			return m_impl;
		}

		~task() = default;

		void run()
		{
			m_impl->run();
		}

		void run_on_this_thread() noexcept
		{
			m_impl->run_on_this_thread();
		}

		template<typename _Fn, typename ..._Args>
		static task<_R> start(_Fn&& fn, _Args&&...args)
		{
			auto t = task<_R>(std::move(fn), std::move(args)...);
			t.run();
			return t;
		}

		template<typename _Fn, typename ..._Args>
		static task<_R> start_on(stdx::thread_pool &pool,_Fn&& fn, _Args&&...args)
		{
			auto t = task<_R>(std::move(fn), std::move(args)...);
			t.config(pool);
			t.run();
			return t;
		}

		template<typename _Fn, typename __R = typename stdx::function_info<_Fn>::result
			//checkers
			, class = typename std::enable_if<stdx::is_callable<_Fn>::value
			&&
			(
				std::is_same<stdx::value_type<typename stdx::function_info<_Fn>::arguments::First>, _R>::value
				|| std::is_same<stdx::value_type<typename stdx::function_info<_Fn>::arguments::First>, stdx::task_result<_R>>::value
				|| std::is_same<typename stdx::function_info<_Fn>::arguments::First, void>::value
				||
				(
					stdx::is_task<_R>::value &&
					(
						std::is_same<stdx::value_type<typename stdx::function_info<_Fn>::arguments::First>, typename stdx::get_task_result<_R>>::value
						|| std::is_same<stdx::value_type<typename stdx::function_info<_Fn>::arguments::First>, typename stdx::task_result<typename stdx::get_task_result<_R>>>::value
						)
					)
				)
			&&
			(std::is_same<__R, typename stdx::function_info<_Fn>::result>::value)>::type
		>
			stdx::task<__R> then(_Fn&& fn)
		{
			stdx::task<__R> t(m_impl->then(std::move(fn)));
			return t;
		}

		void wait()
		{
			return m_impl->wait();
		}

		task_result<_R> get()
		{
			return m_impl->get();
		}

		bool is_complete() const
		{
			return m_impl->is_complete();
		}

		//template<typename __R>
		//task<void> with(task<__R> other)
		//{
		//	return task<void>(m_impl->with(other.m_impl));
		//}

		operator bool() const
		{
			return (bool)m_impl;
		}

		bool operator==(const stdx::task<_R>& other) const
		{
			return m_impl == other.m_impl;
		}

		stdx::task<_R>& config(stdx::thread_pool &pool)
		{
			m_impl->config(pool);
			return *this;
		}
	private:
		impl_t m_impl;
	};

	template<typename _T>
	struct is_task<stdx::task<_T>>
	{
		enum
		{
			value = 1
		};
	};

	template<typename _T>
	struct _GetTaskResult<stdx::task<_T>>
	{
		using type = _T;
	};

#pragma region InterfaceAndPtrDefined
	//BasicTask
	INTERFACE_CLASS basic_task : public stdx::basic_runable<void>
	{
	public:
		INTERFACE_CLASS_HELPER(basic_task);
		virtual void run_on_this_thread() = 0;
	};

	template<typename _T>
	using task_ptr = std::shared_ptr<stdx::_Task<_T>>;

	template<typename _T, typename _Fn, typename ..._Args>
	inline task_ptr<_T> make_task_ptr(_Fn&& fn, _Args&&...args)
	{
		return std::make_shared<_Task<_T>>(fn, args...);
	}
#pragma endregion

#pragma region TaskCompleter
	//_TaskCompleter模板
	template<typename _t>
	struct _TaskCompleter
	{
		static void call(stdx::runable_ptr<_t> call, promise_ptr<_t> promise, std::shared_ptr<std::shared_ptr<stdx::basic_task>> next, stdx::spin_lock lock, state_ptr state, std::shared_future<_t> future)
		{
			try
			{
				//调用方法
				//设置promise
				auto&& r = call->run();
				promise->set_value(r);
			}
			catch (const std::exception&)
			{
				//加锁
				lock.lock();
				//设置状态为错误
				*state = task_state::error;
				promise->set_exception(std::current_exception());
				//如果有callback
				if (next && *next)
				{
					//解锁
					lock.unlock();
					//运行callback
					(*next)->run_on_this_thread();
					return;
				}
				//解锁
				lock.unlock();
				//future.wait();
				return;
			}
			//加锁
			lock.lock();
			//如果有callback
			if (next && *next)
			{
				*state = task_state::complete;
				//解锁
				lock.unlock();
				//运行callback
				(*next)->run_on_this_thread();
				return;
			}
			//设置状态为完成
			*state = task_state::complete;
			//解锁
			lock.unlock();
			return;
		}
	};

	template<>
	struct _TaskCompleter<void>
	{
		static void call(stdx::runable_ptr<void> call, promise_ptr<void> promise, std::shared_ptr<std::shared_ptr<stdx::basic_task>> next, stdx::spin_lock lock, state_ptr state, std::shared_future<void> future)
		{
			try
			{
				//调用方法
				call->run();
				//设置promise
				promise->set_value();
			}
			catch (const std::exception&)
			{
				//加锁
				lock.lock();
				//设置状态为错误
				*state = task_state::error;
				promise->set_exception(std::current_exception());
				//如果有callback
				if (next && *next)
				{
					//解锁
					lock.unlock();
					//运行callback
					(*next)->run_on_this_thread();
					return;
				}
				//解锁
				lock.unlock();
				//future.wait();
				return;
			}
			//加锁
			lock.lock();
			//如果有callback
			if (next && *next)
			{
				*state = task_state::complete;
				//解锁
				lock.unlock();
				//运行callback
				(*next)->run_on_this_thread();
				return;
			}
			//设置状态为完成
			*state = task_state::complete;
			//解锁
			lock.unlock();
			return;
		}
	};
#pragma endregion

#pragma region TaskContinuationBuilder
	//无法通过编译的情况
	template<typename Input, typename Result, typename Arg>
	struct _TaskContinuationBuilder
	{
		template<typename Fn>
		static std::shared_ptr<_Task<Result>> build(Fn&& fn, std::shared_future<Input> future, state_ptr state, stdx::spin_lock lock, std::shared_ptr<std::shared_ptr<stdx::basic_task>> next)
		{
			using arg_t = typename stdx::function_info<Fn>::arguments;
			static_assert(IS_ARGUMENTS_TYPE(Fn, stdx::task_result<Result>) || IS_ARGUMENTS_TYPE(Fn, Result) || IS_ARGUMENTS_TYPE(Fn, void), "the input function not be allowed");
			return nullptr;
		}
	};

	//上一个Task有返回值,但用户选择忽略的情况
	template<typename Input, typename Result>
	struct _TaskContinuationBuilder<Input, Result, void>
	{
		template<typename Fn>
		static std::shared_ptr<_Task<Result>> build(Fn&& fn, std::shared_future<Input> future, state_ptr state, stdx::spin_lock lock, std::shared_ptr<std::shared_ptr<stdx::basic_task>> next)
		{
			auto t = stdx::make_task_ptr<Result>([](Fn fn, std::shared_future<Input> future)
				{
					future.wait();
					return fn();
				}, fn, future);
			lock.lock();
			if ((*state == task_state::complete) || (*state == task_state::error))
			{
				//解锁
				lock.unlock();
				//运行
				t->run();
				return t;
			}
			*next = t;
			lock.unlock();
			return t;
		}
	};

	//上一个Task有返回值,用户选择使用stdx::task_result<_T>的情况
	template<typename Input, typename Result>
	struct _TaskContinuationBuilder<Input, Result, stdx::task_result<Input>>
	{
		template<typename Fn>
		static std::shared_ptr<_Task<Result>> build(Fn&& fn, std::shared_future<Input> future, state_ptr state, stdx::spin_lock lock, std::shared_ptr<std::shared_ptr<stdx::basic_task>> next)
		{
			auto t = stdx::make_task_ptr<Result>([](Fn fn, std::shared_future<Input> future)
				{
					return fn(task_result<Input>(future));
				}, fn, future);
			lock.lock();
			if ((*state == task_state::complete) || (*state == task_state::error))
			{
				//解锁
				lock.unlock();
				//运行
				t->run();
				return t;
			}
			*next = t;
			lock.unlock();
			return t;
		}
	};

	template<typename Result>
	struct _TaskContinuationBuilder<stdx::task<void>, Result, void>;

	//上一个Task返回void
	template<typename Result>
	struct _TaskContinuationBuilder<void, Result, void>
	{
		template<typename Fn>
		static std::shared_ptr<_Task<Result>> build(Fn&& fn, std::shared_future<void> future, state_ptr state, stdx::spin_lock lock, std::shared_ptr<std::shared_ptr<stdx::basic_task>> next)
		{
			auto t = stdx::make_task_ptr<Result>([](Fn fn, std::shared_future<void> future)
				{
					future.wait();
					return fn();
				}, fn, future);
			lock.lock();
			if ((*state == task_state::complete) || (*state == task_state::error))
			{
				//解锁
				lock.unlock();
				//运行
				t->run();
				return t;
			}
			*next = t;
			lock.unlock();
			return t;
		}
	};

	//上一个Task有返回值,用户选择直接使用返回值的情况
	template<typename Input, typename Result>
	struct _TaskContinuationBuilder<Input, Result, Input>
	{
		template<typename Fn>
		static std::shared_ptr<_Task<Result>> build(Fn&& fn, std::shared_future<Input> future, state_ptr state, stdx::spin_lock lock, std::shared_ptr<std::shared_ptr<stdx::basic_task>> next)
		{
			auto t = stdx::make_task_ptr<Result>([](Fn fn, std::shared_future<Input> future)
				{
					return fn(future.get());
				}, fn, future);
			lock.lock();
			if ((*state == task_state::complete) || (*state == task_state::error))
			{
				//解锁
				lock.unlock();
				//运行
				t->run();
				return t;
			}
			*next = t;
			lock.unlock();
			return t;
		}
	};

	//上一个Task返回一个新的Task,用户选择使用新的Task的返回值的情况
	template<typename Input, typename Result>
	struct _TaskContinuationBuilder<stdx::task<Input>, Result, Input>
	{
		template<typename Fn>
		static std::shared_ptr<_Task<Result>> build(Fn&& fn, std::shared_future<stdx::task<Input>> future, state_ptr state, stdx::spin_lock lock, std::shared_ptr<std::shared_ptr<stdx::basic_task>> next)
		{
			promise_ptr<Input> promise = stdx::make_promise_ptr<Input>();
			auto t = stdx::make_task_ptr<Result>([](Fn fn, std::shared_future<Input> result)
				{
					return fn(result.get());
				}, fn, (std::shared_future<Input>)promise->get_future());
			auto start = stdx::make_task_ptr<void>([](std::shared_ptr<_Task<Result>> t, std::shared_future<stdx::task<Input>> future, promise_ptr<Input> input_promise)
				{
					try
					{
						auto task = future.get();
						auto x = task.then([input_promise, t](Input r) mutable
							{
								input_promise->set_value(r);
								t->run_on_this_thread();
							});
					}
					catch (const std::exception&)
					{
						input_promise->set_exception(std::current_exception());
						t->run_on_this_thread();
					}

				}, t, future, promise);
			lock.lock();
			if ((*state == task_state::complete) || (*state == task_state::error))
			{
				//解锁
				lock.unlock();
				//运行
				start->run();
				return t;
			}
			*next = start;
			lock.unlock();
			return t;
		}
	};

	//上一个Task返回一个新的Task,用户选择使用新的Task的task_result的情况
	template<typename Input, typename Result>
	struct _TaskContinuationBuilder<stdx::task<Input>, Result, stdx::task_result<Input>>
	{
		template<typename Fn>
		static std::shared_ptr<_Task<Result>> build(Fn&& fn, std::shared_future<stdx::task<Input>> future, state_ptr state, stdx::spin_lock lock, std::shared_ptr<std::shared_ptr<stdx::basic_task>> next)
		{
			promise_ptr<Input> promise = stdx::make_promise_ptr<Input>();
			auto t = stdx::make_task_ptr<Result>([](Fn fn, std::shared_future<Input> result)
				{
					//使用future来制作task_result
					return fn(stdx::task_result<Input>(result));
				}, fn, (std::shared_future<Input>)promise->get_future());
			auto start = stdx::make_task_ptr<void>([](std::shared_ptr<_Task<Result>> t, std::shared_future<stdx::task<Input>> future, promise_ptr<Input> input_promise)
				{
					try
					{
						//获取task
						auto task = future.get();
						//延续task
						auto x = task.then([input_promise, t](stdx::task_result<Input> r) mutable
							{
								try
								{
									//设置promise
									input_promise->set_value(r.get());
								}
								catch (const std::exception&)
								{
									//出错则设置异常
									input_promise->set_exception(std::current_exception());
								}
								t->run_on_this_thread();
							});
					}
					catch (const std::exception&)
					{
						//获取Task都已出错
						//设置异常
						input_promise->set_exception(std::current_exception());
						t->run_on_this_thread();
					}

				}, t, future, promise);
			lock.lock();
			if ((*state == task_state::complete) || (*state == task_state::error))
			{
				//解锁
				lock.unlock();
				//运行
				start->run();
				return t;
			}
			*next = start;
			lock.unlock();
			return t;
		}
	};

	//上一个Task返回新的Task<void>,用户选择忽略的情况
	template<typename Result>
	struct _TaskContinuationBuilder<stdx::task<void>, Result, void>
	{
		template<typename Fn>
		static std::shared_ptr<_Task<Result>> build(Fn&& fn, std::shared_future<stdx::task<void>> future, state_ptr state, stdx::spin_lock lock, std::shared_ptr<std::shared_ptr<stdx::basic_task>> next)
		{
			promise_ptr<stdx::task_result<void>> promise = stdx::make_promise_ptr<stdx::task_result<void>>();
			auto t = stdx::make_task_ptr<Result>([](Fn fn, std::shared_future<stdx::task_result<void>> result)
				{
					result.wait();
					return fn();
				}, fn, (std::shared_future<stdx::task_result<void>>)promise->get_future());
			auto start = stdx::make_task_ptr<void>([](std::shared_ptr<_Task<Result>> t, std::shared_future<stdx::task<void>> future, promise_ptr<stdx::task_result<void>> input_promise)
				{
					auto task = future.get();
					auto x = task.then([input_promise, t](stdx::task_result<void> r) mutable
						{
							input_promise->set_value(r);
							t->run_on_this_thread();
						});
				}, t, future, promise);
			lock.lock();
			if ((*state == task_state::complete) || (*state == task_state::error))
			{
				//解锁
				lock.unlock();
				//运行
				start->run();
				return t;
			}
			*next = start;
			lock.unlock();
			return t;
		}
	};

	//上一个Task返回新的Task,用户选择忽略的情况
	template<typename Input, typename Result>
	struct _TaskContinuationBuilder<stdx::task<Input>, Result, void>
	{
		template<typename Fn>
		static std::shared_ptr<_Task<Result>> build(Fn&& fn, std::shared_future<stdx::task<Input>> future, state_ptr state, stdx::spin_lock lock, std::shared_ptr<std::shared_ptr<stdx::basic_task>> next)
		{
			promise_ptr<void> promise = stdx::make_promise_ptr<void>();
			auto t = stdx::make_task_ptr<Result>([](Fn fn, std::shared_future<void> result)
				{
					result.wait();
					return fn();
				}, fn, (std::shared_future<void>)promise->get_future());
			auto start = stdx::make_task_ptr<void>([](std::shared_ptr<_Task<Result>> t, std::shared_future<stdx::task<Input>> future, promise_ptr<void> input_promise)
				{
					auto task = future.get();
					auto x = task.then([input_promise, t]() mutable
						{
							input_promise->set_value();
							t->run_on_this_thread();
						});
				}, t, future, promise);
			lock.lock();
			if ((*state == task_state::complete) || (*state == task_state::error))
			{
				//解锁
				lock.unlock();
				//运行
				start->run();
				return t;
			}
			*next = start;
			lock.unlock();
			return t;
		}
	};
#pragma endregion

	//Task模板的实现
	template<typename R>
	class _Task :public stdx::basic_task
	{
	public:
		//构造函数

		template<typename _Fn, typename ..._Args>
		explicit _Task(_Fn&& f, _Args&&...args)
			:m_action(stdx::make_runable<R>(std::move(f), args...))
			, m_promise(std::make_shared<std::promise<R>>())
			, m_future(std::shared_future<R>(m_promise->get_future()))
			, m_next(std::make_shared<std::shared_ptr<stdx::basic_task>>(nullptr))
			, m_state(std::make_shared<task_state>(stdx::task_state::ready))
			, m_lock()
			, m_pool(nullptr)
		{
		}

		template<typename _Fn>
		explicit _Task(_Fn&& f)
			:m_action(stdx::make_runable<R>(std::move(f)))
			, m_promise(std::make_shared<std::promise<R>>())
			, m_future(std::shared_future<R>(m_promise->get_future()))
			, m_next(std::make_shared<std::shared_ptr<stdx::basic_task>>(nullptr))
			, m_state(std::make_shared<task_state>(stdx::task_state::ready))
			, m_lock()
			, m_pool(nullptr)
		{
		}

		//析构函数
		virtual ~_Task() noexcept
		{}
		//启动一个Task
		virtual void run() noexcept override
		{
			//加锁
			std::unique_lock<stdx::spin_lock> lock(m_lock);
			//如果不在运行
			if (*m_state != stdx::task_state::running)
			{
				//设置状态运行中
				*m_state = stdx::task_state::running;
			}
			else
			{
				//如果正在运行
				//解锁返回
				return;
			}
			//解锁
			lock.unlock();
			//创建方法
			auto f = [](stdx::runable_ptr<R> r
				, promise_ptr<R> promise
				, std::shared_ptr<std::shared_ptr<stdx::basic_task>>  next
				, stdx::spin_lock lock
				, stdx::state_ptr state
				, std::shared_future<R> future)
			{
				stdx::_TaskCompleter<R>::call(r, promise, next, lock, state, future);
			};
			//放入线程池
			if (m_pool)
			{
				m_pool->run(f, m_action, m_promise, m_next, m_lock, m_state, m_future);
				return;
			}
			stdx::threadpool.run(f, m_action, m_promise, m_next, m_lock, m_state, m_future);
		}

		virtual void run_on_this_thread() noexcept override
		{
			std::unique_lock<stdx::spin_lock> lock(m_lock);
			if (!m_state)
			{
				return;
			}
			if ((*m_state) != stdx::task_state::running)
			{
				*m_state = stdx::task_state::running;
			}
			else
			{
				return;
			}
			lock.unlock();
			try
			{
				stdx::_TaskCompleter<R>::call(m_action, m_promise, m_next, m_lock, m_state, m_future);
			}
			catch (const std::exception& err)
			{
				DBG_VAR(err);
#ifdef DEBUG
				::printf("[Task Model]发生未处理的异常:%s", err.what());
#endif // DEBUG
			}
		}

		//等待当前Task(不包括后续)完成
		void wait()
		{
			m_future.wait();
		}

		//等待当前Task(不包括后续)完成并获得结果
		//发生异常则抛出异常
		task_result<R> get()
		{
			return m_future;
		}

		//询问Task是否完成
		bool is_complete() const
		{
			auto c = (*m_state == task_state::complete) || (*m_state == task_state::error);
			return c;
		}

		template<typename _Fn, typename ..._Args>
		static std::shared_ptr<_Task<R>> make(_Fn&& fn, _Args&&...args)
		{
			return std::make_shared<_Task<R>>(fn, args...);
		}

		//延续Task
		template<typename _Fn, typename _R = typename stdx::function_info<_Fn>::result>
		std::shared_ptr<_Task<_R>> then(_Fn&& fn)
		{
			using args_tl = typename stdx::function_info<_Fn>::arguments;
			std::shared_ptr<_Task<_R>> t = _TaskContinuationBuilder<R, _R, stdx::value_type<stdx::type_at<0, args_tl>>>::build(fn, m_future, m_state, m_lock, m_next);
			return t;
		}

		//合并Task
		//template<typename _R>
		//std::shared_ptr<_Task<void>> with(std::shared_ptr<_Task<_R>> other)
		//{
		//	return then([other](stdx::task_result<void>&)
		//		{
		//			other->lock();
		//		});
		//}

		void config(stdx::thread_pool &pool)
		{
			m_pool = &pool;
		}

	protected:
		stdx::runable_ptr<R> m_action;
		stdx::promise_ptr<R> m_promise;
		std::shared_future<R> m_future;
		std::shared_ptr<std::shared_ptr<stdx::basic_task>> m_next;
		stdx::state_ptr m_state;
		stdx::spin_lock m_lock;
		stdx::thread_pool *m_pool;
	};

	//启动一个Task
	template<typename _Fn, typename ..._Args, typename _R = typename stdx::function_info<_Fn>::result, class = typename std::enable_if<stdx::is_callable<_Fn>::value>::type>
	inline stdx::task<_R> async(_Fn&& fn, _Args&&...args)
	{
		return task<_R>::start(fn, args...);
	}

	template<typename _Fn, typename ..._Args, typename _R = typename stdx::function_info<_Fn>::result, class = typename std::enable_if<stdx::is_callable<_Fn>::value>::type>
	inline stdx::task<_R> async_on(stdx::thread_pool &pool,_Fn&& fn, _Args&&...args)
	{
		return task<_R>::start_on(pool,fn, args...);
	}

#pragma region TaskCompleteEvent
	template<typename _R>
	class _TaskCompleteEvent
	{
	public:
		_TaskCompleteEvent()
			:m_promise(stdx::make_promise_ptr<_R>())
			, m_task([](promise_ptr<_R> promise)
				{
					return promise->get_future().get();
				}, m_promise)
		{}

		_TaskCompleteEvent(stdx::thread_pool &pool)
			:m_promise(stdx::make_promise_ptr<_R>())
			, m_task([](promise_ptr<_R> promise)
				{
					return promise->get_future().get();
				}, m_promise)
		{
			if (m_task)
			{
				m_task.config(pool);
			}
		}

		~_TaskCompleteEvent() = default;

		void set_value(_R&& value)
		{
			m_promise->set_value(value);
		}

		void set_value(const _R& value)
		{
			m_promise->set_value(value);
		}

		void set_exception(const std::exception_ptr& error)
		{
			m_promise->set_exception(error);
		}

		stdx::task<_R> get_task()
		{
			return m_task;
		}

		void run()
		{
			m_task.run();
		}

		void run_on_this_thread()
		{
			m_task.run_on_this_thread();
		}

		bool check_task() const
		{
			return m_task;
		}
	private:
		promise_ptr<_R> m_promise;
		stdx::task<_R> m_task;
	};

	template<>
	class _TaskCompleteEvent<void>
	{
	public:
		_TaskCompleteEvent()
			:m_promise(stdx::make_promise_ptr<void>())
			, m_task([](promise_ptr<void> promise)
		{

			promise->get_future().get();
		}, m_promise)
		{}

		_TaskCompleteEvent(stdx::thread_pool &pool)
			:m_promise(stdx::make_promise_ptr<void>())
			, m_task([](promise_ptr<void> promise)
				{

					promise->get_future().get();
				}, m_promise)
		{
			if (m_task)
			{
				m_task.config(pool);
			}
		}

		~_TaskCompleteEvent() = default;
		void set_value()
		{
			m_promise->set_value();
		}
		void set_exception(const std::exception_ptr& error)
		{
			m_promise->set_exception(error);
		}
		stdx::task<void> get_task()
		{
			return m_task;
		}
		void run()
		{
			m_task.run();
		}
		void run_on_this_thread()
		{
			m_task.run_on_this_thread();
		}

		bool check_task() const
		{
			return m_task;
		}
	private:
		promise_ptr<void> m_promise;
		stdx::task<void> m_task;
	};

	template<typename _R>
	class task_completion_event
	{
		using impl_t = std::shared_ptr<_TaskCompleteEvent<_R>>;
	public:
		task_completion_event()
			:m_impl(std::make_shared<_TaskCompleteEvent<_R>>())
		{}

		task_completion_event(stdx::thread_pool &pool)
			:m_impl(std::make_shared<_TaskCompleteEvent<_R>>(pool))
		{}

		task_completion_event(const task_completion_event<_R>& other)
			:m_impl(other.m_impl)
		{}

		task_completion_event(task_completion_event<_R> &&other) noexcept
			:m_impl(std::move(other.m_impl))
		{}

		~task_completion_event() = default;

		task_completion_event<_R>& operator=(const task_completion_event<_R>& other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		task_completion_event<_R>& operator=(task_completion_event<_R>&& other) noexcept
		{
			m_impl = std::move(other.m_impl);
			return *this;
		}

		bool operator==(const task_completion_event<_R>& other) const
		{
			return other.m_impl == m_impl;
		}

		void set_value(_R&& value)
		{
			m_impl->set_value(std::move(value));
		}

		void set_value(const _R& value)
		{
			m_impl->set_value(value);
		}

		void set_exception(const std::exception_ptr& error)
		{
			m_impl->set_exception(error);
		}

		stdx::task<_R> get_task()
		{
			return m_impl->get_task();
		}

		void run()
		{
			m_impl->run();
		}

		void run_on_this_thread()
		{
			m_impl->run_on_this_thread();
		}

		operator bool() const
		{
			return (bool)m_impl;
		}

		bool check_task() const
		{
			return m_impl->check_task();
		}
	private:
		impl_t m_impl;
	};

	template<>
	class task_completion_event<void>
	{
		using impl_t = std::shared_ptr<_TaskCompleteEvent<void>>;
	public:
		task_completion_event()
			:m_impl(std::make_shared<_TaskCompleteEvent<void>>())
		{}

		task_completion_event(stdx::thread_pool &pool)
			:m_impl(std::make_shared<_TaskCompleteEvent<void>>(pool))
		{}

		task_completion_event(const task_completion_event<void>& other)
			:m_impl(other.m_impl)
		{}

		task_completion_event(task_completion_event<void> &&other) noexcept
			:m_impl(std::move(other.m_impl))
		{}

		~task_completion_event() = default;

		task_completion_event<void>& operator=(const task_completion_event<void>& other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		task_completion_event<void>& operator=(task_completion_event<void>&& other) noexcept
		{
			m_impl = std::move(other.m_impl);
			return *this;
		}

		bool operator==(const task_completion_event<void>& other) const
		{
			return other.m_impl == m_impl;
		}

		void set_value()
		{
			m_impl->set_value();
		}

		void set_exception(const std::exception_ptr& error)
		{
			m_impl->set_exception(error);
		}

		stdx::task<void> get_task()
		{
			return m_impl->get_task();
		}

		void run()
		{
			m_impl->run();
		}

		void run_on_this_thread()
		{
			m_impl->run_on_this_thread();
		}

		operator bool() const
		{
			return (bool)m_impl;
		}

		bool check_task() const
		{
			return m_impl->check_task();
		}
	private:
		impl_t m_impl;
	};
#pragma endregion

#pragma region task_flag
	class _TaskFlag
	{
	public:
		_TaskFlag();
		_TaskFlag(stdx::thread_pool &pool);
		~_TaskFlag();
		stdx::task<void> lock();
		void unlock() noexcept;
	private:
		void _RunOrPush(stdx::task_completion_event<void> &ce);

		stdx::spin_lock m_lock;
		bool m_locked;
		std::queue<stdx::task_completion_event<void>> m_wait_queue;
		stdx::thread_pool* m_pool;
	};

	class task_flag
	{
		using impl_t = std::shared_ptr<_TaskFlag>;
	public:
		task_flag()
			:m_impl(std::make_shared<_TaskFlag>())
		{}

		task_flag(stdx::thread_pool &pool)
			:m_impl(std::make_shared<_TaskFlag>(pool))
		{}

		task_flag(const task_flag& other)
			:m_impl(other.m_impl)
		{}

		task_flag(task_flag&& other) noexcept
			:m_impl(std::move(other.m_impl))
		{}

		~task_flag() = default;

		task_flag& operator=(const task_flag& other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		task_flag& operator=(task_flag&& other) noexcept
		{
			m_impl = std::move(other.m_impl);
			return *this;
		}

		bool operator==(const task_flag& other) const
		{
			return m_impl == other.m_impl;
		}

		stdx::task<void> lock()
		{
			return m_impl->lock();
		}

		void unlock() noexcept
		{
			m_impl->unlock();
		}

		operator bool() const
		{
			return (bool)m_impl;
		}

	private:
		impl_t m_impl;
	};
#pragma endregion

	template<typename _T>
	inline stdx::task<_T> complete_task(_T&& arg)
	{
		stdx::task_completion_event<_T> ev;
		ev.set_value(arg);
		ev.run_on_this_thread();
		return ev.get_task();
	}

	inline stdx::task<void> complete_task()
	{
		stdx::task_completion_event<void> ev;
		ev.set_value();
		ev.run_on_this_thread();
		return ev.get_task();
	}

	extern stdx::task<void> lazy(uint64_t ms);

	extern stdx::task<void> lazy_on(stdx::thread_pool &pool,uint64_t ms);

	template<typename _T>
	inline stdx::task<_T> error_task(const std::exception_ptr& err)
	{
		stdx::task_completion_event<_T> ev;
		ev.set_exception(err);
		ev.run_on_this_thread();
		return ev.get_task();
	}

	class _RWFlag
	{
	public:

		enum class lock_state 
		{
			free,
			read,
			write
		};

		_RWFlag();

		_RWFlag(stdx::thread_pool& pool);

		~_RWFlag();

		stdx::task<void> lock_read();

		stdx::task<void> lock_write();

		stdx::task<void> relock_to_write();

		stdx::task<void> relock_to_read();

		void unlock() noexcept;

	private:

		void _RunOrPushRead(stdx::task_completion_event<void>& ce);

		void _RunOrPushWrite(stdx::task_completion_event<void>& ce);

		stdx::spin_lock m_lock;
		lock_state m_state;
		std::queue<stdx::task_completion_event<void>> m_write_queue;
		std::queue<stdx::task_completion_event<void>> m_read_queue;
		stdx::thread_pool* m_pool;
		size_t m_read_ref;
	};

	class rw_flag
	{
		using impl_t = std::shared_ptr<stdx::_RWFlag>;
		using self_t = stdx::rw_flag;
	public:
		using lock_state = stdx::_RWFlag::lock_state;

		rw_flag()
			:m_impl(std::make_shared<stdx::_RWFlag>())
		{}

		rw_flag(stdx::thread_pool &pool)
			:m_impl(std::make_shared<stdx::_RWFlag>(pool))
		{}

		rw_flag(const self_t &other)
			:m_impl(other.m_impl)
		{}

		rw_flag(self_t &&other) noexcept
			:m_impl(std::move(other.m_impl))
		{}

		~rw_flag() = default;

		self_t& operator=(const self_t& other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		self_t& operator=(self_t&& other) noexcept
		{
			m_impl = std::move(other.m_impl);
			return *this;
		}

		bool operator==(const self_t& other) const
		{
			return m_impl == other.m_impl;
		}

		operator bool() const
		{
			return (bool)m_impl;
		}

		stdx::task<void> lock_read()
		{
			return m_impl->lock_read();
		}

		stdx::task<void> lock_write()
		{
			return m_impl->lock_write();
		}

		stdx::task<void> relock_to_write()
		{
			return m_impl->relock_to_write();
		}

		stdx::task<void> relock_to_read()
		{
			return m_impl->relock_to_read();
		}

		void unlock() noexcept
		{
			return m_impl->unlock();
		}
	private:
		impl_t m_impl;
	};

	class _SharedFlag
	{
	public:
		_SharedFlag(size_t count);

		_SharedFlag(size_t count,stdx::thread_pool &pool);

		~_SharedFlag();

		stdx::task<void> lock();

		void unlock() noexcept;

	private:
		void _RunOrPush(stdx::task_completion_event<void> &ce);

		stdx::spin_lock m_lock;
		size_t m_count;
		std::queue<stdx::task_completion_event<void>> m_wait_queue;
		stdx::thread_pool* m_pool;
	};

	class shared_flag
	{
		using impl_t = std::shared_ptr<stdx::_SharedFlag>;
		using self_t = stdx::shared_flag;
	public:
		shared_flag(size_t count)
			:m_impl(std::make_shared<stdx::_SharedFlag>(count))
		{}

		shared_flag(size_t count,stdx::thread_pool& pool)
			:m_impl(std::make_shared<stdx::_SharedFlag>(count,pool))
		{}

		shared_flag(const self_t& other)
			:m_impl(other.m_impl)
		{}

		shared_flag(self_t &&other) noexcept
			:m_impl(std::move(other.m_impl))
		{}

		~shared_flag() = default;

		self_t& operator=(const self_t& other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		self_t& operator=(self_t&& other) noexcept
		{
			m_impl = std::move(other.m_impl);
			return *this;
		}

		bool operator==(const self_t& other) const
		{
			return m_impl == other.m_impl;
		}

		operator bool() const
		{
			return (bool)m_impl;
		}

		stdx::task<void> lock()
		{
			return m_impl->lock();
		}

		void unlock() noexcept
		{
			return m_impl->unlock();
		}

	private:
		impl_t m_impl;
	};

	class _NoticeFlag
	{
	public:
		_NoticeFlag(size_t count);

		_NoticeFlag(size_t count,stdx::thread_pool &pool);

		~_NoticeFlag() = default;

		void notice();

		void notice_on_this_thread();

		stdx::task<void> get_task();

		void reset();
	private:
		std::atomic_size_t m_count;
		stdx::task_completion_event<void> m_ce;
		size_t m_max_count;
	};

	class notice_flag
	{
		using impl_t = std::shared_ptr<stdx::_NoticeFlag>;
		using self_t = stdx::notice_flag;
	public:
		notice_flag(size_t count)
			:m_impl(std::make_shared<stdx::_NoticeFlag>(count))
		{}

		notice_flag(size_t count,stdx::thread_pool &pool)
			:m_impl(std::make_shared<stdx::_NoticeFlag>(count,pool))
		{}

		notice_flag(const self_t &other)
			:m_impl(other.m_impl)
		{}

		notice_flag(self_t &&other) noexcept
			:m_impl(std::move(other.m_impl))
		{}

		~notice_flag() = default;


		self_t& operator=(const self_t& other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		self_t& operator=(self_t&& other) noexcept
		{
			m_impl = std::move(other.m_impl);
			return *this;
		}

		bool operator==(const self_t& other) const
		{
			return m_impl == other.m_impl;
		}

		operator bool() const
		{
			return (bool)m_impl;
		}

		void notice()
		{
			return m_impl->notice();
		}

		void notice_on_this_thread()
		{
			return m_impl->notice_on_this_thread();
		}

		stdx::task<void> get_task()
		{
			return m_impl->get_task();
		}

		void reset()
		{
			return m_impl->reset();
		}

	private:
		impl_t m_impl;
	};
}