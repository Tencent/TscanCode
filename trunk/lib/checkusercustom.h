#pragma once


#include "config.h"
#include "check.h"


class TSCANCODELIB CheckUserCustom : public Check 
{
public:
	CheckUserCustom() : Check(myName()) 
	{
	}

	/** @brief This constructor is used when running checks. */
	CheckUserCustom(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
		: Check(myName(), tokenizer, settings, errorLogger)
	{

	}

	void runChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
	{

	}

	/** @brief Run checks against the simplified token list */
	void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
	{
		CheckUserCustom checkUserCustom(tokenizer, settings, errorLogger);
		if (checkUserCustom._settings->IsCheckIdOpened(ErrorType::ToString(ErrorType::UserCustom).c_str(), "WeShoot_SizeUnmatch"))
			checkUserCustom.checkWeShoot_AssignTypeSizeUnmatch();
        if (checkUserCustom._settings->IsCheckIdOpened(ErrorType::ToString(ErrorType::UserCustom).c_str(), "SGame_PointerMember") ||
            checkUserCustom._settings->IsCheckIdOpened(ErrorType::ToString(ErrorType::UserCustom).c_str(), "SGame_PointerMemberTemplate") )
            checkUserCustom.checkSGame_PointerMember();
		if (checkUserCustom._settings->IsCheckIdOpened(ErrorType::ToString(ErrorType::UserCustom).c_str(), "NonThreadSafeFunc"))
			checkUserCustom.checkNonThreadSafeFunc();
		checkUserCustom.CheckSSK_UnCheckedIndex();

		if (checkUserCustom._settings->IsCheckIdOpened(ErrorType::ToString(ErrorType::Logic).c_str(), "info_sgamePossibleAssignZero") ||
			checkUserCustom._settings->IsCheckIdOpened(ErrorType::ToString(ErrorType::Logic).c_str(), "info_sgameAssignZero") ||
			checkUserCustom._settings->IsCheckIdOpened(ErrorType::ToString(ErrorType::Logic).c_str(), "warning_sgameAssignZero") ||
			checkUserCustom._settings->IsCheckIdOpened(ErrorType::ToString(ErrorType::Logic).c_str(), "warning_sgameAssignZero_64bit") ||
			checkUserCustom._settings->IsCheckIdOpened(ErrorType::ToString(ErrorType::Logic).c_str(), "warning_sgameAssignZero_TArray"))
		{
			checkUserCustom.checkSgameAssignZeroContinuously();
		}
		checkUserCustom.SGameClientCheck();
		if (settings->IsCheckIdOpened(ErrorType::ToString(ErrorType::UserCustom).c_str(), "structSpaceWasted"))
			checkUserCustom.checkStructSpaceWasted();
	}

	std::string classInfo() const
	{
		return "User Custom Checks";
	}

	void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const
	{

	}

	static std::string myName() 
	{
		return "User Custom Checks";
	}

private:
	void checkWeShoot_AssignTypeSizeUnmatch();
	void checkWeShoot_AssignTypeSizeUnmatch_Error(const Token* tok, bool isCast, bool bPossible);
    
    
    void checkSGame_PointerMember();
    void checkSGame_PointerMember_Error(const Token* tok, bool bTemplate);

	void CheckSSK_UnCheckedIndex();

	static std::set<std::string> s_nonThreadSafeFunc;
	void checkNonThreadSafeFunc();
	void NonThreadSafeFuncError(const Token* tok);

	//report error if RecursiveFunction call other function
	
	void SGameServerCheck();
	void SGameCheckCond();
	void SGameCheckCondInternal(const Token* tokCond, const Token* tokStart);

	void SGameForLoopVarType();
	// check argument count of tlog_log
	void SGameArgumentCountOfTlog_log();

	void SGamePlusOperatorTypeCheck();

	void SGameClientCheck();
	void SGameClient_FloatCheck();
	void SGameClient_SortCheck();
	void SGameClient_RandomCheck();
	void SGameStructAlignCheck();
	void checkSgameAssignZeroContinuously();
	bool IsDerivedFromPooledObject(const Type* pType);
	void checkStructSpaceWasted();
	bool TryGetVarUnsigned(const Token* tokVar, bool& bUnsigned);
};











