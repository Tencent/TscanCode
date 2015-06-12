/*
* Tencent is pleased to support the open source community by making TscanCode available.
* Copyright (C) 2015 THL A29 Limited, a Tencent company. All rights reserved.
* Licensed under the GNU General Public License as published by the Free Software Foundation, version 3 (the "License"); you may not use this file except in compliance with the License. You may obtain a * copy of the License at
* http://www.gnu.org/licenses/gpl.html
* TscanCode is free software: you can redistribute it and/or modify it under the terms of License.    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef CONFIG_H
#define CONFIG_H

#ifdef _WIN32
#  ifdef CPPCHECKLIB_EXPORT
#    define CPPCHECKLIB __declspec(dllexport)
#  elif defined(CPPCHECKLIB_IMPORT)
#    define CPPCHECKLIB __declspec(dllimport)
#  else
#    define CPPCHECKLIB
#  endif
#else
#  define CPPCHECKLIB
#endif


#endif
