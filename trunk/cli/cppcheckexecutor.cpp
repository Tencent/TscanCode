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
*/

#include "cppcheckexecutor.h"
#include "cppcheck.h"
#include "threadexecutor.h"
#include "preprocessor.h"
#include "errorlogger.h"
#include <iostream>
#include <sstream>
#include <cstdlib> // EXIT_SUCCESS and EXIT_FAILURE
#include <cstring>
#include <algorithm>
#include <climits>

#include "cmdlineparser.h"
#include "filelister.h"
#include "path.h"
#include "pathmatch.h"

int  splitString2(const string & strSrc, const std::string& strDelims, vector<string>& strDest)  
{  
typedef std::string::size_type ST;  
string delims = strDelims;  
std::string STR;  
if(delims.empty()) delims = "/n/r";  
ST pos=0, LEN = strSrc.size();  
while(pos < LEN ){  
STR="";   
while( (delims.find(strSrc[pos]) != std::string::npos) && (pos < LEN) ) ++pos;  
if(pos==LEN) return strDest.size();  
while( (delims.find(strSrc[pos]) == std::string::npos) && (pos < LEN) ) STR += strSrc[pos++];  
	if( ! STR.empty() ) strDest.push_back(STR);  
	}  
	return strDest.size();  
}
CppCheckExecutor::CppCheckExecutor()
	: _settings(0), time1(0), errorlist(false)
{
}

CppCheckExecutor::~CppCheckExecutor()
{
}

bool CppCheckExecutor::parseFromArgs(CppCheck *cppcheck, int argc, const char* const argv[])
{
	if( cppcheck == NULL )
	{
		return false;
	}
	Settings& settings = cppcheck->settings();
	CmdLineParser parser(&settings);
	bool success = parser.ParseFromArgs(argc, argv);

	if (success) {

		if (parser.GetShowVersion() && !parser.GetShowErrorMessages()) {
			const char * extraVersion = cppcheck->extraVersion();
			if (extraVersion && *extraVersion != 0)
				std::cout << "Cppcheck " << cppcheck->version() << " ("
				<< extraVersion << ')' << std::endl;
			else
				std::cout << "Cppcheck " << cppcheck->version() << std::endl;
		}

		if (parser.GetShowErrorMessages()) {
			errorlist = true;
			std::cout << ErrorLogger::ErrorMessage::getXMLHeader(settings._xml_version);
			cppcheck->getErrorMessages();
			std::cout << ErrorLogger::ErrorMessage::getXMLFooter(settings._xml_version) << std::endl;
		}

		if (parser.ExitAfterPrinting())
			std::exit(EXIT_SUCCESS);
	} else {
		std::exit(EXIT_FAILURE);
	}

	// Check that all include paths exist
	{
		std::list<std::string>::iterator iter;
		for (iter = settings._includePaths.begin();
			iter != settings._includePaths.end();
			) {
				const std::string path(Path::toNativeSeparators(*iter));
				if (FileLister::isDirectory(path))
					++iter;
				else {
					// If the include path is not found, warn user and remove the
					// non-existing path from the list.
					std::cout << "TscanCode: warning: Couldn't find path given by -I '" << path << '\'' << std::endl;
					iter = settings._includePaths.erase(iter);
				}
		}
	}

	// 创建文件依赖表
	const std::vector<std::string>& pathnames = parser.GetPathNames();
	std::vector<std::string> ignored = parser.GetIgnoredPaths();
	std::vector<std::string> includePaths;
	for (std::list<std::string>::iterator iter = settings._includePaths.begin(); iter != settings._includePaths.end(); iter++)
	{
		includePaths.push_back(*iter);
	}
	if(!_fileDependTable.Create(pathnames, ignored, includePaths))
	{
		std::cout << "TscanCode: File Dependence Table is created failed." << std::endl;
		std::exit(EXIT_FAILURE);
	}
	cppcheck->SetFileDependTable(&_fileDependTable);
	_files.clear();
	CCodeFile* pCode = _fileDependTable.GetFirstFile();
	while(pCode)
	{
		if (Path::acceptFile(pCode->GetName()))
		{
			_files[pCode->GetFullPath()] = pCode->GetSize();
		}
		pCode = pCode->GetNext();
	}

	if (!_files.empty()) {
		return true;
	} else {
		std::cout << "TscanCode: error: no files to check - all paths ignored." << std::endl;
		return false;
	}
}
//from TSC 20131028  filter the code of third lib 
void CppCheckExecutor::readcfg()
{
	if ( _settings == NULL )
	{
		return;
	}
	std::string temppath=_settings->_configpath + "filter.ini";
	fstream cfgFile(temppath.c_str());
	//list<std::string> filterdir;
	if(!cfgFile)
	{
		cout<<"can not find filter.ini"<<endl;
		return;
	}
	char tmp[200]="";  

	while(!cfgFile.eof())
	{
		cfgFile.getline(tmp,200);//每行读取前200个字符，200个足够了
		std::string ttmp=tmp;
		//去掉注释
		if(tmp[0]=='#')
		{
			continue;
		}
		if(ttmp!="")
		{

			int pos=0;
			pos=ttmp.find("\\");
			while(pos!=-1)
			{
				ttmp.replace(pos,1,"/");
				pos=ttmp.find("\\");
			}
			filterdir.insert(ttmp);
		}
	}
}

int CppCheckExecutor::check(int argc, const char* const argv[])
{
	Preprocessor::missingIncludeFlag = false;

	CppCheck cppCheck(*this, true);

	Settings& settings = cppCheck.settings();
	_settings = &settings;

	if (!parseFromArgs(&cppCheck, argc, argv)) {
		return EXIT_FAILURE;
	}

	// warning: this method must be invoked before any check works begin, since the dealconfig is setting-dependency now
	cppCheck.Initialize();

	if (settings.reportProgress)
		time1 = std::time(0);

	if (settings._xml) {
		reportErr(ErrorLogger::ErrorMessage::getXMLHeader(settings._xml_version));
	}

	unsigned int returnValue = 0;
	if (settings._jobs == 1) {
		// Single process

		std::size_t totalfilesize = 0;

		//read cfg 20131028
		readcfg();
		set<std::string>::iterator x;
		for (std::map<std::string, std::size_t>::iterator i = _files.begin(); i != _files.end();) {
			{
				bool flag=true;
				for(x=filterdir.begin();x!=filterdir.end();x++)
				{
					if(strstr(i->first.c_str(),x->c_str()))
					{	
						_files.erase(i++);
						flag=false;
						break;
					}

				}
				if(flag &&  i != _files.end())
					i++;

			}
		}
		//end 
		for (std::map<std::string, std::size_t>::const_iterator i = _files.begin(); i != _files.end(); ++i) {
			totalfilesize += i->second;
		}

		std::size_t processedsize = 0;
		unsigned int c = 0;

		for (std::map<std::string, std::size_t>::const_iterator i = _files.begin(); i != _files.end(); ++i) {
			returnValue += cppCheck.check(i->first);
			processedsize += i->second;
			if (!settings._errorsOnly)
				reportStatus(c + 1, _files.size(), processedsize, totalfilesize);
			c++;
		}

		cppCheck.checkFunctionUsage();

		// check work after checking each file
		cppCheck.PostCheck();

	} else if (!ThreadExecutor::isEnabled()) {
		std::cout << "No thread support yet implemented for this platform." << std::endl;
	} else {
		// Multiple processes
		ThreadExecutor executor(_files, settings, *this);
		returnValue = executor.check();
	}

	if (!settings.checkConfiguration) {
		if (!settings._errorsOnly)
			reportUnmatchedSuppressions(settings.nomsg.getUnmatchedGlobalSuppressions());
    }

	if (settings._xml) {
		reportErr(ErrorLogger::ErrorMessage::getXMLFooter(settings._xml_version));
	}
	//add by TSC 20141017 if _setting->_force=true ,autoFilterResult
	if(_settings->_force)
	{
		autoFilterResult();
	}
	//if _setting->_force=true and  !_settings->_writexmlfile=false,here print autofilter results
	if(!_settings->_writexmlfile && _settings->_force)
	{
		for (std::list<std::string>::iterator itr = errorListresult.begin();itr != errorListresult.end();itr++)
		{
			std::cerr << *itr << std::endl;
		}

	}
	if(_settings->_writexmlfile)
	{
		WriteXml();
	}
	//end
    _settings = 0;
    if (returnValue)
        return settings._exitCode;
    else
        return 0;
}

void CppCheckExecutor::reportErr(const std::string &errmsg)
{
	// Alert only about unique errors
	if (_errorList.find(errmsg) != _errorList.end())
		return;

	_errorList.insert(errmsg);
	//from TSC 20140709
	errorListresult.push_back(errmsg);
	//from TSC 20141017 _settings->_force=false means not need auto filter
	if (_settings && !_settings->_writexmlfile && !_settings->_force)
	{
		std::cerr << errmsg << std::endl;
	}
}

void CppCheckExecutor::reportOut(const std::string &outmsg)
{
	std::cout << outmsg << std::endl;
}

void CppCheckExecutor::reportProgress(const std::string &filename, const char stage[], const std::size_t value)
{
    UNREFERENCED_PARAMETER(filename);   

	if (!time1)
		return;

	// Report progress messages every 10 seconds
	const std::time_t time2 = std::time(NULL);
	if (time2 >= (time1 + 10)) {
		time1 = time2;

		// current time in the format "Www Mmm dd hh:mm:ss yyyy"
		const std::string str(std::ctime(&time2));

		// format a progress message
		std::ostringstream ostr;
		ostr << "progress: "
			<< stage
			<< ' ' << value << '%';
		if (_settings && _settings->_verbose)
			ostr << " time=" << str.substr(11, 8);

		// Report progress message
		reportOut(ostr.str());
	}
}

void CppCheckExecutor::reportInfo(const ErrorLogger::ErrorMessage &msg)
{
	reportErr(msg);
}

void CppCheckExecutor::reportStatus(std::size_t fileindex, std::size_t filecount, std::size_t sizedone, std::size_t sizetotal)
{
	if (filecount > 1) {
		std::ostringstream oss;
		oss << fileindex << '/' << filecount
			<< " files checked " <<
			(sizetotal > 0 ? static_cast<long>(static_cast<long double>(sizedone) / sizetotal*100) : 0)
			<< "% done";
		std::cout << oss.str() << std::endl;
	}
}

void CppCheckExecutor::reportErr(const ErrorLogger::ErrorMessage &msg)
{
	if (errorlist) {
		reportOut(msg.toXML(false, _settings->_xml_version));
	} else if (_settings->_xml) {
		reportErr(msg.toXML(_settings->_verbose, _settings->_xml_version));
	}
	else {
		reportErr(msg.toString(_settings->_verbose, _settings->_outputFormat));
	}
}
//from TSC  20141016 check if error's subid is in AUTOFILTER_SUBID
bool CppCheckExecutor::isInAutoFilterSubID(std::string erroritem)
{
	std::vector<std::string> autoFilterSubidList;
	splitString2(AUTOFILTER_SUBID,"|",autoFilterSubidList); 
	for(unsigned int i=0;i<autoFilterSubidList.size();i++)
	{
		if(std::strstr(erroritem.c_str(),autoFilterSubidList[i].c_str()))
		{
			return true;
		}

	}
	return false;
}
//from TSC 20141016 add Intelligent filtering
void CppCheckExecutor::autoFilterResult()
{

	std::list<std::string>::iterator iter;
	std::map<std::string,int> errCount;
	for(iter=errorListresult.begin();iter!=errorListresult.end();iter++)
	{
		std::string errkey=*iter;
		unsigned int pos=0;
		if(_settings->_xml)
		{
		if((pos=errkey.find("subid="))!=string::npos)
		{   
			 
			 unsigned int posend=0;
			 if((posend=errkey.find("content="))!=string::npos)
			 {
				 errkey =errkey.substr(pos,errkey.size()-pos-(errkey.size()-posend+1));
			 }
			 else
			 {
				  errkey =errkey.substr(pos);
			 }
			 errCount[errkey]++;
		}
		}
		else
		{
		 //original error format not xml 
		unsigned int pos=0;
		if((pos=errkey.find("]"))!=string::npos)
		{   
			 errkey  = errkey.substr(pos+2);
			 errCount[errkey]++;
		}
		}
	}
	
	std::map<std::string,int>::iterator miter;
	for(miter=errCount.begin();miter!=errCount.end();miter++)
	{
		std::string errkey=miter->first;
		if(miter->second>=MAXERRORCNT)
		{
		for(iter=errorListresult.begin();iter!=errorListresult.end();)
		{
			if((*iter).find(errkey)!=string::npos && isInAutoFilterSubID(*iter))
			{
				iter=errorListresult.erase(iter);
			}
			else
			{
				iter++;
			}
		}
		}
	}
	
}
void CppCheckExecutor::WriteXml()
{
	if(_settings->_writexmlfile)
	{
		fstream writexml(_settings->_xmlfilename.c_str(),ios::out);
		
		for (std::list<std::string>::iterator itr1 = errorListresult.begin();itr1 != errorListresult.end();itr1++)
			writexml<<*itr1<<endl;
		writexml.close();
	}
}
