#pragma once
#include "globalsymboldatabase.h"

struct SExprLocation;

class TSCANCODELIB CheckTSCOverrun : public Check
{
public:
	CheckTSCOverrun() : Check(myName())
	{ }

	/** @brief This constructor is used when running checks. */
	CheckTSCOverrun(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
		: Check(myName(), tokenizer, settings, errorLogger)
	{
		m_reportedError = nullptr;
	}

	void runChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
	{
	}



	/** @brief Run checks against the simplified token list */
	void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
	{
		CheckTSCOverrun checkTSCOverrun(tokenizer, settings, errorLogger);
		checkTSCOverrun.SetReportedErrors(m_reportedError);
		checkTSCOverrun.checkFuncRetLengthUsage();
		checkTSCOverrun.checkIndexPlusOneInFor();
		checkTSCOverrun.checkIndexBeforeCheck();
		checkTSCOverrun.checkIndexCheckDefect();
		checkTSCOverrun.checkMemcpy();
		checkTSCOverrun.checkStrncat();
	}

	std::string classInfo() const
	{
		return "TSC overrun checks";
	}

	void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const
	{
	}

	static std::string myName()
	{
		return "CheckTSCOverrun";
	}

	void SetReportedErrors(std::set<const Token*>* reportedErrors);

private:
	void checkFuncRetLengthUsage();

	void checkIndexPlusOneInFor();

	void checkIndexBeforeCheck();

	void checkIndexCheckDefect();

	bool CheckIndexTheSame(SExprLocation elIndex, const Token* tok);

	void checkIndexPlusOneInForByScope(const Scope* scope);

	void FuncRetLengthUsageError(const Token* tok, const std::string& func);

	void IndexPlusOneInForError(const Token* tok);

	void IndexBeforeCheckError(const Token* tok);

	void MemcpyError(const Token* tokFunc, const Token* tokVar, const Token* tokStr);

	void IndexCheckDefectError(const SExprLocation& elIndex, const SExprLocation& elArray, gt::CField::Dimension d, const Token* tokCond);

	bool IsScopeReturn(const Token* tok);

	gt::CField::Dimension GetDimensionInfo(const Token* tok);

	void checkMemcpy();

	void checkStrncat();
	void StrncatError(const Token* tokVar);

	static std::map<std::string, int> s_returnLengthFunc;
    
	std::set<const Token*>* m_reportedError;
};
