#pragma once

#include "config.h"
#include "check.h"

struct FuncRetInfo;

class TSCANCODELIB CheckStatistic : public Check
{
public:

	CheckStatistic() : Check(myName())
	{
	}

	/** @brief This constructor is used when running checks. */
	CheckStatistic(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
		: Check(myName(), tokenizer, settings, errorLogger)
	{

	}

	void runChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
	{

	}

	/** @brief Run checks against the simplified token list */
	void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
	{
		CheckStatistic checkStatistic(tokenizer, settings, errorLogger);
		if (checkStatistic._settings->IsCheckIdOpened(ErrorType::ToString(ErrorType::NullPointer).c_str(), "funcRetNullStatistic"))
			checkStatistic.checkFuncRetNull();
	}

	std::string classInfo() const
	{
		return "Checks based on statistic";
	}

	void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const
	{

	}

	static std::string myName()
	{
		return "Statistic Checks";
	}

private:

	void checkFuncRetNull();
	bool IsDerefTok(const Token* tok, bool bLeft);
	FuncRetInfo checkVarUsageAfterFuncReturn(const Token* tokVar, const Token* tokAstFunc);
};











