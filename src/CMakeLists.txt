
cmake_minimum_required (VERSION 3.8)
# 将源添加到此项目的可执行文件。
aux_source_directory(${PROJECT_SOURCE_DIR}/src/stdx/ src)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/stdx/async async_src)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/stdx/net net_src)

message(${src})
message(${async_src})
message(${net_src})

add_library (stdx SHARED ${src} ${async_src} ${net_src})
add_library(libstdx STATIC ${src} ${async_src} ${net_src})

include_directories(${PROJECT_SOURCE_DIR}/include)
set_target_properties(stdx PROPERTIES OUTPUT_NAME "stdx")

if(UNIX)
	set_target_properties(libstdx PROPERTIES OUTPUT_NAME "stdx")
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	add_definitions(-DDEBUG)
endif()

# TODO: 如有需要，请添加测试并安装目标。

if(UNIX)
	#pthread
	find_package(Threads REQUIRED)
	if(THREADS_HAVE_PTHREAD_ARG)
  		set_property(TARGET stdx  PROPERTY COMPILE_OPTIONS "-pthread")
  		set_property(TARGET stdx  PROPERTY INTERFACE_COMPILE_OPTIONS "-pthread")
		set_property(TARGET libstdx  PROPERTY COMPILE_OPTIONS "-pthread")
  		set_property(TARGET libstdx  PROPERTY INTERFACE_COMPILE_OPTIONS "-pthread")
	endif()
	if(CMAKE_THREAD_LIBS_INIT)
		target_link_libraries(stdx  PRIVATE "${CMAKE_THREAD_LIBS_INIT}")
 		 target_link_libraries(libstdx  PRIVATE "${CMAKE_THREAD_LIBS_INIT}")
	endif()
endif()