/*
* Tencent is pleased to support the open source community by making TscanCode available.
* Copyright (C) 2015 THL A29 Limited, a Tencent company. All rights reserved.
* Licensed under the GNU General Public License as published by the Free Software Foundation, version 3 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
* http://www.gnu.org/licenses/gpl.html
* TscanCode is free software: you can redistribute it and/or modify it under the terms of License.    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR * A PARTICULAR PURPOSE.  See the GNU General Public License for more details. 
*/

#ifndef DEALCONFIG_H
#define DEALCONFIG_H
#pragma once
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string> 
#include <vector>
#include "tinyxml.h"
#include <map>
#include <list>

using namespace std;

// TSC:from TSC 20140918  add ExitProcess(win32)
//#define JUMPSTR "return|abort|exit|continue|break|goto|ExitProcess|throw"
static std::string JUMPSTR="return|abort|exit|continue|break|goto|ExitProcess|throw";

#define MAXERRORCNT 20
#define AUTOFILTER_SUBID "FuncPossibleRetNULL|FuncRetNULL|dereferenceAfterCheck|dereferenceBeforeCheck|possibleNullDereferenced|nullpointerarg|nullpointerclass"
struct CUSTOMNULLPOINTER
{
	string functionname;
	int var;
};
// TSC:from TSC 20131029
struct  CANNOTBENULL
{
	string functionname;
	int var;
};

// by TSC 20140912
// identify one method
struct SFuncSignature
{
	std::string classname;
	std::string functionname;
	std::vector<std::string> params;

	bool operator<(const SFuncSignature& funcSign) const
	{
		if (classname < funcSign.classname)
		{
			return true;
		}
		else if (classname == funcSign.classname)
		{
			if (functionname < funcSign.functionname)
			{
				return true;
			}
			else if (functionname == funcSign.functionname)
			{
				//bool bLess = false;
				if (params.size() != funcSign.params.size())
				{
					return params.size() < funcSign.params.size();
				}
				int paramCount = params.size();
				for (int i = 0; i<paramCount; i++)
				{
					if (params[i] < funcSign.params[i])
					{
						return true;
					}
					else if (params[i] == funcSign.params[i])
					{
						continue;
					}
					else
					{
						return false;
					}
				}
				return false;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
};
//add by TSC
void trim(string& str);
//string& trimall(string &str, string::size_type pos = 0);
void TrimLeft(string &str);
void TrimRight(string &str);
void TrimAll(string &str);
//int splitString(const string & strSrc, const std::string& strDelims, vector<string>& strDest);
class dealconfig {
public:
	static dealconfig* instance()
	{
		return &s_instance;
	}
	static void SetConfigDir(const char* szDir)
	{
		if (NULL != szDir)
		{
			s_strDir = szDir;
		}
		
		// init from files
		s_instance.readxmlconf();
		s_instance.readConfig();
		//s_instance.readCustomNullPointerList();
		//s_instance.readCustomNullPointerList2();
		//s_instance.readCustomNullPointerList2_overload();
		//s_instance.readJumpMacros();
		s_instance.isinit = true;
	}

private:
	static std::string s_strDir;
	static dealconfig s_instance;
public:
	bool isinit;

	bool writelog;

	bool showfuncinfo;
	bool Suspicious;
	bool compute;

	bool nullpointer;
	bool bufoverrun;
	bool logic;
	bool memleak;
	bool autovar;
	bool unsafeFun;
	bool portability64;

	bool explicitNullDereferenced;
	bool possibleNullDereferenced;
	bool dereferenceIfNull;
	bool nullPointerLinkedList;
	bool dereferenceAfterCheck;
	bool dereferenceBeforeCheck;
	bool FuncPossibleRetNULL;
	bool FuncRetNULL;
	bool nullpointer_arg;
	bool nullpointer_class;

	bool UnconditionalBreakinLoop;
	bool NoFirstCase;
	bool PreciseComparison;
	bool AssignAndCompute;
	bool SuspiciousPriority;
	bool RecursiveFunc;

	bool uninitvar;
	//checkclass类，this用于减法运算的表达式，如：手抖把this->写成了this-x
	bool thisSubtraction;
	//checkAssinif类，不匹配的赋值或比较，导致结果永远为真或假
	bool assignIf;
	//checkAssinif类，等号左右两边不匹配
	bool comparison;
	//checkAssinif类，匹配if和else if ，表达式总是false，因为else if匹配前一个条件
	bool multiCondition;
	//if判断条件重复，如：if (a) { b = 1; } else if (a) { b = 2; }
	bool DuplicateIf;
	//checkstl invalidIterator
	bool invalidIterator;
	/*
	//checkstl,可疑的判断，如std::set<int> s;  if (s.find(12)) { };正确写法std::set<int> s; if (s.find(12) != s.end()) { }
	bool iffind;
	//不同容器的迭代器被放在一起使用，很可能是写迭代器名称写错了。如：std::find(foo.begin(), bar.end(), x)
	bool mismatchingContainers;
	//通过iterator删除容器中元素出错
	bool eraseDereference;
	*/
	//分支内容重复
	bool DuplicateBranch;;
	//表达式重复如if (a && a)，if( a>a)等
	bool DuplicateExpression;
	//布尔结果用在了位运算中，如a && b写成了a & b
	bool clarifyCondition;
	//switch中重复赋值，可能是缺少break
	bool RedundantAssignmentInSwitch;
	//数组作为函数参数时，对数组用sizeof计算，得到的是指针的大小，而非数组大小，很可能是想得到数组中大小，但写错了（用sizeof）。如void f( int a[]) std::cout << sizeof(a) / sizeof(int) << std::endl，实际这里sizeof(a)永远是4（指针占4个字节）。但如果int a[10];cout<<sizeof(a)/sizeof(int)<<endl;就是正确的，这里a没作为参数，sizeof(a)=40
	bool SizeofForArrayParameter;
	//对一个数字常量使用sizeof，正常编程不会这么用，很有可能写错了。
	bool SizeofForNumericParameter;
	//对指针使用了sizeof，得到的不是实际数据的大小，而是指针大小（4字节）
	bool SizeofForPointerSize;
	///sizeofsizeof，很可能是手抖多复制了一个，如  int i = sizeof sizeof char;
	bool sizeofsizeof;
	//将布尔值与int类型值进行比较，如:if(!x==4)，不知道为啥run里面有，Simplifiedrun中还有
	bool ComparisonOfBoolExpressionWithInt;
	//可疑的分号使用，在if/for/while语句后直接跟分号。如while(a==2);{a=b;}或for(i=0;i<5;i++);{cout<<i;}
	bool SuspiciousSemicolon;
	//字符串与字符变量相加，一般为错误使用单引号。字符串与字符变量相加，相当于字符串指针偏移N（N为字符变量的ASC码值，char相当于整数）如：const char *p = "/usr" + '/';
	bool strPlusChar;
	//除数和被除数分别是有符号和无符号类型，很可能是类型定义错了。int ivar = -2; unsigned int uvar = 2;return ivar / uvar;结果并非预期的-1，报错Division with signed and unsigned operators. The result might be wrong
	bool UnsignedDivision;
	//在断言中对变量进行赋值，多半是写错位置了。（断言只在debug中有效的宏）
	bool AssignmentInAssert;
	//相反的内部条件，导致代码不可达，很可能是条件判断写错了。例如:if(a==b){if(a!=b)cout<<a;}
	bool oppositeInnerCondition;
	//取模结果的最大值小于被取模的值，导致表达式总是真或总是假。如if(var%4==4)，一般是手抖把边界值写错了
	bool ModuloAlwaysTrueFalse;
	//除数为0
	bool ZeroDivision;
	//布尔值用在了位运算中
	bool BitwiseOnBoolean;
	//错误的逻辑运算，如：总是真，或总是假
	bool IncorrectLogicOperator;
	//将布尔值与int类型值进行比较，如:if(!x==4)
	bool ComparisonOfBoolWithInt;
	//bool类型变量间的比较，函数返回值是bool变量，却对该类函数使用关系运算符<,>,<=,>=.例如  bool b = compare2(6);bool a = compare1(4); if(b > a){}
	bool ComparisonOfBoolWithBool;
	//switch 缺少break 和 default的强制检查
	bool SwitchNoBreak;
	//switch default的强制检查
	bool SwitchNoDefault;
	// check break UP
	bool SwitchNoBreakUP;
	//对负的右操作数进行位移运算，如：a=5;a<<-1
	bool NegativeBitwiseShift;
	//对bool变量使用自增，导致bool变量的值可能不符合预期。如：bool bValue = true;bValue++;
	bool IncrementBoolean;
	//将bool值赋值给指针，有可能是bool变量声明时多加了个*，如void foo(bool *p) {p = false;}
	bool AssignBoolToPointer;
	//调用memset函数但只填充0字节，一般是第2和3个参数写反了。如：memset(p, sizeof(p), 0)，正确写法是memset(p,0,sizeof(p))
	bool MemsetZeroBytes;
	bool IfCondition;
	bool FuncReturn;
	//怀疑出错的for循环
	bool suspiciousfor;
	bool wrongvarinfor;

	bool suspiciousArrayIndex;
	//TSC:from TSC 20140726
	bool formatbufoverrun;
	//成员变量未初始化
	bool uninitMemberVar;
	//end TSC
	bool nestedloop;
	//无符号数小于0
	bool Unsignedlessthanzero;
	//bool函数返回了非bool值
	bool BoolFuncReturn;
	// add by TSC, 20014-08-27
	// Validate the methods which have variable parameters 
	bool InvalidVarArgs;

	//std::string JumpMacrosConf;
    vector<CUSTOMNULLPOINTER> m_CustomPointerlList;
	// TSC:from  20131029
	vector<CANNOTBENULL> f_CustomPointerlList;
	// TSC:from  20140903
	vector<SFuncSignature> func_CustomPointerlList;
	
	dealconfig();
	~dealconfig();
	//from TSC init checks
	void init_true();
	void init_false();
	//from TSC handle checklist.xml and CustomNullPointerList.ini
	bool isOpen(const char* str);
	bool readxmlconf();
	void readCustomNullPointerList();
	void readCustomNullPointerList2();
	void readCustomNullPointerList2_overload();
	//from TSC 20141112 handle JumpMacros.ini
	void readJumpMacros();
	SFuncSignature parseFuncInfo(string funcinfo);

	void readConfig();

	void DumpCfg();

};


class IniParser
{
public:
	struct SPair
	{
		std::string sKey;
		std::string sValue;
	};
public:
	bool Parse(const std::string& iniPath);
	bool GetSection(const std::string& sName, std::list<SPair>*& pMapKeyValue);
private:
	std::map<std::string, std::list<SPair> > m_ini;
};

#endif // DEALCONFIG_H