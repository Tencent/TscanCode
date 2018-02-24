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

#include "settings.h"
#include "path.h"
#include "preprocessor.h"       // Preprocessor
#include "utils.h"
#include "tinyxml2.h"

#include <utility>
#include <fstream>
#include <set>
#include <algorithm>


Settings* Settings::_instance = nullptr;

Settings::Settings()
    : _terminate(false),
      debug(false),
      debugnormal(false),
      debugwarnings(false),
      debugFalsePositive(false),
	  
	  debugDumpGlobal(false),
	  debugDumpNP(false),
	  debugDumpSimp1(false),
	  debugDumpSimp2(false),
      
	  dump(false),
      exceptionHandling(false),
      inconclusive(true), // default to true
      jointSuppressionReport(false),
      experimental(true),//default enable this feature
      quiet(false),
      _inlineSuppressions(false),
      _verbose(false),
      _checkLua(false),
	  luaPath(""),
      _force(false),
      _relativePaths(false),
      _xml(false), _xml_version(1),
      _jobs(1),
      _loadAverage(0),
      _exitCode(0),
      _showtime(SHOWTIME_NONE),
      _maxConfigs(1),
      enforcedLang(None),
      reportProgress(false),
      checkConfiguration(false),
      checkLibrary(false),
	  ANY_TYPE_T("T_2016_7_14_10_44_55_123_xxxx"),
	  returnKeyWord("return|break|continue|goto|throw|THROW_EXCEPTION"),//fix THROW_EXCEPTION not expand
	  _no_analyze(false),
	  _no_check(false),
	  _big_file_token_size(512 * 1024),
	  _big_header_file_size(-1),
	_large_includes(-1),
	_recordFuncinfo(false),
	_e2o(false)

{

    // This assumes the code you are checking is for the same architecture this is compiled on.
#if defined(_WIN64)
    platform(Win64);
#elif defined(_WIN32)
    platform(Win32A);
#else
    platform(Unspecified);//ignore TSC
#endif
}


void Settings::Destroy()
{
	if (_instance)
	{
		delete _instance;
	}
}

namespace {
    static const std::set<std::string> id = make_container< std::set<std::string> > ()
                                            << "warning"
                                            << "style"
                                            << "performance"
                                            << "portability"
                                            << "information"
                                            << "missingInclude"
#ifdef CHECK_INTERNAL
                                            << "internal"
#endif
                                            ;
}
std::string Settings::addEnabled(const std::string &str)
{
    // Enable parameters may be comma separated...
    if (str.find(',') != std::string::npos) {
        std::string::size_type prevPos = 0;
        std::string::size_type pos = 0;
        while ((pos = str.find(',', pos)) != std::string::npos) {
            if (pos == prevPos)
                return std::string("tscancode: --enable parameter is empty");
            const std::string errmsg(addEnabled(str.substr(prevPos, pos - prevPos)));
            if (!errmsg.empty())
                return errmsg;
            ++pos;
            prevPos = pos;
        }
        if (prevPos >= str.length())
            return std::string("tscancode: --enable parameter is empty");
        return addEnabled(str.substr(prevPos));
    }

    if (str == "all") {
        std::set<std::string>::const_iterator it;
        for (it = id.begin(); it != id.end(); ++it) {
            if (*it == "internal")
                continue;

            _enabled.insert(*it);
        }
    } else if (id.find(str) != id.end()) {
        _enabled.insert(str);
        if (str == "information") {
            _enabled.insert("missingInclude");
        }
    } else {
        if (str.empty())
            return std::string("TscanCode: --enable parameter is empty");
        else
            return std::string("TscanCode: there is no --enable parameter with the name '" + str + "'");
    }

    return std::string("");
}


bool Settings::append(const std::string &filename)
{
    std::ifstream fin(filename.c_str());
    if (!fin.is_open()) {
        return false;
    }
    std::string line;
    while (std::getline(fin, line)) {
        _append += line + "\n";
    }
    Preprocessor::preprocessWhitespaces(_append);
    return true;
}

const std::string &Settings::append() const
{
    return _append;
}

bool Settings::platform(PlatformType type)
{
    switch (type) {
    case Unspecified: // same as system this code was compile on
        platformType = type;
        sizeof_bool = sizeof(bool);
        sizeof_short = sizeof(short);
        sizeof_int = sizeof(int);
        sizeof_long = sizeof(long);
        sizeof_long_long = sizeof(long long);
        sizeof_float = sizeof(float);
        sizeof_double = sizeof(double);
        sizeof_long_double = sizeof(long double);
        sizeof_wchar_t = sizeof(wchar_t);
        sizeof_size_t = sizeof(std::size_t);
        sizeof_pointer = sizeof(void *);
        return true;
    case Win32W:
    case Win32A:
        platformType = type;
        sizeof_bool = 1; // 4 in Visual C++ 4.2
        sizeof_short = 2;
        sizeof_int = 4;
        sizeof_long = 4;
        sizeof_long_long = 8;
        sizeof_float = 4;
        sizeof_double = 8;
        sizeof_long_double = 8;
        sizeof_wchar_t = 2;
        sizeof_size_t = 4;
        sizeof_pointer = 4;
        return true;
    case Win64:
        platformType = type;
        sizeof_bool = 1;
        sizeof_short = 2;
        sizeof_int = 4;
        sizeof_long = 4;
        sizeof_long_long = 8;
        sizeof_float = 4;
        sizeof_double = 8;
        sizeof_long_double = 8;
        sizeof_wchar_t = 2;
        sizeof_size_t = 8;
        sizeof_pointer = 8;
        return true;
    case Unix32:
        platformType = type;
        sizeof_bool = 1;
        sizeof_short = 2;
        sizeof_int = 4;
        sizeof_long = 4;
        sizeof_long_long = 8;
        sizeof_float = 4;
        sizeof_double = 8;
        sizeof_long_double = 12;
        sizeof_wchar_t = 4;
        sizeof_size_t = 4;
        sizeof_pointer = 4;
        return true;
    case Unix64:
        platformType = type;
        sizeof_bool = 1;
        sizeof_short = 2;
        sizeof_int = 4;
        sizeof_long = 8;
        sizeof_long_long = 8;
        sizeof_float = 4;
        sizeof_double = 8;
        sizeof_long_double = 16;
        sizeof_wchar_t = 4;
        sizeof_size_t = 8;
        sizeof_pointer = 8;
        return true;
    }

    // unsupported platform
    return false;
}

bool Settings::platformFile(const std::string &filename)
{
    (void)filename;
    /** @todo TBD */

    return false;
}

const void Settings::trim(std::string &str) const
{
	const std::size_t start = str.find_first_not_of(' ');
	if (start == std::string::npos)
	{
		str.clear();
		return;
	}

	const std::size_t end = str.find_last_not_of(' ');
	if (start == std::string::npos)
	{
		str.clear();
		return;
	}
	str = str.substr(start, end - start + 1);
}

void Settings::split(std::vector<std::string> &tokens, const std::string &str, const std::string &delimiters) const
{
	using namespace std;
	// Skip delimiters at beginning.
	string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	string::size_type pos = str.find_first_of(delimiters, lastPos);

	while (string::npos != pos || string::npos != lastPos)
	{
		// Found a token, add it to the vector.
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		// Skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiters, pos);
		// Find next "non-delimiter"
		pos = str.find_first_of(delimiters, lastPos);
	}
}

bool Settings::ParseFuncInfo(std::string& functionname, std::pair<std::string,
	std::vector<std::string> >& sig, const std::string &funcinfo)
{
	if (funcinfo.empty())
	{
		return false;
	}

	using namespace std;
	string::size_type pos_functionname = 0;

	string funcinfo_bak = funcinfo;
	string classname_parse = "";
	string functionname_parse = "";
	vector<std::string> classList;
	vector<std::string> classnameList;
	vector<std::string> funcinfoList;
	vector<std::string> funcnameList;
	string str_head;

	if ((pos_functionname = funcinfo.find("(")) != string::npos)
	{
		str_head = funcinfo.substr(0, pos_functionname);
	}
	//only function name
	else
	{
		str_head = funcinfo;
	}
	//split function name
	trim(str_head);
	if (str_head.empty())
	{
		return false;
	}
	//split string by "::"
	split(classList, str_head, "::");
	if (classList.size() >= 2) {
		classname_parse = classList[classList.size() - 2];
		trim(classname_parse);
		split(classnameList, classname_parse, " ");
		if (classnameList.size() >= 2 && classnameList[classnameList.size() - 1] == ">") {
			string tmp;
			bool flag_template = false;
			for (std::size_t i = 0; i < classnameList.size(); i++) {
				if (classnameList[i] == "<" && i > 0) {
					flag_template = true;
					tmp += classnameList[i - 1];
				}
				if (flag_template)
					tmp += classnameList[i];
			}
			classname_parse = tmp;
		}
		else if (!classnameList.empty())
			classname_parse = classnameList[classnameList.size() - 1];

		functionname_parse = classList[classList.size() - 1];
		trim(functionname_parse);

	}
	else //no class name
	{
		size_t nPos = str_head.find('<');
		if (nPos != string::npos)
		{
			functionname_parse = str_head.substr(0, nPos);
		}
		else
		{
			functionname_parse = str_head;
		}
	}
	//only function name
	if (pos_functionname == string::npos)
	{
		sig.first = classname_parse;
		sig.second.push_back(ANY_TYPE_T);
		functionname = functionname_parse;
		return true;
	}

	string::size_type params_start = 0;
	string::size_type params_end = 0;
	string params_line;

	if ((params_start = funcinfo.find("(")) != string::npos) {
		if ((params_end = funcinfo.rfind(")")) != string::npos && params_start < params_end) {
			params_line = funcinfo.substr(params_start + 1, params_end - params_start - 1);
			trim(params_line);
		}
	}

	if (params_line != "") {
		string::size_type pos;
		string pattern(",");

		params_line += pattern;
		bool find_map = false;

		if ((params_line.find("map")) != string::npos)
			find_map = true;

		unsigned int size = params_line.size();
		for (std::size_t i = 0; i < size; i++) {
			pos = params_line.find(pattern, i);
			if (pos < size) {
				int count = 0;
				if (find_map) {
					for (unsigned int j = pos; j < size; j++) {
						if (params_line[j] == '<')
							count++;
						else if (params_line[j] == '>')
							count--;
					}
				}
				if (count < 0) {
					pos = params_line.find(pattern, pos + 1);
					if (!(pos < size))
						continue;
				}
				string s = params_line.substr(i, pos - i);
				trim(s);
				string::size_type pos_param_name;
				if ((pos_param_name = s.rfind(" ")) != string::npos) {
					char c2 = s[pos_param_name + 1];
					s = s.substr(0, pos_param_name);
					trim(s);
					char c = s[s.length() - 1];
					if (c2 == '&' || c2 == '*') {
						s += c2;
					}
					else if (c == '&' || c == '*') {
						s = s.substr(0, s.length() - 1);
						trim(s);
						s += c;
					}

				}
				if (_equalTypeForFuncSig.find(s) != _equalTypeForFuncSig.end())
				{
					sig.second.push_back("int");
				}
				else
				{
					sig.second.push_back(s);
				}
				i = pos;
			}
		}
	}

	sig.first = classname_parse;
	functionname = functionname_parse;
	return true;
}


bool Settings::IsCheckIdOpened(const char* szId, const char* szSubid) const
{
	if (!szId || !szSubid)
	{
		return false;
	}

	// don't report error
	if (0 == strcmp(szId, "none"))
	{
		return false;
	}

	std::map<std::string, std::map<std::string, bool> > ::const_iterator CI = _openedChecks.find(szId);
	if (CI != _openedChecks.end())
	{
		const std::map< std::string, bool >& subidSet = CI->second;
		std::map< std::string, bool >::const_iterator CI2 = subidSet.find(szSubid);
		if (CI2 != subidSet.end())
		{
			return CI2->second;
		}
	}

	return false;
}

std::string Settings::GetCheckSeverity(const std::string& szSubid) const
{
	std::map<std::string, std::string>::const_iterator I = _checkSeverity.find(szSubid);

	if (I != _checkSeverity.end())
		return I->second;
	else
		return "";
}

bool Settings::LoadCustomCfgXml(const std::string &filePath, const char szExeName[])
{
	OpenCodeTrace = false;//init false
	OpenAutofilter = false;//init false

	//load types
	std::string baseType[] = {
		"int",
		"long",
		"short",
		"float",
		"double",
		"char",
		"bool",
		"size_t",
		"int8_t",
		"int16_t",
		"int32_t",
		"int64_t",
		"uint8_t",
		"uint16_t",
		"uint32_t",
		"uint64_t",
		"int_fast8_t",
		"int_fast16_t",
		"int_fast32_t",
		"int_fast64_t",
		"int_least8_t",
		"int_least16_t",
		"int_least32_t",
		"int_least64_t",
		"uint_fast8_t",
		"uint_fast16_t",
		"uint_fast32_t",
		"uint_fast64_t",
		"uint_least8_t",
		"uint_least16_t",
		"uint_least32_t",
		"uint_least64_t"
	};
	for (int index = 0; index < 32; ++index)
	{
		_equalTypeForFuncSig.insert(baseType[index]);
		_equalTypeForFuncSig.insert(std::string("long ") + baseType[index]);
		_equalTypeForFuncSig.insert(std::string("unsigned ") + baseType[index]);
		_equalTypeForFuncSig.insert(std::string("unsigned long ") + baseType[index]);
	}

	_keywordBeforeFunction.insert("extern");
	_keywordBeforeFunction.insert("static");
	_keywordBeforeFunction.insert("inline");
	_keywordBeforeFunction.insert("constexpr");
	
	_nonPtrType.insert("Int32");
	_nonPtrType.insert("UInt32");
	_nonPtrType.insert("time_t");

	//load from file
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLError error = doc.LoadFile(filePath.c_str());

	if (error == tinyxml2::XML_ERROR_FILE_NOT_FOUND)
	{
		//try abs path
#ifdef CFGDIR
		const std::string cfgfolder(CFGDIR);
#else
		if (!szExeName)
		{
			std::cout << "Failed to load setting file 'cfg.xml'. File not found" << std::endl;
			return false;
		}
		const std::string cfgfolder(Path::fromNativeSeparators(Path::getPathFromFilename(szExeName)) + "cfg");
#endif
		const char *sep = (!cfgfolder.empty() && cfgfolder[cfgfolder.size() - 1U] == '/' ? "" : "/");
		
		const std::string filename(cfgfolder + sep + "cfg.xml");
		error = doc.LoadFile(filename.c_str());
		
		if (error == tinyxml2::XML_ERROR_FILE_NOT_FOUND)
		{
			const std::string filename2(cfgfolder + sep + "cfg_setup.xml");
			error = doc.LoadFile(filename2.c_str());
		}
		if (error == tinyxml2::XML_ERROR_FILE_NOT_FOUND)
		{
			std::cout << "Failed to load setting file 'cfg.xml'. File not found" << std::endl;
			return false;
		}
	}

	const tinyxml2::XMLElement * const rootnode = doc.FirstChildElement();

	if (rootnode == nullptr)
	{
		std::cout << "cfg.xml format illegal" << std::endl;
		return false;
	}

	if (strcmp(rootnode->Name(), "def") != 0)
	{
		std::cout << "cfg.xml format not supported" << std::endl;
		return false;
	}

	// version check like other .cfg files
	const char* format_string = rootnode->Attribute("format");
	int format = 1;
	if (format_string)
	{
		format = atoi(format_string);
	}

	if (format > 2 || format <= 0)
	{
		std::cout << "cfg.xml format version not supported" << std::endl;
		return false;
	}


	for (const tinyxml2::XMLElement *node = rootnode->FirstChildElement(); node; node = node->NextSiblingElement())
	{
		//ignore unknown tag
		if (!node->Name() || 0 != strcmp(node->Name(), "section"))
		{
			continue;
		}
		const char * pNodeName = node->Attribute("name");
		if (!pNodeName)
		{
			continue;
		}
		if (0 == strcmp(pNodeName, "FunctionNotReturnNull"))
		{
			for (const tinyxml2::XMLElement *func = node->FirstChildElement(); func; func = func->NextSiblingElement())
			{
				//only record known tag
				if (func->Name() && strcmp(func->Name(), "function") == 0)
				{
					const char *pAttr = func->Attribute("name");
					std::pair<std::string, std::vector<std::string> > sig;
					std::string functionName;
					if (pAttr && ParseFuncInfo(functionName, sig, pAttr))
					{
						_functionNotRetNull.insert(std::make_pair(functionName, sig));
					}
				}
			}
		}
		else if (0 == strcmp(pNodeName, "PathToIgnore"))
		{
			for (const tinyxml2::XMLElement *path = node->FirstChildElement(); path; path = path->NextSiblingElement())
			{
				//only record known tag
				if (path->Name() && strcmp(path->Name(), "path") == 0)
				{
					const char *pAttr = path->Attribute("name");
					if (pAttr)
					{
						_pathToIgnore.push_back(path->Attribute("name"));
					}
				}
			}
		}
		else if (0 == strcmp(pNodeName, "JumpCode"))
		{
			std::set<int> paramIndex;
			for (const tinyxml2::XMLElement *jmp = node->FirstChildElement(); jmp; jmp = jmp->NextSiblingElement())
			{
				//only record known tag
				if (jmp->Name() && strcmp(jmp->Name(), "jumpcode") == 0)
				{
					for (const tinyxml2::XMLElement *arg = jmp->FirstChildElement(); arg; arg = arg->NextSiblingElement())
					{
						if (arg->Name() && strcmp(arg->Name(), "arg") == 0)
						{
							const char *pIndex = arg->Attribute("index");
							if (pIndex)
							{
								paramIndex.insert(atoi(pIndex));
							}
						}
					}
				}
				const char *pName = jmp->Attribute("name");
				if (pName)
				{
					_jumpCode[pName] = paramIndex;
					//clear paramIndex for reuse
					paramIndex.clear();
				}
			}
		}
		else if (0 == strcmp(pNodeName, "IgnoreFileExtension"))
		{
			for (const tinyxml2::XMLElement *jmp = node->FirstChildElement(); jmp; jmp = jmp->NextSiblingElement())
			{
				//only record known tag
				if (jmp->Name() && strcmp(jmp->Name(), "ext") == 0)
				{
					const char *pName = jmp->Attribute("name");

					if (pName)
					{
						std::string strName = pName;
						std::transform(strName.begin(), strName.end(), strName.begin(), tolower);
						_extensionToIgnore.push_back(strName);
					}
				}
			}
		}
		else if (0 == strcmp(pNodeName, "Checks"))
		{
			for (const tinyxml2::XMLElement *id2 = node->FirstChildElement(); id2; id2 = id2->NextSiblingElement())
			{
				if (id2->Name() && (0 == strcmp(id2->Name(), "id")))
				{
					bool bOpenId = id2->BoolAttribute("value");
					const char* szId = id2->Attribute("name");
					if (szId)
					{
						std::map < std::string, bool >& subidSet = _openedChecks[szId];

						for (const tinyxml2::XMLElement *subid = id2->FirstChildElement(); subid; subid = subid->NextSiblingElement())
						{
							if (subid->Name() && (0 == strcmp(subid->Name(), "subid")))
							{
								const char* szCheck = subid->Attribute("name");
								if (szCheck)
								{
									bool bOpen = subid->BoolAttribute("value");
									subidSet[szCheck] = bOpenId ? bOpen : false;

									const char* szSeverity = subid->Attribute("severity");
									if (szSeverity)
										_checkSeverity[szCheck] = szSeverity;
									else
										_checkSeverity[szCheck] = "Warning";
									
								}
							}
						}
					}

				}
			}
		}
		else if (0 == strcmp(pNodeName, "PerformanceCfg"))
		{
			for (const tinyxml2::XMLElement *id2 = node->FirstChildElement(); id2; id2 = id2->NextSiblingElement())
			{
				if (id2->Name() && (0 == strcmp(id2->Name(), "entry")))
				{
					const char* szEntry = id2->Attribute("name");
					if (szEntry)
					{
						if (0 == strcmp(szEntry, "large_token_count"))
						{
							int iValue = id2->IntAttribute("value");
							_big_file_token_size = iValue;
						}
						else if (0 == strcmp(szEntry, "large_header_size"))
						{
							int iValue = id2->IntAttribute("value");
							_big_header_file_size = iValue;
						}
						else if (0 == strcmp(szEntry, "large_includes"))
						{
							int iValue = id2->IntAttribute("value");
							_large_includes = iValue;
						}
					}
				}
			}
		}
		else if (0 == strcmp(pNodeName, "NonReferenceType"))
		{
			for (const tinyxml2::XMLElement *rType = node->FirstChildElement(); rType; rType = rType->NextSiblingElement())
			{
				//only record known tag
				if (rType->Name() && strcmp(rType->Name(), "typename") == 0)
				{
					const char *pAttr = rType->Attribute("name");
					if (pAttr)
					{
						std::string typestr = rType->Attribute("name");
						transform(typestr.begin(), typestr.end(), typestr.begin(), ::tolower);
						_NonReferenceType.insert(typestr);
					}
				}
			}
		}
		else if (0 == strcmp(pNodeName, "SelfMemMalloc"))
		{
			for (const tinyxml2::XMLElement *rType = node->FirstChildElement(); rType; rType = rType->NextSiblingElement())
			{
				//only record known tag
				if (rType->Name() && strcmp(rType->Name(), "function") == 0)
				{
					const char *pAttr = rType->Attribute("customSelfMalloc");
					const char *pAttr2 = rType->Attribute("customSelfDelloc");
					if (pAttr && pAttr2)
					{
						std::string selfMallocStr = rType->Attribute("customSelfMalloc");
						std::string selfDeallocStr = rType->Attribute("customSelfDelloc");

						_customSelfMalloc.push_back(selfMallocStr);
						_customSelfDelloc.push_back(selfDeallocStr);
					}
				}
			}
		}
		else if (0 == strcmp(pNodeName, "CodeTrace"))
		{
			OpenCodeTrace = node->BoolAttribute("value");
		}
		else if (0 == strcmp(pNodeName, "Autofilter"))
		{
			OpenAutofilter = node->BoolAttribute("value");
		}
		else if (0 == strcmp(pNodeName, "HeaderExtension"))
		{
			for (const tinyxml2::XMLElement *rType = node->FirstChildElement(); rType; rType = rType->NextSiblingElement())
			{
				//only record known tag
				if (rType->Name() && strcmp(rType->Name(), "ext") == 0)
				{
					const char *pAttr = rType->Attribute("name");
					if (pAttr && pAttr[0] != '\0')
					{
						_headerExtension.insert(pAttr);
					}
				}
			}
		}
	}
	
	return true;
}
