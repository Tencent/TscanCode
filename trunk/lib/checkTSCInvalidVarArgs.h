/*
* Tencent is pleased to support the open source community by making TscanCode available.
* Copyright (C) 2015 THL A29 Limited, a Tencent company. All rights reserved.
* Licensed under the GNU General Public License as published by the Free Software Foundation, version 3 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
* http://www.gnu.org/licenses/gpl.html
* TscanCode is free software: you can redistribute it and/or modify it under the terms of License.    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR * A PARTICULAR PURPOSE.  See the GNU General Public License for more details. 
*/


//---------------------------------------------------------------------------
#ifndef CheckTSCInvalidVarArgsH
#define CheckTSCInvalidVarArgsH
//---------------------------------------------------------------------------

#include "config.h"
#include "check.h"
#include "token.h"
#include <stack>

class CPPCHECKLIB CheckTSCInvalidVarArgs : public Check {
public:
    /** This constructor is used when registering the CheckClass */
    CheckTSCInvalidVarArgs() : Check(myName())
    { }

    /** This constructor is used when running checks. */
    CheckTSCInvalidVarArgs(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
        : Check(myName(), tokenizer, settings, errorLogger)
    { }

    /** @brief Run checks against the normal token list */
    void runChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger);

    void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger);


private:

	void CheckVarArgs();

	const Token * CheckParamCount(const string &strFormatted, const Token * tok, const std::string& strFunc, bool inconclusive, bool bPrintf);

	void CheckScope(const Token *tokBegin, const Token *tokEnd, const std::string &varname, unsigned int varid, std::string& strFormatted, bool& inconclusive);
	Token* GetCode(const Token *tokBegin, const Token *tokEnd, std::stack<const Token *>& assignStack, const unsigned int varid);

	static void addtoken(Token **rettail, const Token *tok, const std::string &str);

    void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const;
	void ReportErrorParamDismatch(const Token *tok, const string& funcName, bool inconclusive = false);

    static std::string myName() 
	{
        return "InvalidVarArgs";
    }

    std::string classInfo() const 
	{
		// TODO
        return "Invalid Variable Parameters";
    }

	static void InitFuncMap();
private:
	// restore the functions to be checked
	// key - the func name
	// value - the position index of the formatted string, started with 0
	static std::map<std::string, int> s_funcMap;
};
//---------------------------------------------------------------------------
#endif
