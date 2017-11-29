
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

class TSCANCODELIB CheckTSCLogic : public Check {
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
		CheckTSCLogic.checkRecursiveFunc();
		CheckTSCLogic.checkNoFirstCase();	
		CheckTSCLogic.checkSwitchNoDefault();
		CheckTSCLogic.checkSwitchNoBreakUP();	
		CheckTSCLogic.CheckUnintentionalOverflow();
		CheckTSCLogic.CheckReferenceParam();

		CheckTSCLogic.checkSTLFind();
		CheckTSCLogic.checkSignedUnsignedMixed();
		}
		
			

	/** @brief Run checks against the simplified token list */
	void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger);


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

	}

	void checkSwitchNoBreakUP();
	//report error if switch case without break
	void SwitchNoBreakUPError(const Token *tok);

	void checkSwitchNoDefault();
	//report error if switch case without Default
	void SwitchNoDefaultError(const Token *tok);
	//report error if switch case with more one Default
	void SwitchMoreDefaultError(const Token *tok);

	void checkNoFirstCase();
	//report error if switch case without firstcase
	void NoFirstCaseError(const Token *tok);

	void checkRecursiveFunc();

	void CheckCompareDefectInFor();
	
	

	void CheckUnintentionalOverflow();
	bool GetExprSize(const Token* tokStart, const Token* tokEnd, unsigned& size, const Token*& tokFlag);
	void UnintentionalOverflowError(const Token *tok, unsigned size, unsigned sizeVar);



	void CheckReferenceParam();



	void checkSTLFind();
	void checkSignedUnsignedMixed();

	void RecursiveFuncError(const Token *tok);

};
/// @}

