/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2012 Daniel Marjamäki and Cppcheck team.
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
 * The above software in this distribution may have been modified by THL A29 Limited (“Tencent Modifications”).
 * All Tencent Modifications are Copyright (C) 2015 THL A29 Limited.
 */


//---------------------------------------------------------------------------
#ifndef checknullpointerH
#define checknullpointerH
//---------------------------------------------------------------------------

#include "config.h"
#include "check.h"
#include "settings.h"

class Token;
class SymbolDatabase;

/// @addtogroup Checks
/// @{


/** @brief check for null pointer dereferencing */

class CPPCHECKLIB CheckNullPointer : public Check {
public:
	vector<CUSTOMNULLPOINTER> m_nullPointer;


		//vector<CUSTOMNULLPOINTER> m_NullPointerList;
    /** @brief This constructor is used when registering the CheckNullPointer */
    CheckNullPointer() : Check(myName())
    { }

    /** @brief This constructor is used when running checks. */
    CheckNullPointer(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
        : Check(myName(), tokenizer, settings, errorLogger)
    { }

    /** @brief Run checks against the normal token list */
    void runChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) {
		 	(void)tokenizer;
        (void)settings;
        (void)errorLogger;
		if(getCheckConfig()->nullpointer)
		{
        CheckNullPointer checkNullPointer(tokenizer, settings, errorLogger);
        checkNullPointer.nullPointer();
		}
		
    }

    /** @brief Run checks against the simplified token list */
    void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) {
		UNREFERENCED_PARAMETER(tokenizer);
		UNREFERENCED_PARAMETER(settings);
		UNREFERENCED_PARAMETER(errorLogger);

		//获取配置，如果配置了空指针检查，则执行如下检查
		//空指针检查
		if(getCheckConfig()->nullpointer)
		{
			 CheckNullPointer checkNullPointer(tokenizer, settings, errorLogger);
			 checkNullPointer.nullConstantDereference();
			 checkNullPointer.executionPaths();
		}
    }

    /**
     * @brief parse a function call and extract information about variable usage
     * @param tok first token
     * @param var variables that the function read / write.
     * @param value 0 => invalid with null pointers as parameter.
     *              non-zero => invalid with uninitialized data.
     */
    static void parseFunctionCall(const Token &tok,
                                  std::list<const Token *> &var,
                                  unsigned char value);

    /**
     * Is there a pointer dereference? Everything that should result in
     * a nullpointer dereference error message will result in a true
     * return value. If it's unknown if the pointer is dereferenced false
     * is returned.
     * @param tok token for the pointer
     * @param unknown it is not known if there is a pointer dereference (could be reported as a debug message)
     * @param symbolDatabase symbol database
     * @return true => there is a dereference
     */
    static bool isPointerDeRef(const Token *tok, bool &unknown, const SymbolDatabase* symbolDatabase);

    /** @brief possible null pointer dereference */
   // void nullPointer();
	void nullPointer();
    /**
     * @brief Does one part of the check for nullPointer().
     * Checking if pointer is NULL and then dereferencing it..
     */
	std::vector<Token*> CustomNullPointerCheck(Token *tok);
	Token* checkreturnnull(Token *tok);
	void readreturnnull();
    void nullPointerByCheckAndDeRef();

    /** @brief dereferencing null constant (after Tokenizer::simplifyKnownVariables) */
    void nullConstantDereference();

    /** @brief new type of check: check execution paths */
    void executionPaths();

    void nullPointerError(const Token *tok);  // variable name unknown / doesn't exist

    void nullPointerError(const Token *tok, const std::string &varname, const Token* nullcheck, bool inconclusive = false);
	
	int getFuncOutArgPointerIndex(const Scope* funcSc);
	int getVarIdOfFuncArg(int outArgIndex, const Token* tokFunc);
private:

    /** Get error messages. Used by --errorlist */
    void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const {
        CheckNullPointer c(0, settings, errorLogger);
        c.nullPointerError(0);
    }

    /** Name of check */
    static std::string myName() {
        return "Nullpointer";
    }

    /** class info in WIKI format. Used by --doc */
    std::string classInfo() const {
        return "Null pointers\n"
               "* null pointer dereferencing\n";
    }

    /**
     * @brief Does one part of the check for nullPointer().
     * looping through items in a linked list in a inner loop..
     */
    void nullPointerLinkedList();

    /**
     * @brief Does one part of the check for nullPointer().
     * Dereferencing a struct pointer and then checking if it's NULL..
     */
	// TSC:from TSC 2013-05-15
    //void nullPointerStructByDeRefAndChec();
	
	// TSC:from TSC 20140403
	void nullPointerChec();
    /**
     * @brief Does one part of the check for nullPointer().
     * Dereferencing a pointer and then checking if it's NULL..
     */
    void nullPointerByDeRefAndChec();

    /**
     * @brief Does one part of the check for nullPointer().
     * -# initialize pointer to 0
     * -# conditionally assign pointer
     * -# dereference pointer
     */
    void nullPointerConditionalAssignment();

    /**
     * @brief Investigate if function call can make pointer null. If
     * the pointer is passed by value it can't be made a null pointer.
     */
    bool CanFunctionAssignPointer(const Token *functiontoken, unsigned int varid, bool& unknown) const;
};
/// @}
//---------------------------------------------------------------------------
#endif
