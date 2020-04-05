# CMakeList.txt : 顶级 CMake 项目文件，执行全局配置
# 和此处的包含子项目。
#
cmake_minimum_required (VERSION 3.8)
project ("stdx")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

# 包含子项目。
add_subdirectory ("src")
add_subdirectory ("test")