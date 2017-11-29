/*
 * TscanCode - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2012 Daniel Marjamäki and TscanCode team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


//---------------------------------------------------------------------------
#ifndef CheckTSCInvalidVarArgsH
#define CheckTSCInvalidVarArgsH
//---------------------------------------------------------------------------

#include "config.h"
#include "check.h"
#include "token.h"
#include <stack>

class TSCANCODELIB CheckTSCInvalidVarArgs : public Check {
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

	static void InitFuncMap();

private:

	void CheckVarArgs_new();

	const Token * CheckParamCount(const std::string &strFormatted, const Token * tok, const std::string& strFunc, bool inconclusive, bool bPrintf);

	void CheckScope(const Token *tokBegin, const Token *tokEnd, const std::string &varname, unsigned int varid, std::string& strFormatted, bool& inconclusive);
	Token* GetCode(const Token *tokBegin, const Token *tokEnd, std::stack<const Token *>& assignStack, const unsigned int varid);

	static void addtoken(Token **rettail, const Token *tok, const std::string &str);

    void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const;
	void ReportErrorParamDismatch(const Token *tok, const std::string& funcName/*, bool inconclusive = false*/);

    static std::string myName() 
	{
        return "InvalidVarArgs";
    }

    std::string classInfo() const 
	{
		// TODO
        return "Invalid Variable Parameters";
    }

	int CountFormatStrParam(const std::string &str, bool bFormatString, bool bPrintf) const;
private:
	// restore the functions to be checked
	// key - the func name
	// value - the position index of the formatted string, started with 0
	static std::map<std::string, int> s_funcMap;
};
//---------------------------------------------------------------------------
#endif
