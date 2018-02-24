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
#include "tscancode.h"

#include "preprocessor.h" // Preprocessor
#include "tokenize.h" // Tokenizer

#include "check.h"
#include "path.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include "timer.h"
#include "version.h"

#ifdef HAVE_RULES
#define PCRE_STATIC
#include <pcre.h>
#endif

#include "globaltokenizer.h"
#include "filelister.h"

#include "tinyxml2.h"
using namespace tinyxml2;

static const char Version[] = TSCANCODE_VERSION_STRING;
static const char ExtraVersion[] = "";

static TimerResults S_timerResults;

TscanCode::TscanCode(ErrorLogger &errorLogger, bool useGlobalSuppressions)
    : _errorLogger(errorLogger), exitcode(0), _useGlobalSuppressions(useGlobalSuppressions), tooManyConfigs(false)
	, _settings(*Settings::Instance()), _simplify(true)
{
	
}

TscanCode::~TscanCode()
{
    while (!fileInfo.empty()) {
        delete fileInfo.back();
        fileInfo.pop_back();
    }
    S_timerResults.ShowResults(_settings._showtime);
}

const char * TscanCode::version()
{
    return Version;
}

const char * TscanCode::extraVersion()
{
    return ExtraVersion;
}

unsigned int TscanCode::check(const std::string &path)
{
    std::ifstream fin(path.c_str());
    return processFile(path, fin);
}

unsigned int TscanCode::check(const std::string &path, const std::string &content)
{
    std::istringstream iss(content);
    return processFile(path, iss);
}

unsigned int TscanCode::analyze(const std::string &path)
{
    std::ifstream fin(path.c_str());
    return analyzeFile(fin, path);
}

unsigned int TscanCode::analyze(const std::string &path, const std::string &content)
{
    std::istringstream iss(content);
    return analyzeFile(iss, path);
}

void TscanCode::replaceAll(std::string& code, const std::string &from, const std::string &to)
{
    std::size_t pos = 0;
    while ((pos = code.find(from, pos)) != std::string::npos) {
        code.replace(pos, from.length(), to);
        pos += to.length();
    }
}

void TscanCode::SyncErrorList(std::list<std::string>& errorList)
{
	errorList.insert(errorList.end(), _errorList.begin(), _errorList.end());
}

std::set<CCodeFile*>& TscanCode::GetLargeHeaderSet()
{
	return _largeHeaderSet;
}

XMLElement* findSection(XMLDocument& doc, const char* szName)
{
	
	XMLElement* secNode = doc.RootElement()->FirstChildElement("section");

	while (secNode && strcmp(secNode->Name(), "section") == 0 )
	{
		const char* secName = secNode->Attribute("name");
		if (!secName)
		{
			secNode = secNode->NextSiblingElement();
			continue;
		}

		if (strcmp(secName, szName) == 0)
		{
			return secNode;
		}
		secNode = secNode->NextSiblingElement();
	}
	return NULL;
}

XMLElement* findName(XMLElement* section, const char* szName, const char* szNodeName = NULL)
{
	XMLElement* child = section->FirstChildElement(szNodeName);
	while (child)
	{
		const char* childName = child->Attribute("name");
		if (!childName)
		{
			child = child->NextSiblingElement(szNodeName);
			continue;
		}

		if (strcmp(childName, szName) == 0)
		{
			return child;
		}
		child = child->NextSiblingElement(szNodeName);
	}

	return NULL;
}

bool mergeStrategy_Direct(XMLDocument& doc, XMLElement* oldSection, XMLElement* newSection, bool bUseOld = false)
{
	const char* szOldSecValue = oldSection->Attribute("value");
	const char* szNewSecValue = newSection->Attribute("value");
	if (szOldSecValue && szNewSecValue)
	{
		if (strcmp(szOldSecValue, szNewSecValue) != 0)
		{
			oldSection->Attribute("value", bUseOld ? szOldSecValue : szOldSecValue);
		}
	}

	XMLElement* newChild = newSection->FirstChildElement();

	while (newChild)
	{
		const char* newChildName = newChild->Attribute("name");
		if (!newChildName)
		{
			newChild = newChild->NextSiblingElement();
			continue;
		}

		XMLElement* oldChild = findName(oldSection, newChildName);

		if (!oldChild)
		{
			XMLElement* newNode = doc.NewElement(newChild->Name());
			newNode->SetAttribute("name", newChildName);
			if (const char* szValue = newChild->Attribute("value"))
			{
				newNode->SetAttribute("value", szValue);
			}
			if (const char* szValue = newChild->Attribute("severity"))
			{
				newNode->SetAttribute("severity", szValue);
			}
			if (const char* szValue = newChild->Attribute("desc"))
			{
				newNode->SetAttribute("desc", szValue);
			}
			oldSection->InsertEndChild(newNode);
		}
		else
		{
			if (!bUseOld)
			{
				const char* szOldValue = oldChild->Attribute("value");
				const char* szNewValue = newChild->Attribute("value");
				if (szOldValue && szNewValue)
				{
					if (strcmp(szOldValue, szNewValue) != 0)
					{
						oldChild->Attribute("value", szNewValue);
					}
				}
			}
		}
		newChild = newChild->NextSiblingElement();
	}

	return true;
}

bool mergeStrategy_Check(XMLDocument& doc, XMLElement* oldSection, XMLElement* newSection)
{
	XMLElement* newid = newSection->FirstChildElement("id");

	while (newid)
	{
		const char* newIDName = newid->Attribute("name");
		if (!newIDName)
		{
			newid = newid->NextSiblingElement("id");
			continue;
		}

		XMLElement* oldID = findName(oldSection, newIDName, "id");

		if (!oldID)
		{
			XMLElement* newNode = doc.NewElement(newid->Name());
			newNode->SetAttribute("name", newIDName);
			if (const char* szValue = newid->Attribute("value"))
			{
				newNode->SetAttribute("value", szValue);
			}
			mergeStrategy_Direct(doc, newNode, newid, true);//ignore TSC
			oldSection->InsertEndChild(newNode);
		}
		else
		{
			mergeStrategy_Direct(doc, oldID, newid, true);//ignore TSC
		}
		newid = newid->NextSiblingElement("id");
	}

	return true;
}

bool mergeSection(XMLDocument& doc, XMLElement* oldSection, XMLElement* newSection)
{
	const char* szOldSecName = oldSection->Attribute("name");
	const char* szNewSecName = newSection->Attribute("name");
	if (!szOldSecName || !szNewSecName)
	{
		return false;
	}

	if (strcmp(szOldSecName, szNewSecName) != 0)
	{
		return false;
	}

	if (strcmp(szOldSecName, "PathToIgnore") == 0)
	{
		mergeStrategy_Direct(doc, oldSection, newSection);
	}
	else if (strcmp(szOldSecName, "JumpCode") == 0)
	{
		mergeStrategy_Direct(doc, oldSection, newSection);
	}
	else if (strcmp(szOldSecName, "IgnoreFileExtension") == 0)
	{
		mergeStrategy_Direct(doc, oldSection, newSection);
	}
	else if (strcmp(szOldSecName, "FunctionNotReturnNull") == 0)
	{
		mergeStrategy_Direct(doc, oldSection, newSection);
	}
	else if (strcmp(szOldSecName, "NonReferenceType") == 0)
	{
		mergeStrategy_Direct(doc, oldSection, newSection);
	}
	else if (strcmp(szOldSecName, "PerformanceCfg") == 0)
	{
		mergeStrategy_Direct(doc, oldSection, newSection, true);//ignore TSC
	}
	else if (strcmp(szOldSecName, "Checks") == 0)
	{
		mergeStrategy_Check(doc, oldSection, newSection);//ignore TSC
	}
	else
	{
		mergeStrategy_Direct(doc, oldSection, newSection);
	}


	return true;
}

bool TscanCode::mergeCfgXml(const std::string& cfgNew, const std::string& cfgOld)
{
	if (!FileLister::fileExists(cfgNew))
	{
		return false;
	}

	if (!FileLister::fileExists(cfgOld))
	{
		Path::CopyFile2(cfgNew, cfgOld);
		return true;
	}

	XMLDocument docOld, docNew;

	XMLError error = docOld.LoadFile(cfgOld.c_str());
	if (error != XML_SUCCESS)
	{
		return false;
	}

	error = docNew.LoadFile(cfgNew.c_str());
	if (error != XML_SUCCESS)
	{
		return false;
	}

	XMLElement* secNewNode = docNew.RootElement()->FirstChildElement("section");

	while (secNewNode)
	{
		const char* secNewName = secNewNode->Attribute("name");
		if (!secNewName)
		{
			secNewNode = secNewNode->NextSiblingElement();
			continue;
		}

		XMLElement* secOldNode = findSection(docOld, secNewName);

		if (!secOldNode)
		{
			secNewNode = secNewNode->NextSiblingElement();
			continue;
		}

		mergeSection(docOld, secOldNode, secNewNode);//ignore TSC
		secNewNode = secNewNode->NextSiblingElement();
	}

	return (docOld.SaveFile(cfgOld.c_str()) == XML_NO_ERROR);
}
bool TscanCode::findError(std::string code, const char FileName[])
{
    std::set<unsigned long long> checksums;
    // First make sure that error occurs with the original code
    bool internalErrorFound(false);
    checkFile(code, FileName, checksums, internalErrorFound);//ignore TSC
    if (_errorList.empty()) {
        // Error does not occur with this code
        return false;
    }

    const std::string previousCode = code;
    std::string error = _errorList.front();
    for (;;) {

        // Try to remove included files from the source
        const std::size_t found = previousCode.rfind("\n#endfile");
        if (found == std::string::npos) {
            // No modifications can be done to the code
        } else {
            // Modify code and re-check it to see if error
            // is still there.
            code = previousCode.substr(found+9);
            _errorList.clear();
            checksums.clear();
            checkFile(code, FileName, checksums, internalErrorFound);//ignore TSC
        }

        if (_errorList.empty()) {
            // Latest code didn't fail anymore. Fall back
            // to previous code
            code = previousCode;
        } else {
            error = _errorList.front();
        }

        // Add '\n' so that "\n#file" on first line would be found
        code = "// " + error + "\n" + code;
        replaceAll(code, "\n#file", "\n// #file");
        replaceAll(code, "\n#endfile", "\n// #endfile");

        // We have reduced the code as much as we can. Print out
        // the code and quit.
        _errorLogger.reportOut(code);
        break;
    }

    return true;
}

unsigned int TscanCode::processFile(const std::string& filename, std::istream& fileStream)
{
    exitcode = 0;

    // only show debug warnings for accepted C/C++ source files
    if (!Path::acceptFile(filename))
        _settings.debugwarnings = false;

    if (_settings.terminated())
        return exitcode;

    bool internalErrorFound(false);
    try {
        Preprocessor preprocessor(_settings, this);
        std::list<std::string> configurations;
        std::string filedata;

        {
            Timer t("Preprocessor::preprocess", _settings._showtime, &S_timerResults);
            preprocessor.preprocess(fileStream, filedata, configurations, filename, _settings._includePaths);
        }

        if (_settings.checkConfiguration) {
            return 0;
        }

        // Run rules on this code
        for (std::list<Settings::Rule>::const_iterator it = _settings.rules.begin(); it != _settings.rules.end(); ++it) {
            if (it->tokenlist == "define") {
                Tokenizer tokenizer2(&_settings, this);
                std::istringstream istr2(filedata);
                tokenizer2.list.createTokens(istr2, filename);

                for (const Token *tok = tokenizer2.list.front(); tok; tok = tok->next()) {
                    if (tok->str() == "#define") {
                        std::string code = std::string(tok->linenr()-1U, '\n');
                        for (const Token *tok2 = tok; tok2 && tok2->linenr() == tok->linenr(); tok2 = tok2->next())
                            code += " " + tok2->str();
                        Tokenizer tokenizer3(&_settings, this);
                        std::istringstream istr3(code);
                        tokenizer3.list.createTokens(istr3, tokenizer2.list.file(tok));
                        executeRules("define", tokenizer3);
                    }
                }
                break;
            }
        }

        if (!_settings.userDefines.empty() && _settings._maxConfigs==1U) {
            configurations.clear();
            configurations.push_back(_settings.userDefines);
        }

        if (!_settings._force && configurations.size() > _settings._maxConfigs) {
            if (_settings.isEnabled("information")) {
                tooManyConfigsError(Path::toNativeSeparators(filename),configurations.size());
            } else {
                tooManyConfigs = true;
            }
        }

        std::set<unsigned long long> checksums;
        unsigned int checkCount = 0;
        for (std::list<std::string>::const_iterator it = configurations.begin(); it != configurations.end(); ++it) {
            // Check only a few configurations (default 12), after that bail out, unless --force
            // was used.
            if (!_settings._force && ++checkCount > _settings._maxConfigs)
                break;

            cfg = *it;

            if (!_settings.userDefines.empty()) {
                if (!cfg.empty())
                    cfg = ";" + cfg;
                cfg = _settings.userDefines + cfg;
            }

            Timer t("Preprocessor::getcode", _settings._showtime, &S_timerResults);
            std::string codeWithoutCfg = preprocessor.getcode(filedata, cfg, filename);
            t.Stop();

            codeWithoutCfg += _settings.append();

			if (!checkFile(codeWithoutCfg, filename.c_str(), checksums, internalErrorFound)) {
				if (_settings.isEnabled("information") && _settings._verbose)
					purgedConfigurationMessage(filename, cfg);
			}
            
        }
    } catch (const std::runtime_error &e) {
        internalError(filename, e.what());
    } catch (const InternalError &e) {
        internalError(filename, e.errorMessage);
        exitcode=1; // e.g. reflect a syntax error
    }

    // In jointSuppressionReport mode, unmatched suppressions are
    // collected after all files are processed
    if (!_settings.jointSuppressionReport && (_settings.isEnabled("information") || _settings.checkConfiguration)) {
        reportUnmatchedSuppressions(_settings.nomsg.getUnmatchedLocalSuppressions(filename, unusedFunctionCheckIsEnabled()));
    }

    //_errorList.clear();
    if (internalErrorFound && (exitcode==0)) {
        exitcode=1;
    }
    return exitcode;
}

void TscanCode::internalError(const std::string &filename, const std::string &msg)
{
    const std::string fixedpath = Path::toNativeSeparators(filename);
    const std::string fullmsg("Bailing out from checking " + fixedpath + " since there was an internal error: " + msg);

    if (_settings.isEnabled("information")) {
        const ErrorLogger::ErrorMessage::FileLocation loc1(filename, 0);
        std::list<ErrorLogger::ErrorMessage::FileLocation> callstack;
        callstack.push_back(loc1);

        ErrorLogger::ErrorMessage errmsg(callstack,
                                         Severity::information,
                                         fullmsg,
										 ErrorType::None,
                                         "internalError",
                                         false);

        _errorLogger.reportErr(errmsg);

    } else {
        // Report on stdout
        _errorLogger.reportOut(fullmsg);
    }
}

unsigned int TscanCode::analyzeFile(std::istream &fileStream, const std::string &filename)
{ 
    exitcode = 0;
    
    // only show debug warnings for accepted C/C++ source files
    if (!Path::acceptFile(filename))
        _settings.debugwarnings = false;
    
    if (_settings.terminated())
        return exitcode;
    
    bool internalErrorFound(false);
    try {
        Preprocessor preprocessor(_settings, this);
        std::list<std::string> configurations;
        std::string filedata;
        
        {
            Timer t("Preprocessor::preprocess", _settings._showtime, &S_timerResults);
            preprocessor.preprocess(fileStream, filedata, configurations, filename, _settings._includePaths);
        }
        
        if (_settings.checkConfiguration) {
            return 0;
        }
        
        if (!_settings.userDefines.empty() && _settings._maxConfigs==1U) {
            configurations.clear();
            configurations.push_back(_settings.userDefines);
        }
        
        if (!_settings._force && configurations.size() > _settings._maxConfigs) {
            if (_settings.isEnabled("information")) {
                tooManyConfigsError(Path::toNativeSeparators(filename),configurations.size());
            } else {
                tooManyConfigs = true;
            }
        }
        
        std::set<unsigned long long> checksums;
        unsigned int checkCount = 0;
        for (std::list<std::string>::const_iterator it = configurations.begin(); it != configurations.end(); ++it) {
            if (!_settings._force && ++checkCount > _settings._maxConfigs)
                break;
            
            cfg = *it;
            
            if (!_settings.userDefines.empty()) {
                if (!cfg.empty())
                    cfg = ";" + cfg;
                cfg = _settings.userDefines + cfg;
            }
            
            Timer t("Preprocessor::getcode", _settings._showtime, &S_timerResults);
            std::string codeWithoutCfg = preprocessor.getcode(filedata, cfg, filename);
            t.Stop();

            codeWithoutCfg += _settings.append();

            if (!analyzeFile_internal(codeWithoutCfg, filename.c_str(), checksums, internalErrorFound)) {
                if (_settings.isEnabled("information") && _settings._verbose)
                    purgedConfigurationMessage(filename, cfg);
            }

        }
    } catch (const std::runtime_error &e) {
        internalError(filename, e.what());
    } catch (const InternalError &e) {
        internalError(filename, e.errorMessage);
        exitcode=1; // e.g. reflect a syntax error
    }

    return exitcode;
}


bool TscanCode::analyzeFile_internal(const std::string &code, const char FileName[], std::set<unsigned long long>& checksums, bool& internalErrorFound)
{
    internalErrorFound=false;
    if (_settings.terminated() || _settings.checkConfiguration)
        return true;
    
    Tokenizer _tokenizer(&_settings, this);
    if (_settings._showtime != SHOWTIME_NONE)
        _tokenizer.setTimerResults(&S_timerResults);
    try {
        
        // Tokenize the file
        std::istringstream istr(code);
        
        Timer timer("Tokenizer::tokenize", _settings._showtime, &S_timerResults);
        bool result = _tokenizer.tokenize(istr, FileName, cfg, false, true);
        timer.Stop();
        
        if (_settings._force || _settings._maxConfigs > 1) {
            const unsigned long long checksum = _tokenizer.list.calculateChecksum();
            if (checksums.find(checksum) != checksums.end())
                return false;
            checksums.insert(checksum);
        }
        
        if (!result) {
            // File had syntax errors, abort
            return true;
        }
        
        // Init all scopes
        CGlobalTokenizeData* globalData = CGlobalTokenizer::Instance()->GetGlobalData(this);
        globalData->InitScopes(_tokenizer.getSymbolDatabase());
		globalData->RecordFuncInfo(&_tokenizer, FileName);
		globalData->RecordInfoForLua(&_tokenizer, FileName);
		if (_settings.IsCheckIdOpened(ErrorType::ToString(ErrorType::Unity).c_str(), "sgameStructAlignError"))
			globalData->RecordRiskTypes(&_tokenizer);
        
        if (_settings.terminated())
            return true;
        
    } catch (const InternalError &e) {

        internalErrorFound=true;
        std::list<ErrorLogger::ErrorMessage::FileLocation> locationList;
        ErrorLogger::ErrorMessage::FileLocation loc;
        if (e.token) {
            loc.line = e.token->linenr();
            const std::string fixedpath = Path::toNativeSeparators(_tokenizer.list.file(e.token));
            loc.setfile(fixedpath);
        } else {
            ErrorLogger::ErrorMessage::FileLocation loc2;
            loc2.setfile(Path::toNativeSeparators(FileName));
            locationList.push_back(loc2);
            loc.setfile(_tokenizer.list.getSourceFilePath());
        }
        locationList.push_back(loc);
        const ErrorLogger::ErrorMessage errmsg(locationList,
                                               Severity::error, 
                                               e.errorMessage,
											   ErrorType::None,
                                               e.id,
                                               false);
        
        reportErr(errmsg);
    }
    return true;
}

bool TscanCode::checkFile(const std::string &code, const char FileName[], std::set<unsigned long long>& checksums, bool& internalErrorFound)
{
    internalErrorFound=false;
    if (_settings.terminated() || _settings.checkConfiguration)
        return true;

    Tokenizer _tokenizer(&_settings, this);
    if (_settings._showtime != SHOWTIME_NONE)
        _tokenizer.setTimerResults(&S_timerResults);
    try {
        // Execute rules for "raw" code
        for (std::list<Settings::Rule>::const_iterator it = _settings.rules.begin(); it != _settings.rules.end(); ++it) {
            if (it->tokenlist == "raw") {
                Tokenizer tokenizer2(&_settings, this);
                std::istringstream istr(code);
                tokenizer2.list.createTokens(istr, FileName);
                executeRules("raw", tokenizer2);
                break;
            }
        }

        // Tokenize the file
        std::istringstream istr(code);

        Timer timer("Tokenizer::tokenize", _settings._showtime, &S_timerResults);
        bool result = _tokenizer.tokenize(istr, FileName, cfg);
        timer.Stop();

        if (_settings._force || _settings._maxConfigs > 1) {
            const unsigned long long checksum = _tokenizer.list.calculateChecksum();
            if (checksums.find(checksum) != checksums.end())
                return false;
            checksums.insert(checksum);
        }

        if (!result) {
            // File had syntax errors, abort
            return true;
        }

        // dump
        if (_settings.dump) {
            std::string dumpfile = std::string(FileName) + ".dump";
            std::ofstream fdump(dumpfile.c_str());
            if (fdump.is_open()) {
                fdump << "<?xml version=\"1.0\"?>" << std::endl;
                fdump << "<dump cfg=\"" << cfg << "\">" << std::endl;
                _tokenizer.dump(fdump);
                fdump << "</dump>" << std::endl;
            }
            return true;
        }

        // call all "runChecks" in all registered Check classes
        for (std::list<Check *>::const_iterator it = Check::instances().begin(); it != Check::instances().end(); ++it) {
            if (_settings.terminated())
                return true;

            Timer timerRunChecks((*it)->name() + "::runChecks", _settings._showtime, &S_timerResults);
            (*it)->runChecks(&_tokenizer, &_settings, this);
        }

        // Analyse the tokens..
        for (std::list<Check *>::const_iterator it = Check::instances().begin(); it != Check::instances().end(); ++it) {
            Check::FileInfo *fi = (*it)->getFileInfo(&_tokenizer, &_settings);
            if (fi != nullptr)
                fileInfo.push_back(fi);
        }

        executeRules("normal", _tokenizer);

        if (!_simplify)
            return true;

        Timer timer3("Tokenizer::simplifyTokenList2", _settings._showtime, &S_timerResults);
        result = _tokenizer.simplifyTokenList2();
        timer3.Stop();
        if (!result)
            return true;

        // call all "runSimplifiedChecks" in all registered Check classes
        for (std::list<Check *>::const_iterator it = Check::instances().begin(); it != Check::instances().end(); ++it) {
            if (_settings.terminated())
                return true;

            Timer timerSimpleChecks((*it)->name() + "::runSimplifiedChecks", _settings._showtime, &S_timerResults);
            (*it)->runSimplifiedChecks(&_tokenizer, &_settings, this);
        }

        if (_settings.terminated())
            return true;

        executeRules("simple", _tokenizer);

        if (_settings.terminated())
            return true;
    } catch (const InternalError &e) {
        internalErrorFound=true;
        std::list<ErrorLogger::ErrorMessage::FileLocation> locationList;
        ErrorLogger::ErrorMessage::FileLocation loc;
        if (e.token) {
            loc.line = e.token->linenr();
            const std::string fixedpath = Path::toNativeSeparators(_tokenizer.list.file(e.token));
            loc.setfile(fixedpath);
        } else {
            ErrorLogger::ErrorMessage::FileLocation loc2;
            loc2.setfile(Path::toNativeSeparators(FileName));
            locationList.push_back(loc2);
            loc.setfile(_tokenizer.list.getSourceFilePath());
        }
        locationList.push_back(loc);
        const ErrorLogger::ErrorMessage errmsg(locationList,
                                               Severity::error, 
                                               e.errorMessage,
											   ErrorType::None,
                                               e.id,
                                               false);

        reportErr(errmsg);
    }
    return true;
}

void TscanCode::executeRules(const std::string &tokenlist, const Tokenizer &tokenizer)
{
    (void)tokenlist;
    (void)tokenizer;

#ifdef HAVE_RULES
    // Are there rules to execute?
    bool isrule = false;
    for (std::list<Settings::Rule>::const_iterator it = _settings.rules.begin(); it != _settings.rules.end(); ++it) {
        if (it->tokenlist == tokenlist)
            isrule = true;
    }

    // There is no rule to execute
    if (isrule == false)
        return;

    // Write all tokens in a string that can be parsed by pcre
    std::ostringstream ostr;
    for (const Token *tok = tokenizer.tokens(); tok; tok = tok->next())
        ostr << " " << tok->str();
    const std::string str(ostr.str());

    for (std::list<Settings::Rule>::const_iterator it = _settings.rules.begin(); it != _settings.rules.end(); ++it) {
        const Settings::Rule &rule = *it;
        if (rule.pattern.empty() || rule.id.empty() || rule.severity.empty() || rule.tokenlist != tokenlist)
            continue;

        const char *error = nullptr;
        int erroffset = 0;
        pcre *re = pcre_compile(rule.pattern.c_str(),0,&error,&erroffset,nullptr);
        if (!re) {
            if (error) {
                ErrorLogger::ErrorMessage errmsg(std::list<ErrorLogger::ErrorMessage::FileLocation>(),
                                                 Severity::error, ErrorType::None,
                                                 error,
                                                 "pcre_compile",
                                                 false);

                reportErr(errmsg);
            }
            continue;
        }

        int pos = 0;
        int ovector[30]= {0};
        while (pos < (int)str.size() && 0 <= pcre_exec(re, nullptr, str.c_str(), (int)str.size(), pos, 0, ovector, 30)) {
            const unsigned int pos1 = (unsigned int)ovector[0];
            const unsigned int pos2 = (unsigned int)ovector[1];

            // jump to the end of the match for the next pcre_exec
            pos = (int)pos2;

            // determine location..
            ErrorLogger::ErrorMessage::FileLocation loc;
            loc.setfile(tokenizer.list.getSourceFilePath());
            loc.line = 0;

            std::size_t len = 0;
            for (const Token *tok = tokenizer.tokens(); tok; tok = tok->next()) {
                len = len + 1U + tok->str().size();
                if (len > pos1) {
                    loc.setfile(tokenizer.list.getFiles().at(tok->fileIndex()));
                    loc.line = tok->linenr();
                    break;
                }
            }

            const std::list<ErrorLogger::ErrorMessage::FileLocation> callStack(1, loc);

            // Create error message
            std::string summary;
            if (rule.summary.empty())
                summary = "found '" + str.substr(pos1, pos2 - pos1) + "'";
            else
                summary = rule.summary;
            const ErrorLogger::ErrorMessage errmsg(callStack, Severity::fromString(rule.severity), summary, rule.id, false);

            // Report error
            reportErr(errmsg);
        }

        pcre_free(re);
    }
#endif
}

Settings &TscanCode::settings()
{
    return _settings;
}

void TscanCode::tooManyConfigsError(const std::string &file, const std::size_t numberOfConfigurations)
{
    if (!_settings.isEnabled("information") && !tooManyConfigs)
        return;

    tooManyConfigs = false;

    if (_settings.isEnabled("information") && file.empty())
        return;

    std::list<ErrorLogger::ErrorMessage::FileLocation> loclist;
    if (!file.empty()) {
        ErrorLogger::ErrorMessage::FileLocation location;
        location.setfile(file);
        loclist.push_back(location);
    }

    std::ostringstream msg;
    msg << "Too many #ifdef configurations - TscanCode only checks " << _settings._maxConfigs;
    if (numberOfConfigurations > _settings._maxConfigs)
        msg << " of " << numberOfConfigurations << " configurations. Use --force to check all configurations.\n";
    if (file.empty())
        msg << " configurations. Use --force to check all configurations. For more details, use --enable=information.\n";
    msg << "The checking of the file will be interrupted because there are too many "
        "#ifdef configurations. Checking of all #ifdef configurations can be forced "
        "by --force command line option or from GUI preferences. However that may "
        "increase the checking time.";
    if (file.empty())
        msg << " For more details, use --enable=information.";


    ErrorLogger::ErrorMessage errmsg(loclist,
                                     Severity::information,
                                     msg.str(),
									 ErrorType::None,
                                     "toomanyconfigs",
                                     false);

    reportErr(errmsg);
}

void TscanCode::purgedConfigurationMessage(const std::string &file, const std::string& configuration)
{

    tooManyConfigs = false;

    if (_settings.isEnabled("information") && file.empty())
        return;

    std::list<ErrorLogger::ErrorMessage::FileLocation> loclist;
    if (!file.empty()) {
        ErrorLogger::ErrorMessage::FileLocation location;
        location.setfile(file);
        loclist.push_back(location);
    }

    ErrorLogger::ErrorMessage errmsg(loclist,
                                     Severity::information,
                                     "The configuration '" + configuration + "' was not checked because its code equals another one.",
									 ErrorType::None,
                                     "purgedConfiguration",
                                     false);

    reportErr(errmsg);
}

//---------------------------------------------------------------------------

void TscanCode::reportErr(const ErrorLogger::ErrorMessage &msg)
{
	if (msg._severity == Severity::SeverityType::debug)
	{
		return;//not report debug error
	}
	if (!_settings.IsCheckIdOpened(ErrorType::ToString(msg._type).c_str(), msg._id.c_str()))
		return;

	const std::vector<std::string> ignored = _settings._pathToIgnore;
	if (!ignored.empty())
	{
		for (std::vector<std::string>::const_iterator I = ignored.begin(), E = ignored.end(); I != E; ++I)
		{
			const std::string& path = *I;
			const std::string& file = msg._callStack.back().getfile();
			if (file.find(path) == 0)
			{
				if (file.length() == path.length())
				{
					return;
				}
				else 
				{
					if (*path.rbegin() == PATH_SEP)
					{
						return;
					}
					else if (file[path.length()] == PATH_SEP)
					{
						return;
					}
				}
			}
			
		}
	}

    if (!_settings.library.reportErrors(msg.file0))
        return;

	std::string errmsg;
	

	if (_settings._xml)
	{
		if (_settings.OpenCodeTrace)
		{
			errmsg = msg.toXML_codetrace(_settings._verbose, _settings._xml_version);
		}
		else
		{
			errmsg = msg.toXML(_settings._verbose, _settings._xml_version);
		}
		
	}
	else
		errmsg = msg.toString(_settings._verbose);

    if (errmsg.empty())
        return;

    std::string file;
    unsigned int line(0);
    if (!msg._callStack.empty()) {
        file = msg._callStack.back().getfile(false);
        line = msg._callStack.back().line;
    }

    if (_useGlobalSuppressions) {
        if (_settings.nomsg.isSuppressed(msg._id, file, line))
            return;
    } else {
        if (_settings.nomsg.isSuppressedLocal(msg._id, file, line))
            return;
    }

    if (!_settings.nofail.isSuppressed(msg._id, file, line))
        exitcode = 1;

    _errorList.push_back(errmsg);
}

void TscanCode::reportOut(const std::string &outmsg)
{
    _errorLogger.reportOut(outmsg);
}

void TscanCode::reportProgress(const std::string &filename, const char stage[], const std::size_t value)
{
    _errorLogger.reportProgress(filename, stage, value);
}

void TscanCode::reportInfo(const ErrorLogger::ErrorMessage &msg)
{
    // Suppressing info message?
    std::string file;
    unsigned int line(0);
    if (!msg._callStack.empty()) {
        file = msg._callStack.back().getfile(false);
        line = msg._callStack.back().line;
    }
    if (_useGlobalSuppressions) {
        if (_settings.nomsg.isSuppressed(msg._id, file, line))
            return;
    } else {
        if (_settings.nomsg.isSuppressedLocal(msg._id, file, line))
            return;
    }

    _errorLogger.reportInfo(msg);
}

void TscanCode::reportStatus(unsigned int /*fileindex*/, unsigned int /*filecount*/, std::size_t /*sizedone*/, std::size_t /*sizetotal*/)
{

}

void TscanCode::getErrorMessages()
{
    Settings &s = _settings;
    s.addEnabled("warning");
    s.addEnabled("style");
    s.addEnabled("portability");
    s.addEnabled("performance");
    s.addEnabled("information");

    tooManyConfigs = true;
    tooManyConfigsError("",0U);

    // call all "getErrorMessages" in all registered Check classes
    for (std::list<Check *>::const_iterator it = Check::instances().begin(); it != Check::instances().end(); ++it)
        (*it)->getErrorMessages(this, &s);

    Tokenizer::getErrorMessages(this, &s);
    Preprocessor::getErrorMessages(this, &s);
}

void TscanCode::analyseWholeProgram()
{
    // Analyse the tokens
    for (std::list<Check *>::const_iterator it = Check::instances().begin(); it != Check::instances().end(); ++it)
        (*it)->analyseWholeProgram(fileInfo, _settings, *this);
}

bool TscanCode::unusedFunctionCheckIsEnabled() const
{
    return (_settings._jobs == 1 && _settings.isEnabled("unusedFunction"));
}

