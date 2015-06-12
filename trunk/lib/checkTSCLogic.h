/*
* Tencent is pleased to support the open source community by making TscanCode available.
* Copyright (C) 2015 THL A29 Limited, a Tencent company. All rights reserved.
* Licensed under the GNU General Public License as published by the Free Software Foundation, version 3 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
* http://www.gnu.org/licenses/gpl.html
* TscanCode is free software: you can redistribute it and/or modify it under the terms of License.    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR * A PARTICULAR PURPOSE.  See the GNU General Public License for more details. 
*/


//---------------------------------------------------------------------------


//---------------------------------------------------------------------------

#include "config.h"
#include "check.h"
#include "settings.h"

class Token;
class Function;
class Variable;

/// @addtogroup Checks
/// @{
struct STfunclist
{
	std::string funcname;
	int argNum;
	std::vector<std::string> argType;
	STfunclist()
	{
		funcname="";
		argNum=0;
		argType.clear();
	}
};
/** @brief Various small checks */

class CPPCHECKLIB CheckTSCLogic : public Check {
public:
	/** @brief This constructor is used when registering the CheckClass */
	CheckTSCLogic() : Check(myName())
	{ }

	/** @brief This constructor is used when running checks. */
	CheckTSCLogic(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
		: Check(myName(), tokenizer, settings, errorLogger)
	{ }

	/** @brief Run checks against the normal token list */
	void runChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) {
		CheckTSCLogic CheckTSCLogic(tokenizer, settings, errorLogger);
		//time_t ts,te;
		//TIMELOG(
		if(getCheckConfig()->logic)
		{
			if(getCheckConfig()->RecursiveFunc)
			{
				CheckTSCLogic.checkRecursiveFunc();
			}

			if(getCheckConfig()->NoFirstCase)
			{
				CheckTSCLogic.checkNoFirstCase();	
			}

			if (getCheckConfig()->SwitchNoDefault)
			{
				CheckTSCLogic.checkSwitchNoDefault();
			}

			if (getCheckConfig()->SwitchNoBreakUP)
			{
				CheckTSCLogic.checkSwitchNoBreakUP();	
			}
		}
		//," CheckTSCLogic ",checklogfile)
			
	}

	/** @brief Run checks against the simplified token list */
	void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) {
		UNREFERENCED_PARAMETER(tokenizer);
		UNREFERENCED_PARAMETER(settings);
		UNREFERENCED_PARAMETER(errorLogger);

	}


	static std::string myName() {
		return "CheckTSCLogic";
	}

	std::string classInfo() const {
		return "Check if there is TSCLogic issues:\n"
			"* SwitchNoBreak\n"
			"* NoFirstCase\n";
	}

	void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const {
		CheckTSCLogic c(0, settings, errorLogger);
		c.RecursiveFuncError(0);


	}

	//rule

	/* @TSC Check for switch case without Break or Default */
	void checkSwitchNoBreak();
	void SwitchNoBreak(const Token *tok);
	void checkSwitchNoBreakUP();
	//report error if switch case without break
	void SwitchNoBreakUPError(const Token *tok);

	void checkSwitchNoDefault();
	//report error if switch case without Default
	void SwitchNoDefaultError(const Token *tok);
	//report error if switch case with more one Default
	void SwitchMoreDefaultError(const Token *tok);

	/* @TSC 20130708 switch { here missing first case} */
	void checkNoFirstCase();
	//report error if switch case without firstcase
	void NoFirstCaseError(const Token *tok);

	/* @TSC 20130709  myfunc(){  myfunc();} */
	void checkRecursiveFunc();
	//report error if RecursiveFunction call other function
	void RecursiveFuncError(const Token *tok);

};
/// @}

