#pragma once
#include <stdx/env.h>
#include <stdx/io.h>
#include <stdx/async/task.h>
#include <stdx/string.h>
#include <atomic>

#ifndef WIN32
#define MAX_PATH 4096
#endif

#ifdef WIN32

#include <Shlwapi.h>
#pragma comment(lib,"Shlwapi.lib")
#define _ThrowWinError auto _ERROR_CODE = GetLastError(); \
						if(_ERROR_CODE != ERROR_IO_PENDING) \
						{ \
							throw std::system_error(std::error_code(_ERROR_CODE,std::system_category())); \
						}
#define _STDX_HAS_FILE
#else
#include <fcntl.h>
#include <unistd.h>
#include <stdx/io.h>
#include <sys/stat.h>
#include <stdio.h>
#define _ThrowLinuxError auto _ERROR_CODE = errno;\
						 throw std::system_error(std::error_code(_ERROR_CODE,std::system_category()),strerror(_ERROR_CODE)); 
#define _STDX_HAS_FILE
#ifdef STDX_USE_NATIVE_AIO
#ifndef STDX_NATIVE_AIO_EVENTS
#define STDX_NATIVE_AIO_EVENTS 2048
#endif
#endif
#endif

#ifdef _STDX_HAS_FILE
namespace stdx
{
#ifdef WIN32
	using native_file_handle = HANDLE;
#else
	using native_file_handle = int;
#endif

#ifdef LINUX
	struct file_bio_op_code
	{
		enum
		{
			write = 0,
			read = 1
		};
	};
#endif

	struct file_io_context:public stdx::stand_context
	{
		file_io_context()
			:stdx::stand_context()
		{
#ifdef WIN32
			memset(&m_ol, 0, sizeof(OVERLAPPED));
#else
			is_io_operation = true;
#endif
		}

		~file_io_context() = default;

		native_file_handle file;
		stdx::buffer buf;
#ifdef WIN32
		DWORD size;
#else
		size_t size;
#endif
		uint64_t offset;
		bool eof;
		std::function<void(file_io_context*, std::exception_ptr)> callback;
#ifdef LINUX
		int32_t op_code;
		int err_code;
#endif
};
	//文件读取完成事件
	struct file_read_event
	{
		file_read_event()
			: buffer(0, nullptr)
			, offset(0)
			, eof(false)
		{}

		~file_read_event() = default;

		file_read_event(const file_read_event& other)
			:buffer(other.buffer)
			, offset(other.offset)
			, eof(other.eof)
		{}

		file_read_event(file_read_event&& other) noexcept
			:buffer(std::move(other.buffer))
			, offset(std::move(other.offset))
			, eof(std::move(other.eof))
		{
			other.offset = 0;
		}

		file_read_event& operator=(const file_read_event& other)
		{
			buffer = other.buffer;
			offset = other.offset;
			eof = other.eof;
			return *this;
		}
		file_read_event& operator=(file_read_event&& other) noexcept
		{
			buffer = std::move(other.buffer);
			offset = other.offset;
			eof = other.eof;
			other.offset = 0;
			return *this;
		}
		file_read_event(file_io_context* ptr)
			: buffer(ptr->buf)
			, offset(ptr->offset)
			, eof(ptr->eof)
		{
		}
		stdx::buffer buffer;
		uint64_t offset;
		bool eof;
		};

	//文件写入完成事件
	struct file_write_event
	{
		file_write_event()
			:size(0)
			,buffer(0,nullptr)
		{}

		~file_write_event() = default;

		file_write_event(const file_write_event& other)
			:size(other.size)
			,buffer(other.buffer)
		{}
		file_write_event(file_write_event&& other) noexcept
			:size(std::move(other.size))
			,buffer(std::move(other.buffer))
		{}
		file_write_event& operator=(const file_write_event& other)
		{
			size = other.size;
			buffer = other.buffer;
			return *this;
		}

		file_write_event& operator=(file_write_event&& other) noexcept
		{
			size = other.size;
			other.size = 0;
			buffer = std::move(other.buffer);
			return *this;
		}

		file_write_event(file_io_context* ptr)
			:size(ptr->size)
			,buffer(ptr->buf)
		{}

		size_t size;
		stdx::buffer buffer;
	};


#ifdef WIN32
	using file_enum_value_t = DWORD;
#else
	using file_enum_value_t = int32_t;
#endif



	//文件访问类型
	enum class file_access_type : file_enum_value_t
	{
#ifdef WIN32
		execute = FILE_GENERIC_EXECUTE,	//以执行的形式访问
		read = FILE_GENERIC_READ,		//以只读的形式访问
		write = FILE_GENERIC_WRITE,		//以只写的形式访问
		all = GENERIC_ALL				//以读写的形式访问
#else
		read = O_RDONLY,	//以只读的形式访问
		write = O_WRONLY,	//以只写的形式访问
		all = O_RDWR,		//以读写的形式访问
#endif
	};

#ifdef WIN32
	//文件共享类型
	enum class file_shared_model : file_enum_value_t
	{
		unique = 0UL,							//独占
		shared_read = FILE_SHARE_READ,			//共享读
		shared_write = FILE_SHARE_WRITE,		//共享写
		shared_delete = FILE_SHARE_DELETE		//共享删除
	};
#endif

	//文件打开类型
	enum class file_open_type : file_enum_value_t
	{
#ifdef WIN32
		open = OPEN_EXISTING,					//打开已存在的文件
		create_always = CREATE_ALWAYS,			//总时创建新文件,若文件已存在会被截断
		create = CREATE_NEW,					//若文件不存在则创建,存在则出错
		open_always = OPEN_ALWAYS				//若文件存在则打开,不存在则创建
#else
		open,							//打开已存在的文件
		create_always = O_TRUNC,		//总时创建新文件,若文件已存在会被截断
		create = O_CREAT | O_EXCL,		//若文件不存在则创建,存在则出错
		open_always = O_CREAT,			//若文件存在则打开,不存在则创建
#endif
	};



	extern file_enum_value_t forward_file_access_type(const file_access_type& access_type);

#ifdef WIN32
	extern DWORD forward_file_shared_model(const file_shared_model& shared_model);
#endif
	extern file_enum_value_t forward_file_open_type(const file_open_type& open_type);

#ifdef WIN32
	using file_size_t = DWORD;
#else
	using file_size_t = size_t;
#endif

#pragma region IO_SERVICE
	//文件IO服务实现
	class _FileIOService
	{
	public:
#ifdef WIN32
		using poller_t = stdx::io_poller<file_io_context>;
#else
#ifdef STDX_USE_NATIVE_AIO
		using poller_t = stdx::aiocp<file_io_context>;
		using aiocp_t = stdx::aiocp<file_io_context>;
#else
		using poller_t = stdx::io_poller<stdx::file_io_context>;
#endif
#endif
		_FileIOService();

		DELETE_COPY(_FileIOService);

		~_FileIOService();

#ifdef WIN32
		native_file_handle create_file(const stdx::string& path, file_enum_value_t access_type, file_enum_value_t file_open_type, file_enum_value_t shared_model);
#else
		native_file_handle create_file(const stdx::string& path, file_enum_value_t access_type, file_enum_value_t file_open_type, file_enum_value_t model);
		native_file_handle create_file(const stdx::string& path, file_enum_value_t access_type, file_enum_value_t file_open_type);
#endif

		void read_file(native_file_handle file, stdx::buffer buf, const uint64_t& offset, std::function<void(file_read_event, std::exception_ptr)> callback);

		void write_file(native_file_handle file,stdx::buffer buf, const file_size_t& size, const uint64_t& offset, std::function<void(file_write_event, std::exception_ptr)> callback);

		uint64_t get_file_size(native_file_handle file) const;

		void close_file(native_file_handle file)
		{
#ifdef WIN32
			::CloseHandle(file);
#else
			::close(file);
#endif
		}
	private:
		void prepare_callback(stdx::file_io_context *context);
	};

	//文件IO服务
	class file_io_service
	{
		using impl_t = std::shared_ptr<_FileIOService>;
	public:
		file_io_service()
			:m_impl(std::make_shared<stdx::_FileIOService>())
		{}
		file_io_service(const impl_t& impl)
			:m_impl(impl)
		{}
		file_io_service(const file_io_service& other)
			:m_impl(other.m_impl)
		{}
		file_io_service(file_io_service&& other) noexcept
			:m_impl(std::move(other.m_impl))
		{}
		file_io_service& operator=(const file_io_service& other)
		{
			m_impl = other.m_impl;
			return *this;
		}
		operator bool() const
		{
			return (bool)m_impl;
		}
#ifdef WIN32
		native_file_handle create_file(const stdx::string& path, file_enum_value_t access_type, file_enum_value_t file_open_type, file_enum_value_t shared_model)
		{
			return m_impl->create_file(path, access_type, file_open_type, shared_model);
		}
#else
		native_file_handle create_file(const stdx::string& path, file_enum_value_t access_type, file_enum_value_t file_open_type, file_enum_value_t model)
		{
			return m_impl->create_file(path, access_type, file_open_type, model);
		}
		native_file_handle create_file(const stdx::string& path, file_enum_value_t access_type, file_enum_value_t file_open_type)
		{
			return m_impl->create_file(path, access_type, file_open_type);
		}
#endif

		void read_file(native_file_handle file,stdx::buffer buf, const uint64_t& offset, std::function<void(file_read_event, std::exception_ptr)>&& callback)
		{
			return m_impl->read_file(file, buf, offset, callback);
		}
		void write_file(native_file_handle file,stdx::buffer buf, const file_size_t& size, const uint64_t& offset, std::function<void(file_write_event, std::exception_ptr)>&& callback)
		{
			return m_impl->write_file(file, buf, size, offset, callback);
		}
		void close_file(native_file_handle file)
		{
			return m_impl->close_file(file);
		}
		uint64_t get_file_size(native_file_handle file) const
		{
			return m_impl->get_file_size(file);
		}

		bool operator==(const file_io_service& other) const
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
		_FileStream(const io_service_t& io_service);

		~_FileStream();

#ifdef WIN32
		void init(const stdx::string& path, const file_enum_value_t& access_type, const file_enum_value_t& open_type, const DWORD& shared_model)
		{
			m_file = m_io_service.create_file(path, access_type, open_type, shared_model);
		}
#else
		void init(const stdx::string& path, const file_enum_value_t& access_type, const file_enum_value_t& open_type, const file_enum_value_t& model)
		{
			m_file = m_io_service.create_file(path, access_type, open_type, model);
		}

		void init(const stdx::string& path, const file_enum_value_t& access_type, const file_enum_value_t& open_type)
		{
			m_file = m_io_service.create_file(path, access_type, open_type);
		}
#endif

		stdx::task<file_read_event> read(stdx::buffer buf, const uint64_t& offset);

		void read_until(stdx::cancel_token token,stdx::buffer buf,uint64_t offset,std::function<void(stdx::file_read_event)> fn,std::function<void(std::exception_ptr)> err_handler);

		stdx::task<stdx::file_read_event> read_to_end(const uint64_t& offset)
		{
			uint32_t u;
			u = stdx::implicit_cast<uint32_t>(size() - offset);
			stdx::buffer buf = stdx::make_buffer(u);
			if (!buf.check())
			{
				return stdx::error_task<stdx::file_read_event>(std::make_exception_ptr(std::bad_alloc()));
			}
			return this->read(buf, offset);
		}

		stdx::task<file_write_event> write(stdx::buffer buf, const file_size_t& size, const uint64_t& offset);

		void close();

		uint64_t size() const
		{
			return m_io_service.get_file_size(m_file);
		}

		native_file_handle get_handle() const
		{
			return m_file;
		}
	private:
		io_service_t m_io_service;
		std::atomic<native_file_handle> m_file;
	};

	class file_stream
	{
		using impl_t = std::shared_ptr<_FileStream>;
		using io_service_t = file_io_service;
	public:

		file_stream() = default;

		explicit file_stream(const io_service_t& io_service)
			:m_impl(std::make_shared<_FileStream>(io_service))
		{}

		file_stream(const file_stream& other)
			:m_impl(other.m_impl)
		{}

		file_stream(file_stream&& other) noexcept
			:m_impl(std::move(other.m_impl))
		{}

		~file_stream() = default;

#ifdef WIN32
		void init(const stdx::string& path, file_enum_value_t access_type, file_enum_value_t open_type, DWORD shared_model)
		{
			return m_impl->init(path, access_type, open_type, shared_model);
		}
#else
		void init(const stdx::string& path, const file_enum_value_t& access_type, const file_enum_value_t& open_type, const file_enum_value_t& model)
		{
			return m_impl->init(path, access_type, open_type, model);
		}

		void init(const stdx::string& path, const file_enum_value_t& access_type, const file_enum_value_t& open_type)
		{
			return m_impl->init(path, access_type, open_type);
		}
#endif

		file_stream& operator=(const file_stream& other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		stdx::task<file_read_event> read(stdx::buffer buf, const uint64_t& offset)
		{
			return m_impl->read(buf, offset);
		}

		stdx::task<file_write_event> write(stdx::buffer buf, const file_size_t& size, const uint64_t& offset)
		{
			return m_impl->write(buf, size, offset);
		}

		void read_until(stdx::cancel_token token, stdx::buffer buf, uint64_t offset, std::function<void(stdx::file_read_event)> fn, std::function<void(std::exception_ptr)> err_handler)
		{
			return m_impl->read_until(token, buf, offset, fn, err_handler);
		}

		stdx::task<stdx::file_read_event> read_to_end(const uint64_t& offset)
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

		native_file_handle get_handle() const
		{
			return m_impl->get_handle();
		}

		bool operator==(const file_stream& other) const
		{
			return m_impl == other.m_impl;
		}

		operator bool() const
		{
			return (bool)m_impl;
		}
	private:
		impl_t m_impl;
	};
#pragma endregion

	class _FileHandle
	{
	public:
		_FileHandle(const native_file_handle& file)
			:m_file(file)
		{}
		~_FileHandle()
		{
#ifdef WIN32
			if (m_file != INVALID_HANDLE_VALUE)
			{
				CloseHandle(m_file);
			}
#else
			if (m_file != -1)
			{
				::close(m_file);
			}
#endif
		}
		operator native_file_handle() const
		{
			return m_file;
	}
		native_file_handle get_file_handle() const
		{
			return m_file;
		}
		void set_file_handle(native_file_handle& file)
		{
			m_file = file;
		}
		DELETE_COPY(_FileHandle);
	private:
		native_file_handle m_file;
	};

	class file_handle
	{
		using impl_t = std::shared_ptr<_FileHandle>;
	public:
		file_handle()
			:m_impl(nullptr)
		{}
		file_handle(const native_file_handle& file)
			:m_impl(std::make_shared<_FileHandle>(file))
		{}
		file_handle(const file_handle& other)
			:m_impl(other.m_impl)
		{}
		~file_handle() = default;
		file_handle& operator=(const file_handle& other)
		{
			m_impl = other.m_impl;
			return *this;
		}
		operator native_file_handle() const
		{
			return (native_file_handle)(*m_impl);
		}
		native_file_handle get_file_handle() const
		{
			return m_impl->get_file_handle();
		}
		void set_file_handle(native_file_handle& file)
		{
			m_impl->set_file_handle(file);
		}
		bool operator==(const file_handle& other) const
		{
			return (m_impl == other.m_impl) || (m_impl->get_file_handle() == m_impl->get_file_handle());
		}
		operator bool() const
		{
			return (bool)m_impl;
		}
	private:
		impl_t m_impl;
	};
	}

namespace stdx
{
	extern stdx::file_stream open_file_stream(const stdx::file_io_service& io_service, const stdx::string& path, const file_enum_value_t& access_type, const file_enum_value_t& open_type);

	extern stdx::file_stream open_file_stream(const stdx::file_io_service& io_service, const stdx::string& path, file_access_type access_type, file_open_type open_type);

	extern stdx::file_handle open_for_senfile(const stdx::string& path, const file_enum_value_t& access_type, const file_enum_value_t& open_type);
}

namespace stdx
{
	extern stdx::unique_flag _FullpathNameFlag;

	extern stdx::task<stdx::string> realpath(stdx::string path);

	using file_cancel_token_ptr = int*;
	struct file_cancel_token_value
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

		file()
			:m_io_service(nullptr)
			, m_path(std::make_shared<stdx::string>())
		{}

		file(const stdx::file_io_service& io_service, const stdx::string& path);

		file(const file& other);

		file(file&& other) noexcept;

		file& operator=(file&& other) noexcept;

		file& operator=(const file& other);

		bool operator==(const file& other) const;

		~file() = default;

		const stdx::string& path() const;

		uint64_t size() const;

		void remove();

		void copy_to(const stdx::string& path, file_cancel_token_ptr cancel_ptr, std::function<void(uint64_t, uint64_t)>&& on_progress_change, std::function<void(uint64_t, uint64_t)>&& on_cancel/*, std::function<void(std::exception_ptr)> &&on_error*/);

		bool exist() const;

#ifdef WIN32
		stdx::file_stream open_stream(const DWORD& access_type, const DWORD& open_type);


		HANDLE open_native_handle(const DWORD& access_type, const DWORD& open_type) const;
#else
		stdx::file_stream open_stream(const stdx::file_io_service& io_service, const int32_t& access_type, const int32_t& open_type);

		int open_native_handle(const int32_t& access_type, const int32_t& open_type) const;
#endif // WIN32

#ifdef WIN32
		HANDLE open_for_sendfile_native(const stdx::file_access_type& access_type, const stdx::file_open_type& open_type);
#else
		int open_for_sendfile_native(const stdx::file_access_type& access_type, const stdx::file_open_type& open_type);
#endif

		stdx::file_handle open_for_sendfile(const stdx::file_access_type& access_type, const stdx::file_open_type& open_type);

#ifdef WIN32
		static void close_handle(HANDLE file);
#else
		static void close_handle(int file);
#endif // WIN32

		stdx::file_stream open_stream(const stdx::file_access_type& access_type, const stdx::file_open_type& open_type);

	private:
		std::shared_ptr<stdx::string> m_path;
		stdx::file_io_service m_io_service;
	};
}
#endif

#ifdef WIN32
#undef _ThrowWinError
#else
#undef _ThrowLinuxError
#endif