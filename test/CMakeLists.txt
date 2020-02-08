cmake_minimum_required (VERSION 3.8)

aux_source_directory(${PROJECT_SOURCE_DIR}/test test_src)
message(${test_src})
add_executable(test ${test_src})
include_directories(${PROJECT_SOURCE_DIR}/include)

target_link_libraries(test libstdx)

if(UNIX)
	#pthread
	find_package(Threads REQUIRED)
	if(THREADS_HAVE_PTHREAD_ARG)
  		set_property(TARGET test  PROPERTY COMPILE_OPTIONS "-pthread")
  		set_property(TARGET test  PROPERTY INTERFACE_COMPILE_OPTIONS "-pthread")
	endif()
	#if(CMAKE_THREAD_LIBS_INIT)
		#target_link_libraries(test  PRIVATE "${CMAKE_THREAD_LIBS_INIT}")
	#endif()
endif()