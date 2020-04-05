cmake_minimum_required (VERSION 3.8)

aux_source_directory(${PROJECT_SOURCE_DIR}/test test_src)
message(${test_src})
add_executable(teststdx ${test_src})
include_directories(${PROJECT_SOURCE_DIR}/include)

find_package(Jemalloc)
include_directories(${JEMALLOC_INCLUDE_DIRS})
#add_dependencies(test libstdx)
target_link_libraries(teststdx PUBLIC ${JEMALLOC_LIBRARIES} libstdx)