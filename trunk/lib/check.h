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
 
#pragma once
#include "dealconfig.h"

#ifndef checkH
#define checkH
#include <fstream>
using namespace std;
#include "config.h"
#include "token.h"
#include "tokenize.h"
#include "settings.h"
#include "errorlogger.h"
#include "timer.h"

#include <list>
#include <iostream>
#include <set>
#include <string>   
static fstream checklogfile("check_time.log",ios::out);
#define TIMELOG(statements, recordStr, logFile) ts=time(NULL); \
	statements \
	te=time(NULL); \
	if(getCheckConfig()->writelog){ \
	logFile<<recordStr<<difftime(te,ts)<<" ";\
	}

#define LOG(statements, recordStr, logFile)  ts=time(NULL); \
	statements \
	te=time(NULL); \
	if(getCheckConfig()->writelog){ \
	logFile<<recordStr;\
	}


#define WINGUI
/// @addtogroup Core
/// @{

/**
 * @brief Interface class that cppcheck uses to communicate with the checks.
 * All checking classes must inherit from this class
 */
class CPPCHECKLIB Check {
public:

    /** This constructor is used when registering the CheckClass */
    explicit Check(const std::string &aname);

    /** This constructor is used when running checks. */
	Check(const std::string &aname, const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
		: _tokenizer(tokenizer), _settings(settings), _errorLogger(errorLogger), _name(aname)
	{
	}
	/*
    Check(const std::string &aname, const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
        : _tokenizer(tokenizer), _settings(settings), _errorLogger(errorLogger), _name(aname)
    { }
	*/

    virtual ~Check() {
#if !defined(DJGPP) && !defined(__sun)
        instances().remove(this);
#endif
    }

    /** List of registered check classes. This is used by Cppcheck to run checks and generate documentation */
    static std::list<Check *> &instances() {
        static std::list<Check *> _instances;
        return _instances;
    }

	 /**
     * analyse code - must be thread safe
     * @param tokens The tokens to analyse
     * @param result container where results are stored
     */
    virtual void analyse(const Token *tokens, std::set<std::string> &result) const {
        // suppress compiler warnings
        (void)tokens;
        (void)result;
    }

    /**
     * Save analysis data - the caller ensures thread safety
     * @param data The data where the results are saved
     */
    virtual void saveAnalysisData(const std::set<std::string> &data) const {
        // suppress compiler warnings
        (void)data;
    }

    /** run checks, the token list is not simplified */
    virtual void runChecks(const Tokenizer *, const Settings *, ErrorLogger *)
    { }

    /** run checks, the token list is simplified */
    virtual void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) = 0;

    /** get error messages */
    virtual void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const = 0;

    /** class name, used to generate documentation */
    const std::string& name() const {
        return _name;
    }

    /** get information about this class, used to generate documentation */
    virtual std::string classInfo() const = 0;

    /**
     * Write given error to errorlogger or to out stream in xml format.
     * This is for for printout out the error list with --errorlist
     * @param errmsg Error message to write
     */
    static void reportError(const ErrorLogger::ErrorMessage &errmsg) {
	
        std::cout << errmsg.toXML(true, 1) << std::endl;
    }

    bool inconclusiveFlag() const {
        return _settings && _settings->inconclusive;
    }

protected:
    const Tokenizer * const _tokenizer;
    const Settings * const _settings;
    ErrorLogger * const _errorLogger;

		 /** report an error from TSC 20130617*/
    void reportError(const Token *tok, const Severity::SeverityType severity, const std::string &id, const std::string &subid, const std::string &msg, bool inconclusive = false) {
        std::list<const Token *> callstack(1, tok);
		// TSC add funcinfo in errmsg 20140408
		std::string mymsg=(std::string)msg;
		if(tok && getCheckConfig()->showfuncinfo)
		{
			mymsg=((Token*)tok)->getfuncinfo()+":"+mymsg;
		}

		reportError(callstack, severity, id,subid, mymsg, inconclusive);
    }

	/** report an error*/
    void reportError(const std::list<const Token *> &callstack, Severity::SeverityType severity, const std::string &id, const std::string &subid,const std::string& msg, bool inconclusive = false) {
        ErrorLogger::ErrorMessage errmsg(callstack, _tokenizer?&_tokenizer->list:0, severity, id, subid,msg, inconclusive);
        if (_errorLogger)
            _errorLogger->reportErr(errmsg);
        else
            reportError(errmsg);
    }

	inline dealconfig* getCheckConfig()
	{
		return dealconfig::instance();
	}

private:
    const std::string _name;

    /** disabled assignment operator */
    void operator=(const Check &);
};

namespace std {
    /** compare the names of Check classes, used when sorting the Check descendants */
    template <> struct less<Check *> {
        bool operator()(const Check *p1, const Check *p2) const {
		    if(!p1 || !p2)
				return false;
            return (p1->name() < p2->name());
        }
    };
}

inline Check::Check(const std::string &aname)
    : _tokenizer(0), _settings(0), _errorLogger(0), _name(aname)
{
    instances().push_back(this);
    instances().sort(std::less<Check *>());
}

/// @}

#endif
