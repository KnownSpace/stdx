#include <stdx/file.h>
#ifdef WIN32
#define _ThrowWinError auto _ERROR_CODE = GetLastError(); \
						if(_ERROR_CODE != ERROR_IO_PENDING) \
						{ \
							throw std::system_error(std::error_code(_ERROR_CODE,std::system_category())); \
						}
#else
#include <sys/sendfile.h>
#define _ThrowLinuxError auto _ERROR_CODE = errno;\
						 throw std::system_error(std::error_code(_ERROR_CODE,std::system_category())); 
#endif

#ifdef _STDX_HAS_FILE
stdx::file_enum_value_t stdx::forward_file_access_type(const stdx::file_access_type & access_type)
{
	return static_cast<stdx::file_enum_value_t>(access_type);
}

#ifdef WIN32
DWORD stdx::forward_file_shared_model(const stdx::file_shared_model & shared_model)
{
	return static_cast<DWORD>(shared_model);
}
#endif

stdx::file_enum_value_t stdx::forward_file_open_type(const stdx::file_open_type & open_type)
{
	return static_cast<stdx::file_enum_value_t>(open_type);
}

std::once_flag stdx::_FileIOService::_once_flag;

std::shared_ptr<stdx::_FileIOService> stdx::_FileIOService::_instance(nullptr);

std::shared_ptr<stdx::_FileIOService> stdx::_FileIOService::get_instance()
{
	std::call_once(stdx::_FileIOService::_once_flag, []() 
	{
		stdx::_FileIOService::_instance = std::make_shared<stdx::_FileIOService>();
	});
	return stdx::_FileIOService::_instance;
}

stdx::_FileIOService::_FileIOService()
#ifdef WIN32
	:m_poller(stdx::make_iocp_poller<stdx::file_io_context>())
#else
#ifdef STDX_USE_NATIVE_AIO
	:m_poller(STDX_NATIVE_AIO_EVENTS)
#else
	:m_poller(stdx::make_bio_poller<stdx::file_io_context>())
#endif
#endif
	,m_token()
	,m_thread_pool(stdx::make_fixed_size_thread_pool(cpu_cores()))
{
	init_threadpoll();
}


stdx::_FileIOService::~_FileIOService()
{
	m_token.cancel();
#ifdef WIN32
	for (size_t i = 0, size = cpu_cores(); i < size; i++)
	{
		m_poller.post(nullptr);
	}
#endif
}

#ifdef WIN32
stdx::native_file_handle stdx::_FileIOService::create_file(const stdx::string & path, stdx::file_enum_value_t access_type, stdx::file_enum_value_t file_open_type, stdx::file_enum_value_t shared_model)
{
	HANDLE file = CreateFileW(path.c_str(), access_type, shared_model, 0, file_open_type, FILE_FLAG_OVERLAPPED, 0);
	if (file == INVALID_HANDLE_VALUE)
	{
		_ThrowWinError
	}
	m_poller.bind(file);
	return file;
}
#else
stdx::native_file_handle stdx::_FileIOService::create_file(const stdx::string & path, stdx::file_enum_value_t access_type, stdx::file_enum_value_t file_open_type, stdx::file_enum_value_t model)
{
#ifdef STDX_USE_NATIVE_AIO
	int fd = ::open(path.c_str(), access_type | file_open_type | O_DIRECT, model);
#else
	int fd = ::open(path.c_str(), access_type | file_open_type, model);
#endif
	if (fd == -1)
	{
		_ThrowLinuxError
	}
	return fd;
}

stdx::native_file_handle stdx::_FileIOService::create_file(const stdx::string & path, stdx::file_enum_value_t access_type, stdx::file_enum_value_t open_type)
{
#ifdef STDX_USE_NATIVE_AIO
	int fd = ::open(path.c_str(), access_type | open_type | O_DIRECT);
#else
	int fd = ::open(path.c_str(), access_type | open_type);
#endif
	if (fd == -1)
	{
		_ThrowLinuxError
	}
	return fd;
}
#endif



void stdx::_FileIOService::read_file(stdx::native_file_handle file, stdx::file_size_t size, const uint64_t & offset, std::function<void(file_read_event, std::exception_ptr)> callback)
{
#ifdef WIN32
	file_io_context* context = new file_io_context;
	if (context == nullptr)
	{
		callback(stdx::file_read_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	uint64_union li;
	li.value = offset;
	context->m_ol.Offset = li.low;
	context->m_ol.OffsetHigh = li.height;
	context->eof = false;
	context->file = file;
	context->offset = offset;
	context->buffer = (char*)stdx::calloc(size, sizeof(char));
	if (context->buffer == nullptr)
	{
		delete context;
		callback(stdx::file_read_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	memset(context->buffer, 0, size);
	context->size = size;
	std::function<void(file_io_context*, std::exception_ptr)>* call = new std::function<void(file_io_context*, std::exception_ptr)>;
	if (call == nullptr)
	{
		stdx::free(context->buffer);
		delete context;
		callback(stdx::file_read_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	*call = [callback, size](file_io_context* context_ptr, std::exception_ptr error)
	{
		if (error)
		{
			stdx::free(context_ptr->buffer);
			callback(file_read_event(), error);
			delete context_ptr;
			return;
		}
		if (context_ptr->size < size)
		{
			context_ptr->eof = true;
		}
		file_read_event context(context_ptr);
		stdx::finally fin([context_ptr]()
			{
				delete context_ptr;
			});
		callback(context, nullptr);
	};
	context->callback = call;
	if (!ReadFile(file, context->buffer, size, &(context->size), &(context->m_ol)))
	{
		try
		{
			//处理错误
			DWORD code = GetLastError();
			if (code != ERROR_IO_PENDING)
			{
				if (code == ERROR_HANDLE_EOF)
				{
					context->eof = true;
				}
				else
				{
					LPVOID msg;
					if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&msg, 0, NULL))\
					{
						throw std::runtime_error((char*)msg);
					}
					else
					{
						std::string _ERROR_MSG("windows system error:");
						_ERROR_MSG.append(std::to_string(code));
						throw std::runtime_error(_ERROR_MSG.c_str());
					}
				}
			}
		}
		catch (const std::exception&)
		{
			free(context->buffer);
			delete call;
			delete context;
			callback(stdx::file_read_event(), std::current_exception());
			return;
		}
	}
#else
#ifdef STDX_USE_NATIVE_AIO
	//Native AIO
	auto  r_size = size;
	auto tmp = size % 512;
	if (tmp != 0)
	{
		r_size += (512 - tmp);
	}
	char* buffer = (char*)stdx::calloc(r_size, sizeof(char));
	if (buffer == nullptr)
	{
		callback(stdx::file_read_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	stdx::posix_memalign((void**)&buffer, 512, r_size);
	memset(buffer, 0, r_size);
	auto context = m_poller.get_context();
	file_io_context* ptr = new file_io_context;
	if (ptr == nullptr)
	{
		stdx::free(buffer);
		callback(stdx::file_read_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	ptr->size = r_size;
	ptr->buffer = buffer;
	ptr->offset = offset;
	ptr->file = file;
	//设置回调
	std::function<void(file_io_context*, std::exception_ptr)>* call = new std::function<void(file_io_context*, std::exception_ptr)>;
	if (call == nullptr)
	{
		stdx::free(buffer);
		delete ptr;
		callback(stdx::file_read_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	*call = [callback, size](file_io_context* context_ptr, std::exception_ptr error)
	{
		if (error)
		{
			stdx::free(context_ptr->buffer);
			callback(file_read_event(), error);
			delete context_ptr;
			return;
		}
		if (context_ptr->size < size)
		{
			context_ptr->eof = true;
		}
		file_read_event context(context_ptr);
		stdx::finally fin([context_ptr]()
			{
				delete context_ptr;
			});
		callback(context, nullptr);
	};
	ptr->callback = call;
	//投递操作
	try
	{
		stdx::aio_read(context, file, buffer, r_size, offset, INVALID_EVENTFD, ptr);
	}
	catch (const std::exception&)
	{
		stdx::free(buffer);
		delete call;
		delete ptr;
		callback(file_read_event(), std::current_exception());
	}
#else
	//Buffer IO
	char* buf =  (char*)stdx::calloc(size, sizeof(char));
	if (buf == nullptr)
	{
		callback(stdx::file_read_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	file_io_context* ptr = new file_io_context;
	if (ptr == nullptr)
	{
		stdx::free(buf);
		callback(stdx::file_read_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	ptr->size = size;
	ptr->buffer = buf;
	ptr->offset = offset;
	ptr->file = file;
	std::function<void(file_io_context*, std::exception_ptr)>* call = new std::function<void(file_io_context*, std::exception_ptr)>;
	if (call == nullptr)
	{
		stdx::free(buf);
		delete ptr;
		callback(stdx::file_read_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	*call = [callback, size](file_io_context* context_ptr, std::exception_ptr error)
	{
		if (error)
		{
			stdx::free(context_ptr->buffer);
			callback(file_read_event(), error);
			delete context_ptr;
			return;
		}
		if (context_ptr->size < size)
		{
			context_ptr->eof = true;
		}
		file_read_event context(context_ptr);
		stdx::finally fin([context_ptr]()
			{
				delete context_ptr;
			});
		callback(context, nullptr);
	};
	ptr->callback = call;
	ptr->op_code = stdx::file_bio_op_code::read;
	try
	{
		m_poller.post(ptr);
	}
	catch (const std::exception&)
	{
		stdx::free(buf);
		delete call;
		delete ptr;
		callback(file_read_event(), std::current_exception());
	}
#endif
#endif
	return;
}

void stdx::_FileIOService::write_file(stdx::native_file_handle file, const char* buffer, const stdx::file_size_t & size, const uint64_t & offset, std::function<void(file_write_event, std::exception_ptr)> callback)
{
#ifdef WIN32
	file_io_context* context_ptr = new file_io_context;
	if (context_ptr == nullptr)
	{
		callback(stdx::file_write_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	uint64_union li;
	li.value = offset;
	context_ptr->m_ol.Offset = li.low;
	context_ptr->m_ol.OffsetHigh = li.height;
	context_ptr->size = 0;
	context_ptr->offset = 0;
	char* buf = (char*)stdx::calloc(size, sizeof(char));
	if (buf == nullptr)
	{
		delete context_ptr;
		callback(stdx::file_write_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	memcpy(buf, buffer, size);
	auto* call = new std::function<void(file_io_context*, std::exception_ptr)>;
	if (call == nullptr)
	{
		delete context_ptr;
		stdx::free(buf);
		callback(stdx::file_write_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	*call = [callback, buf](file_io_context* context_ptr, std::exception_ptr error)
	{
		stdx::free(buf);
		if (error)
		{
			delete context_ptr;
			callback(file_write_event(), error);
			return;
		}
		file_write_event context(context_ptr);
		delete context_ptr;
		callback(context, nullptr);
	};
	context_ptr->callback = call;
	if (!WriteFile(file, buf, size, &(context_ptr->size), &(context_ptr->m_ol)))
	{
		try
		{
			_ThrowWinError
		}
		catch (const std::exception&)
		{
			stdx::free(context_ptr->buffer);
			delete call;
			stdx::finally fin([context_ptr]()
				{
					delete context_ptr;
				});
			callback(stdx::file_write_event(), std::current_exception());
			return;
		}
	}
#else
#ifdef STDX_USE_NATIVE_AIO
	//Native AIO
	auto  r_size = size;
	auto tmp = size % 512;
	if (tmp != 0)
	{
		r_size += (512 - tmp);
	}
	char* buf = (char*)stdx::calloc(r_size, sizeof(char));
	if (buf == nullptr)
	{
		callback(stdx::file_write_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	stdx::posix_memalign((void**)&buf, 512, r_size);
	memset(buf, 0, r_size);
	memcpy(buf, buffer, size);
	auto context = m_poller.get_context();
	file_io_context* ptr = new file_io_context;
	if (ptr == nullptr)
	{
		stdx::free(buf);
		callback(stdx::file_write_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	ptr->size = r_size;
	ptr->buffer = buf;
	ptr->offset = offset;
	ptr->file = file;
	//设置回调
	std::function<void(file_io_context*, std::exception_ptr)>* call = new std::function<void(file_io_context*, std::exception_ptr)>;
	if (call == nullptr)
	{
		stdx::free(buf);
		delete ptr;
		callback(stdx::file_write_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	*call = [callback, size](file_io_context* context_ptr, std::exception_ptr error)
	{
		if (context_ptr->buffer != nullptr)
		{
			stdx::free(context_ptr->buffer);
		}
		if (error)
		{
			callback(file_write_event(), error);
			delete context_ptr;
			return;
		}
		if (context_ptr->size < size)
		{
			context_ptr->eof = true;
		}
		file_write_event context(context_ptr);
		stdx::finally fin([context_ptr]()
			{
				delete context_ptr;
			});
		callback(context, nullptr);
	};
	ptr->callback = call;
	//投递操作
	try
	{
		stdx::aio_write(context, file, buf, r_size, offset, INVALID_EVENTFD, ptr);
	}
	catch (const std::exception&)
	{
		stdx::free(buf);
		delete call;
		delete ptr;
		callback(file_write_event(), std::current_exception());
	}
#else
	//Buffered IO
	char* buf = (char*)stdx::calloc(size,sizeof(char));
	if (buf == nullptr)
	{
		callback(stdx::file_write_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	memcpy(buf, buffer, size);
	file_io_context* ptr = new file_io_context;
	if (ptr == nullptr)
	{
		stdx::free(buf);
		callback(stdx::file_write_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	ptr->size = size;
	ptr->buffer = buf;
	ptr->offset = offset;
	ptr->file = file;
	std::function<void(file_io_context*, std::exception_ptr)>* call = new std::function<void(file_io_context*, std::exception_ptr)>;
	if (call == nullptr)
	{
		stdx::free(buf);
		delete ptr;
		callback(stdx::file_write_event(), std::make_exception_ptr(std::bad_alloc()));
		return;
	}
	*call = [callback, size](file_io_context* context_ptr, std::exception_ptr error)
	{
		if (context_ptr->buffer != nullptr)
		{
			stdx::free(context_ptr->buffer);
		}
		if (error)
		{
			callback(file_write_event(), error);
			delete context_ptr;
			return;
		}
		if (context_ptr->size < size)
		{
			context_ptr->eof = true;
		}
		file_write_event context(context_ptr);
		stdx::finally fin([context_ptr]()
			{
				delete context_ptr;
			});
		callback(context, nullptr);
	};
	ptr->callback = call;
	ptr->op_code = stdx::file_bio_op_code::write;
	try
	{
		m_poller.post(ptr);
	}
	catch (const std::exception&)
	{
		stdx::free(buf);
		delete call;
		delete ptr;
		callback(file_write_event(), std::current_exception());
	}
#endif
#endif
	return;
}

uint64_t stdx::_FileIOService::get_file_size(stdx::native_file_handle file) const
{
#ifdef WIN32
	LARGE_INTEGER li;
	::GetFileSizeEx(file, &li);
	return li.QuadPart;
#else
	struct stat state;
	if (fstat(file, &state) == -1)
	{
		_ThrowLinuxError
	}
	return state.st_size;
#endif
}

void stdx::_FileIOService::init_threadpoll() noexcept
{
#ifdef WIN32
	for (uint32_t i = 0, cores = cpu_cores(); i < cores; i++)
	{
		m_thread_pool.long_loop(m_token,[](poller_t poller)
			{
				stdx::file_io_context* context_ptr = nullptr;
				try
				{
					context_ptr = poller.get();
				}
				catch (const std::exception&)
				{

				}
				if (context_ptr == nullptr)
				{
					return;
				}
				std::exception_ptr error(nullptr);
				try
				{
					if (!GetOverlappedResult(context_ptr->file, &(context_ptr->m_ol), &(context_ptr->size), false))
					{
						DWORD code = GetLastError();
						if (code != ERROR_IO_PENDING)
						{
							if (code == ERROR_HANDLE_EOF)
							{
								context_ptr->eof = true;
							}
							else
							{
								LPVOID msg;
								if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&msg, 0, NULL))\
								{
									throw std::runtime_error((char*)msg);
								}
								else
								{
									std::string _ERROR_MSG("windows system error:");
									_ERROR_MSG.append(std::to_string(code));
									throw std::runtime_error(_ERROR_MSG.c_str());
								}
							}
						}
					}
				}
				catch (const std::exception&)
				{
					error = std::current_exception();
				}
#ifdef DEBUG
				::printf("[IOCP]IO操作完成\n");
#endif
				auto* call = context_ptr->callback;
				stdx::threadpool.run([call,context_ptr,error]() 
				{
						stdx::finally fin([call]()
							{
								delete call;
							});
						try
						{
							(*call)(context_ptr, error);
						}
						catch (const std::exception&)
						{
						}
				});
			}, m_poller);
	}
#else
#ifdef STDX_USE_NATIVE_AIO
	//Native AIO
	for (uint32_t i = 0, cores = cpu_cores(); i < cores; i++)
	{
		m_thread_pool.long_loop(m_token, [](poller_t poller)
			{
				std::exception_ptr error(nullptr);
				int64_t res = 0;
				auto* context_ptr = poller.get(res);
				if (context_ptr == nullptr)
				{
					return;
				}
				auto* call = context_ptr->callback;
				stdx::threadpool.run([res,context_ptr,call,error]() mutable 
				{
						try
						{
							if (res < 0)
							{
								throw std::system_error(std::error_code(-res, std::system_category()), strerror(-res));
							}
							else
							{
								context_ptr->size = res;
							}
							(*call)(context_ptr, error);
							delete call;
						}
						catch (const std::exception&)
						{
							error = std::current_exception();
							(*call)(nullptr, error);
							delete call;
							try
							{
								delete context_ptr;
							}
							catch (const std::exception&)
							{

							}
						}
				});
			}, m_poller);
	}
#else
	//Buffered IO
	for (uint32_t i = 0, cores = cpu_cores(); i < cores; i++)
	{
		m_thread_pool.long_loop(m_token, [](poller_t poller)
		{
			auto* context = poller.get();
			if (context == nullptr)
			{
				return;
			}
			ssize_t r = 0;
			if (context->op_code == stdx::file_bio_op_code::write)
			{
				//pwrite
				r = ::pwrite(context->file, context->buffer, context->size, context->offset);
			}
			else if (context->op_code == stdx::file_bio_op_code::read)
			{
				//pread
				r = ::pread(context->file, context->buffer, context->size, context->offset);
				if (r == 0)
				{
					context->eof = true;
				}
			}
			auto* callback = context->callback;
			std::exception_ptr err(nullptr);
			try
			{
				if (r == -1)
				{
					_ThrowLinuxError
				}
			}
			catch (const std::exception&)
			{
				err = std::current_exception();
			}
			if (callback != nullptr)
			{
				stdx::threadpool.run([callback,err,context]() 
				{
						stdx::finally fin([callback]()
							{
								delete callback;
							});
						try
						{
							(*callback)(context, err);
						}
						catch (const std::exception&)
						{

						}
				});
			}
		}, m_poller);
	}
#endif
#endif
}

stdx::_FileStream::_FileStream(const io_service_t & io_service)
	:m_io_service(io_service)
#ifdef WIN32
	, m_file(INVALID_HANDLE_VALUE)
#else
	, m_file(-1)
#endif
{}

stdx::_FileStream::~_FileStream()
{
#ifdef WIN32
	if (m_file != INVALID_HANDLE_VALUE)
	{
		m_io_service.close_file(m_file);
		m_file = INVALID_HANDLE_VALUE;
	}
#else
	if (m_file != -1)
	{
		m_io_service.close_file(m_file);
		m_file = -1;
	}
#endif
}

stdx::task<stdx::file_read_event> stdx::_FileStream::read(const stdx::file_size_t & size, const uint64_t & offset)
{
	if (!m_io_service)
	{
		throw std::logic_error("this io service has been free");
	}
	stdx::task_completion_event<stdx::file_read_event> ce;
	m_io_service.read_file(m_file, size, offset, [ce](file_read_event context, std::exception_ptr error) mutable
		{
			if (error)
			{
				ce.set_exception(error);
			}
			else
			{
				ce.set_value(context);
			}
			ce.run_on_this_thread();
		});
	auto t = ce.get_task();
	return t;
}

stdx::task<stdx::file_write_event> stdx::_FileStream::write(const char* buffer, const stdx::file_size_t & size, const uint64_t & offset)
{
	if (!m_io_service)
	{
		throw std::logic_error("this io service has been free");
	}
	stdx::task_completion_event<stdx::file_write_event> ce;
	m_io_service.write_file(m_file, buffer, size, offset, [ce](file_write_event context, std::exception_ptr error) mutable
		{
			if (error)
			{
				ce.set_exception(error);
			}
			else
			{
				ce.set_value(context);
			}
			ce.run_on_this_thread();
		});
	auto t = ce.get_task();
	return t;
}

void stdx::_FileStream::close()
{
#ifdef WIN32
	native_file_handle file = m_file.exchange(INVALID_HANDLE_VALUE);
	if (file != INVALID_HANDLE_VALUE)
	{
		m_io_service.close_file(file);
	}
#else
	native_file_handle file = m_file.exchange(-1);
	if (file != -1)
	{
		m_io_service.close_file(file);
	}
#endif
}

void stdx::_FileStream::read_until(stdx::cancel_token token, file_size_t size, uint64_t offset, std::function<void(stdx::file_read_event)> fn, std::function<void(std::exception_ptr)> err_handler)
{
	if (token.is_cancel())
	{
		return;
	}
	m_io_service.read_file(m_file, size,offset, [err_handler,fn,token,offset,size,this](stdx::file_read_event ev,std::exception_ptr err) mutable
	{
		try
		{
			if (err)
			{
				err_handler(err);
			}
			else
			{
				if (!ev.eof)
				{
					offset += ev.buffer.size();
				}
				fn(ev);
			}
		}
		catch(const std::exception&)
		{
		}
		if (!token.is_cancel())
		{
			read_until(token, size, offset, fn, err_handler);
		}
	});
}

stdx::file_stream stdx::open_file_stream(const stdx::file_io_service & io_service, const stdx::string & path, const stdx::file_enum_value_t & access_type, const stdx::file_enum_value_t & open_type)
{
#ifdef WIN32
	stdx::file_stream file(io_service);
	DWORD shared = 0;
	if (access_type == FILE_GENERIC_READ)
	{
		shared = FILE_SHARE_READ;
	}
	if (access_type == FILE_GENERIC_WRITE)
	{
		shared = FILE_SHARE_WRITE;
	}
	if (access_type == GENERIC_ALL)
	{
		shared = FILE_SHARE_READ | FILE_SHARE_WRITE;
	}
	file.init(path, access_type, open_type, shared);
	return file;
#else
	stdx::file_stream file(io_service);
	file.init(path, access_type, open_type);
	return file;
#endif // WIN32
}

stdx::file_stream stdx::open_file_stream(const stdx::file_io_service & io_service, const stdx::string & path, file_access_type access_type, file_open_type open_type)
{
	return stdx::open_file_stream(io_service, path, forward_file_access_type(access_type), forward_file_open_type(open_type));
}

stdx::file_handle stdx::open_for_senfile(const stdx::string & path, const stdx::file_enum_value_t & access_type, const stdx::file_enum_value_t & open_type)
{
#ifdef WIN32
	DWORD shared = 0;
	if (access_type == FILE_GENERIC_READ)
	{
		shared = FILE_SHARE_READ;
	}
	if (access_type == FILE_GENERIC_WRITE)
	{
		shared = FILE_SHARE_WRITE;
	}
	if (access_type == GENERIC_ALL)
	{
		shared = FILE_SHARE_READ | FILE_SHARE_WRITE;
	}
	HANDLE file = CreateFileW(path.c_str(), access_type, shared, 0, open_type, FILE_FLAG_SEQUENTIAL_SCAN, 0);
	if (file == INVALID_HANDLE_VALUE)
	{
		_ThrowWinError
	}
	return stdx::file_handle(file);
#else
	return ::open(path.c_str(), access_type | open_type);
#endif
}

stdx::task_flag stdx::_FullpathNameFlag;
stdx::task<stdx::string> stdx::realpath(stdx::string path)
{
	return _FullpathNameFlag.lock()
		.then([path]()
			{
#ifdef WIN32
				wchar_t* buf = (wchar_t*)stdx::calloc(MAX_PATH, sizeof(wchar_t));
				if (buf == nullptr)
				{
					_FullpathNameFlag.unlock();
					throw std::bad_alloc();
				}
				if (!GetFullPathNameW(path.c_str(), MAX_PATH, buf, nullptr))
				{
					_FullpathNameFlag.unlock();
					stdx::free(buf);
					_ThrowWinError
				}
				_FullpathNameFlag.unlock();
				stdx::string str(buf);
				stdx::free(buf);
				return str;
#else
				char* buf = (char*)stdx::calloc(MAX_PATH, sizeof(char));
				if (!buf)
				{
					throw std::bad_alloc();
				}
				if (::realpath(path.c_str(), buf) == nullptr)
				{
					_FullpathNameFlag.unlock();
					stdx::free(buf);
					_ThrowLinuxError
				}
				_FullpathNameFlag.unlock();
				stdx::string str(buf);
				stdx::free(buf);
				return str;
#endif
			});
}

stdx::file::file(const stdx::file_io_service & io_service, const stdx::string & path)
	:m_path(std::make_shared<stdx::string>(path))
	, m_io_service(io_service)
{
#ifdef WIN32
	m_path->replace(U("/"), U("\\"));
#endif // WIN32
}

stdx::file::file(const file & other)
	:m_io_service(other.m_io_service)
	, m_path(other.m_path)
{
}

stdx::file::file(file&& other) noexcept
	:m_io_service(std::move(other.m_io_service))
	,m_path(std::move(other.m_path))
{
}

stdx::file& stdx::file::operator=(file&& other) noexcept
{
	m_path = std::move(other.m_path);
	m_io_service = std::move(other.m_io_service);
	return *this;
}

stdx::file& stdx::file::operator=(const file & other)
{
	stdx::file tmp(other);
	stdx::atomic_copy(*this, std::move(tmp));
	return *this;
}

bool stdx::file::operator==(const file & other) const
{
	return (*m_path) == (*other.m_path);
}

const stdx::string& stdx::file::path() const
{
	return *m_path;
}

uint64_t stdx::file::size() const
{
#ifdef WIN32
	HANDLE handle = open_native_handle(FILE_GENERIC_READ, OPEN_EXISTING);
	LARGE_INTEGER li;
	::GetFileSizeEx(handle, &li);
	return li.QuadPart;
#else
	struct stat buf;
	if (::stat(m_path->c_str(), &buf) != 0)
	{
		_ThrowLinuxError
	}
	return buf.st_size;
#endif // LINUX
}

void stdx::file::remove()
{
#ifdef WIN32
	if (::DeleteFileW(m_path->c_str()) == 0)
	{
		_ThrowWinError
	}
#else
	if (::remove(m_path->c_str()) != 0)
	{
		_ThrowLinuxError
	}
#endif
}

#ifdef WIN32

struct copy_struct
{
	copy_struct()
		:on_progress_change()
		, on_cancel()
		, cancel_ptr(nullptr)
	{}
	copy_struct(const copy_struct& other)
		:on_progress_change(other.on_progress_change)
		, on_cancel(other.on_cancel)
		, cancel_ptr(other.cancel_ptr)
	{}
	copy_struct(copy_struct&& other) noexcept
		:on_progress_change(other.on_progress_change)
		, on_cancel(other.on_cancel)
		, cancel_ptr(other.cancel_ptr)
	{}
	~copy_struct() = default;
	copy_struct& operator=(const copy_struct& other)
	{
		on_progress_change = other.on_progress_change;
		on_cancel = other.on_cancel;
		cancel_ptr = other.cancel_ptr;
		return *this;
	}
	std::function<void(uint64_t, uint64_t)> on_progress_change;
	std::function<void(uint64_t, uint64_t)> on_cancel;
	int* cancel_ptr;
};

DWORD CALLBACK copy_callback(
	LARGE_INTEGER TotalFileSize,			//文件总大小
	LARGE_INTEGER TotalBytesTransferred,	//已传输的大小
	LARGE_INTEGER StreamSize,				//流的大小
	LARGE_INTEGER StreamBytesTransferred,	//流已传输的大小
	DWORD dwStreamNumber,					//流编号
	DWORD dwCallbackReason,					//回调原因
	HANDLE hSourceFile,						//源文件句柄
	HANDLE hDestinationFile,				//目标文件句柄
	LPVOID lpData							//自定义数据
)
{
	uint64_t total_size = TotalFileSize.QuadPart;
	uint64_t bytes_transferred = TotalBytesTransferred.QuadPart;
	copy_struct* cpy_ptr = reinterpret_cast<copy_struct*>(lpData);
	if (!cpy_ptr)
	{
		//出错则取消
		return PROGRESS_CANCEL;
	}
	if (*(cpy_ptr->cancel_ptr))
	{
		//操作被取消
		cpy_ptr->on_cancel(total_size, bytes_transferred);
		delete cpy_ptr;
		return PROGRESS_CANCEL;
	}
	//复制进程改变
	cpy_ptr->on_progress_change(total_size, bytes_transferred);
	if (total_size == bytes_transferred)
	{
		//复制完成
		delete cpy_ptr;
	}
	return PROGRESS_CONTINUE;
}
#endif


void stdx::file::copy_to(const stdx::string & path, file_cancel_token_ptr cancel_ptr, std::function<void(uint64_t, uint64_t)> && on_progress_change, std::function<void(uint64_t, uint64_t)> && on_cancel/*, std::function<void(std::exception_ptr)> &&on_error*/)
{
#ifdef WIN32
	copy_struct* cpy_ptr = new copy_struct;
	cpy_ptr->on_progress_change = on_progress_change;
	cpy_ptr->on_cancel = on_cancel;
	cpy_ptr->cancel_ptr = cancel_ptr;
	if (!CopyFileExW(m_path->c_str(), path.c_str(), (LPPROGRESS_ROUTINE)copy_callback, cpy_ptr, (LPBOOL)cancel_ptr, COPY_FILE_FAIL_IF_EXISTS))
	{
		delete cpy_ptr;
		auto _ERROR_CODE = GetLastError();
		LPVOID _MSG;
		if ((_ERROR_CODE != ERROR_IO_PENDING))
		{
			if (_ERROR_CODE == ERROR_REQUEST_ABORTED)
			{
				//调用取消回调
				on_cancel(0, 0);
				return;
			}
			if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, _ERROR_CODE, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&_MSG, 0, NULL))
			{
				throw std::system_error(std::error_code(_ERROR_CODE, std::system_category()), (char*)_MSG);
			}
			else
			{
				std::string _ERROR_MSG("windows system error:");
				_ERROR_MSG.append(std::to_string(_ERROR_CODE));
				throw std::system_error(std::error_code(_ERROR_CODE, std::system_category()), _ERROR_MSG.c_str());
			}
		}
	}
#else
	int in_fd = -1, out_fd = -1;
	uint64_t size = this->size();
	loff_t in_off = 0;
	if (!(*cancel_ptr))
	{
		in_fd = open_native_handle(stdx::forward_file_access_type(stdx::file_access_type::read), stdx::forward_file_open_type(stdx::file_open_type::open));
		if (in_fd == -1)
		{
			_ThrowLinuxError
		}
		out_fd = ::open(path.c_str(), stdx::forward_file_access_type(stdx::file_access_type::write) | stdx::forward_file_open_type(stdx::file_open_type::create));
		if (out_fd == -1)
		{
			_ThrowLinuxError
		}
	}
	while (!(*cancel_ptr))
	{
		size_t len = 0;
		if ((size - in_off) < 4096)
		{
			len = (size - in_off);
		}
		else
		{
			len = 4096;
		}
		//ssize_t cp_size = splice(in_fd, &in_off, out_fd, &out_off,len, SPLICE_F_MOVE);
		ssize_t cp_size = sendfile(out_fd, in_fd, &in_off, len);
		if (cp_size == -1)
		{
			_ThrowLinuxError
		}
		in_off += cp_size;
		if (in_off >= size)
		{
			in_off = size;
			on_progress_change(in_off, size);
			return;
		}
		on_progress_change(in_off, size);
	}
	on_cancel(in_off, size);
#endif
}

bool stdx::file::exist() const
{
#ifdef WIN32
	return ::PathFileExistsW(m_path->c_str());
#else
	return ::access(m_path->c_str(), F_OK) == 0;
#endif
}

#ifdef WIN32
stdx::file_stream stdx::file::open_stream(const DWORD & access_type, const DWORD & open_type)
{
	return stdx::open_file_stream(m_io_service, *m_path, access_type, open_type);
}

HANDLE stdx::file::open_native_handle(const DWORD & access_type, const DWORD & open_type) const
{
	HANDLE file = CreateFileW(m_path->c_str(), access_type, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, open_type, FILE_ATTRIBUTE_NORMAL, 0);
	if (file == INVALID_HANDLE_VALUE)
	{
		_ThrowWinError
	}
	return file;
}
#else
stdx::file_stream stdx::file::open_stream(const stdx::file_io_service & io_service, const int32_t & access_type, const int32_t & open_type)
{
	return stdx::open_file_stream(io_service, *m_path, access_type, open_type);
}
int stdx::file::open_native_handle(const int32_t & access_type, const int32_t & open_type) const
{
	int fd = ::open(m_path->c_str(), access_type | open_type);
	if (fd == -1)
	{
		_ThrowLinuxError
	}
	return fd;
}
#endif

stdx::file_stream stdx::file::open_stream(const stdx::file_access_type & access_type, const stdx::file_open_type & open_type)
{
	return stdx::open_file_stream(m_io_service, *m_path, access_type, open_type);
}

#ifdef WIN32
HANDLE stdx::file::open_for_sendfile_native(const stdx::file_access_type & access_type, const stdx::file_open_type & open_type)
{
	DWORD shared = 0;
	DWORD access = stdx::forward_file_access_type(access_type);
	if (access == FILE_GENERIC_READ)
	{
		shared = FILE_SHARE_READ;
	}
	else if (access == FILE_GENERIC_WRITE)
	{
		shared = FILE_SHARE_WRITE;
	}
	else if (access == GENERIC_ALL)
	{
		shared = FILE_SHARE_READ | FILE_SHARE_WRITE;
	}
	HANDLE file = CreateFileW(m_path->c_str(), access, shared, 0, stdx::forward_file_open_type(open_type), FILE_FLAG_SEQUENTIAL_SCAN, 0);
	if (file == INVALID_HANDLE_VALUE)
	{
		_ThrowWinError
	}
	return file;
}
#else
int stdx::file::open_for_sendfile_native(const stdx::file_access_type & access_type, const stdx::file_open_type & open_type)
{
	return ::open(m_path->c_str(), stdx::forward_file_access_type(access_type) | stdx::forward_file_open_type(open_type));
}
#endif

stdx::file_handle stdx::file::open_for_sendfile(const stdx::file_access_type & access_type, const stdx::file_open_type & open_type)
{
	return stdx::open_for_senfile(*m_path, stdx::forward_file_access_type(access_type), stdx::forward_file_open_type(open_type));
}

#ifdef WIN32
void stdx::file::close_handle(HANDLE file)
{
	::CloseHandle(file);
}
#else
void stdx::file::close_handle(int file)
{
	::close(file);
}
#endif
#endif

#ifdef WIN32
#undef _ThrowWinError
#endif
#ifdef LINUX
#undef _ThrowLinuxError
#endif