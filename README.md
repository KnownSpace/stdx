# Standard Expansion

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/1d46abfe845b4b9fac4989e4d4ea2fc1)](https://www.codacy.com/manual/NaturalSelect/stdx?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=NaturalSelect/stdx&amp;utm_campaign=Badge_Grade)


## 注意：
* 这个库仍在开发中
* 部分接口变动比较频繁

## 依赖
* [Jemalloc](https://github.com/jemalloc/jemalloc)
* CMake
* C++ 11

## 受支持的系统
* Windows	7+
* Linux		2.5+

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

[中文文档](https://github.com/NaturalSelect/stdx/wiki)