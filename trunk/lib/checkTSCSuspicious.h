
/*
* TscanCode - A tool for static C/C++ code analysis
* Copyright (C) 2013-2014 TSC team.
*/

//---------------------------------------------------------------------------


//---------------------------------------------------------------------------

#include "config.h"
#include "check.h"
#include "settings.h"
#include "tscancode.h"

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

class TSCANCODELIB CheckTSCSuspicious : public Check {
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
		
		checkTSCSuspicious.formatbufoverrun();
		checkTSCSuspicious.suspiciousArrayIndex();
		checkTSCSuspicious.checkSuspiciousPriority();
		checkTSCSuspicious.checkSuspiciousPriority2();
		checkTSCSuspicious.checkUnconditionalBreakinLoop();
		checkTSCSuspicious.checkIfCondition();
		checkTSCSuspicious.checkFuncReturn();
		checkTSCSuspicious.checksuspiciousfor();	
		checkTSCSuspicious.checksuspiciousStrErrorInLoop();
		checkTSCSuspicious.checkwrongvarinfor();
		checkTSCSuspicious.checkboolFuncReturn();
		checkTSCSuspicious.checkNestedLoop();
		checkTSCSuspicious.unsafeFunctionUsage();
		checkTSCSuspicious.checkRenameLocalVariable();
				
				
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
	}

	//rule
	/*sprintf_s( szMessage, CS_CHAT_MSG_LEN, "%s", pszCheckMsg );pszCheckMsg length is larger than szMessage*/
	void formatbufoverrun();
	void formatbufoverrunError(const Token *tok);

	/* for nested loop check   */
	void checkNestedLoop();
	void NestedLoopError(const Token *tok);

	/*for(int i=0;i<CS_MSG_LEN;i++){ array[j]=fun();} here array use wrong index j */
	void suspiciousArrayIndex();
	void suspiciousArrayIndexError(const Token *tok);

	/*%Check if Condition  with "="*/
	void checkIfCondition();
	void IfConditionError(const Token *tok);

	/*check bool return not bool*/
	void checkboolFuncReturn();
	void boolFuncReturnError(const Token* tok);
	
	/*for(int i=0;i<10;++j)error 　for(;a[k].length>k;k++)error */
	void checkwrongvarinfor();
	void wrongvarinforError(const Token *tok);
	void wrongvarinforError2(const Token *tok);
	
	/* while(i!=XXX){break;} */
	void checkUnconditionalBreakinLoop();
	void UnconditionalBreakinLoopError(const Token *tok);

	/* if(a==b || c==d && a==c) Priority of the '&&' operation is higher than that of the '||' operation,here my be a mistake  */
	void checkSuspiciousPriority();
	void SuspiciousPriorityError(const Token *tok);

	/*a=b<<i+2;The priority of the '+' operation is higher than that of the '<<' operation,here my be a mistake. */
	void checkSuspiciousPriority2();
	void SuspiciousPriorityError2(const Token *tok);
	
	/*Function return value is not being used*/
	void checkFuncReturn();
	void FuncReturnError(const Token* tok);

	/* @brief Check suspicious "for" */
	void checksuspiciousfor();
	void suspiciousforError(const Token *tok);

	void suspiciousWhileStrError(const Token *tok);

	/* check StringsStream.Str() in a dead Loop*/
	void checksuspiciousStrErrorInLoop();

	/*get,sprint... unsafeFunction */
	void unsafeFunctionUsage();
	void unsafeFunctionUsageError(const Token *tok, const std::string& unsafefuncname,const std::string& safefuncname);


	void checkRenameLocalVariable();
	void RenameLocalVariableError(const Token *tok);



};
/// @}

