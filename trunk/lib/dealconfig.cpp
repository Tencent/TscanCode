/*
* Tencent is pleased to support the open source community by making TscanCode available.
* Copyright (C) 2015 THL A29 Limited, a Tencent company. All rights reserved.
* Licensed under the GNU General Public License as published by the Free Software Foundation, version 3 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
* http://www.gnu.org/licenses/gpl.html
* TscanCode is free software: you can redistribute it and/or modify it under the terms of License.    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR * A PARTICULAR PURPOSE.  See the GNU General Public License for more details. 
*/


#include "dealconfig.h"
#include "FileDepend.h"
#include "path.h"


dealconfig dealconfig::s_instance;
std::string dealconfig::s_strDir;

void dealconfig::init_true()
{
	Suspicious = true;
    compute = true;
    nullpointer = true;
    bufoverrun = true;
    logic = true;
    memleak = true;
    autovar = true;

	explicitNullDereferenced=true;
    dereferenceAfterCheck = true;
    dereferenceBeforeCheck = true;
	FuncRetNULL = true; 
	dereferenceIfNull = true;
	nullPointerLinkedList=true;
    UnconditionalBreakinLoop = true;
    NoFirstCase = true;
    AssignAndCompute = true;
    SuspiciousPriority = true;
    RecursiveFunc = true;

    uninitvar = true;
    //checkclass类，this用于减法运算的表达式，如：手抖把this->写成了this-x
    thisSubtraction = true;
    //checkAssinif类，不匹配的赋值或比较，导致结果永远为真或假
    assignIf = true;
    //checkAssinif类，等号左右两边不匹配
    comparison = true;
    //checkAssinif类，匹配if和else if ，表达式总是false，因为else if匹配前一个条件
    multiCondition = true;
    //if判断条件重复，如：if (a) { b = 1; } else if (a) { b = 2; }
    DuplicateIf = true;
   
	invalidIterator=true;

    //分支内容重复
    DuplicateBranch = true;;
    //表达式重复如if (a && a)，if( a>a)等
    DuplicateExpression = true;
    //布尔结果用在了位运算中，如a && b写成了a & b
    clarifyCondition = true;
    //switch中重复赋值，可能是缺少break
    RedundantAssignmentInSwitch = true;
    //数组作为函数参数时，对数组用sizeof计算，得到的是指针的大小，而非数组大小，很可能是想得到数组中大小，但写错了（用sizeof）。如void f( int a[]) std::cout << sizeof(a) / sizeof(int) << std::endl，实际这里sizeof(a)永远是4（指针占4个字节）。但如果int a[10];cout<<sizeof(a)/sizeof(int)<<endl;就是正确的，这里a没作为参数，sizeof(a)=40
    SizeofForArrayParameter = true;
    //对一个数字常量使用sizeof，正常编程不会这么用，很有可能写错了。
    SizeofForNumericParameter = true;
    //对指针使用了sizeof，得到的不是实际数据的大小，而是指针大小（4字节）
    SizeofForPointerSize = true;
    ///sizeofsizeof，很可能是手抖多复制了一个，如  int i = sizeof sizeof char;
    sizeofsizeof = true;
    //将布尔值与int类型值进行比较，如:if(!x==4)，不知道为啥run里面有，Simplifiedrun中还有
    ComparisonOfBoolExpressionWithInt = true;
    //可疑的分号使用，在if/for/while语句后直接跟分号。如while(a==2);{a=b;}或for(i=0;i<5;i++);{cout<<i;}
    SuspiciousSemicolon = true;
    //字符串与字符变量相加，一般为错误使用单引号。字符串与字符变量相加，相当于字符串指针偏移N（N为字符变量的ASC码值，char相当于整数）如：const char *p = "/usr" + '/';
    strPlusChar = true;
    //除数和被除数分别是有符号和无符号类型，很可能是类型定义错了。int ivar = -2; unsigned int uvar = 2;return ivar / uvar;结果并非预期的-1，报错Division with signed and unsigned operators. The result might be wrong
    UnsignedDivision = true;
    //在断言中对变量进行赋值，多半是写错位置了。（断言只在debug中有效的宏）
    AssignmentInAssert = true;
    //相反的内部条件，导致代码不可达，很可能是条件判断写错了。例如:if(a==b){if(a!=b)cout<<a;}
    oppositeInnerCondition = true;
    //取模结果的最大值小于被取模的值，导致表达式总是真或总是假。如if(var%4==4)，一般是手抖把边界值写错了
    ModuloAlwaysTrueFalse = true;
    //除数为0
    ZeroDivision = true;
    //布尔值用在了位运算中
    BitwiseOnBoolean = true;
    //错误的逻辑运算，如：总是真，或总是假
    IncorrectLogicOperator = true;
    //将布尔值与int类型值进行比较，如:if(!x==4)
    ComparisonOfBoolWithInt = true;
    //bool类型变量间的比较，函数返回值是bool变量，却对该类函数使用关系运算符<,>,<=,>=.例如  bool b = compare2(6);bool a = compare1(4); if(b > a){}
    ComparisonOfBoolWithBool = true;
    SwitchNoBreakUP = true;
    //对负的右操作数进行位移运算，如：a=5;a<<-1
    NegativeBitwiseShift = true;
    //对bool变量使用自增，导致bool变量的值可能不符合预期。如：bool bValue = true;bValue++;
    IncrementBoolean = true;
    //将bool值赋值给指针，有可能是bool变量声明时多加了个*，如void foo(bool *p) {p = false;}
    AssignBoolToPointer = true;
    //调用memset函数但只填充0字节，一般是第2和3个参数写反了。如：memset(p, sizeof(p), 0)，正确写法是memset(p,0,sizeof(p))
    MemsetZeroBytes = true;
    IfCondition = true;
    suspiciousfor = true;
    wrongvarinfor = true;
    formatbufoverrun = true;  
   
    Unsignedlessthanzero = true;
    BoolFuncReturn = true;
	InvalidVarArgs = true;
}
void dealconfig::init_false()
{
	unsafeFun = false;
    portability64 = false;
	FuncPossibleRetNULL = false;
	possibleNullDereferenced=false;
	nullpointer_arg = false;
	nullpointer_class = false;

	suspiciousArrayIndex = false;
	PreciseComparison = false;
	SwitchNoBreak = false;
    SwitchNoDefault = false;
	nestedloop = false;
	FuncReturn = false;
	uninitMemberVar = false;
}
dealconfig::dealconfig()
{
	isinit=false;
	writelog=false;
    showfuncinfo = false;

	init_true();
	init_false();
    m_CustomPointerlList.clear();

}

void trim(string& str)
{
	str.erase(str.find_last_not_of(' ') + 1, string::npos);
	str.erase(0, str.find_first_not_of(' '));
}

void trim_2(string& str)
{
    str.erase(str.find_last_not_of(" \t\n\r") + 1, string::npos);
    str.erase(0, str.find_first_not_of(" \t\n\r"));
}

void TrimLeft(string &str)//去左边空格
{
	size_t index;
	index = str.find_first_not_of(" \t\n");
	if (index != string::npos)
	{
		str.erase(0,index);
	}
}
void TrimRight(string &str)//去右边空格
{
	size_t index;
	index = str.find_last_not_of(" \t\n");
	if (index != string::npos)
	{
		str = str.substr(0,index+1);
	}
}
void TrimAll(string &str)//去所有空格
{
	TrimLeft(str);
	TrimRight(str);
}

bool dealconfig::isOpen(const char* str)
{
	if(strcmp(str, "0") == 0){
		return false;
	}
	else{
	    return true;
	}
}
//from TSC read config file "checklist.xml";
bool dealconfig::readxmlconf()
{
    TiXmlDocument doc;

	std::string filePath = s_strDir + "checklist.xml";
    if (doc.LoadFile(filePath.c_str()))
    {
        TiXmlElement *node = doc.FirstChildElement();

        if(!node)
            return false;

        const char* ttt = node->Value();

        if(!ttt)
            return false;
		   //log
        TiXmlElement *log_conf = node->FirstChildElement("log");

        if(log_conf && log_conf->Attribute("isopen"))
        {
			const char* str=log_conf->Attribute("isopen");
            writelog = isOpen(str);
        }

        //showfuncinfo_conf
        TiXmlElement *showfuncinfo_conf = node->FirstChildElement("showfuncinfo");

        if(showfuncinfo_conf && showfuncinfo_conf->Attribute("isopen"))
        {
			const char* str=showfuncinfo_conf->Attribute("isopen");
            showfuncinfo = isOpen(str);
        }

        //	nullpointer_conf
        TiXmlElement *nullpointer_conf = node->FirstChildElement("nullpointer");

        if(nullpointer_conf && nullpointer_conf->Attribute("isopen"))
        {
			const char* str=nullpointer_conf->Attribute("isopen");
            nullpointer = isOpen(str);
        }
		TiXmlElement *sub=NULL;
		if(nullpointer_conf)
		{
			sub = nullpointer_conf->FirstChildElement("subid");

        for(; sub; sub = sub->NextSiblingElement())
        {
            ttt = sub->Attribute("name");

			if(strcmp(ttt, "possibleNullDereferenced") == 0  && sub->Attribute("isopen"))
            {
				const char* str=sub->Attribute("isopen");
                possibleNullDereferenced = isOpen(str);
                continue;
            }

            if(strcmp(ttt, "explicitNullDereferenced") == 0 && sub->Attribute("isopen"))
            {
				const char* str=sub->Attribute("isopen");
                explicitNullDereferenced = isOpen(str);
                continue;
            }
            if(strcmp(ttt, "dereferenceAfterCheck") == 0 && sub->Attribute("isopen"))
            {
				const char* str=sub->Attribute("isopen");
                dereferenceAfterCheck = isOpen(str);
                continue;
            }

            if(strcmp(ttt, "dereferenceBeforeCheck") == 0 && sub->Attribute("isopen"))
            {
				const char* str=sub->Attribute("isopen");
                dereferenceBeforeCheck = isOpen(str);
                continue;
            }

            if(strcmp(ttt, "FuncPossibleRetNULL") == 0 && sub->Attribute("isopen"))
            {
				const char* str=sub->Attribute("isopen");
                FuncPossibleRetNULL = isOpen(str);
                continue;
            }

            if(strcmp(ttt, "FuncRetNULL") == 0 && sub->Attribute("isopen"))
            {
				const char* str=sub->Attribute("isopen");
                FuncRetNULL = isOpen(str);
                continue;
            }


            if(strcmp(ttt, "nullpointerarg") == 0 && sub->Attribute("isopen") )
            {
				const char* str=sub->Attribute("isopen");
                nullpointer_arg = isOpen(str);
                continue;
            }

			if(strcmp(ttt, "nullpointerclass") == 0 && sub->Attribute("isopen") )
            {
				const char* str=sub->Attribute("isopen");
                nullpointer_class = isOpen(str);
                continue;
            }
			if(strcmp(ttt, "dereferenceIfNull") == 0 && sub->Attribute("isopen") )
            {
				const char* str=sub->Attribute("isopen");
                dereferenceIfNull = isOpen(str);
                continue;
            }
			if(strcmp(ttt, "nullPointerLinkedList") == 0 && sub->Attribute("isopen") )
			{
				const char* str=sub->Attribute("isopen");
				nullPointerLinkedList = isOpen(str);
				continue;
			}
        }
	}
        //	bufoverrun_conf
        TiXmlElement *bufoverrun_conf = node->FirstChildElement("bufoverrun");

        if(bufoverrun_conf && bufoverrun_conf->Attribute("isopen"))
        {
			const char* str=bufoverrun_conf->Attribute("isopen");
            bufoverrun = isOpen(str);
        }
		if(bufoverrun_conf)
		{
        //from TSC 20140726
        sub = bufoverrun_conf->FirstChildElement("subid");

        if(sub)
            ttt = sub->Value();

        for(; sub; sub = sub->NextSiblingElement())
        {
            ttt = sub->Attribute("name");

            if(strcmp(ttt, "formatbufoverrun") == 0 && sub->Attribute("isopen"))
            {
				const char* str=sub->Attribute("isopen");
                formatbufoverrun = isOpen(str);
                continue;
            }
        }
        //end bufoverrun_conf
		}
        //	memleak_conf
        TiXmlElement *memleak_conf = node->FirstChildElement("memleak");

        if(memleak_conf && memleak_conf->Attribute("isopen"))
        {
			const char* str=memleak_conf->Attribute("isopen");
            memleak = isOpen(str);
        }

        //	unsafeFun_conf
        TiXmlElement *unsafeFun_conf = node->FirstChildElement("unsafeFunc");

        if(unsafeFun_conf && unsafeFun_conf->Attribute("isopen"))
        {
			const char* str=unsafeFun_conf->Attribute("isopen");
            unsafeFun = isOpen(str);
        }

        //logic
        TiXmlElement *logic_conf = node->FirstChildElement("logic");

        if(logic_conf)
        {
            if(logic_conf->Attribute("isopen"))
            {
				const char* str=logic_conf->Attribute("isopen");
                logic = isOpen(str);
            }

            sub = logic_conf->FirstChildElement("subid");

            if(sub)
                ttt = sub->Value();

            for(; sub; sub = sub->NextSiblingElement())
            {
                ttt = sub->Attribute("name");

                if(strcmp(ttt, "uninitvar") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    uninitvar = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "assignIf") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    assignIf = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "NoFirstCase") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    NoFirstCase = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "RecursiveFunc") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    RecursiveFunc = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "comparison") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    comparison = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "DuplicateIf") == 0 && sub->Attribute("isopen") )
                {
					const char* str=sub->Attribute("isopen");
                    DuplicateIf = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "DuplicateBranch") == 0 && sub->Attribute("isopen") )
                {
					const char* str=sub->Attribute("isopen");
                    DuplicateBranch = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "DuplicateExpression") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    DuplicateExpression = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "clarifyCondition") == 0 && sub->Attribute("isopen") )
                {
					const char* str=sub->Attribute("isopen");
                    clarifyCondition = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "SwitchNoDefault") == 0 && sub->Attribute("isopen") )
                {
					const char* str=sub->Attribute("isopen");
                    SwitchNoDefault = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "SwitchNoBreakUP") == 0 && sub->Attribute("isopen") )
                {
					const char* str=sub->Attribute("isopen");
                    SwitchNoBreakUP = isOpen(str);
                    continue;
                }
				if(strcmp(ttt, "ComparisonOfBoolWithInt") == 0 && sub->Attribute("isopen") )
				{
					const char* str=sub->Attribute("isopen");
					ComparisonOfBoolWithInt = isOpen(str);
					continue;
				}
				
                if(strcmp(ttt, "ComparisonOfBoolExpressionWithInt") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    ComparisonOfBoolExpressionWithInt = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "oppositeInnerCondition") == 0 && sub->Attribute("isopen") )
                {
					const char* str=sub->Attribute("isopen");
                    oppositeInnerCondition = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "BitwiseOnBoolean") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    BitwiseOnBoolean = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "IncorrectLogicOperator") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    IncorrectLogicOperator = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "ComparisonOfBoolExpressionWithInt") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    ComparisonOfBoolExpressionWithInt = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "ComparisonOfBoolWithBool") == 0 && sub->Attribute("isopen") )
                {
					const char* str=sub->Attribute("isopen");
                    ComparisonOfBoolWithBool = isOpen(str);
                    continue;
                }

                //from TSC 20140726
                if(strcmp(ttt, "uninitMemberVar") == 0 && sub->Attribute("isopen") )
                {
					const char* str=sub->Attribute("isopen");
                    uninitMemberVar = isOpen(str);
                    continue;
                }

				if(strcmp(ttt, "InvalidVarArgs") == 0 && sub->Attribute("isopen") )
				{
					const char* str=sub->Attribute("isopen");
					InvalidVarArgs = isOpen(str);
					continue;
				}
            }
        }

        //Suspicious_conf
        TiXmlElement *Suspicious_conf = node->FirstChildElement("Suspicious");

        if(Suspicious_conf)
        {
            if(Suspicious_conf->Attribute("isopen"))
            {
				const char* str=Suspicious_conf->Attribute("isopen");
                Suspicious = isOpen(str);
            }

            sub = Suspicious_conf->FirstChildElement("subid");

            if(sub)
                ttt = sub->Value();

            for(; sub; sub = sub->NextSiblingElement())
            {
                ttt = sub->Attribute("name");
                if(strcmp(ttt, "suspiciousArrayIndex") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    suspiciousArrayIndex = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "thisSubtraction") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    thisSubtraction = isOpen(str);
                    continue;
                }
                if(strcmp(ttt, "UnconditionalBreakinLoop") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    UnconditionalBreakinLoop = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "AssignmentInAssert") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    AssignmentInAssert = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "SuspiciousSemicolon") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    SuspiciousSemicolon = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "AssignBoolToPointer") == 0 && sub->Attribute("isopen") )
                {
					const char* str=sub->Attribute("isopen");
                    AssignBoolToPointer = isOpen(str);
                    continue;
                }
                if(strcmp(ttt, "IfCondition") == 0 && sub->Attribute("isopen") )
                {
					const char* str=sub->Attribute("isopen");
                    IfCondition = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "FuncReturn") == 0 && sub->Attribute("isopen") )
                {
					const char* str=sub->Attribute("isopen");
                    FuncReturn = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "strPlusChar") == 0 && sub->Attribute("isopen") )
                {
					const char* str=sub->Attribute("isopen");
                    strPlusChar = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "MemsetZeroBytes") == 0 && sub->Attribute("isopen") )
                {
					const char* str=sub->Attribute("isopen");
                    MemsetZeroBytes = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "SuspiciousPriority") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    SuspiciousPriority = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "suspiciousfor") == 0 && sub->Attribute("isopen") )
                {
					const char* str=sub->Attribute("isopen");
                    suspiciousfor = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "nestedloop") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    nestedloop = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "wrongvarinfor") == 0 && sub->Attribute("isopen") )
                {
					const char* str=sub->Attribute("isopen");
                    wrongvarinfor = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "autovar") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    autovar = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "portability64") == 0 && sub->Attribute("isopen") )
                {
					const char* str=sub->Attribute("isopen");
                    portability64 = isOpen(str);
                    continue;
                }

				if(strcmp(ttt, "BoolFuncReturn") == 0 && sub->Attribute("isopen"))
				{
					const char* str=sub->Attribute("isopen");
					BoolFuncReturn = isOpen(str);
					continue;
				}
				if(strcmp(ttt, "invalidIterator") == 0 && sub->Attribute("isopen"))
				{
					const char* str=sub->Attribute("isopen");
					invalidIterator = isOpen(str);
					continue;
				}
            }
        }

        //compute_conf
        TiXmlElement *compute_conf = node->FirstChildElement("compute");

        if(compute_conf)
        {
            if(compute_conf->Attribute("isopen"))
            {
				const char* str=compute_conf->Attribute("isopen");
                compute = isOpen(str);
            }

            sub = compute_conf->FirstChildElement("subid");

            if(sub)
                ttt = sub->Value();

            for(; sub; sub = sub->NextSiblingElement())
            {
                ttt = sub->Attribute("name");

                if(strcmp(ttt, "AssignAndCompute") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    AssignAndCompute = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "PreciseComparison") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    PreciseComparison = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "UnsignedDivision") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    UnsignedDivision = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "UnsignedDivision") == 0 && strcmp(sub->Attribute("isopen"), "0") == 0)
                {
					const char* str=sub->Attribute("isopen");
                    UnsignedDivision = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "ModuloAlwaysTrueFalse") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    ModuloAlwaysTrueFalse = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "Unsignedlessthanzero") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    Unsignedlessthanzero = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "ZeroDivision") == 0 && sub->Attribute("isopen") )
                {
					const char* str=sub->Attribute("isopen");
                    ZeroDivision = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "NegativeBitwiseShift") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    NegativeBitwiseShift = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "IncrementBoolean") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    IncrementBoolean = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "SizeofForArrayParameter") == 0 && sub->Attribute("isopen") )
                {
					const char* str=sub->Attribute("isopen");
                    SizeofForArrayParameter = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "SizeofForNumericParameter") == 0 && sub->Attribute("isopen") )
                {
					const char* str=sub->Attribute("isopen");
                    SizeofForNumericParameter = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "sizeofsizeof") == 0 && sub->Attribute("isopen") )
                {
					const char* str=sub->Attribute("isopen");
                    sizeofsizeof = isOpen(str);
                    continue;
                }

                if(strcmp(ttt, "SizeofForPointerSize") == 0 && sub->Attribute("isopen"))
                {
					const char* str=sub->Attribute("isopen");
                    SizeofForPointerSize = isOpen(str);
                    continue;
                }
            }
        }
		return true;
    }
    else
    {
        cout << "没有找到checklist.xml配置文件，请检查配置文件是否存在！" << endl;
        return false;
    }
}


dealconfig::~dealconfig()
{
}

void dealconfig::readCustomNullPointerList()
{
	std::string filePath = s_strDir + "CustomNullPointerCheckList.ini";
    fstream cfgFile(filePath.c_str());

    if(!cfgFile)
    {
        cout << "没有找到CustomNullPointerCheckList.ini配置文件，请检查配置文件是否存在！" << endl;
        return;
    }

    char tmp[1000] = {0};

    while(!cfgFile.eof())
    {
        cfgFile.getline(tmp, 1000); //每行读取前1000个字符，1000个足够了
        //去掉注释
        if(tmp[0] == '#' || tmp[0] < 0)
        {
            continue;
        }

        string line(tmp);

        //去掉空行
        if(line.length() == 0)
        {
            continue;
        }

        size_t pos = line.find('=');//找到每行的“=”号位置，之前是key之后是value

        if(pos == string::npos)
        {
            continue ;
        }

        string tmpKey = line.substr(0, pos); //取=号之前
        trim(tmpKey);
        string value;//对应变量值
        value = line.substr(pos + 1); //取=号之后
        CUSTOMNULLPOINTER temp;
        int i = atoi(value.c_str());

        if ( i != 0)
        {
            temp.functionname = tmpKey;
            temp.var = i;
            m_CustomPointerlList.push_back(temp);
        }
    }

    cfgFile.close();
}

void dealconfig::readCustomNullPointerList2()
{
	std::string filePath = s_strDir + "CustomNullPointerCheckList2.ini";
	fstream cfgFile(filePath.c_str());
    if(!cfgFile)
    {
        return;
    }

    char tmp[1000];

    while(!cfgFile.eof())
    {
        cfgFile.getline(tmp, 1000); //每行读取前1000个字符，1000个足够了

        //去掉注释
        if(tmp[0] == '#')
        {
            continue;
        }

        string line(tmp);

        //去掉空行
        if(line.length() == 0)
        {
            continue;
        }

        size_t pos = line.find('=');//找到每行的“=”号位置，之前是key之后是value
        if (pos == string::npos)
        {
            continue;
        }
		string tmpKey = line.substr(0, pos); //取=号之前
		trim(tmpKey);
        string value;//对应变量值
		value = line.substr(pos + 1); //取=号之后
        CANNOTBENULL temp;
        int i = atoi(value.c_str());

        if ( i != 0)
        {
            temp.functionname = tmpKey;
            temp.var = i;
            f_CustomPointerlList.push_back(temp);
        }
    }

    cfgFile.close();
}

void dealconfig::readCustomNullPointerList2_overload()
{
	std::string filePath = s_strDir + "CustomNullPointerCheckList2.ini";
	fstream cfgFile(filePath.c_str());

	if(!cfgFile)
	{
		//cout<<"没有找到CustomNullPointerCheckList2.ini配置文件，请检查配置文件是否存在！"<<endl;
		return;
	}

	char tmp[1000];

	while(!cfgFile.eof())
	{
		cfgFile.getline(tmp, 1000); //每行读取前1000个字符，1000个足够了

		if(tmp[0] == '#')//去掉注释
			continue;

		string line(tmp);

		if(line.length() == 0)//去掉空行
			continue;

		SFuncSignature temp;
		temp = parseFuncInfo(line);
		func_CustomPointerlList.push_back(temp);
	}

	cfgFile.close();
}

SFuncSignature dealconfig::parseFuncInfo(string funcinfo)
{
	string classname;
	string functionname;
	vector<string> params;
	SFuncSignature temp;

	string::size_type pos_class = 0;
	string::size_type pos_functionname = 0;

	if((pos_class=funcinfo.find("::"))!=string::npos)
	{
		classname  = funcinfo.substr(0,pos_class);
		trim(classname);
	}
	else
		pos_class = 0;

	if((pos_functionname=funcinfo.find('(',pos_class))!=string::npos)
	{
		string::size_type start = 0;
		if(pos_class)
			start=pos_class+2;
		else
			start=0;
		functionname  = funcinfo.substr(start,pos_functionname-start);
	}
	string::size_type pos_params_start = pos_functionname + 1;
	string params_line;
	if(funcinfo[pos_params_start]!=')')
		params_line = funcinfo.substr(pos_params_start,funcinfo.size()-pos_params_start-1);
	if(params_line!="")
	{
		string::size_type pos;
		string pattern(",");

		params_line+=pattern;	
		bool find_map = false;

		if((funcinfo.find("::",pos_class+1))!=string::npos)
			find_map = true;
		string::size_type size=params_line.size();
		for(string::size_type i=0; i<size; i++)
		{
			pos=params_line.find(pattern,i);
			if(pos<size)
			{
				int count = 0;
				if(find_map)
				{
					for(string::size_type i = pos;i<params_line.length();i++)
					{
						if(params_line[i]=='<')
							count++;
						else if(params_line[i]=='>')
							count--;
					}
				}
				if(count<0)
				{
					pos=params_line.find(pattern,pos+1);
					if(!(pos<size))
						continue;
				}
				string s=params_line.substr(i,pos-i);
				trim(s);
				string::size_type pos_param_name;
				if((pos_param_name=s.rfind(" "))!=string::npos)
				{
					char c2 = s[pos_param_name+1];
					s=s.substr(0,pos_param_name);
					trim(s);
					char c = s[s.length()-1];
					if(c2=='&'||c2=='*')
					{
						s+=c2;
					}
					else if(c=='&'||c=='*')
					{
						s = s.substr(0,s.length()-1);
						trim(s);
						s+=c;
					}

				}
				//add by TSC  20141010
				if(s=="anyTypeTSC")
				{
					s="";
				}
				params.push_back(s);
				i=pos;
			}
		}
	}
	temp.classname = classname;
	temp.functionname = functionname;
	temp.params = params;
	return temp;
}

//from TSC 20141112 handle JumpMacros.ini
void dealconfig::readJumpMacros()
{
	std::string filePath = s_strDir + "JumpMacros.ini";
    fstream cfgFile(filePath.c_str());

    if(!cfgFile)
    {
        return;
    }

    char tmp[1000] = {0};

    while(!cfgFile.eof())
    {
        cfgFile.getline(tmp, 1000); //每行读取前1000个字符，1000个足够了
        //去掉注释
        if(tmp[0] == '#' || tmp[0] < 0)
        {
            continue;
        }
        string line(tmp);
        //去掉空行
        if(line.length() == 0)
        {
            continue;
        }
        trim(line);
        JUMPSTR=JUMPSTR+"|"+line;
        
    }

    cfgFile.close();
}


void dealconfig::DumpCfg()
{
	std::ofstream ofs;
	std::string sPath = CFileDependTable::GetProgramDirectory();
	sPath += "log/cfg.log";
	CFileDependTable::CreateLogDirectory();
	ofs.open(Path::toNativeSeparators(sPath).c_str(), std::ios_base::trunc);

	ofs << "[CustomNullPointerCheckList]" << std::endl;

	std::vector<CUSTOMNULLPOINTER>::iterator iter = m_CustomPointerlList.begin();
	while(iter != m_CustomPointerlList.end())
	{
		ofs << iter->functionname << "=" <<iter->var << std::endl;
		++iter;
	}

	ofs << std::endl;

	ofs << "[CustomNullPointerCheckList2]" << std::endl;
	std::vector<CANNOTBENULL>::iterator iter2 = f_CustomPointerlList.begin();
	while(iter2 != f_CustomPointerlList.end())
	{
		ofs << iter2->functionname << "=" <<iter2->var << std::endl;
		++iter2;
	}

	std::vector<SFuncSignature>::iterator iter3 = func_CustomPointerlList.begin();
	while(iter3 != func_CustomPointerlList.end())
	{
		ofs << iter3->functionname << std::endl;
		++iter3;
	}

	ofs << std::endl;

	ofs << "[JumpMacros]" << std::endl;
	ofs << JUMPSTR << std::endl;
	
	ofs.close();
}

void dealconfig::readConfig()
{
	std::string filePath = s_strDir + "cfg.ini";
	IniParser parser;
	if (!parser.Parse(filePath))
	{
		std::cout<< "Parsing [cfg.ini] failed, please check whether cfg.ini exist or the format is valid." << std::endl;
		return;
	}
	std::list<IniParser::SPair>* pListPair = NULL;
	if (parser.GetSection("CustomNullPointerCheckList", pListPair))
	{
		std::list<IniParser::SPair>::iterator iter = pListPair->begin();
		for (;iter != pListPair->end(); ++iter)
		{
			CUSTOMNULLPOINTER temp;
			int i = atoi(iter->sValue.c_str());
			if ( i != 0)
			{
				temp.functionname = iter->sKey;
				temp.var = i;
				m_CustomPointerlList.push_back(temp);
			}
		}
	}

	if (parser.GetSection("CustomNullPointerCheckList2", pListPair))
	{
		std::list<IniParser::SPair>::iterator iter = pListPair->begin();
		for (;iter != pListPair->end(); ++iter)
		{
			CANNOTBENULL temp;
			int i = atoi(iter->sValue.c_str());
			if ( i != 0)
			{
				temp.functionname = iter->sKey;
				temp.var = i;
				f_CustomPointerlList.push_back(temp);
			}
			else
			{
				SFuncSignature temp2;
				temp2 = parseFuncInfo(iter->sKey);
				func_CustomPointerlList.push_back(temp2);
			}
		}
	}

	if (parser.GetSection("JumpMacros", pListPair))
	{
		std::list<IniParser::SPair>::iterator iter = pListPair->begin();
		for (;iter != pListPair->end(); ++iter)
		{
			JUMPSTR=JUMPSTR+"|"+iter->sKey;
		}
	}

	if (writelog)
	{
		DumpCfg();
	}
}

bool IniParser::Parse(const std::string& iniPath)
{
	fstream iniFile(iniPath.c_str());

	if(!iniFile)
	{
		return false;
	}

	m_ini.clear();

	char line_buf[1000] = {0};
	std::string curSection;
	std::list<IniParser::SPair> listPair;
	while(!iniFile.eof())
	{
		iniFile.getline(line_buf, 1000); //每行读取前1000个字符，1000个足够了

		//去掉注释
		if(line_buf[0] == '#' || line_buf[0] <= 0)
			continue;

		std::string sLine(line_buf);
		trim_2(sLine);

		//去掉空行
		if(sLine.length() == 0)
			continue;

		if (sLine[0] == '[' && *sLine.rbegin() == ']')
		{
			m_ini[curSection] = listPair;
			curSection = sLine.substr(1, sLine.length() - 2);
			listPair.clear();
		}
		else
		{
			
			size_t pos = sLine.find('=');//找到每行的“=”号位置，之前是key之后是value
			if(pos == string::npos)
			{
				IniParser::SPair newPair = {sLine, ""};
				listPair.push_back(newPair);
			}
			else
			{
				std::string sKey = sLine.substr(0, pos); 
				trim_2(sKey);
				std::string sValue = sLine.substr(pos + 1); 
				trim_2(sValue);
				IniParser::SPair newPair = {sKey, sValue};
				listPair.push_back(newPair);
			}
		}
	}

	m_ini[curSection] = listPair;
	iniFile.close();

	return true;
}

bool IniParser::GetSection(const std::string& sName, std::list<SPair>*& pMapKeyValue)
{
	if (m_ini.count(sName))
	{
		pMapKeyValue = &m_ini[sName];
		return true;
	}
	return false;
}
