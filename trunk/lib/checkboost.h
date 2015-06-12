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
#ifndef CHECKBOOST_H
#define CHECKBOOST_H
//---------------------------------------------------------------------------

#include "config.h"
#include "check.h"

class Token;

/// @addtogroup Checks
/// @{


/** @brief %Check Boost usage */
class CPPCHECKLIB CheckBoost : public Check {
public:
    /** This constructor is used when registering the CheckClass */
    CheckBoost() : Check(myName())
    { }

    /** This constructor is used when running checks. */
    CheckBoost(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
        : Check(myName(), tokenizer, settings, errorLogger)
    { }

    /** Simplified checks. The token list is simplified. */
    void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) {
		(void)tokenizer;
        (void)settings;
        (void)errorLogger;
       #ifdef TSC_IGNORE_LOWCHECK
		;
#else
		if (tokenizer && !tokenizer->isCPP())
            return;

        CheckBoost checkBoost(tokenizer, settings, errorLogger);

        checkBoost.checkBoostForeachModification();
#endif
    }

    /** @brief %Check for container modification while using the BOOST_FOREACH macro */
    void checkBoostForeachModification();

private:
    void boostForeachError(const Token *tok);

    void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const {
        CheckBoost c(0, settings, errorLogger);
        c.boostForeachError(0);
    }

    static std::string myName() {
        return "Boostusage";
    }

    std::string classInfo() const {
        return "Check for invalid usage of Boost:\n"
               "* container modification during BOOST_FOREACH\n";
    }
};
/// @}
//---------------------------------------------------------------------------
#endif
