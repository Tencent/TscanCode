# **TscanCode** 

![Release version](https://img.shields.io/badge/version-2.14.24-blue.svg)

## A fast and accurate static analysis solution for C/C++, C#, Lua codes

Tencent is pleased to support the open source community by making TscanCode available.

Copyright (C) 2017 Tencent company and TscanCode Team. All rights reserved.

## Introduction

TscanCode is devoted to help programmers to find out code defects at the very beginning.  
* TscanCode supports multi-language: `C/C++`, `C#` and `Lua` codes;
* TscanCode is `fast` and `accurate`, The performance can be 200K lines per minute and  the accuracy rate is about 90%;   
* TscanCode is `easy to use`, It doesn't require strict compiling enviroment and one single command can make it work; 
* TscanCode is `extensible`, you can implement your own checks with TscanCode.

## Highlights in v2.14.24 (2018-02-24)
* `Rule Package` was released on GUI, easier for rule customization;
* GUI supports `marking false-positive errors` now.

For other changes please refer to [change log](CHANGELOG.md).

## Compiling

Any C++11 compiler should work. For compilers with partial C++11 support it may work. If your compiler has the C++11 features that are available in Visual Studio 2015 then it will work. If nullptr is not supported by your compiler then this can be emulated using the header lib/cxx11emu.h.

There are multiple compilation choices:
* Windows: Visual Studio (Visual Studio 2015 and above)
* Linux: g++ 4.6 (or later)
* Mac: clang++

### Visual Studio

Use the tsancode.sln file. The file is configured for Visual Studio 2015, but the platform toolset can be changed easily to older or newer versions. The solution contains platform targets for both x86 and x64.

Select option `Release` to build release version.

### g++ or clang++

Simple build (no dependencies):

```shell
make
```

## Usage at a glance

This simple example contains a potential null pointer defect. Checking if p is null indicates that p might be null, so dereferencing p `*p` is not safe outside the `if-scope`.

~~~~~~~~~~cpp
// func.cpp
void func(int* p) {
    if(p == NULL) {
        printf("p is null!");
    }

    printf("p is %d", *p);
}
~~~~~~~~~~

Run TscanCode:
```shell
./tscancode --xml func.cpp 2>result.xml
```
Error list, result.xml:

~~~~~~~~~~xml
<?xml version="1.0" encoding="UTF-8"?>
<results>
    <error file="func.cpp" line="7" id="nullpointer" subid="dereferenceAfterCheck" severity="error" 
           msg="Comparing [p] to null at line 3 implies [p] might be null. Dereferencing null pointer [p]." />
</results>
~~~~~~~~~~

There are more examples:
* [CPP samples](samples/cpp);
* [C# samples](samples/csharp);
* [Lua samples](samples/lua);

For now, codes under [trunk](trunk) are only for TscanCode `CPP` version, `C#` and `Lua` version are in the internal review process. Sorry for the inconvenience.

