
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


#ifdef UNREFERENCED_PARAMETER
#undef UNREFERENCED_PARAMETER
#endif
#define UNREFERENCED_PARAMETER(P) (void)(P)

/// @addtogroup Checks
/// @{
struct STfunclist
{
	std::string funcname;
	int argNum;
	std::vector<std::string> argType;
	STfunclist()
	{
		funcname = "";
		argNum = 0;
		argType.clear();
	}
};
/** @brief Various small checks */

class TSCANCODELIB CheckTSCCompute : public Check {
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

		CheckTSCCompute.checkPreciseComparison();
		CheckTSCCompute.checkUnsignedlessthanzero();//from  TSC1.0 
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

	/**
	 * It's probably better to use a comparison with defined precision: fabs(A - B) <Epsilon or fabs(A - B) > Epsilon
	 */
	void checkPreciseComparison();
	void PreciseComparisonError(const Token *tok);

	void checkAssignAndCompute();
	void AssignAndComputeError(const Token *tok);


};
/// @}

