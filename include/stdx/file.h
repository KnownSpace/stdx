#pragma once
#include <stdx/env.h>
#include <stdx/io.h>
#include <stdx/async/task.h>
#include <stdx/string.h>

#ifndef WIN32
#define MAX_PATH 4096
#endif

#ifdef WIN32
#include <Shlwapi.h>
#pragma comment(lib,"Shlwapi.lib")
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

namespace stdx
{
#pragma region TypeDef
	using native_file_handle = HANDLE;
	struct file_io_context
	{
		file_io_context()
		{
			memset(&m_ol, 0, sizeof(OVERLAPPED));
		}
		~file_io_context() = default;
		OVERLAPPED m_ol;
		HANDLE file;
		char *buffer;
		DWORD size;
		uint64_t offset;
		bool eof;
		std::function<void(file_io_context*, std::exception_ptr)> *callback;
	};
	//文件读取完成事件
	struct file_read_event
	{
		file_read_event()
			:file(INVALID_HANDLE_VALUE)
			, buffer(0, nullptr)
			, offset(0)
			, eof(false)
		{}
		~file_read_event() = default;
		file_read_event(const file_read_event &other)
			:file(other.file)
			, buffer(other.buffer)
			, offset(other.offset)
			, eof(other.eof)
		{}
		file_read_event(file_read_event &&other) noexcept
			:file(std::move(other.file))
			, buffer(std::move(other.buffer))
			, offset(std::move(other.offset))
			, eof(std::move(other.eof))
		{}
		file_read_event &operator=(const file_read_event &other)
		{
			file = other.file;
			buffer = other.buffer;
			offset = other.offset;
			eof = other.eof;
			return *this;
		}
		file_read_event(file_io_context *ptr)
			:file(ptr->file)
			, buffer(ptr->size, ptr->buffer)
			, offset(ptr->offset)
			, eof(ptr->eof)
		{
		}
		HANDLE file;
		stdx::buffer buffer;
		uint64_t offset;
		bool eof;
	};

	//文件写入完成事件
	struct file_write_event
	{
		file_write_event()
			:file(INVALID_HANDLE_VALUE)
			, size(0)
		{}
		~file_write_event() = default;
		file_write_event(const file_write_event &other)
			:file(other.file)
			, size(other.size)
		{}
		file_write_event(file_write_event &&other) noexcept
			:file(std::move(other.file))
			, size(std::move(other.size))
		{}
		file_write_event &operator=(const file_write_event &other)
		{
			file = other.file;
			size = other.size;
			return *this;
		}
		file_write_event(file_io_context *ptr)
			:file(ptr->file)
			, size(ptr->size)
		{}
		HANDLE file;
		size_t size;
	};
#pragma endregion

	
#pragma region EnumDef
	//文件访问类型
	enum class file_access_type : DWORD
	{
		execute = FILE_GENERIC_EXECUTE,	//以执行的形式访问
		read = FILE_GENERIC_READ,		//以只读的形式访问
		write = FILE_GENERIC_WRITE,		//以只写的形式访问
		all = GENERIC_ALL				//以读写的形式访问
	};

	//文件共享类型
	enum class file_shared_model : DWORD
	{
		unique = 0UL,							//独占
		shared_read = FILE_SHARE_READ,			//共享读
		shared_write = FILE_SHARE_WRITE,		//共享写
		shared_delete= FILE_SHARE_DELETE		//共享删除
	};

	//文件打开类型
	enum class file_open_type : DWORD
	{
		open = OPEN_EXISTING,					//打开已存在的文件
		create_always = CREATE_ALWAYS,			//总时创建新文件,若文件已存在会被截断
		create = CREATE_NEW,					//若文件不存在则创建,存在则出错
		open_always	= OPEN_ALWAYS				//若文件存在则打开,不存在则创建
	};

	extern DWORD forward_file_access_type(const file_access_type & access_type);

	extern DWORD forward_file_shared_model(const file_shared_model &shared_model);

	extern DWORD forward_file_open_type(const file_open_type &open_type);
#pragma endregion

#pragma region IO_SERVICE
	//文件IO服务实现
	class _FileIOService
	{
	public:
		using iocp_t = stdx::iocp<file_io_context>;
		_FileIOService();

		delete_copy(_FileIOService);

		~_FileIOService();

		HANDLE create_file(const stdx::string &path, DWORD access_type, DWORD file_open_type, DWORD shared_model);

		void read_file(HANDLE file, DWORD size, const uint64_t &offset, std::function<void(file_read_event, std::exception_ptr)> callback);

		void write_file(HANDLE file, const char *buffer, const DWORD &size, const uint64_t &offset, std::function<void(file_write_event, std::exception_ptr)> callback);


		uint64_t get_file_size(HANDLE file) const;

		void close_file(HANDLE file)
		{
			CloseHandle(file);
		}
	private:
		iocp_t m_iocp;
		std::shared_ptr<bool> m_alive;

		void init_threadpoll() noexcept;
	};

	//文件IO服务
	class file_io_service
	{
		using impl_t = std::shared_ptr<_FileIOService>;
		using iocp_t = typename _FileIOService::iocp_t;
	public:
		file_io_service()
			:m_impl(std::make_shared<_FileIOService>())
		{}
		//file_io_service(const iocp_t &iocp)
		//	:m_impl(std::make_shared<_FileIOService>(iocp))
		//{}
		file_io_service(const file_io_service &other)
			:m_impl(other.m_impl)
		{}
		file_io_service(file_io_service &&other) noexcept
			:m_impl(std::move(other.m_impl))
		{}
		file_io_service &operator=(const file_io_service &other)
		{
			m_impl = other.m_impl;
			return *this;
		}
		operator bool() const
		{
			return (bool)m_impl;
		}
		HANDLE create_file(const stdx::string &path, DWORD access_type, DWORD file_open_type, DWORD shared_model)
		{
			return m_impl->create_file(path, access_type, file_open_type, shared_model);
		}
		void read_file(HANDLE file,const DWORD &size, const uint64_t &offset, std::function<void(file_read_event, std::exception_ptr)> &&callback)
		{
			return m_impl->read_file(file, size, offset, callback);
		}
		void write_file(HANDLE file, const char *buffer, const DWORD &size, const uint64_t &offset, std::function<void(file_write_event, std::exception_ptr)> &&callback)
		{
			return m_impl->write_file(file, buffer, size, offset,callback);
		}
		void close_file(HANDLE file)
		{
			return m_impl->close_file(file);
		}
		uint64_t get_file_size(HANDLE file) const
		{
			return m_impl->get_file_size(file);
		}

		bool operator==(const file_io_service &other) const
		{
			return m_impl == other.m_impl;
		}

	private:
		impl_t m_impl;
	};
#pragma endregion

#pragma region FILE_STREAM

	//异步文件流实现
	class _FileStream
	{
		using io_service_t = file_io_service;
	public:
		_FileStream(const io_service_t &io_service);

		~_FileStream();

		void init(const stdx::string &path, const DWORD &access_type, const DWORD &open_type, const DWORD &shared_model)
		{
			m_file = m_io_service.create_file(path, access_type, open_type, shared_model);
		}

		stdx::task<file_read_event> read(const DWORD &size, const uint64_t &offset);


		//返回true则继续
		template<typename _Fn>
		void read_utill(const size_t &size,uint64_t offset, _Fn call)
		{
			using args_t = typename stdx::function_info<_Fn>::arguments;
			static_assert(std::is_same<std::remove_const<std::remove_reference<typename args_t::First>>, stdx::task_result<stdx::file_read_event>>::value, "the input function not be allowed");
			this->read(size, offset).then([call, offset, size, this](stdx::task_result<stdx::file_read_event> r) mutable
			{
				if (stdx::invoke(call, r))
				{
					auto e = r.get();
					read_utill(size, e.buffer.size() + offset, ex);
				}
			});
		}

		template<typename _Fn, typename _ErrHandler>
		void read_utill_eof(const size_t &size,uint64_t offset, _Fn call, _ErrHandler err_handler)
		{
			using args_t = typename stdx::function_info<_Fn>::arguments;
			static_assert(std::is_same<std::remove_const<std::remove_reference<typename args_t::First>>, stdx::file_read_event>::value, "the input function not be allowed");
			return read_utill(size, offset, [call](stdx::task_result<stdx::file_read_event> r) mutable
			{
				try
				{
					auto e = r.get();
					stdx::invoke(call, e);
					if (e.eof)
					{
						return false;
					}
					else
					{
						return true;
					}
				}
				catch (const std::exception&)
				{
					stdx::invoke(err_handler, std::current_exception());
					return false;
				}
			});
		}

		stdx::task<stdx::file_read_event> read_to_end(const uint64_t &offset)
		{
			uint64_union u;
			u.value = size() - offset;
			if (u.height != 0)
			{
				throw std::out_of_range("file is too big");
			}
			return read(u.low, offset);
		}


		stdx::task<file_write_event> write(const char* buffer, const DWORD&size, const uint64_t &offset);


		void close();

		uint64_t size() const
		{
			return m_io_service.get_file_size(m_file);
		}

		HANDLE get_handle() const
		{
			return m_file;
		}
	private:
		io_service_t m_io_service;
		HANDLE m_file;
	};

	class file_stream
	{
		using impl_t = std::shared_ptr<_FileStream>;
		using io_service_t = file_io_service;
	public:

		file_stream() = default;

		explicit file_stream(const io_service_t &io_service)
			:m_impl(std::make_shared<_FileStream>(io_service))
		{}

		file_stream(const file_stream &other)
			:m_impl(other.m_impl)
		{}

		file_stream(file_stream &&other) noexcept
			:m_impl(std::move(other.m_impl))
		{}

		~file_stream() = default;

		void init(const stdx::string &path, DWORD access_type, DWORD open_type, DWORD shared_model)
		{
			return m_impl->init(path, access_type, open_type, shared_model);
		}

		file_stream &operator=(const file_stream &other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		stdx::task<file_read_event> read(const DWORD&size, const uint64_t &offset)
		{
			return m_impl->read(size, offset);
		}

		stdx::task<file_write_event> write(const char* buffer, const DWORD&size, const uint64_t &offset)
		{
			return m_impl->write(buffer, size, offset);
		}

		stdx::task<file_write_event> write(const stdx::string &str, const uint64_t &offset)
		{
			uint64_union u64;
			u64.value = str.size()*sizeof(wchar_t);
			return write((char*)str.c_str(),u64.low, offset);
		}

		template<typename _Fn>
		void read_utill(const size_t &size, const uint64_t &offset, _Fn &&call)
		{
			return m_impl->read_utill(size, offset, call);
		}

		template<typename _Fn, typename _ErrHandler>
		void read_utill_eof(const size_t &size, const uint64_t &offset, _Fn &&call, _ErrHandler &&err_handler)
		{
			return m_impl->read_utill_eof(size, offset,call,err_handler);
		}

		stdx::task<stdx::file_read_event> read_to_end(const uint64_t &offset)
		{
			return m_impl->read_to_end(offset);
		}

		uint64_t size() const
		{
			return m_impl->size();
		}

		void close()
		{
			return m_impl->close();
		}

		HANDLE get_handle() const
		{
			return m_impl->get_handle();
		}

		bool operator==(const file_stream &other) const
		{
			return m_impl == other.m_impl;
		}
	private:
		impl_t m_impl;
	};
#pragma endregion

	class _FileHandle
	{
	public:
		_FileHandle(const HANDLE &file)
			:m_file(file)
		{}
		~_FileHandle()
		{
			::CloseHandle(m_file);
		}
		operator HANDLE() const
		{
			return m_file;
		}
		HANDLE get_file_handle() const
		{
			return m_file;
		}
		delete_copy(_FileHandle);
	private:
		HANDLE m_file;
	};

	class file_handle
	{
		using impl_t = std::shared_ptr<_FileHandle>;
	public:
		file_handle(const HANDLE &file)
			:m_impl(std::make_shared<_FileHandle>(file))
		{}
		file_handle(const file_handle &other)
			:m_impl(other.m_impl)
		{}
		~file_handle() = default;
		file_handle &operator=(const file_handle &other)
		{
			m_impl = other.m_impl;
			return *this;
		}
		operator HANDLE() const
		{
			return (HANDLE)(*m_impl);
		}
		HANDLE get_file_handle() const
		{
			return m_impl->get_file_handle();
		}
		bool operator==(const file_handle &other) const
		{
			return (m_impl == other.m_impl) || (m_impl->get_file_handle() == m_impl->get_file_handle());
		}
	private:
		impl_t m_impl;
	};
}

#pragma region APIs
namespace stdx
{
	extern stdx::file_stream open_file_stream(const stdx::file_io_service &io_service, const stdx::string &path, const DWORD &access_type, const DWORD &open_type);

	extern stdx::file_stream open_file_stream(const stdx::file_io_service &io_service, const stdx::string &path, file_access_type access_type, file_open_type open_type);

	extern stdx::file_handle open_for_senfile(const stdx::string &path, const DWORD &access_type, const DWORD &open_type);
}
#pragma endregion


#undef _ThrowWinError
#endif // WIN32
#ifdef LINUX
#include <fcntl.h>
#include <unistd.h>
#include <stdx/io.h>
#include <sys/stat.h>
#include <stdio.h>
#define _ThrowLinuxError auto _ERROR_CODE = errno;\
						 throw std::system_error(std::error_code(_ERROR_CODE,std::system_category()),strerror(_ERROR_CODE)); \

namespace stdx
{
#pragma region TypeDef
	using native_file_handle = int;

	struct file_io_context
	{
		int file;
		size_t size;
		char* buffer;
		int64_t offset;
		bool eof;
		std::function<void(file_io_context*, std::exception_ptr)> *callback;
	};
	//文件读取完成事件
	struct file_read_event
	{
		file_read_event()
			:file(-1)
			, buffer(0, nullptr)
			, offset(0)
			, eof(false)
		{}
		~file_read_event() = default;
		file_read_event(const file_read_event &other)
			:file(other.file)
			, buffer(other.buffer)
			, offset(other.offset)
			, eof(other.eof)
		{}
		file_read_event(file_read_event &&other)
			:file(std::move(other.file))
			, buffer(std::move(other.buffer))
			, offset(std::move(other.offset))
			, eof(std::move(other.eof))
		{}
		file_read_event &operator=(const file_read_event &other)
		{
			file = other.file;
			buffer = other.buffer;
			offset = other.offset;
			eof = other.eof;
			return *this;
		}
		file_read_event(file_io_context *ptr)
			:file(ptr->file)
			, buffer(ptr->size, ptr->buffer)
			, offset(ptr->offset)
			, eof(ptr->eof)
		{
		}
		int file;
		stdx::buffer buffer;
		int64_t offset;
		bool eof;
	};

	//文件写入完成事件
	struct file_write_event
	{
		file_write_event()
			:file(-1)
			, size(0)
		{}
		~file_write_event() = default;
		file_write_event(const file_write_event &other)
			:file(other.file)
			, size(other.size)
		{}
		file_write_event(file_write_event &&other)
			:file(std::move(other.file))
			, size(std::move(other.size))
		{}
		file_write_event &operator=(const file_write_event &other)
		{
			file = other.file;
			size = other.size;
			return *this;
		}
		file_write_event(file_io_context *ptr)
			:file(ptr->file)
			, size(ptr->size)
		{}
		int file;
		size_t size;
	};
#pragma endregion
	
#pragma region TypeDef
	//文件访问类型
	enum class file_access_type : uint32_t
	{
		read = O_RDONLY,	//以只读的形式访问
		write = O_WRONLY,	//以只写的形式访问
		all = O_RDWR,		//以读写的形式访问
	};
	//文件打开类型
	enum class file_open_type : uint32_t
	{
		open,							//打开已存在的文件
		create_always = O_TRUNC,		//总时创建新文件,若文件已存在会被截断
		create = O_CREAT | O_EXCL,		//若文件不存在则创建,存在则出错
		open_always= O_CREAT,			//若文件存在则打开,不存在则创建
	};

	extern int32_t forward_file_access_type(const file_access_type &access_type);

	extern int32_t forward_file_open_type(const file_open_type &open_type);
#pragma endregion

#pragma region IO_SERVICE
	class _FileIOService
	{
		using aiocp_t = stdx::aiocp<file_io_context>;
	public:

		_FileIOService();

		_FileIOService(uint32_t nr_events);

		delete_copy(_FileIOService);

		~_FileIOService();

		int create_file(const stdx::string &path, int32_t access_type, int32_t open_type, mode_t mode);

		int create_file(const stdx::string &path, int32_t access_type, int32_t open_type);

		void read_file(int file,size_t size, const uint64_t &offset, std::function<void(file_read_event, std::exception_ptr)> callback);

		void write_file(int file, const char *buffer,size_t size, const uint64_t &offset, std::function<void(file_write_event, std::exception_ptr)> callback);

		uint64_t get_file_size(int file) const;

		void close_file(int file);
	private:
		aiocp_t m_aiocp;
		std::shared_ptr<bool> m_alive;

		void init_thread();
	};

	//文件IO服务
	class file_io_service
	{
		using impl_t = std::shared_ptr<_FileIOService>;
	public:
		file_io_service()
			:m_impl(std::make_shared<_FileIOService>())
		{}
		file_io_service(uint32_t nr_events)
			:m_impl(std::make_shared<_FileIOService>(nr_events))
		{}
		file_io_service(const file_io_service &other)
			:m_impl(other.m_impl)
		{}
		file_io_service(file_io_service &&other)
			:m_impl(std::move(other.m_impl))
		{}
		file_io_service &operator=(const file_io_service &other)
		{
			m_impl = other.m_impl;
			return *this;
		}
		operator bool() const
		{
			return (bool)m_impl;
		}

		int create_file(const stdx::string &path, int32_t access_type, int32_t file_open_type, mode_t model)
		{
			return m_impl->create_file(path, access_type, file_open_type, model);
		}

		int create_file(const stdx::string &path, int32_t access_type, int32_t file_open_type)
		{
			return m_impl->create_file(path, access_type, file_open_type);
		}

		void read_file(int file, const size_t &size, const uint64_t &offset, std::function<void(file_read_event, std::exception_ptr)> &&callback)
		{
			return m_impl->read_file(file, size, offset,callback);
		}
		void write_file(int file, const char *buffer, const size_t &size, const uint64_t &offset, std::function<void(file_write_event, std::exception_ptr)> &&callback)
		{
			return m_impl->write_file(file, buffer, size, offset,callback);
		}
		void close_file(int file)
		{
			return m_impl->close_file(file);
		}
		uint64_t get_file_size(int file) const
		{
			return m_impl->get_file_size(file);
		}

		bool operator==(const file_io_service &other) const
		{
			return m_impl == other.m_impl;
		}

	private:
		impl_t m_impl;
	};
#pragma endregion

#pragma region FILE_STREAM

	//异步文件流实现
	class _FileStream
	{
		using io_service_t = file_io_service;
	public:
		_FileStream(const io_service_t &io_service);

		~_FileStream();

		void init(const stdx::string &path, const int32_t &access_type, const int32_t &open_type, const mode_t &model)
		{
			m_file = m_io_service.create_file(path, access_type, open_type, model);
		}

		void init(const stdx::string &path, const int32_t &access_type, const int32_t &open_type)
		{
			m_file = m_io_service.create_file(path, access_type, open_type);
		}

		stdx::task<file_read_event> read(const size_t &size, const uint64_t &offset);

		//返回true则继续
		template<typename _Fn>
		void read_utill(const size_t &size, const uint64_t &offset, _Fn call)
		{
			using args_t = typename stdx::function_info<_Fn>::arguments;
			static_assert(std::is_same<std::remove_const<std::remove_reference<typename args_t::First>>, stdx::task_result<stdx::file_read_event>>::value, "the input function not be allowed");
			this->read(size, offset).then([call, offset, size, this](stdx::task_result<stdx::file_read_event> r) mutable
			{
				if (stdx::invoke(call, r))
				{
					auto e = r.get();
					read_utill(size, e.buffer.size() + offset, call);
				}
			});
		}

		template<typename _Fn, typename _ErrHandler>
		void read_utill_eof(const size_t &size, const uint64_t &offset, _Fn call, _ErrHandler err_handler)
		{
			using args_t = typename stdx::function_info<_Fn>::arguments;
			static_assert(std::is_same<std::remove_const<std::remove_reference<typename args_t::First>>, stdx::file_read_event>::value, "the input function not be allowed");
			return read_utill(size, offset, [call, err_handler](stdx::task_result<stdx::file_read_event> r) mutable
			{
				try
				{
					auto e = r.get();
					stdx::invoke(call, e);
					if (e.eof)
					{
						return false;
					}
					else
					{
						return true;
					}
				}
				catch (const std::exception&)
				{
					stdx::invoke(err_handler, std::current_exception());
					return false;
				}
			});
		}

		stdx::task<stdx::file_read_event> read_to_end(const uint64_t &offset)
		{
			return read(size() - offset, offset);
		}


		stdx::task<stdx::file_write_event> write(const char* buffer, const size_t &size, const uint64_t &offset);

		void close();

		uint64_t size() const
		{
			return m_io_service.get_file_size(m_file);
		}

		int get_handle() const
		{
			return m_file;
		}
	private:
		io_service_t m_io_service;
		int m_file;
	};

	class file_stream
	{
		using impl_t = std::shared_ptr<stdx::_FileStream>;
		using io_service_t = stdx::file_io_service;
	public:

		explicit file_stream(const io_service_t &io_service)
			:m_impl(std::make_shared<_FileStream>(io_service))
		{}


		file_stream(const file_stream &other)
			:m_impl(other.m_impl)
		{}

		file_stream(file_stream &&other)
			:m_impl(std::move(other.m_impl))
		{}

		~file_stream() = default;

		void init(const stdx::string &path, int32_t access_type, int32_t open_type, const mode_t &model)
		{
			return m_impl->init(path, access_type, open_type, model);
		}

		void init(const stdx::string &path, int32_t access_type, int32_t open_type)
		{
			return m_impl->init(path, access_type, open_type);
		}

		file_stream &operator=(const file_stream &other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		stdx::task<file_read_event> read(const size_t &size, const uint64_t &offset)
		{
			return m_impl->read(size, offset);
		}

		stdx::task<file_write_event> write(const char* buffer, const size_t &size, const uint64_t &offset)
		{
			return m_impl->write(buffer, size, offset);
		}


		stdx::task<file_write_event> write(const stdx::string &str, const uint64_t &offset)
		{
			return write(str.c_str(), str.size(), offset);
		}

		template<typename _Fn>
		void read_utill(const size_t &size, const uint64_t &offset, _Fn &&call)
		{
			return m_impl->read_utill(size, offset,call);
		}

		template<typename _Fn, typename _ErrHandler>
		void read_utill_eof(const size_t &size, const uint64_t &offset, _Fn &&call, _ErrHandler &&err_handler)
		{
			return m_impl->read_utill_eof(size, offset,call,err_handler);
		}

		stdx::task<stdx::file_read_event> read_to_end(const uint64_t &offset)
		{
			return m_impl->read_to_end(offset);
		}

		uint64_t size() const
		{
			return m_impl->size();
		}

		void close()
		{
			return m_impl->close();
		}

		int get_handle() const
		{
			return m_impl->get_handle();
		}

		bool operator==(const file_stream &other) const
		{
			return m_impl == other.m_impl;
		}

	private:
		impl_t m_impl;
	};
#pragma endregion

	

	class _FileHandle
	{
	public:
		_FileHandle(const int &file)
			:m_file(file)
		{}
		~_FileHandle()
		{
			::close(m_file);
		}
		operator int() const
		{
			return m_file;
		}
		int get_file_handle() const
		{
			return m_file;
		}
		delete_copy(_FileHandle);
	private:
		int m_file;
	};

	class file_handle
	{
		using impl_t = std::shared_ptr<_FileHandle>;
	public:
		file_handle(const int &file)
			:m_impl(std::make_shared<_FileHandle>(file))
		{}
		file_handle(const file_handle &other)
			:m_impl(other.m_impl)
		{}
		~file_handle() = default;
		file_handle &operator=(const file_handle &other)
		{
			m_impl = other.m_impl;
			return *this;
		}
		operator int() const
		{
			return (int)(*m_impl);
		}
		int get_file_handle() const
		{
			return m_impl->get_file_handle();
		}
		bool operator==(const file_handle &other) const
		{
			return (m_impl == other.m_impl)||(m_impl->get_file_handle() == m_impl->get_file_handle());
		}
	private:
		impl_t m_impl;
	};
}

#pragma region APIs
namespace stdx
{
	extern stdx::file_stream open_file_stream(const stdx::file_io_service &io_service, const stdx::string &path, const int32_t &access_type, const int32_t &open_type);

	extern stdx::file_stream open_file_stream(const stdx::file_io_service &io_service, const stdx::string &path, file_access_type access_type, file_open_type open_type);

	extern stdx::file_handle open_for_senfile(const stdx::string &path, const int32_t &access_type, const int32_t &open_type);
}
#pragma endregion

#undef _ThrowLinuxError
#endif //LINUX




#if defined(WIN32) | defined(LINUX)

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

#endif
namespace stdx
{
	extern stdx::task_flag _FullpathNameFlag;

	extern stdx::task<stdx::string> realpath(stdx::string path);

	using cancel_token = int;
	using cancel_token_ptr = int*;
	struct cancel_token_value
	{
		enum 
		{
			none = 0,
			cancel = 1
		};
	};
	class file
	{
	public:
		

		file(const stdx::file_io_service &io_service,const stdx::string &path);

		file(const file &other);

		file &operator=(const file &other);

		bool operator==(const file &other) const;

		~file() = default;

		const stdx::string &path() const;

		uint64_t size() const;

		void remove();

		void copy_to(const stdx::string &path, cancel_token_ptr cancel_ptr, std::function<void(uint64_t,uint64_t)> &&on_progress_change, std::function<void(uint64_t, uint64_t)> &&on_cancel/*, std::function<void(std::exception_ptr)> &&on_error*/);

		bool exist() const;

#ifdef WIN32
		stdx::file_stream open_stream(const DWORD &access_type, const DWORD &open_type);


		HANDLE open_native_handle(const DWORD &access_type, const DWORD &open_type) const;
#else
		stdx::file_stream open_stream(const stdx::file_io_service &io_service, const int32_t &access_type, const int32_t &open_type);

		int open_native_handle(const int32_t &access_type, const int32_t &open_type) const;
#endif // WIN32

		stdx::file_stream open_stream(const stdx::file_access_type &access_type, const stdx::file_open_type &open_type);

	private:
		std::shared_ptr<stdx::string> m_path;
		stdx::file_io_service m_io_service;
	};
}
#endif

#ifdef WIN32
#undef _ThrowWinError
#endif // WIN32
