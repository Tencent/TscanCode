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
#ifndef settingsH
#define settingsH
//---------------------------------------------------------------------------

#include <list>
#include <vector>
#include <string>
#include <set>
#include "config.h"
#include "library.h"
#include "suppressions.h"
#include "standards.h"
#include "timer.h"

/// @addtogroup Core
/// @{


/**
 * @brief This is just a container for general settings so that we don't need
 * to pass individual values to functions or constructors now or in the
 * future when we might have even more detailed settings.
 */
class TSCANCODELIB Settings {
public:
	static inline Settings *Instance()	{ if (!_instance) 	_instance = new Settings();	return _instance; }
	static void Destroy();
private:
	static Settings *_instance;
	//disable outter instance
	Settings();
	//disable copy
	Settings(const Settings& r) {};
	Settings & operator=(Settings&) { return *this; };
private:
    /** @brief Code to append in the checks */
    std::string _append;

    /** @brief enable extra checks by id */
    std::set<std::string> _enabled;

    /** @brief terminate checking */
    bool _terminate;
public:

	bool _no_analyze;
	bool _no_check;

	int _big_file_token_size;
	int _big_header_file_size;
	int _large_includes;

    /** @brief Is --debug given? */
    bool debug;

    /** @brief Is --debug-normal given? */
    bool debugnormal;

    /** @brief Is --debug-warnings given? */
    bool debugwarnings;

    /** @brief Is --debug-fp given? */
    bool debugFalsePositive;

	/** @brief Is --debug-dump-global given? */
	bool debugDumpGlobal;

	/** @brief Is --debug-dump-np given? */
	bool debugDumpNP;

	/** @brief Is --debug-dump-simp1 given? */
	bool debugDumpSimp1;

	/** @brief Is --debug-dump-simp2 given? */
	bool debugDumpSimp2;

    /** @brief Is --dump given? */
    bool dump;

    /** @brief Is --exception-handling given */
    bool exceptionHandling;

    /** @brief Inconclusive checks */
    bool inconclusive;

    /** @brief Collect unmatched suppressions in one run.
      * This delays the reporting until all files are checked.
      * It is needed by checks that analyse the whole code base. */
    bool jointSuppressionReport;

    /**
     * When this flag is false (default) then experimental
     * heuristics and checks are disabled.
     *
     * It should not be possible to enable this from any client.
     */
    bool experimental;

    /** @brief Is --quiet given? */
    bool quiet;

    /** @brief Is --inline-suppr given? */
    bool _inlineSuppressions;

    /** @brief Is --verbose given? */
    bool _verbose;

    /** @brief Request termination of checking */
    void terminate() {
        _terminate = true;
    }

    /** @brief termination requested? */
    bool terminated() const {
        return _terminate;
    }

    /** @brief Force checking the files with "too many" configurations (--force). */
    bool _force;

    /** @brief Use relative paths in output. */
    bool _relativePaths;

    /** @brief Paths used as base for conversion to relative paths. */
    std::vector<std::string> _basePaths;

    /** @brief write XML results (--xml) */
    bool _xml;

    /** @brief XML version (--xmlver=..) */
    int _xml_version;

    /** @brief How many processes/threads should do checking at the same
        time. Default is 1. (-j N) */
    unsigned int _jobs;

    /** @brief Load average value */
    unsigned int _loadAverage;

    /** @brief If errors are found, this value is returned from main().
        Default value is 0. */
    int _exitCode;

    /** @brief The output format in which the errors are printed in text mode,
        e.g. "{severity} {file}:{line} {message} {id}" */
    std::string _outputFormat;

    /** @brief show timing information (--showtime=file|summary|top5) */
    SHOWTIME_MODES _showtime;

    /** @brief List of include paths, e.g. "my/includes/" which should be used
        for finding include files inside source files. (-I) */
    std::list<std::string> _includePaths;

    /** @brief assign append code (--append) */
    bool append(const std::string &filename);

    /** @brief get append code (--append) */
    const std::string &append() const;

    /** @brief Maximum number of configurations to check before bailing.
        Default is 1. (--max-configs=N) */
    unsigned int _maxConfigs;

    /**
     * @brief Returns true if given id is in the list of
     * enabled extra checks (--enable)
     * @param str id for the extra check, e.g. "style"
     * @return true if the check is enabled.
     */
    template<typename T>
    bool isEnabled(T&& str) const {
        return bool(_enabled.find(str) != _enabled.end());
    }

    /**
     * @brief Enable extra checks by id. See isEnabled()
     * @param str single id or list of id values to be enabled
     * or empty string to enable all. e.g. "style,possibleError"
     * @return error message. empty upon success
     */
    std::string addEnabled(const std::string &str);

    /**
     * @brief Disables all severities, except from error.
     */
    void clearEnabled() {
        _enabled.clear();
    }

    enum Language {
        None, C, CPP
    };

    /** @brief Name of the language that is enforced. Empty per default. */
    Language enforcedLang;

    /** @brief suppress message (--suppressions) */
    Suppressions nomsg;

    /** @brief suppress exitcode */
    Suppressions nofail;

    /** @brief defines given by the user */
    std::string userDefines;

    /** @brief undefines given by the user */
    std::set<std::string> userUndefs;

    /** @brief forced includes given by the user */
    std::list<std::string> userIncludes;

    /** @brief include paths excluded from checking the configuration */
    std::set<std::string> configExcludePaths;


    /** @brief --report-progress */
    bool reportProgress;

    /** Library (--library) */
    Library library;

    /** Rule */
    class TSCANCODELIB Rule {
    public:
        Rule()
            : tokenlist("simple") // use simple tokenlist
            , id("rule")          // default id
            , severity("style") { // default severity
        }

        std::string tokenlist;
        std::string pattern;
        std::string id;
        std::string severity;
        std::string summary;
    };

    /**
     * @brief Extra rules
     */
    std::list<Rule> rules;

    /** Is the 'configuration checking' wanted? */
    bool checkConfiguration;

	/** checkLua dump cpp varInfo for lua */
	bool _checkLua;
	std::string luaPath;
	bool   checkLua() const {
		return _checkLua;
	}
    /** Check for incomplete info in library files? */
    bool checkLibrary;

    /** Struct contains standards settings */
    Standards standards;

    /** size of standard types */
    unsigned int sizeof_bool;
    unsigned int sizeof_short;
    unsigned int sizeof_int;
    unsigned int sizeof_long;
    unsigned int sizeof_long_long;
    unsigned int sizeof_float;
    unsigned int sizeof_double;
    unsigned int sizeof_long_double;
    unsigned int sizeof_wchar_t;
    unsigned int sizeof_size_t;
    unsigned int sizeof_pointer;

    enum PlatformType {
        Unspecified, // whatever system this code was compiled on
        Win32A,
        Win32W,
        Win64,
        Unix32,
        Unix64
    };

    /** platform type */
    PlatformType platformType;

    /** set the platform type for predefined platforms */
    bool platform(PlatformType type);

    /** set the platform type for user specified platforms */
    bool platformFile(const std::string &filename);

    /**
     * @brief Returns true if platform type is Windows
     * @return true if Windows platform type.
     */
    bool isWindowsPlatform() const {
        return platformType == Win32A ||
               platformType == Win32W ||
               platformType == Win64;
    }

    /**
     * @brief return true if a file is to be excluded from configuration checking
     * @return true for the file to be excluded.
     */
    bool configurationExcluded(const std::string &file) const {
        for (std::set<std::string>::const_iterator i=configExcludePaths.begin(); i!=configExcludePaths.end(); ++i) {
            if (file.length()>=i->length() && file.compare(0,i->length(),*i)==0) {
                return true;
            }
        }
        return false;
    }

	
	std::vector<std::string> _pathToIgnore;

	/**
	* int , short, etc. are considered the same where used in function signiture check
	*/
	std::set<std::string> _equalTypeForFuncSig;

	/**
	* function signiture load from cfg.xml that never return null
	*/
	std::multimap<std::string, std::pair<std::string, std::vector<std::string> > >  _functionNotRetNull;

	/**
	* typename load from cfg.xml that not need pass by Reference
	*/
	std::set<std::string>  _NonReferenceType;

	std::vector<std::string>  _customSelfMalloc;
	std::vector<std::string>  _customSelfDelloc;

	bool OpenCodeTrace;

	bool OpenAutofilter;


	/**
	* type name that is considered the same in function signiture match
	*/
	const std::string ANY_TYPE_T;
	/**
	* function or macro containing jump code
	* parameter index is supported
	*/
	std::map<std::string, std::set<int> > _jumpCode;
	bool CheckIfJumpCode(const std::string& str) const { return _jumpCode.find(str) != _jumpCode.end(); }
	/**
	* if file extension is ignored
	* load from cfg.xml width section name ignoreFileExtension
	*/
	std::vector<std::string> _extensionToIgnore;
	std::set<std::string> _headerExtension;

	/************************************************************************/
	/* settings indicate a check is open or not								*/
	/************************************************************************/
	std::map<std::string, std::map<std::string, bool> > _openedChecks;

	std::map<std::string, std::string> _checkSeverity;

	bool IsCheckIdOpened(const char* szId, const char* szSubid) const;

	std::string GetCheckSeverity(const std::string&  szSubid) const;
	/**
	* load project custom config
	* try open directly and then abs path
	* return true if success
	*/
	bool LoadCustomCfgXml(const std::string &strPath, const char szExeName[]);
	
	/**
	* return|break|continue|goto
	* consider to put it somewhere else
	*/
	const char* returnKeyWord;

	/**
	* write funcinfo.txt
	* 
	*/
	bool _recordFuncinfo;

	bool recordFuncinfo() const {

		return _recordFuncinfo;
	}

	// for vi plugin
	bool _e2o;

	bool Error2Output() const 
	{
		return _e2o;
	}

	/**
	* keyword to be erased in function return when analyzing function properties
	*/
	std::set<std::string> _keywordBeforeFunction;

	bool CheckIfKeywordInFuncRet(const std::string &strName) const { return _keywordBeforeFunction.count(strName) != 0; }

	std::set<std::string> _nonPtrType;

private:
	bool ParseFuncInfo(std::string&, std::pair<std::string, std::vector<std::string> >& funcNotRetNull, const std::string &funcinfo);
	const void trim(std::string &str) const;
	void split(std::vector<std::string> &out, const std::string &input, const std::string &delim) const;
};

/// @}
//---------------------------------------------------------------------------
#endif // settingsH
