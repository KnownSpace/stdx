
cmake_minimum_required (VERSION 3.8)
# 将源添加到此项目的可执行文件。
aux_source_directory(${PROJECT_SOURCE_DIR}/src/stdx/ src)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/stdx/async async_src)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/stdx/net net_src)

message("--${src}")
message("--${async_src}")
message("--${net_src}")

add_library(libstdx STATIC ${src} ${async_src} ${net_src})

include_directories(${PROJECT_SOURCE_DIR}/include)

#判断位数
if(CMAKE_CL_64)
	#64位
	set(CMAKE_SYS_BIT 64)
else()
	#32位
	set(CMAKE_SYS_BIT 32)
endif()

#判断平台
if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
	add_definitions(-DLINUX)
	message(STATUS "Current platform: Linux ")
endif()

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
	add_definitions(-DWIN32)
	message(STATUS "Current platform: Windows ")
	target_compile_options(libstdx  PUBLIC "/MT$<$<STREQUAL:$<CONFIGURATION>,Debug>:d>")
endif()

#定义DEBUG符号
if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	message(STATUS "Enable debug generating.")
	add_definitions(-DDEBUG)
	if(UNIX)
		set_target_properties(libstdx PROPERTIES OUTPUT_NAME "stdxd")
	else()
		set_target_properties(libstdx PROPERTIES OUTPUT_NAME "libstdxd")
	endif()
else()
	if(UNIX)
		set_target_properties(libstdx PROPERTIES OUTPUT_NAME "stdx")
		target_compile_options(libstdx  PUBLIC "-O3")
	else()
		target_compile_options(libstdx  PUBLIC "/O2")
	endif()
endif()

#类Unix使用pthread
if(UNIX)
	find_package(Threads REQUIRED)
	if(THREADS_HAVE_PTHREAD_ARG)
		set_property(TARGET libstdx  PROPERTY COMPILE_OPTIONS "-pthread")
  		set_property(TARGET libstdx  PROPERTY INTERFACE_COMPILE_OPTIONS "-pthread")
	endif()
	if(CMAKE_THREAD_LIBS_INIT)
 		 target_link_libraries(libstdx  PRIVATE "${CMAKE_THREAD_LIBS_INIT}")
	endif()
endif()