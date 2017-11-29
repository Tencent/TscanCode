#ifndef TSCEXECUTOR_H
#define TSCEXECUTOR_H

#include "errorlogger.h"
#include <ctime>
#include <set>
#include <string>
#include "filedepend.h"

class TscanCode;
class Settings;
class Library;

/**
 * This class works as an example of how tscancode can be used in external
 * programs without very little knowledge of the internal parts of the
 * program itself. If you wish to use tscancode e.g. as a part of IDE,
 * just rewrite this class for your needs and possibly use other methods
 * from tscancode class instead the ones used here.
 */
 class  TscanCodeExecutor : public ErrorLogger {
public:
    /**
     * Constructor
     */
    TscanCodeExecutor();

    /**
     * Destructor
     */
    virtual ~TscanCodeExecutor();

    /**
     * Starts the checking.
     *
     * @param argc from main()
     * @param argv from main()
     * @return EXIT_FAILURE if arguments are invalid or no input files
     *         were found.
     *         If errors are found and --error-exitcode is used,
     *         given value is returned instead of default 0.
     *         If no errors are found, 0 is returned.
     */
    int check(int argc, const char* const argv[]);

    /**
     * Information about progress is directed here. This should be
     * called by the tscancode class only.
     *
     * @param outmsg Progress message e.g. "Checking main.cpp..."
     */
    virtual void reportOut(const std::string &outmsg);

    /** xml output of errors */
    virtual void reportErr(const ErrorLogger::ErrorMessage &msg);

	/**
	* Helper function to print out errors. Appends a line change.
	* @param errmsg String printed to error stream
	*/
	void reportErr(const std::string &errmsg);

    void reportProgress(const std::string &filename, const char stage[], const std::size_t value);

    /**
     * Output information messages.
     */
    virtual void reportInfo(const ErrorLogger::ErrorMessage &msg);

    /**
     * Information about how many files have been checked
     *
     * @param fileindex This many files have been checked.
     * @param filecount This many files there are in total.
     * @param sizedone The sum of sizes of the files checked.
     * @param sizetotal The total sizes of the files.
     */
    static void reportStatus(int threadIndex, std::size_t fileindex, std::size_t filecount, std::size_t sizedone, std::size_t sizetotal, bool bAnalyze,  bool bStart, const std::string& fileName);

    /**
     * @param fn file name to be used from exception handler
     */
    static void setExceptionOutput(const std::string& fn);
    /**
    * @return file name to be used for output from exception handler
    */
    static const std::string& getExceptionOutput();

    /**
    * Tries to load a library and prints warning/error messages
    * @return false, if an error occured (except unknown XML elements)
    */
    static bool tryLoadLibrary(Library& destination, const char* basepath, const char* filename);

protected:

    /**
     * @brief Parse command line args and get settings and file lists
     * from there.
     *
     * @param tscancode tscancode instance
     * @param argc argc from main()
     * @param argv argv from main()
     * @return false when errors are found in the input
     */
    bool parseFromArgs(TscanCode *tscancode, int argc, const char* const argv[]);

private:

    /**
     * Wrapper around check_internal
     *   - installs optional platform dependent signal handling
     *
     * * @param tscancode tscancode instance
    * @param argc from main()
    * @param argv from main()
     **/
    int check_wrapper(TscanCode& tscancode, int argc, const char* const argv[]);

    /**
    * Starts the checking.
    *
    * @param tscancode tscancode instance
    * @param argc from main()
    * @param argv from main()
    * @return EXIT_FAILURE if arguments are invalid or no input files
    *         were found.
    *         If errors are found and --error-exitcode is used,
    *         given value is returned instead of default 0.
    *         If no errors are found, 0 is returned.
    */
    int check_internal(TscanCode& tscancode, int argc, const char* const argv[]);

	void uninit();

    /**
     *  analyze files
     *
     *  @param settings of tscancode instance
     *
     *  @return EXIT_FAILURE if some error happens.
     *          0 is returned if no errors are found.
     */
    unsigned int analyze(Settings& settings);
    
    /**
     * Pointer to current settings; set while check() is running.
     */
    const Settings* _settings;

    /**
     * Used to filter out duplicate error messages.
     */
    std::set<std::string> _errorList;

	/**
	* File Table contains files to be checked
	*/
	CFileDependTable _fileDependTable;

    /**
     * Report progress time
     */
    std::time_t time1;

    /**
     * Output file name for exception handler
     */
    static std::string exceptionOutput;

    /**
     * Has --errorlist been given?
     */
    bool errorlist;

};

#endif // tscancodeEXECUTOR_H
