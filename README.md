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
确保你正确已安装Jemalloc & CMake
且系统中安装有GCC或MSVC
```
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DJEMALLOC_PATH={你的jemalloc安装目录,不提供将找查默认目录}
make
make install
```

[中文文档(编写中)](https://github.com/NaturalSelect/stdx/wiki)