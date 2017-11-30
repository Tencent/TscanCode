/*
 * TscanCode - A tool for static C/C++ code analysis
 * Copyright (C) 2017 TscanCode team.
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
#ifndef checkuninitvarH
#define checkuninitvarH
//---------------------------------------------------------------------------

#include "config.h"
#include "check.h"

class Scope;
class Variable;

/// @addtogroup Checks
/// @{


/** @brief Checking for uninitialized variables */

class TSCANCODELIB CheckUninitVar : public Check {
public:
    /** @brief This constructor is used when registering the CheckUninitVar */
    CheckUninitVar() : Check(myName()), bCheckCtor(false), tokCaller(nullptr){
    }

    /** @brief This constructor is used when running checks. */
    CheckUninitVar(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
        : Check(myName(), tokenizer, settings, errorLogger), bCheckCtor(false),bGoInner(false), bCtorIf(false), tokCaller(nullptr) {
    }

    /** @brief Run checks against the simplified token list */
    void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) {
        CheckUninitVar checkUninitVar(tokenizer, settings, errorLogger);
		checkUninitVar.check();
		checkUninitVar.deadPointer();
#ifdef TSCANCODE_RULE_OPEN
        //checkUninitVar.check();
        //checkUninitVar.deadPointer();
#endif
    }

    /** Check for uninitialized variables */
    void check();
    void checkScope(const Scope* scope);
    void checkStruct(const Token *tok, const Variable &structvar);
    enum Alloc { NO_ALLOC, NO_CTOR_CALL, CTOR_CALL, ARRAY };
    bool checkScopeForVariable(const Token *tok, const Variable& var, bool* const possibleInit, bool* const noreturn, Alloc* const alloc, const std::string &membervar, bool bPointer);
    bool checkIfForWhileHead(const Token *startparentheses, const Variable& var, bool suppressErrors, bool isuninit, Alloc alloc, const std::string &membervar, bool bPointer);
    bool checkLoopBody(const Token *tok, const Variable& var, Alloc &alloc, const std::string &membervar, bool bPointer, bool * const possibleInit, const bool suppressErrors, bool * const noreturn);
    void checkRhs(const Token *tok, const Variable &var, Alloc alloc, unsigned int number_of_if, const std::string &membervar, bool bPointer);
    int isVariableUsage(const Token *vartok, bool ispointer, Alloc alloc, const std::string &member = emptyString) const;
    int isFunctionParUsage(const Token *vartok, bool ispointer, Alloc alloc, const std::string &member) const;
    bool isMemberVariableAssignment(const Token *tok, const std::string &membervar, bool bPointer) const;
    int isMemberVariableUsage(const Token *tok, bool isPointer, Alloc alloc, const std::string &membervar, bool bPointer) const;
    /** ValueFlow-based checking for dead pointer usage */
    void deadPointer();
    void deadPointerError(const Token *pointer, const Token *alias);

    /* data for multifile checking */
    class MyFileInfo : public Check::FileInfo {
    public:
        /* functions that must have initialized data */
        std::set<std::string>  uvarFunctions;

        /* functions calls with uninitialized data */
        std::set<std::string>  functionCalls;
    };

    void uninitstringError(const Token *tok, const std::string &varname, bool strncpy_);
    void uninitdataError(const Token *tok, const std::string &varname, bool bPossible);
    void uninitvarError(const Token *tok, const std::string &varname, bool bPossible);
    void uninitvarError(const Token *tok, const std::string &varname, Alloc alloc, bool bPossible) {
        if (alloc == NO_CTOR_CALL || alloc == CTOR_CALL)
            uninitdataError(tok, varname, bPossible);
        else
            uninitvarError(tok, varname, bPossible);
    }
    void uninitStructMemberError(const Token *tok, const std::string &membername, bool bPossible);

private:
	void CheckCtor();
	const Token *FindGotoLabel(const std::string &label, const Token *tokStart) const;
	const Token *FindNextCase(const Token *tok) const;
	int checkScopeTokenForVariable(const Token * &tok, unsigned int &number_of_if, const Variable& var, 
		bool * const possibleInit, bool * const noreturn, Alloc* const alloc,
		const std::string &membervar, std::map<unsigned int, int>&, bool, bool);
private:
    void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const {
        CheckUninitVar c(0, settings, errorLogger);

        // error
        c.uninitstringError(0, "varname", false);
        c.uninitdataError(0, "varname", false);
        c.uninitvarError(0, "varname", false);
        c.uninitStructMemberError(0, "a.b", false);
        c.deadPointerError(0,0);
    }
	typedef std::map<unsigned int, std::list<std::pair<std::string, const Token *> > >::const_iterator CI_UMERR;
	std::map<unsigned int, std::list<std::pair<std::string, const Token *> > > _uninitMem;
	std::map<unsigned int, std::list<std::pair<std::string, const Token *> > > _uninitPossibleMem;
	void ReportUninitStructError(const std::string&, const Token* tok, bool bPossible, CI_UMERR iterErr, CI_UMERR endErr);
	bool CheckMemUseInFun(const Variable &var, const Function* fc, Alloc &alloc);
	const char *ErrorType(const Token * tok, bool bPossible) const;
	bool bCheckCtor;
	bool bGoInner;
	bool bCtorIf;
	bool HandleSpecialCase(const Token *tok);
	const Token *tokCaller; //for extra error info
    static std::string myName() {
        return "Uninitialized variables";
    }

    std::string classInfo() const {
        return "Uninitialized variables\n"
               "- using uninitialized local variables\n"
               "- using allocated data before it has been initialized\n"
               "- using dead pointer\n";
    }
};
/// @}
//---------------------------------------------------------------------------
#endif // checkuninitvarH
