#include <stdx/file.h>
#ifdef WIN32
#define _ThrowWinError auto _ERROR_CODE = GetLastError(); \
						LPVOID _MSG;\
						if(_ERROR_CODE != ERROR_IO_PENDING) \
						{ \
							if(FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,NULL,_ERROR_CODE,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),(LPTSTR) &_MSG,0,NULL))\
							{ \
								throw std::runtime_error((char*)_MSG);\
							}else \
							{ \
								std::string _ERROR_MSG("windows system error:");\
								_ERROR_MSG.append(std::to_string(_ERROR_CODE));\
								throw std::system_error(std::error_code(_ERROR_CODE,std::system_category()),_ERROR_MSG.c_str()); \
							} \
						}\

DWORD stdx::forward_file_access_type(const stdx::file_access_type & access_type)
{
	return static_cast<DWORD>(access_type);
}

DWORD stdx::forward_file_shared_model(const stdx::file_shared_model &shared_model)
{
	return static_cast<DWORD>(shared_model);
}

DWORD stdx::forward_file_open_type(const stdx::file_open_type &open_type)
{
	return static_cast<DWORD>(open_type);
}

stdx::_FileIOService::_FileIOService()
	:m_iocp()
	, m_alive(std::make_shared<bool>(true))
{
	init_threadpoll();
}

stdx::_FileIOService::~_FileIOService()
{
	*m_alive = false;
	for (size_t i = 0,size = cpu_cores()*2; i < size; i++)
	{
		m_iocp.post(0, nullptr, nullptr);
	}
}

HANDLE stdx::_FileIOService::create_file(const std::string &path, DWORD access_type, DWORD file_open_type, DWORD shared_model)
{
	HANDLE file = CreateFileA(path.c_str(), access_type, shared_model, 0, file_open_type, FILE_FLAG_OVERLAPPED, 0);
	if (file == INVALID_HANDLE_VALUE)
	{
		_ThrowWinError
	}
	m_iocp.bind(file);
	return file;
}

void stdx::_FileIOService::read_file(HANDLE file, const DWORD &size, const int_64 &offset, std::function<void(file_read_event, std::exception_ptr)> callback)
{
	file_io_context *context = new file_io_context;
	int64_union li;
	li.value = offset;
	context->m_ol.Offset = li.low;
	context->m_ol.OffsetHigh = li.height;
	context->eof = false;
	context->file = file;
	context->offset = offset;
	context->buffer = (char*)std::calloc(size, sizeof(char));
	if (context->buffer == nullptr)
	{
		delete context;
		try
		{
			throw std::bad_alloc();
		}
		catch (const std::exception&)
		{
			callback(stdx::file_read_event(), std::current_exception());
		}
	}
	::memset(context->buffer, 0, size);
	context->size = size;
	std::function<void(file_io_context*, std::exception_ptr)> *call = new std::function<void(file_io_context*, std::exception_ptr)>;
	*call = [callback, size](file_io_context *context_ptr, std::exception_ptr error)
	{
		if (error)
		{
			free(context_ptr->buffer);
			callback(file_read_event(), error);
			delete context_ptr;
			return;
		}
		if (context_ptr->size < size)
		{
			context_ptr->eof = true;
		}
		file_read_event context(context_ptr);
		delete context_ptr;
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
			delete call;
			delete context;
			callback(stdx::file_read_event(), std::current_exception());
			return;
		}
	}
	return;
}

void stdx::_FileIOService::write_file(HANDLE file, const char *buffer, const size_t &size, const int_64 &offset, std::function<void(file_write_event, std::exception_ptr)> callback)
{
	file_io_context *context_ptr = new file_io_context;
	int64_union li;
	li.value = offset;
	context_ptr->m_ol.Offset = li.low;
	context_ptr->m_ol.OffsetHigh = li.height;
	context_ptr->size = 0;
	context_ptr->offset = 0;
	char* buf = (char*)::calloc(size, sizeof(char));
	if (buf == nullptr)
	{
		delete context_ptr;
		try
		{
			throw std::bad_alloc();
		}
		catch (const std::exception&)
		{
			callback(stdx::file_write_event(), std::current_exception());
		}
	}
	::memcpy(buf, buffer, size);
	auto *call = new std::function<void(file_io_context*, std::exception_ptr)>;
	*call = [callback,buf](file_io_context *context_ptr, std::exception_ptr error)
	{
		::free(buf);
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
			delete call;
			delete context_ptr;
			callback(stdx::file_write_event(), std::current_exception());
			return;
		}
	}
}

int_64 stdx::_FileIOService::get_file_size(HANDLE file) const
{
	LARGE_INTEGER li;
	::GetFileSizeEx(file, &li);
	return li.QuadPart;
}

void stdx::_FileIOService::init_threadpoll() noexcept
{
	for (size_t i = 0, cores = cpu_cores() * 2; i < cores; i++)
	{
		stdx::threadpool::run([](iocp_t iocp, std::shared_ptr<bool> alive)
		{
			while (*alive)
			{
				auto *context_ptr = iocp.get();
				if (context_ptr == nullptr)
				{
					continue;
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
				auto *call = context_ptr->callback;
				try
				{
					(*call)(context_ptr, error);
				}
				catch (const std::exception&)
				{
				}
				delete call;
			}
		}, m_iocp, m_alive);
	}
}

stdx::_FileStream::_FileStream(const io_service_t &io_service)
	:m_io_service(io_service)
	, m_file(INVALID_HANDLE_VALUE)
{}

stdx::_FileStream::~_FileStream()
{
	if (m_file != INVALID_HANDLE_VALUE)
	{
		m_io_service.close_file(m_file);
		m_file = INVALID_HANDLE_VALUE;
	}
}

stdx::task<stdx::file_read_event> stdx::_FileStream::read(const size_t & size, const int_64 & offset)
{
	if (!m_io_service)
	{
		throw std::logic_error("this io service has been free");
	}
	stdx::task_complete_event<stdx::file_read_event> ce;
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
	return ce.get_task();
}

stdx::task<stdx::file_write_event> stdx::_FileStream::write(const char* buffer, const size_t &size, const int_64 &offset)
{
	if (!m_io_service)
	{
		throw std::logic_error("this io service has been free");
	}
	stdx::task_complete_event<stdx::file_write_event> ce;
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
	return ce.get_task();
}

void stdx::_FileStream::close()
{
	if (m_file != INVALID_HANDLE_VALUE)
	{
		m_io_service.close_file(m_file);
		m_file = INVALID_HANDLE_VALUE;
	}
}

stdx::file_stream stdx::open_file_stream(const stdx::file_io_service &io_service, const std::string &path, const DWORD &access_type, const DWORD &open_type)
{
	stdx::file_stream file(io_service);
	DWORD shared = 0;
	if (access_type  == FILE_GENERIC_READ)
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
	file.init(path, access_type, open_type,shared);
	return file;
}

stdx::file_stream stdx::open_file_stream(const stdx::file_io_service & io_service, const std::string & path, file_access_type access_type, file_open_type open_type)
{
	return stdx::open_file_stream(io_service,path,forward_file_access_type(access_type),forward_file_open_type(open_type));
}

stdx::file_handle stdx::open_for_senfile(const std::string &path, const int_32 & access_type, const int_32 & open_type)
{
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
	HANDLE file = CreateFile(path.c_str(), access_type, shared, 0, open_type, FILE_FLAG_SEQUENTIAL_SCAN, 0);
	if (file == INVALID_HANDLE_VALUE)
	{
		_ThrowWinError
	}
	return stdx::file_handle(file);
}
#endif // WIN32
#ifdef LINUX
#define _ThrowLinuxError auto _ERROR_CODE = errno;\
						 throw std::system_error(std::error_code(_ERROR_CODE,std::system_category()),strerror(_ERROR_CODE)); \

stdx::_FileIOService::_FileIOService()
	:m_aiocp(2048)
	,m_alive(std::make_shared<bool>(true))
{
	init_thread();
}

stdx::_FileIOService::_FileIOService(uint_32 nr_events)
	:m_aiocp(nr_events)
	, m_alive(std::make_shared<bool>(true))
{
	init_thread();
}

stdx::_FileIOService::~_FileIOService()
{
	*m_alive = false;
}

int_32 stdx::forward_file_access_type(const stdx::file_access_type & access_type)
{
	return static_cast<int_32>(access_type);
}

int_32 stdx::forward_file_open_type(const stdx::file_open_type &open_type)
{
	return static_cast<int_32>(open_type);
}

int stdx::_FileIOService::create_file(const std::string & path, int_32 access_type, int_32 open_type, mode_t mode)
{
	return ::open(path.c_str(), access_type | open_type|O_DIRECT, mode);
}

int stdx::_FileIOService::create_file(const std::string & path, int_32 access_type, int_32 open_type)
{
	return ::open(path.c_str(), access_type | open_type|O_DIRECT);
}

void stdx::_FileIOService::read_file(int file, const size_t & size, const int_64 & offset, std::function<void(file_read_event, std::exception_ptr)> callback)
{
	auto  r_size = size;
	auto tmp = size % 512;
	if (tmp!=0)
	{
		r_size += (512 - tmp);
	}
	char *buffer = (char*)calloc(r_size, sizeof(char));
	if (buffer == nullptr)
	{
		try
		{
			throw std::bad_alloc();
		}
		catch (const std::exception&)
		{
			callback(stdx::file_read_event(), std::current_exception());
		}
	}
	posix_memalign((void**)&buffer, 512, r_size);
	::memset(buffer, 0, r_size);
	auto context = m_aiocp.get_context();
	file_io_context *ptr = new file_io_context;
	ptr->size = r_size;
	ptr->buffer = buffer;
	ptr->offset = offset;
	ptr->file = file;
	//设置回调
	std::function<void(file_io_context*, std::exception_ptr)> *call = new std::function<void(file_io_context*, std::exception_ptr)>;
	*call = [callback, size](file_io_context *context_ptr, std::exception_ptr error)
	{
		if (error)
		{
			free(context_ptr->buffer);
			callback(file_read_event(), error);
			delete context_ptr;
			return;
		}
		if (context_ptr->size < size)
		{
			context_ptr->eof = true;
		}
		file_read_event context(context_ptr);
		delete context_ptr;
		callback(context, nullptr);
	};
	ptr->callback = call;
	//投递操作
	try
	{
		stdx::aio_read(context, file, buffer, r_size, offset, invalid_eventfd, ptr);
	}
	catch (const std::exception&)
	{
		delete call;
		delete ptr;
		callback(file_read_event(), std::current_exception());
	}
}

void stdx::_FileIOService::write_file(int file, const char * buffer, const size_t & size, const int_64 & offset, std::function<void(file_write_event, std::exception_ptr)> callback)
{
	auto  r_size = size;
	auto tmp = size % 512;
	if (tmp != 0)
	{
		r_size += (512 - tmp);
	}
	char *buf = (char*)calloc(r_size, sizeof(char));
	if (buf == nullptr)
	{
		try
		{
			throw std::bad_alloc();
		}
		catch (const std::exception&)
		{
			callback(stdx::file_write_event(), std::current_exception());
		}
	}
	posix_memalign((void**)&buf, 512, r_size);
	::memset(buf, 0, r_size);
	::memcpy(buf, buffer, size);
	auto context = m_aiocp.get_context();
	file_io_context *ptr = new file_io_context;
	ptr->size = r_size;
	ptr->buffer = buf;
	ptr->offset = offset;
	ptr->file = file;
	//设置回调
	std::function<void(file_io_context*, std::exception_ptr)> *call = new std::function<void(file_io_context*, std::exception_ptr)>;
	*call = [callback, size](file_io_context *context_ptr, std::exception_ptr error)
	{
		if (context_ptr->buffer != nullptr)
		{
			free(context_ptr->buffer);
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
		delete context_ptr;
		callback(context, nullptr);
	};
	ptr->callback = call;
	//投递操作
	try
	{
		stdx::aio_write(context,file,buf,r_size,offset,invalid_eventfd,ptr);
	}
	catch (const std::exception&)
	{
		delete call;
		delete ptr;
		callback(file_write_event(), std::current_exception());
	}

}

int_64 stdx::_FileIOService::get_file_size(int file) const
{
	struct stat state;
	if (fstat(file, &state) == -1)
	{
		_ThrowLinuxError
	}
	return state.st_size;
}

void stdx::_FileIOService::close_file(int file)
{
	close(file);
}

void stdx::_FileIOService::init_thread()
{
	for (size_t i = 0, cores = cpu_cores() * 2; i < cores; i++)
	{
		stdx::threadpool::run([](aiocp_t aiocp, std::shared_ptr<bool> alive)
		{
			while (*alive)
			{
				std::exception_ptr error(nullptr);
				int_64 res = 0;
				auto *context_ptr = aiocp.get(res);
				if (context_ptr == nullptr)
				{
					continue;
				}
				auto *call = context_ptr->callback;
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
			}
		}, m_aiocp, m_alive);
	}
}

stdx::_FileStream::_FileStream(const io_service_t &io_service)
	:m_io_service(io_service)
	, m_file(-1)
{}

stdx::_FileStream::~_FileStream()
{
	if (m_file != -1)
	{
		m_io_service.close_file(m_file);
		m_file = -1;
	}
}

stdx::task<stdx::file_read_event> stdx::_FileStream::read(const size_t & size, const int_64 & offset)
{
	if (!m_io_service)
	{
		throw std::logic_error("this io service has been free");
	}
	stdx::task_complete_event<stdx::file_read_event> ce;
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
	return ce.get_task();
}


stdx::task<stdx::file_write_event> stdx::_FileStream::write(const char* buffer, const size_t &size, const int_64 &offset)
{
	if (!m_io_service)
	{
		throw std::logic_error("this io service has been free");
	}
	stdx::task_complete_event<stdx::file_write_event> ce;
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
	return ce.get_task();
}

void stdx::_FileStream::close()
{
	if (m_file != -1)
	{
		m_io_service.close_file(m_file);
		m_file = -1;
	}
}

stdx::file_stream stdx::open_file_stream(const stdx::file_io_service &io_service, const std::string &path, const int_32 &access_type, const int_32 &open_type)
{
	stdx::file_stream file(io_service);
	file.init(path, access_type, open_type);
	return file;
}

stdx::file_stream stdx::open_file_stream(const stdx::file_io_service & io_service, const std::string & path, file_access_type access_type, file_open_type open_type)
{
	return stdx::open_file_stream(io_service, path, forward_file_access_type(access_type), forward_file_open_type(open_type));
}

stdx::file_handle stdx::open_for_senfile(const std::string &path, const int_32 &access_type, const int_32 &open_type)
{
	return ::open(path.c_str(),access_type|open_type);
}
#endif // LINUX

stdx::file::file(const stdx::file_io_service &io_service,const std::string & path)
	:m_path(std::make_shared<std::string>(path))
	,m_io_service(io_service)
{
}

stdx::file::file(const file & other)
	:m_io_service(other.m_io_service)
	,m_path(other.m_path)
{
}

stdx::file & stdx::file::operator=(const file & other)
{
	m_path = other.m_path;
	m_io_service = other.m_io_service;
	return *this;
}

bool stdx::file::operator==(const file & other) const
{
	return (*m_path) == (*other.m_path);
}

const std::string & stdx::file::path() const
{
	return *m_path;
}

int_64 stdx::file::size() const
{
#ifdef WIN32
	HANDLE handle = open_native_handle(FILE_GENERIC_READ, OPEN_EXISTING);
	LARGE_INTEGER li;
	::GetFileSizeEx(handle, &li);
	return li.QuadPart;
#endif

#ifdef LINUX
	struct stat buf;
	if (::stat(m_path->c_str(),&buf) != 0)
	{
		_ThrowLinuxError
	}
	return buf.st_size;
#endif // LINUX
}

void stdx::file::remove()
{
#ifdef WIN32
	if (::DeleteFileA(m_path->c_str()) == 0)
	{
		_ThrowWinError
	}
#endif
	
#ifdef LINUX
	if (::remove(m_path->c_str()) != 0)
	{
		_ThrowLinuxError
	}
#endif // LINUX
}

#ifdef WIN32
//DWORD CALLBACK copy_callback(
//	LARGE_INTEGER TotalFileSize,			//文件总大小
//	LARGE_INTEGER TotalBytesTransferred,	//已传输的大小
//	LARGE_INTEGER StreamSize,				//流的大小
//	LARGE_INTEGER StreamBytesTransferred,	//流已传输的大小
//	DWORD dwStreamNumber,					//流编号
//	DWORD dwCallbackReason,					//回调原因
//	HANDLE hSourceFile,						//源文件句柄
//	HANDLE hDestinationFile,				//目标文件句柄
//	LPVOID lpData							//自定义数据
//)
//{
//	std::function<void(const int&)> *callback = (std::function<void(const int&)>*)lpData;
//}
#endif // WIN32


//void stdx::file::copy_to(const std::string & path,typename stdx::file::cancel_token *cancel_ptr, std::function<void(const int&)>&& on_progress_change)
//{
//#ifdef WIN32
//	//CopyFileExA(m_path.c_str(), path.c_str(),(LPPROGRESS_ROUTINE)copy_callback, &on_progress_change,cancel_ptr, COPY_FILE_FAIL_IF_EXISTS);
//#endif
//
//#ifdef LINUX
//
//#endif // LINUX
//}

bool stdx::file::exist() const
{
#ifdef WIN32
	return ::PathFileExistsA(m_path->c_str());
#endif

#ifdef LINUX
	return ::access(m_path->c_str(), F_OK) == 0;
#endif // LINUX
}

#ifdef WIN32
stdx::file_stream stdx::file::open_stream(const DWORD & access_type, const DWORD & open_type)
{
	return stdx::open_file_stream(m_io_service,*m_path,access_type,open_type);
}

HANDLE stdx::file::open_native_handle(const DWORD & access_type, const DWORD & open_type) const
{
	HANDLE file = CreateFileA(m_path->c_str(), access_type, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, open_type, FILE_ATTRIBUTE_NORMAL, 0);
	if (file == INVALID_HANDLE_VALUE)
	{
		_ThrowWinError
	}
	return file;
}
#endif // WIN32
#ifdef LINUX
stdx::file_stream stdx::file::open_stream(const stdx::file_io_service & io_service, const int_32 & access_type, const int_32 & open_type)
{
	return stdx::open_file_stream(io_service, *m_path, access_type, open_type);
}
int stdx::file::open_native_handle(const int_32 & access_type, const int_32 & open_type) const
{
	return ::open(m_path->c_str(), access_type |open_type);
}
#endif // LINUX

stdx::file_stream stdx::file::open_stream(const stdx::file_access_type & access_type, const stdx::file_open_type & open_type)
{
	return stdx::open_file_stream(m_io_service,*m_path,access_type,open_type);
}

#ifdef WIN32
#undef _ThrowWinError
#endif // WIN32
#ifdef LINUX
#undef _ThrowLinuxError
#endif // LINUX
