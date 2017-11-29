#ifndef TSCTHREADEXECUTOR_H
#define TSCTHREADEXECUTOR_H

#include <map>
#include <string>
#include <list>
#include "errorlogger.h"
#include "filedepend.h"


#ifdef TSC_THREADING_MODEL_WIN
#include <windows.h>
#endif

class Settings;

#ifdef TSC_THREADING_MODEL_WIN
typedef unsigned __stdcall ThreadProc(void*);
#else
typedef void* ThreadProc(void*);
#endif


/// @addtogroup CLI
/// @{

/**
 * This class will take a list of filenames and settings and check then
 * all files using threads.
 */
class TscThreadExecutor : public ErrorLogger {
public:
    TscThreadExecutor(CFileDependTable* pFileTable, Settings &settings, ErrorLogger &_errorLogger);
    virtual ~TscThreadExecutor();
    unsigned int check(bool bAnalyze = false, bool bCheck = true);

	bool isInAutoFilterSubID(const std::string &erroritem);

	void autoFilterResult(std::set< std::string>& errorList);

	unsigned init();
	unsigned int initMacros();

    virtual void reportOut(const std::string &outmsg);
    virtual void reportErr(const ErrorLogger::ErrorMessage &msg);
    virtual void reportInfo(const ErrorLogger::ErrorMessage &msg);

    /**
     * @brief Add content to a file, to be used in unit testing.
     *
     * @param path File name (used as a key to link with real file).
     * @param content If the file would be a real file, this should be
     * the content of the file.
     */
    void addFileContent(const std::string &path, const std::string &content);

private:
	unsigned int multi_thread(ThreadProc threadProc);
private:
	CFileDependTable* _pFileTable;
	std::list<CCodeFile*> _checkList;
	std::list<CCodeFile*>::iterator _checkList_iter;
	std::list<CCodeFile*>::iterator _checkList_end;
	CCodeFile* _curFile;
    Settings &_settings;
    ErrorLogger &_errorLogger;
    unsigned int _fileCount;

private:
    enum MessageType {REPORT_ERROR, REPORT_INFO};

    std::map<std::string, std::string> _fileContents;
    std::size_t _processedFiles;
    std::size_t _totalFiles;
    std::size_t _processedSize;
    std::size_t _totalFileSize;
    bool _analyzeFile;

    std::list<std::string> _errorList;
	int _threadIndex;

    void report(const ErrorLogger::ErrorMessage &msg, MessageType msgType);
    
    TSC_LOCK	 _errorSync;
    TSC_LOCK	 _reportSync;
    TSC_LOCK	 _fileSync;

#if defined(TSC_THREADING_MODEL_WIN)

    static unsigned __stdcall threadProc(void*);
	static unsigned __stdcall threadProc_initMacros(void*);
    
#else
    
	static void* threadProc(void*);
	static void* threadProc_initMacros(void*);

#endif

public:
    /**
     * @return true if support for threads exist.
     */
    static bool isEnabled() {
#if defined(TSC_THREADING_MODEL_WIN)
        return true;
#elif defined(TSC_THREADING_MODEL_NOT_WIN)
        return true;
#else
        return false;
#endif
    }

private:
    /** disabled copy constructor */
    TscThreadExecutor(const TscThreadExecutor &);

    /** disabled assignment operator */
    void operator=(const TscThreadExecutor &);
};

/// @}

#endif // TSCTHREADEXECUTOR_H
