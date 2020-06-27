cmake_minimum_required (VERSION 3.8)

aux_source_directory(${PROJECT_SOURCE_DIR}/test test_src)
message(${test_src})
add_executable(teststdx ${test_src})
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/test/include)

find_package(Jemalloc)
include_directories(${JEMALLOC_INCLUDE_DIRS})
target_link_libraries(teststdx PUBLIC ${JEMALLOC_LIBRARIES} libstdx)

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
	target_link_libraries(teststdx PUBLIC dl)
endif()

#¿‡Unix π”√pthread
if(UNIX)
	find_package(Threads REQUIRED)
	if(THREADS_HAVE_PTHREAD_ARG)
		set_property(TARGET teststdx  PROPERTY COMPILE_OPTIONS "-pthread")
  		set_property(TARGET teststdx  PROPERTY INTERFACE_COMPILE_OPTIONS "-pthread")
	endif()
	if(CMAKE_THREAD_LIBS_INIT)
 		 target_link_libraries(teststdx  PRIVATE "${CMAKE_THREAD_LIBS_INIT}")
	endif()
endif()