

#include "tscthreadexecutor.h"
#include "tscancode.h"
#include "tscexecutor.h"
#include <iostream>
#ifdef __SVR4  // Solaris
#include <sys/loadavg.h>
#endif
#ifdef TSC_THREADING_MODEL_NOT_WIN
#include <pthread.h>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <errno.h>
#include <time.h>
#include <cstring>
#include <sstream>
#endif
#ifdef TSC_THREADING_MODEL_WIN
#include <process.h>
#include <windows.h>
#include <algorithm>
#include <cstring>
#include <errno.h>
#endif

#include "globaltokenizer.h"
#include "preprocessor.h"
#include "path.h"
#include "globalmacros.h"
#include "checktscinvalidvarargs.h"

#define MAXERRORCNT 30
#define AUTOFILTER_SUBID "FuncPossibleRetNULL|FuncRetNULL|dereferenceAfterCheck|dereferenceBeforeCheck|possibleNullDereferenced|nullpointerarg|nullpointerclass"

TscThreadExecutor::TscThreadExecutor(CFileDependTable* pFileTable, Settings &settings, ErrorLogger &errorLogger)
	: _pFileTable(pFileTable)
	, _curFile(nullptr)
	, _settings(settings)
	, _errorLogger(errorLogger)
	, _fileCount(0)
	, _analyzeFile(false)
	, _threadIndex(0)
	, _processedFiles(0)
	, _totalFiles(0)
	, _processedSize(0)
	, _totalFileSize(0)
{

}

TscThreadExecutor::~TscThreadExecutor()
{
    //dtor
}


void TscThreadExecutor::addFileContent(const std::string &path, const std::string &content)
{
	_fileContents[path] = content;
}

void TscThreadExecutor::reportErr(const ErrorLogger::ErrorMessage &msg)
{
	report(msg, REPORT_ERROR);
}

void TscThreadExecutor::reportInfo(const ErrorLogger::ErrorMessage &msg)
{
	report(msg, REPORT_INFO);
}

bool CompareCodeFileSize(const CCodeFile* file1, const CCodeFile* file2)
{
	return file1->GetSize() > file2->GetSize();
}

//#define TSC2_CHECK_ONE_FILE
//#define TSC2_JUST_ANALYZE

unsigned int TscThreadExecutor::check(bool bAnalyze, bool bCheck)
{
#ifdef TSC2_JUST_ANALYZE
	if (!bAnalyze)
	{
		return 0;
	}
#endif // TSC2_JUST_ANALYZE

	if (!bAnalyze && bCheck)
	{
		return 0;
	}

    _analyzeFile = bAnalyze;
	CGlobalTokenizer::Instance()->SetAnalyze(bAnalyze);

	_checkList.clear();
	_processedFiles = 0;
	_processedSize = 0;
	_totalFiles = 0;
	_totalFileSize = 0;

	CCodeFile* pFile = _pFileTable->GetFirstFile();
	while (pFile)
	{
		if (Path::acceptFile(pFile->GetFullPath()) && !pFile->GetIgnore())
		{
			_checkList.push_back(pFile);
			_totalFileSize += pFile->GetSize();
		}
		pFile = pFile->GetNext();
	}
	_checkList.sort(CompareCodeFileSize);


#ifdef TSC2_CHECK_ONE_FILE
	_checkList.clear();
	std::string oneFile = "";
	oneFile = Path::fromNativeSeparators(oneFile);
	CCodeFile newFile(oneFile.c_str(), 100);
	_checkList.push_back(&newFile);
#endif // TSC2_CHECK_ONE_FILE

	_checkList_iter = _checkList.begin();
	_checkList_end = _checkList.end();
	_totalFiles = _checkList.size();

	unsigned ret = multi_thread(TscThreadExecutor::threadProc);
	_checkList.clear();

#ifndef TSC2_CHECK_ONE_FILE
	//find headers not expanded
	pFile = _pFileTable->GetFirstFile();
	_totalFileSize = 0;
	
	while (pFile)
	{
		if (Path::isHeader(pFile->GetFullPath()) && !pFile->GetIgnore() && !pFile->IsExpaned())
		{
			_checkList.push_back(pFile);
			_totalFileSize += pFile->GetSize();
		}
		pFile = pFile->GetNext();
	}
	if (!_checkList.empty())
	{
		std::cout << "Extra Header Start\n";
		_checkList.sort(CompareCodeFileSize);
		_checkList_iter = _checkList.begin();
		_checkList_end = _checkList.end();
		_totalFiles = _checkList.size();
		_processedFiles = 0;
		_processedSize = 0;
		ret = multi_thread(TscThreadExecutor::threadProc);
		std::cout << "Extra Header End\n";
	}
#endif


	if (bAnalyze)
	{
		if (_settings.debugDumpGlobal)
		{
			_pFileTable->DumpFileDependResults();
		}
		CGlobalTokenizer::Instance()->Merge(_settings.debugDumpGlobal);	
	}
	else
	{
		CGlobalErrorList::Instance()->Merge(_settings);

		std::set< std::string >& errorList = CGlobalErrorList::Instance()->GetOneData();
		//autofilter
		if (_settings.OpenAutofilter)
		{
			autoFilterResult(errorList);
		}

		for (std::set<std::string>::iterator I = errorList.begin(), E = errorList.end(); I != E; ++I)
		{
			_errorLogger.reportErr(*I);
		}
	}
	
	return ret;
}

unsigned TscThreadExecutor::init()
{
	CheckTSCInvalidVarArgs::InitFuncMap();
	return initMacros();
}

#ifdef TSC_THREADING_MODEL_WIN
unsigned int __stdcall TscThreadExecutor::threadProc(void *args)
#else
void* TscThreadExecutor::threadProc(void *args)
#endif
{
    unsigned int result = 0;

    TscThreadExecutor *threadExecutor = static_cast<TscThreadExecutor*>(args);
    TSC_LOCK_ENTER(&threadExecutor->_fileSync);

	int threadIndex = threadExecutor->_threadIndex;
    ++threadExecutor->_threadIndex;
	
    TscanCode fileChecker(*threadExecutor, false);
	CGlobalTokenizeData *pGlobalData = CGlobalTokenizer::Instance()->GetGlobalData(&fileChecker);

	for (;;) {
		if (threadExecutor->_checkList_iter == threadExecutor->_checkList_end) {
			TSC_LOCK_LEAVE(&threadExecutor->_fileSync);
			break;
		}

		CCodeFile* curFile = *threadExecutor->_checkList_iter;
		const std::string &file = curFile->GetFullPath();
		const std::size_t fileSize = curFile->GetSize();
		threadExecutor->_checkList_iter++;

		TSC_LOCK_LEAVE(&threadExecutor->_fileSync);

        if (!threadExecutor->_settings.quiet && threadExecutor->_settings.debug) {
            TSC_LOCK_ENTER(&threadExecutor->_reportSync);
            TscanCodeExecutor::reportStatus(threadIndex, threadExecutor->_processedFiles, threadExecutor->_totalFiles, threadExecutor->_processedSize, threadExecutor->_totalFileSize, threadExecutor->_analyzeFile, true, file);
            TSC_LOCK_LEAVE(&threadExecutor->_reportSync);
        }
        
		std::map<std::string, std::string>::const_iterator fileContent = threadExecutor->_fileContents.find(file);
		if (fileContent != threadExecutor->_fileContents.end()) {
			if (threadExecutor->_analyzeFile) {
				pGlobalData->RecoredExportClass(true);
				fileChecker.analyze(file, fileContent->second);
			}
			else {
				pGlobalData->RecoredExportClass(false);
				result += fileChecker.check(file, fileContent->second);
			}
		}
		else {
			if (threadExecutor->_analyzeFile) {
				pGlobalData->RecoredExportClass(true);
				fileChecker.analyze(file);
			}
			else {
				pGlobalData->RecoredExportClass(false);
				result += fileChecker.check(file);
			}
		}

		TSC_LOCK_ENTER(&threadExecutor->_fileSync);

		threadExecutor->_processedSize += fileSize;
		threadExecutor->_processedFiles++;
		if (!threadExecutor->_settings.quiet) {
			TSC_LOCK_ENTER(&threadExecutor->_reportSync);
			TscanCodeExecutor::reportStatus(threadIndex, threadExecutor->_processedFiles, threadExecutor->_totalFiles, threadExecutor->_processedSize, threadExecutor->_totalFileSize, threadExecutor->_analyzeFile, false, file);
			TSC_LOCK_LEAVE(&threadExecutor->_reportSync);
		}

	}

	if (!threadExecutor->_analyzeFile)
	{
		TSC_LOCK_ENTER(&threadExecutor->_fileSync);
		std::list<std::string>& errorList = CGlobalErrorList::Instance()->GetThreadErrorList(&fileChecker);
		TSC_LOCK_LEAVE(&threadExecutor->_fileSync);
		fileChecker.SyncErrorList(errorList);
	}

    return NULL;
}

#ifdef TSC_THREADING_MODEL_WIN
unsigned int __stdcall TscThreadExecutor::threadProc_initMacros(void *args)
#else
void* TscThreadExecutor::threadProc_initMacros(void *args)
#endif
{
	TscThreadExecutor *threadExecutor = static_cast<TscThreadExecutor*>(args);
	TSC_LOCK_ENTER(&threadExecutor->_fileSync);
	int threadIndex = threadExecutor->_threadIndex;
	++threadExecutor->_threadIndex;

	Preprocessor preprocessor(threadExecutor->_settings);

	for (;;) {
		if (nullptr == threadExecutor->_curFile) {
			TSC_LOCK_LEAVE(&threadExecutor->_fileSync);
			break;
		}

		CCodeFile* curFile = threadExecutor->_curFile;
		const std::string &file = curFile->GetFullPath();
		const std::size_t fileSize = curFile->GetSize();
		threadExecutor->_curFile = curFile->GetNext();

		TSC_LOCK_LEAVE(&threadExecutor->_fileSync);

		if (!threadExecutor->_settings.quiet && threadExecutor->_settings.debug) {
			TSC_LOCK_ENTER(&threadExecutor->_reportSync);
			CGlobalMacros::reportStatus(threadIndex, true, threadExecutor->_processedFiles, threadExecutor->_totalFiles, threadExecutor->_processedSize, threadExecutor->_totalFileSize, file);
			TSC_LOCK_LEAVE(&threadExecutor->_reportSync);
		}

		// get macros
		preprocessor.getMacros(curFile);

		TSC_LOCK_ENTER(&threadExecutor->_fileSync);

		threadExecutor->_processedSize += fileSize;
		threadExecutor->_processedFiles++;
		if (!threadExecutor->_settings.quiet) {
			TSC_LOCK_ENTER(&threadExecutor->_reportSync);
			CGlobalMacros::reportStatus(threadIndex, false, threadExecutor->_processedFiles, threadExecutor->_totalFiles, threadExecutor->_processedSize, threadExecutor->_totalFileSize, file);
			TSC_LOCK_LEAVE(&threadExecutor->_reportSync);
		}

	}
	return NULL;
}

unsigned int TscThreadExecutor::initMacros()
{
	CGlobalMacros::SetFileTable(_pFileTable);
	_curFile = _pFileTable->GetFirstFile();

	_processedFiles = 0;
	_processedSize = 0;
	_totalFiles = 0;
	_totalFileSize = 0;

	CCodeFile* pFile = _pFileTable->GetFirstFile();
	while (pFile)
	{
		_totalFileSize += pFile->GetSize();
		++_totalFiles;
		pFile = pFile->GetNext();
	}

	TSC_LOCK_INIT(&CGlobalMacros::MacroLock);
	TSC_LOCK_INIT(&CGlobalTypedefs::TypedefLock);

	unsigned ret = multi_thread(TscThreadExecutor::threadProc_initMacros);

	if (_settings.debugDumpGlobal)
	{
		CGlobalMacros::DumpMacros();
		CGlobalTypedefs::DumpTypedef();
	}
	
	TSC_LOCK_DELETE(&CGlobalMacros::MacroLock);
	TSC_LOCK_DELETE(&CGlobalTypedefs::TypedefLock);

	return ret;
}

void TscThreadExecutor::reportOut(const std::string &outmsg)
{
    TSC_LOCK_ENTER(&_reportSync);

    _errorLogger.reportOut(outmsg);

    TSC_LOCK_LEAVE(&_reportSync);
}

unsigned int TscThreadExecutor::multi_thread(ThreadProc threadProc)
{

	TSC_THREAD *threadHandles = new TSC_THREAD[_settings._jobs];

	TSC_LOCK_INIT(&_fileSync);
	TSC_LOCK_INIT(&_errorSync);
	TSC_LOCK_INIT(&_reportSync);


#ifdef TSC_THREADING_MODEL_WIN

	for (unsigned int i = 0; i < _settings._jobs; ++i) {
		threadHandles[i] = (HANDLE)_beginthreadex(NULL, 0, threadProc, this, 0, NULL);
		if (!threadHandles[i]) {
			std::cerr << "#### .\nTscThreadExecutor::check error, errno :" << errno << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	DWORD waitResult = WaitForMultipleObjects(_settings._jobs, threadHandles, TRUE, INFINITE);
	if (waitResult != WAIT_OBJECT_0) {
		if (waitResult == WAIT_FAILED) {
			std::cerr << "#### .\nTscThreadExecutor::check wait failed, result: " << waitResult << " error: " << GetLastError() << std::endl;
			exit(EXIT_FAILURE);
		}
		else {
			std::cerr << "#### .\nTscThreadExecutor::check wait failed, result: " << waitResult << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	unsigned int result = 0;
	for (unsigned int i = 0; i < _settings._jobs; ++i) {
		DWORD exitCode;

		if (!GetExitCodeThread(threadHandles[i], &exitCode)) {
			std::cerr << "#### .\nTscThreadExecutor::check get exit code failed, error:" << GetLastError() << std::endl;
			exit(EXIT_FAILURE);
		}

		result += exitCode;

		if (!CloseHandle(threadHandles[i])) {
			std::cerr << "#### .\nTscThreadExecutor::check close handle failed, error:" << GetLastError() << std::endl;
			exit(EXIT_FAILURE);
		}
	}

#else

	for (unsigned int i = 0; i < _settings._jobs; ++i) {
		int ret = pthread_create(&threadHandles[i], nullptr, threadProc, this);
		if (ret) {
			std::cerr << "#### .\nTscThreadExecutor::check error, errno :" << ret << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	unsigned int result = 0;
	void* tret = nullptr;
	for (int i = 0; i < _settings._jobs; ++i) {
		int ret = pthread_join(threadHandles[i], &tret);
		if (ret) {
			std::cerr << "#### .\nTscThreadExecutor::check get exit code failed, error:" << ret << std::endl;
			exit(EXIT_FAILURE);
		}
		result += (unsigned int)(intptr_t)(tret);
	}
#endif

	TSC_LOCK_DELETE(&_fileSync);
	TSC_LOCK_DELETE(&_errorSync);
	TSC_LOCK_DELETE(&_reportSync);

	delete[] threadHandles;

	return result;
}

void TscThreadExecutor::report(const ErrorLogger::ErrorMessage &msg, MessageType msgType)
{
    std::string file;
    unsigned int line(0);
    if (!msg._callStack.empty()) {
        file = msg._callStack.back().getfile(false);
        line = msg._callStack.back().line;
    }

    if (_settings.nomsg.isSuppressed(msg._id, file, line))
        return;

    // Alert only about unique errors
    bool reportError = false;
    std::string errmsg = msg.toString(_settings._verbose);

    TSC_LOCK_ENTER(&_errorSync);
    if (std::find(_errorList.begin(), _errorList.end(), errmsg) == _errorList.end()) {
        _errorList.push_back(errmsg);
        reportError = true;
    }
    TSC_LOCK_LEAVE(&_errorSync);

    if (reportError) {
        TSC_LOCK_ENTER(&_reportSync);

        switch (msgType) {
        case REPORT_ERROR:
            _errorLogger.reportErr(msg);
            break;
        case REPORT_INFO:
            _errorLogger.reportInfo(msg);
            break;
        }

        TSC_LOCK_LEAVE(&_reportSync);
    }
}

int splitString(const std::string & strSrc, const std::string& strDelims, std::vector<std::string>& strDest)
{
	typedef std::string::size_type ST;
	std::string delims = strDelims;
	std::string STR;
	if (delims.empty()) delims = "/n/r";
	ST pos = 0, LEN = strSrc.size();
	while (pos < LEN){
		STR = "";
		while ((delims.find(strSrc[pos]) != std::string::npos) && (pos < LEN)) ++pos;
		if (pos == LEN) return strDest.size();
		while ((delims.find(strSrc[pos]) == std::string::npos) && (pos < LEN)) STR += strSrc[pos++];
		if (!STR.empty()) strDest.push_back(STR);
	}
	return strDest.size();
}

bool TscThreadExecutor::isInAutoFilterSubID(const std::string &erroritem)
{
	std::vector<std::string> autoFilterSubidList;
	splitString(AUTOFILTER_SUBID, "|", autoFilterSubidList);
	for (unsigned int i = 0; i<autoFilterSubidList.size(); i++)
	{
		if (std::strstr(erroritem.c_str(), autoFilterSubidList[i].c_str()))
		{
			return true;
		}
	}
	return false;
}

void TscThreadExecutor::autoFilterResult(std::set< std::string>& errorList)
{
	std::set<std::string>::iterator iter;
	std::map<std::string, int> errCount;
	for (iter = errorList.begin(); iter != errorList.end(); iter++)
	{
		std::string errkey = *iter;
		std::string::size_type pos = 0;
		
		if (_settings._xml)
		{
			if ((pos = errkey.find("subid=")) != std::string::npos)
			{
				std::string::size_type posend = 0;
				if ((posend = errkey.find("func_info=")) != std::string::npos && posend>pos)
				{
					errkey = errkey.substr(pos, posend - pos - 1);
				}
				else
				{
					errkey = errkey.substr(pos);
				}
				errCount[errkey]++;
			}
		}
	}
	
	std::map<std::string, int>::iterator miter;
	for (miter = errCount.begin(); miter != errCount.end(); miter++)
	{
		std::string errkey = miter->first;
		if (miter->second >= MAXERRORCNT)
		{
			for (iter = errorList.begin(); iter != errorList.end();)
			{
				if ((*iter).find(errkey) != std::string::npos && isInAutoFilterSubID(*iter))
				{
					 errorList.erase(iter++);
				}
				else
				{
					iter++;
				}
			}
		}
	}
}
