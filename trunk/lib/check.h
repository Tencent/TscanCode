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
#ifndef checkH
#define checkH
//---------------------------------------------------------------------------

#include "config.h"
#include "token.h"
#include "tokenize.h"
#include "settings.h"
#include "errorlogger.h"

#include <list>
#include <set>


/// @addtogroup Core
/// @{

/**
 * @brief Interface class that tscancode uses to communicate with the checks.
 * All checking classes must inherit from this class
 */
class TSCANCODELIB Check {
public:
    /** This constructor is used when registering the CheckClass */
    explicit Check(const std::string &aname);

    /** This constructor is used when running checks. */
    Check(const std::string &aname, const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
        : _tokenizer(tokenizer), _settings(settings), _errorLogger(errorLogger), _name(aname), _lastFuncScope(nullptr) {
    }

    virtual ~Check() {
        if (!_tokenizer)
            instances().remove(this);
    }

    /** List of registered check classes. */
    static std::list<Check *> &instances();

    /** run checks, the token list is not simplified */
    virtual void runChecks(const Tokenizer *, const Settings *, ErrorLogger *) {
    }

    /** run checks, the token list is simplified */
    virtual void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) = 0;
    
    
    /** run analysis, the token list is not simplified */
    virtual void runAnalysis(const Tokenizer *, const Settings *, ErrorLogger *) {}
    
    /** run analysis, the token list is simplified */
    virtual void runSimplifiedAnalysis(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) {}

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
    static void reportError(const ErrorLogger::ErrorMessage &errmsg);

    bool inconclusiveFlag() const {
        return _settings && _settings->inconclusive;
    }

    /** Base class used for whole-program analysis */
    class FileInfo {
    public:
        FileInfo() {}
        virtual ~FileInfo() {}
    };

    virtual FileInfo * getFileInfo(const Tokenizer *tokenizer, const Settings *settings) const {
        (void)tokenizer;
        (void)settings;
        return nullptr;
    }

    virtual void analyseWholeProgram(const std::list<FileInfo*> &fileInfo, const Settings& settings, ErrorLogger &errorLogger) {
        (void)fileInfo;
        (void)settings;
        (void)errorLogger;
    }

protected:
    const Tokenizer * const _tokenizer;
    const Settings * const _settings;
    ErrorLogger * const _errorLogger;


	template<typename T, typename U, typename I>
	void reportError(const Token *tok, const Severity::SeverityType severity, ErrorType::ErrorTypeEnum type, const T& id, const U& msg,
			const I& identity) {
		reportError(tok, severity, type, id, msg, 0U, false, identity);
	}

    /** report an error */
    template<typename T, typename U>
	void reportError(const Token *tok, const Severity::SeverityType severity, ErrorType::ErrorTypeEnum type, const T id, const U msg, unsigned int cwe, bool inconclusive) {
        std::list<const Token *> callstack(1, tok);
        reportError(callstack, severity, type, id, msg, cwe, inconclusive);
    }
	template<typename T, typename U, typename I>
	void reportError(const Token *tok, const Severity::SeverityType severity, 
		ErrorType::ErrorTypeEnum type, const T id, const U msg, unsigned int cwe, bool inconclusive, const I& identity) {
		//add isMacro 
		bool isMacro = false;
		isMacro = tok->isExpandedMacro();
		std::list<const Token *> callstack(1, tok);
		reportError(callstack, severity, type, id, msg, cwe, inconclusive, identity, isMacro);
	}

    /** report an error */
    template<typename T, typename U>
	void reportError(const std::list<const Token *> &callstack, Severity::SeverityType severity, ErrorType::ErrorTypeEnum type, const T id, const U msg) {
        reportError(callstack, severity, type, id, msg, 0U, false);
    }


	template<typename T, typename U, typename I>
	void reportError(const std::list<const Token *> &callstack, Severity::SeverityType severity, ErrorType::ErrorTypeEnum type, const T id, const U msg,
		const I& identity) {
		reportError(callstack, severity, type, id, msg, 0U, false, identity,false);
	}

    /** report an error */
    template<typename T, typename U>
	void reportError(const std::list<const Token *> &callstack, Severity::SeverityType severity, ErrorType::ErrorTypeEnum type, const T id, const U msg, unsigned int cwe, bool inconclusive) {
        ErrorLogger::ErrorMessage errmsg(callstack, _tokenizer?&_tokenizer->list:0, severity, type, id, msg, inconclusive);
        errmsg._cwe = cwe;
		if (!callstack.empty())
		{
			errmsg._funcinfo = GetTokenFuncInfo(*callstack.begin());
		}
        if (_errorLogger)
            _errorLogger->reportErr(errmsg);
        else
            reportError(errmsg);
    }

	template<typename T, typename U, typename I>
	void reportError(const std::list<const Token *> &callstack, Severity::SeverityType severity, ErrorType::ErrorTypeEnum type,
		const T id, const U msg, unsigned int cwe, bool inconclusive, const I& identity,bool isMacro) {
	
		ErrorLogger::ErrorMessage errmsg(callstack, _tokenizer ? &_tokenizer->list : 0, severity, type, id, msg, inconclusive);
		errmsg._cwe = cwe;
		errmsg.SetWebIdentity(identity);
		if (isMacro)
		{
			errmsg.setmsg(errmsg.shortMessage() + " The error is in macros.");
		}
		if (!callstack.empty())
		{
			errmsg._funcinfo = GetTokenFuncInfo(*callstack.begin());
		}
		if (_errorLogger)
			_errorLogger->reportErr(errmsg);
		else
			reportError(errmsg);
	}
	
private:
    const std::string _name;

    /** disabled assignment operator and copy constructor */
    void operator=(const Check &);
    explicit Check(const Check &);

	const std::string&  GetTokenFuncInfo(const Token* tok);
	const Scope *_lastFuncScope;
	std::string _lastFuncName;
};

/// @}
//---------------------------------------------------------------------------
#endif //  checkH
