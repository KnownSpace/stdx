# Standard Expansion

[![Codacy Badge](https://app.codacy.com/project/badge/Grade/d8d4c7de2e564efcb8e26471343500f2)](https://www.codacy.com/gh/KnownSpace/stdx?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=KnownSpace/stdx&amp;utm_campaign=Badge_Grade)


## 注意：
* 这个库仍在开发中
* 部分接口变动比较频繁

## 依赖
* [Jemalloc](https://github.com/jemalloc/jemalloc)
* CMake
* C++ 11

## 受支持的系统
* Windows	7+
* Linux		2.6+

## 构建
要求:
1. 已安装Jemalloc
1. 已安装CMake
1. 已安装GCC或MSVC
```
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DJEMALLOC_PATH={你的jemalloc安装目录,不提供将找查默认目录}
make
make install
```
### Linux下的自动构建脚本
```c++
build.sh			// 构建
build-jemalloc.sh	// 编译安装Jemalloc
build-cmake3.sh		// 使用 cmake3 命令行构建
```

## 贡献代码
1. Clone储存库
1. 安装[Jemalloc](https://github.com/jemalloc/jemalloc)
1. 阅读[约定](https://github.com/KnownSpace/stdx/wiki)
1. 修改代码
1. 测试你的代码
1. commit到dev分支(OR 打开pull request)

[中文文档(编写中)](https://github.com/KnownSpace/stdx/wiki)