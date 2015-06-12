
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

class CPPCHECKLIB CheckTSCCompute : public Check {
public:
	/** @brief This constructor is used when registering the CheckClass */
	CheckTSCCompute() : Check(myName())
	{ }

	/** @brief This constructor is used when running checks. */
	CheckTSCCompute(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
		: Check(myName(), tokenizer, settings, errorLogger)
	{ }

	/** @brief Run checks against the normal token list */
	void runChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) {
		CheckTSCCompute CheckTSCCompute(tokenizer, settings, errorLogger);
		//time_t ts,te;
		//TIMELOG(
			// TSC:from TSC 20130708
		if(getCheckConfig()->compute)
		{
			if(getCheckConfig()->PreciseComparison)
				CheckTSCCompute.checkPreciseComparison();
			if(getCheckConfig()->AssignAndCompute)
				CheckTSCCompute.checkAssignAndCompute();
			
			if(getCheckConfig()->Unsignedlessthanzero)
				CheckTSCCompute.checkUnsignedlessthanzero();
		}
		//,"CheckTSCompute ",checklogfile)
				
	}

	/** @brief Run checks against the simplified token list */
	void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) {
		UNREFERENCED_PARAMETER(tokenizer);
		UNREFERENCED_PARAMETER(settings);
		UNREFERENCED_PARAMETER(errorLogger);

	}

	static std::string myName() {
		return "CheckTSCCompute";
	}

	std::string classInfo() const {
		return "Check if there is TSCCompute issues:\n"
			"* PreciseComparison\n"
			"*  a=+b\n";
	}

	void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const {
		CheckTSCCompute c(0, settings, errorLogger);



	}

	/*check unsigned < 0*/
	void checkUnsignedlessthanzero();
	void UnsignedlessthanzeroError(const Token* tok);

	/* @TSC 20130708 The == or != operator is used to compare floating point numbers.
	 * It's probably better to use a comparison with defined precision: fabs(A - B) <Epsilon or fabs(A - B) > Epsilon 
	 */
	void checkPreciseComparison();
	void PreciseComparisonError(const Token *tok);

	/* @TSC 20130708 a=+b; it may be a+=b */
	void checkAssignAndCompute();
	void AssignAndComputeError(const Token *tok);


};
/// @}

