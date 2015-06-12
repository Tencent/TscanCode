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

class CPPCHECKLIB CheckTSCSuspicious : public Check {
public:
	
	/** @brief This constructor is used when registering the CheckClass */
	CheckTSCSuspicious() : Check(myName())
	{ }

	/** @brief This constructor is used when running checks. */
	CheckTSCSuspicious(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
		: Check(myName(), tokenizer, settings, errorLogger)
	{ }

	/** @brief Run checks against the normal token list */
	void runChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) {
		CheckTSCSuspicious checkTSCSuspicious(tokenizer, settings, errorLogger);

		if(settings && settings->recordFuncinfo())
		{
			checkTSCSuspicious.recordfuncinfo();
		}
			//TSC:from TSC 20140726
		if(getCheckConfig()->bufoverrun)
		{
			if(getCheckConfig()->formatbufoverrun)
			{
				checkTSCSuspicious.formatbufoverrun();
			}
		}

		if(getCheckConfig()->Suspicious)
		{
			//TSC 20140408
			if(getCheckConfig()->suspiciousArrayIndex)
			{
				checkTSCSuspicious.suspiciousArrayIndex();
			}
			// TSC:from TSC 20130709
			if(getCheckConfig()->SuspiciousPriority)
			{
				checkTSCSuspicious.checkSuspiciousPriority();
				checkTSCSuspicious.checkSuspiciousPriority2();
			}
			if(getCheckConfig()->UnconditionalBreakinLoop)
			{
				checkTSCSuspicious.checkUnconditionalBreakinLoop();
			}
			
			/* Suspect "=" within if Condition */
			
			if (getCheckConfig()->IfCondition)
			{
				checkTSCSuspicious.checkIfCondition();
			}
			//TSC 131114
			if (getCheckConfig()->FuncReturn)
			{
				checkTSCSuspicious.checkFuncReturn();
			}
					
			/* for(int a=4;a>3;a++)*/
			
			if(getCheckConfig()->suspiciousfor)
			{
				checkTSCSuspicious.checksuspiciousfor();	
			}
			// TSC:from TSC 20131024
			if(getCheckConfig()->wrongvarinfor)
			{
				checkTSCSuspicious.checkwrongvarinfor();
			}

			//TSC 2014.3.19
			if (getCheckConfig()->BoolFuncReturn)
			{
				checkTSCSuspicious.checkboolFuncReturn();
			}
			if (getCheckConfig()->nestedloop)
			{
				checkTSCSuspicious.checkNestedLoop();
			}
			
		}
				
				
	}

	/** @brief Run checks against the simplified token list */
	void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) {
		 	(void)tokenizer;
        (void)settings;
        (void)errorLogger;

	}

	static std::string myName() {
		return "CheckTSCSuspicious";
	}

	std::string classInfo() const {
		return "Check if there is TSCSuspicious issues:\n"
			"* bool return not bool\n"
			"*  SuspiciousPriority\n";
	}

	void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const {
		CheckTSCSuspicious c(0, settings, errorLogger);
		c.formatbufoverrunError(0);


	}

	//rule
	/* @TSC 20140507  sprintf_s( szMessage, CS_CHAT_MSG_LEN, "%s", pszCheckMsg );pszCheckMsg length is larger than szMessage*/
	void formatbufoverrun();
	void formatbufoverrunError(const Token *tok);

	/* @yy for nested loop check   */
	void checkNestedLoop();
	void NestedLoopError(const Token *tok);

	/* @TSC 20140408  for(int i=0;i<CS_MSG_LEN;i++){ array[j]=fun();} here array use wrong index j */
	void suspiciousArrayIndex();
	void suspiciousArrayIndexError(const Token *tok);

	/* @TSC 20140212 yulong xuqiu  if(p==NULL){ here not return}*/
	/*
	void checksuspiciouschecknull();
	void suspiciouschecknullError( const Token *tok );
	void suspiciouschecknullError2( const Token *tok );
	*/
	/* @TSC %Check if Condition  with "="*/
	void checkIfCondition();
	void IfConditionError(const Token *tok);

	/*check bool return not bool*/
	void checkboolFuncReturn();
	void boolFuncReturnError(const Token* tok);
	
	/* @TSC 20131024 for(int i=0;i<10;++j)error 　for(;a[k].length>k;k++)error */
	void checkwrongvarinfor();
	void wrongvarinforError(const Token *tok);
	void wrongvarinforError2(const Token *tok);
	
	/* @TSC 20130708 while(i!=XXX){break;} */
	void checkUnconditionalBreakinLoop();
	void UnconditionalBreakinLoopError(const Token *tok);

	/* @TSC 20130709 if(a==b || c==d && a==c) Priority of the '&&' operation is higher than that of the '||' operation,here my be a mistake  */
	void checkSuspiciousPriority();
	void SuspiciousPriorityError(const Token *tok);

	/* @TSC 20130709 a=b<<i+2;The priority of the '+' operation is higher than that of the '<<' operation,here my be a mistake. */
	void checkSuspiciousPriority2();
	void SuspiciousPriorityError2(const Token *tok);
	
	/* @TSC Function return value is not being used*/
	void checkFuncReturn();
	void FuncReturnError(const Token* tok);

	/* @brief Check suspicious "for" */
	void checksuspiciousfor();
	void suspiciousforError(const Token *tok);

	/* @TSC get,sprint... unsafeFunction */
	void unsafeFunctionUsage();
	void unsafeFunctionUsageError(const Token *tok, const std::string& unsafefuncname,const std::string& safefuncname);

	/* @TSC 20140623 get funcinfo notes cppname funcname funcstart_line funcend_line*/
	void recordfuncinfo();

};
/// @}

