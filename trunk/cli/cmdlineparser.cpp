#include "cmdlineparser.h"
#include "tscancode.h"
#include "tscexecutor.h"
#include "filelister.h"
#include "path.h"
#include "settings.h"
#include "timer.h"
#include "check.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdlib> // EXIT_FAILURE

#ifdef HAVE_RULES
// xml is used in rules
#include <tinyxml2.h>
#endif

static void AddFilesToList(const std::string& FileList, std::vector<std::string>& PathNames)
{
    // to keep things initially simple, if the file can't be opened, just be
    // silent and move on
    // ideas : we could also require this should be an xml file, with the filenames
    // specified in an xml structure
    // we could elaborate this then, to also include the I-paths, ...
    // basically for everything that makes the command line very long
    // xml is a bonus then, since we can easily extend it
    // we need a good parser then -> suggestion : TinyXml
    // drawback : creates a dependency
    std::istream *Files;
    std::ifstream Infile;
    if (FileList.compare("-") == 0) { // read from stdin
        Files = &std::cin;
    } else {
        Infile.open(FileList.c_str());
        Files = &Infile;
    }
    if (Files) {
        std::string FileName;
        while (std::getline(*Files, FileName)) { // next line
            if (!FileName.empty()) {
                PathNames.push_back(FileName);
            }
        }
    }
}

static void AddInclPathsToList(const std::string& FileList, std::list<std::string>* PathNames)
{
    // to keep things initially simple, if the file can't be opened, just be
    // silent and move on
    std::ifstream Files(FileList.c_str());
    if (Files) {
        std::string PathName;
        while (std::getline(Files, PathName)) { // next line
            if (!PathName.empty()) {
                PathName = Path::fromNativeSeparators(PathName);
                PathName = Path::removeQuotationMarks(PathName);

                // If path doesn't end with / or \, add it
                if (*PathName.rbegin() != '/')
                    PathName += '/';

                PathNames->push_back(PathName);
            }
        }
    }
}

static void AddPathsToSet(const std::string& FileName, std::set<std::string>* set)
{
    std::list<std::string> templist;
    AddInclPathsToList(FileName, &templist);
    set->insert(templist.begin(), templist.end());
}

CmdLineParser::CmdLineParser(Settings *settings)
    : _settings(settings)
    , _showHelp(false)
    , _showVersion(false)
    , _showErrorMessages(false)
    , _exitAfterPrint(false)
	, _mergecfg(false)
	
{
}

void CmdLineParser::PrintMessage(const std::string &message)
{
    std::cout << message << std::endl;
}

bool CmdLineParser::ParseFromArgs(int argc, const char* const argv[])
{
    bool def = false;
    bool maxconfigs = false;

    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--version") == 0) {
            _showVersion = true;
            _exitAfterPrint = true;
            return true;
        }
		else if (std::strcmp(argv[i], "--no-analyze") == 0)
		{
			_settings->_no_analyze = true;
		}
		else if (std::strcmp(argv[i], "--no-check") == 0)
		{
			_settings->_no_check = true;
		}
        // Flag used for various purposes during debugging
        else if (std::strcmp(argv[i], "--debug") == 0)
            _settings->debug = true;
        // Show --debug output after the first simplifications
        else if (std::strcmp(argv[i], "--debug-normal") == 0)
            _settings->debugnormal = true;
        // Show debug warnings
        else if (std::strcmp(argv[i], "--debug-warnings") == 0)
            _settings->debugwarnings = true;

        // Print out code that triggers false positive
		else if (std::strcmp(argv[i], "--debug-fp") == 0)
			_settings->debugFalsePositive = true;
		else if (std::strcmp(argv[i], "--dump-global") == 0)
			_settings->debugDumpGlobal = true;
		else if (std::strcmp(argv[i], "--dump-np") == 0)
			_settings->debugDumpNP = true;
		else if (std::strcmp(argv[i], "--dump-simp1") == 0)
			_settings->debugDumpSimp1 = true;
		else if (std::strcmp(argv[i], "--dump-simp2") == 0)
			_settings->debugDumpSimp2 = true;
        else if (std::strcmp(argv[i], "--dump") == 0)
            _settings->dump = true;

        else if (std::strcmp(argv[i], "--exception-handling") == 0)
            _settings->exceptionHandling = true;
        else if (std::strncmp(argv[i], "--exception-handling=", 21) == 0) {
            _settings->exceptionHandling = true;
            const std::string exceptionOutfilename=&(argv[i][21]);
            TscanCodeExecutor::setExceptionOutput(exceptionOutfilename);
        }

        // Inconclusive checking
        else if (std::strcmp(argv[i], "--inconclusive") == 0)
            _settings->inconclusive = true;

        // Enforce language (--language=, -x)
        else if (std::strncmp(argv[i], "--language=", 11) == 0 || std::strcmp(argv[i], "-x") == 0) {
            std::string str;
            if (argv[i][2]) {
                str = argv[i]+11;
            } else {
                i++;
                if (i >= argc || argv[i][0] == '-') {
                    PrintMessage("TscanCode: No language given to '-x' option.");
                    return false;
                }
                str = argv[i];
            }

            if (str == "c")
                _settings->enforcedLang = Settings::C;
            else if (str == "c++")
                _settings->enforcedLang = Settings::CPP;
            else {
                PrintMessage("TscanCode: Unknown language '" + str + "' enforced.");
                return false;
            }
        }

        // Filter errors
        else if (std::strncmp(argv[i], "--exitcode-suppressions", 23) == 0) {
            std::string filename;

            // exitcode-suppressions filename.txt
            if (std::strcmp(argv[i], "--exitcode-suppressions") == 0) {
                // Error message to be removed in 1.72
                PrintMessage("TscanCode: '--exitcode-suppressions <file>' has been removed, use '--exitcode-suppressions=<file>' instead.");
                return false;
            }

            // exitcode-suppressions=filename.txt
            filename = 24 + argv[i];

            std::ifstream f(filename.c_str());
            if (!f.is_open()) {
                PrintMessage("TscanCode: Couldn't open the file: \"" + filename + "\".");
                return false;
            }
            const std::string errmsg(_settings->nofail.parseFile(f));
            if (!errmsg.empty()) {
                PrintMessage(errmsg);
                return false;
            }
        }

        // Filter errors
        else if (std::strncmp(argv[i], "--suppressions-list=", 20) == 0) {
            std::string filename = argv[i]+20;
            std::ifstream f(filename.c_str());
            if (!f.is_open()) {
                std::string message("TscanCode: Couldn't open the file: \"");
                message += filename;
                message += "\".";
                if (std::count(filename.begin(), filename.end(), ',') > 0 ||
                    std::count(filename.begin(), filename.end(), '.') > 1) {
                    // If user tried to pass multiple files (we can only guess that)
                    // e.g. like this: --suppressions-list=a.txt,b.txt
                    // print more detailed error message to tell user how he can solve the problem
                    message += "\nIf you want to pass two files, you can do it e.g. like this:";
                    message += "\n    TscanCode --suppressions-list=a.txt --suppressions-list=b.txt file.cpp";
                }

                PrintMessage(message);
                return false;
            }
            const std::string errmsg(_settings->nomsg.parseFile(f));
            if (!errmsg.empty()) {
                PrintMessage(errmsg);
                return false;
            }
        }

        else if (std::strcmp(argv[i], "--suppressions") == 0) {
            // Error message to be removed in 1.72
            PrintMessage("TscanCode: '--suppressions' has been removed, use '--suppressions-list=<file>' instead.");
            return false;
        }

        else if (std::strncmp(argv[i], "--suppress=", 11) == 0) {
            std::string suppression = argv[i]+11;
            const std::string errmsg(_settings->nomsg.addSuppressionLine(suppression));
            if (!errmsg.empty()) {
                PrintMessage(errmsg);
                return false;
            }
        }

        // Enables inline suppressions.
        else if (std::strcmp(argv[i], "--inline-suppr") == 0)
            _settings->_inlineSuppressions = true;

        // Verbose error messages (configuration info)
        else if (std::strcmp(argv[i], "-v") == 0 || std::strcmp(argv[i], "--verbose") == 0)
            _settings->_verbose = true;

        // Force checking of files that have "too many" configurations
        else if (std::strcmp(argv[i], "-f") == 0 || std::strcmp(argv[i], "--force") == 0)
            _settings->_force = true;

        // Output relative paths
        else if (std::strcmp(argv[i], "-rp") == 0 || std::strcmp(argv[i], "--relative-paths") == 0)
            _settings->_relativePaths = true;
        else if (std::strncmp(argv[i], "-rp=", 4) == 0 || std::strncmp(argv[i], "--relative-paths=", 17) == 0) {
            _settings->_relativePaths = true;
            if (argv[i][argv[i][3]=='='?4:17] != 0) {
                std::string paths = argv[i]+(argv[i][3]=='='?4:17);
                for (;;) {
                    std::string::size_type pos = paths.find(';');
                    if (pos == std::string::npos) {
                        _settings->_basePaths.push_back(Path::fromNativeSeparators(paths));
                        break;
                    } else {
                        _settings->_basePaths.push_back(Path::fromNativeSeparators(paths.substr(0, pos)));
                        paths.erase(0, pos + 1);
                    }
                }
            } else {
                PrintMessage("TscanCode: No paths specified for the '" + std::string(argv[i]) + "' option.");
                return false;
            }
        }

        // Write results in results.xml
        else if (std::strcmp(argv[i], "--xml") == 0)
            _settings->_xml = true;

        // Define the XML file version (and enable XML output)
        else if (std::strncmp(argv[i], "--xml-version=", 14) == 0) {
            std::string numberString(argv[i]+14);

            std::istringstream iss(numberString);
            if (!(iss >> _settings->_xml_version)) {
                PrintMessage("TscanCode: argument to '--xml-version' is not a number.");
                return false;
            }

            if (_settings->_xml_version < 0 || _settings->_xml_version > 2) {
                // We only have xml versions 1 and 2
                PrintMessage("TscanCode: '--xml-version' can only be 1 or 2.");
                return false;
            }

            // Enable also XML if version is set
            _settings->_xml = true;
        }

        // Only print something when there are errors
        else if (std::strcmp(argv[i], "-q") == 0 || std::strcmp(argv[i], "--quiet") == 0)
            _settings->quiet = true;

        // Append user-defined code to checked source code
        else if (std::strncmp(argv[i], "--append=", 9) == 0) {
            const std::string filename = 9 + argv[i];
            if (!_settings->append(filename)) {
                PrintMessage("TscanCode: Couldn't open the file: \"" + filename + "\".");
                return false;
            }
        }

		// Append user-defined code to checked source code
		else if (std::strncmp(argv[i], "--lua", 5) == 0) {
			++i;
			if (i >= argc || argv[i][0] == '-') {
				PrintMessage("TscanCode: missing --lua filePath");
				return false;
			}
			_settings->luaPath = argv[i];
			_settings->_checkLua = true;

		}

        // Check configuration
        else if (std::strcmp(argv[i], "--check-config") == 0) {
            _settings->checkConfiguration = true;
        }

        // Check library definitions
        else if (std::strcmp(argv[i], "--check-library") == 0) {
            _settings->checkLibrary = true;
        }

        else if (std::strncmp(argv[i], "--enable=", 9) == 0) {
			//do nothing, for compatibility only
        }

        // --error-exitcode=1
        else if (std::strncmp(argv[i], "--error-exitcode=", 17) == 0) {
            std::string temp = argv[i]+17;
            std::istringstream iss(temp);
            if (!(iss >> _settings->_exitCode)) {
                _settings->_exitCode = 0;
                PrintMessage("TscanCode: Argument must be an integer. Try something like '--error-exitcode=1'.");
                return false;
            }
        }

        // User define
        else if (std::strncmp(argv[i], "-D", 2) == 0) {
            std::string define;

            // "-D define"
            if (std::strcmp(argv[i], "-D") == 0) {
                ++i;
                if (i >= argc || argv[i][0] == '-') {
                    PrintMessage("TscanCode: argument to '-D' is missing.");
                    return false;
                }

                define = argv[i];
            }
            // "-Ddefine"
            else {
                define = 2 + argv[i];
            }

            // No "=", append a "=1"
            if (define.find('=') == std::string::npos)
                define += "=1";

            if (!_settings->userDefines.empty())
                _settings->userDefines += ";";
            _settings->userDefines += define;

            def = true;
        }
        // User undef
        else if (std::strncmp(argv[i], "-U", 2) == 0) {
            std::string undef;

            // "-U undef"
            if (std::strcmp(argv[i], "-U") == 0) {
                ++i;
                if (i >= argc || argv[i][0] == '-') {
                    PrintMessage("TscanCode: argument to '-U' is missing.");
                    return false;
                }

                undef = argv[i];
            }
            // "-Uundef"
            else {
                undef = 2 + argv[i];
            }

            _settings->userUndefs.insert(undef);
        }

        // Include paths
        else if (std::strncmp(argv[i], "-I", 2) == 0) {
            std::string path;

            // "-I path/"
            if (std::strcmp(argv[i], "-I") == 0) {
                ++i;
                if (i >= argc || argv[i][0] == '-') {
                    PrintMessage("TscanCode: argument to '-I' is missing.");
                    return false;
                }
                path = argv[i];
            }

            // "-Ipath/"
            else {
                path = 2 + argv[i];
            }
            path = Path::fromNativeSeparators(path);
            path = Path::removeQuotationMarks(path);

            // If path doesn't end with / or \, add it
            if (*path.rbegin() != '/')
                path += '/';

            _settings->_includePaths.push_back(path);
        } else if (std::strncmp(argv[i], "--include=", 10) == 0) {
            std::string path = argv[i] + 10;

            path = Path::fromNativeSeparators(path);

            _settings->userIncludes.push_back(path);
        } else if (std::strncmp(argv[i], "--includes-file=", 16) == 0) {
            // open this file and read every input file (1 file name per line)
            AddInclPathsToList(16 + argv[i], &_settings->_includePaths);
        } else if (std::strncmp(argv[i], "--config-exclude=",17) ==0) {
            std::string path = argv[i] + 17;
            path = Path::fromNativeSeparators(path);
            _settings->configExcludePaths.insert(path);
        } else if (std::strncmp(argv[i], "--config-excludes-file=", 23) == 0) {
            // open this file and read every input file (1 file name per line)
            AddPathsToSet(23 + argv[i], &_settings->configExcludePaths);
        }

        // file list specified
        else if (std::strncmp(argv[i], "--file-list=", 12) == 0) {
            // open this file and read every input file (1 file name per line)
            AddFilesToList(12 + argv[i], _pathnames);
        }

        // Ignored paths
        else if (std::strncmp(argv[i], "-i", 2) == 0) {
            std::string path;

            // "-i path/"
            if (std::strcmp(argv[i], "-i") == 0) {
                ++i;
                if (i >= argc || argv[i][0] == '-') {
                    PrintMessage("TscanCode: argument to '-i' is missing.");
                    return false;
                }
                path = argv[i];
            }

            // "-ipath/"
            else {
                path = 2 + argv[i];
            }

            if (!path.empty()) {
                path = Path::fromNativeSeparators(path);
                path = Path::simplifyPath(path);
                path = Path::removeQuotationMarks(path);

                if (FileLister::isDirectory(path)) {
                    // If directory name doesn't end with / or \, add it
                    if (*path.rbegin() != '/')
                        path += '/';
                }
                //_ignoredPaths.push_back(path);
				//save to _settings directly
				_settings->_pathToIgnore.push_back(path);
            }
        }

        // --library
        else if (std::strncmp(argv[i], "--library=", 10) == 0) {
            if (!TscanCodeExecutor::tryLoadLibrary(_settings->library, argv[0], argv[i]+10))
                return false;
        }

        // Report progress
        else if (std::strcmp(argv[i], "--report-progress") == 0) {
            _settings->reportProgress = true;
        }

        // --std
        else if (std::strcmp(argv[i], "--std=posix") == 0) {
            _settings->standards.posix = true;
        } else if (std::strcmp(argv[i], "--std=c89") == 0) {
            _settings->standards.c = Standards::C89;
        } else if (std::strcmp(argv[i], "--std=c99") == 0) {
            _settings->standards.c = Standards::C99;
        } else if (std::strcmp(argv[i], "--std=c11") == 0) {
            _settings->standards.c = Standards::C11;
        } else if (std::strcmp(argv[i], "--std=c++03") == 0) {
            _settings->standards.cpp = Standards::CPP03;
        } else if (std::strcmp(argv[i], "--std=c++11") == 0) {
            _settings->standards.cpp = Standards::CPP11;
        }

        // Output formatter
        else if (std::strcmp(argv[i], "--template") == 0 ||
                 std::strncmp(argv[i], "--template=", 11) == 0) {
            // "--template path/"
            if (argv[i][10] == '=')
                _settings->_outputFormat = argv[i] + 11;
            else if ((i+1) < argc && argv[i+1][0] != '-') {
                ++i;
                _settings->_outputFormat = argv[i];
            } else {
                PrintMessage("TscanCode: argument to '--template' is missing.");
                return false;
            }

            if (_settings->_outputFormat == "gcc")
                _settings->_outputFormat = "{file}:{line}: {severity}: {message}";
            else if (_settings->_outputFormat == "vs")
                _settings->_outputFormat = "{file}({line}): {severity}: {message}";
            else if (_settings->_outputFormat == "edit")
                _settings->_outputFormat = "{file} +{line}: {severity}: {message}";
        }

        // Checking threads
        else if (std::strncmp(argv[i], "-j", 2) == 0) {
            std::string numberString;

            // "-j 3"
            if (std::strcmp(argv[i], "-j") == 0) {
                ++i;
                if (i >= argc || argv[i][0] == '-') {
                    PrintMessage("TscanCode: argument to '-j' is missing.");
                    return false;
                }

                numberString = argv[i];
            }

            // "-j3"
            else
                numberString = argv[i]+2;

            std::istringstream iss(numberString);
            if (!(iss >> _settings->_jobs)) {
                PrintMessage("TscanCode: argument to '-j' is not a number.");
                return false;
            }

            if (_settings->_jobs > 10000) {
                // This limit is here just to catch typos. If someone has
                // need for more jobs, this value should be increased.
                PrintMessage("TscanCode: argument for '-j' is allowed to be 10000 at max.");
                return false;
            }
        } else if (std::strncmp(argv[i], "-l", 2) == 0) {
            std::string numberString;

            // "-l 3"
            if (std::strcmp(argv[i], "-l") == 0) {
                ++i;
                if (i >= argc || argv[i][0] == '-') {
                    PrintMessage("TscanCode: argument to '-l' is missing.");
                    return false;
                }

                numberString = argv[i];
            }

            // "-l3"
            else
                numberString = argv[i]+2;

            std::istringstream iss(numberString);
            if (!(iss >> _settings->_loadAverage)) {
                PrintMessage("TscanCode: argument to '-l' is not a number.");
                return false;
            }
        }

        // print all possible error messages..
        else if (std::strcmp(argv[i], "--errorlist") == 0) {
            _showErrorMessages = true;
            _settings->_xml = true;
            _exitAfterPrint = true;
        }

        // documentation..
        else if (std::strcmp(argv[i], "--doc") == 0) {
            std::ostringstream doc;
            // Get documentation..
            for (std::list<Check *>::iterator it = Check::instances().begin(); it != Check::instances().end(); ++it) {
                const std::string& name((*it)->name());
                const std::string info((*it)->classInfo());
                if (!name.empty() && !info.empty())
                    doc << "## " << name << " ##\n"
                        << info << "\n";
            }

            std::cout << doc.str();
            _exitAfterPrint = true;
            return true;
        }

        // show timing information..
        else if (std::strncmp(argv[i], "--showtime=", 11) == 0) {
            const std::string showtimeMode = argv[i] + 11;
            if (showtimeMode == "file")
                _settings->_showtime = SHOWTIME_FILE;
            else if (showtimeMode == "summary")
                _settings->_showtime = SHOWTIME_SUMMARY;
            else if (showtimeMode == "top5")
                _settings->_showtime = SHOWTIME_TOP5;
            else if (showtimeMode.empty())
                _settings->_showtime = SHOWTIME_NONE;
            else {
                std::string message("TscanCode: error: unrecognized showtime mode: \"");
                message += showtimeMode;
                message +=  "\". Supported modes: file, summary, top5.";
                PrintMessage(message);
                return false;
            }
        }
#ifdef HAVE_RULES
        // Rule given at command line
        else if (std::strncmp(argv[i], "--rule=", 7) == 0) {
            Settings::Rule rule;
            rule.pattern = 7 + argv[i];
            _settings->rules.push_back(rule);
        }

        // Rule file
        else if (std::strncmp(argv[i], "--rule-file=", 12) == 0) {
            tinyxml2::XMLDocument doc;
            if (doc.LoadFile(12+argv[i]) == tinyxml2::XML_NO_ERROR) {
                tinyxml2::XMLElement *node = doc.FirstChildElement();
                for (; node && strcmp(node->Value(), "rule") == 0; node = node->NextSiblingElement()) {
                    Settings::Rule rule;

                    tinyxml2::XMLElement *tokenlist = node->FirstChildElement("tokenlist");
                    if (tokenlist)
                        rule.tokenlist = tokenlist->GetText();

                    tinyxml2::XMLElement *pattern = node->FirstChildElement("pattern");
                    if (pattern) {
                        rule.pattern = pattern->GetText();
                    }

                    tinyxml2::XMLElement *message = node->FirstChildElement("message");
                    if (message) {
                        tinyxml2::XMLElement *severity = message->FirstChildElement("severity");
                        if (severity)
                            rule.severity = severity->GetText();

                        tinyxml2::XMLElement *id = message->FirstChildElement("id");
                        if (id)
                            rule.id = id->GetText();

                        tinyxml2::XMLElement *summary = message->FirstChildElement("summary");
                        if (summary)
                            rule.summary = summary->GetText() ? summary->GetText() : "";
                    }

                    if (!rule.pattern.empty())
                        _settings->rules.push_back(rule);
                }
            }
        }
#endif

        // Specify platform
        else if (std::strncmp(argv[i], "--platform=", 11) == 0) {
            std::string platform(11+argv[i]);

            if (platform == "win32A")
                _settings->platform(Settings::Win32A);
            else if (platform == "win32W")
                _settings->platform(Settings::Win32W);
            else if (platform == "win64")
                _settings->platform(Settings::Win64);
            else if (platform == "unix32")
                _settings->platform(Settings::Unix32);
            else if (platform == "unix64")
                _settings->platform(Settings::Unix64);
            else if (platform == "native")
                _settings->platform(Settings::Unspecified);
            else {
                std::string message("TscanCode: error: unrecognized platform: \"");
                message += platform;
                message +=  "\".";
                PrintMessage(message);
                return false;
            }
        }

        // Set maximum number of #ifdef configurations to check
        else if (std::strncmp(argv[i], "--max-configs=", 14) == 0) {
            _settings->_force = false;

            std::istringstream iss(14+argv[i]);
            if (!(iss >> _settings->_maxConfigs)) {
                PrintMessage("TscanCode: argument to '--max-configs=' is not a number.");
                return false;
            }

            if (_settings->_maxConfigs < 1) {
                PrintMessage("TscanCode: argument to '--max-configs=' must be greater than 0.");
                return false;
            }

            maxconfigs = true;
        }
        else if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            _pathnames.clear();
            _showHelp = true;
            _exitAfterPrint = true;
            break;
        }
		else if (strcmp(argv[i], "-rf") == 0 || strcmp(argv[i], "--recordfuncinfo") == 0)
			_settings->_recordFuncinfo = true;
		else if (std::strncmp(argv[i], "--cfg-merge", 11) == 0)
		{
			++i;
			if (i >= argc || argv[i][0] == '-') {
				PrintMessage("TscanCode: argument to '--cfg-merge' is missing. The correct format is --cfg-merge [new_cfg_xml] [old_cfg_xml]");
				return false;
			}
			_newcfg = argv[i];
			++i;
			if (i >= argc || argv[i][0] == '-') {
				PrintMessage("TscanCode: argument to '--cfg-merge' is missing. The correct format is --cfg-merge [new_cfg_xml] [old_cfg_xml]");
				return false;
			}
			_oldcfg = argv[i];

			_mergecfg = true;
		}
		else if (std::strncmp(argv[i], "-e2o", 11) == 0)
		{
			_settings->_e2o = true;
		}
        else if (argv[i][0] == '-') {
            std::string message("TscanCode: error: unrecognized command line option: \"");
            message += argv[i];
            message +=  "\".";
            PrintMessage(message);
            return false;
        }


        else {
            std::string path = Path::fromNativeSeparators(argv[i]);
            path = Path::removeQuotationMarks(path);
            _pathnames.push_back(path);
        }
    }

    if (def && !_settings->_force && !maxconfigs)
        _settings->_maxConfigs = 1U;

    if (_settings->_force)
        _settings->_maxConfigs = ~0U;

    if (_settings->isEnabled("unusedFunction") && _settings->_jobs > 1) {
        PrintMessage("TscanCode: unusedFunction check can't be used with '-j' option. Disabling unusedFunction check.");
    }

    if (argc <= 1) {
        _showHelp = true;
        _exitAfterPrint = true;
    }

    if (_showHelp) {
        PrintHelp();
        return true;
    }

    // Print error only if we have "real" command and expect files
    if (!_exitAfterPrint && !_mergecfg && _pathnames.empty()) {
        PrintMessage("TscanCode: No C or C++ source files found.");
        return false;
    }

    // Use paths _pathnames if no base paths for relative path output are given
    if (_settings->_basePaths.empty() && _settings->_relativePaths)
        _settings->_basePaths = _pathnames;

	//default enable all checks
	(void)_settings->addEnabled("all");
    return true;
}

void CmdLineParser::PrintHelp()
{
    std::cout << "TscanCode - A tool for static C/C++ code analysis\n"
              "\n"
              "Syntax:\n"
              "    tscancode [OPTIONS] [files or paths]\n"
              "\n"
              "If a directory is given instead of a filename, *.cpp, *.cxx, *.cc, *.c++, *.c,\n"
              "*.tpp, and *.txx files are checked recursively from the given directory.\n\n"
              "Options:\n"
              "    -D<ID>               Define preprocessor symbol. Unless --max-configs or\n"
              "                         --force is used, TscanCode will only check the given\n"
              "                         configuration when -D is used.\n"
              "                         Example: '-DDEBUG=1 -D__cplusplus'.\n"
              "    -U<ID>               Undefine preprocessor symbol. Use -U to explicitly\n"
              "                         hide certain #ifdef <ID> code paths from checking.\n"
              "                         Example: '-UDEBUG'\n"
              "    --enable=<id>        Enable additional checks. The available ids are:\n"
              "                          * all\n"
              "                                  Enable all checks. It is recommended to only\n"
              "                                  use --enable=all when the whole program is\n"
              "                                  scanned, because this enables unusedFunction.\n"
              "                          * warning\n"
              "                                  Enable warning messages\n"
              "                          * style\n"
              "                                  Enable all coding style checks. All messages\n"
              "                                  with the severities 'style', 'performance' and\n"
              "                                  'portability' are enabled.\n"
              "                          * performance\n"
              "                                  Enable performance messages\n"
              "                          * portability\n"
              "                                  Enable portability messages\n"
              "                          * information\n"
              "                                  Enable information messages\n"
              "                          * unusedFunction\n"
              "                                  Check for unused functions. It is recommend\n"
              "                                  to only enable this when the whole program is\n"
              "                                  scanned.\n"
              "                          * missingInclude\n"
              "                                  Warn if there are missing includes. For\n"
              "                                  detailed information, use '--check-config'.\n"
              "                         Several ids can be given if you separate them with\n"
              "                         commas. See also --std\n"
              "    -h, --help           Print this help.\n"
              "    -I <dir>             Give path to search for include files. Give several -I\n"
              "                         parameters to give several paths. First given path is\n"
              "                         searched for contained header files first. If paths are\n"
              "                         relative to source files, this is not needed.\n"
              "    -j <jobs>            Start [jobs] threads to do the checking simultaneously.\n"
              "    -q, --quiet          Do not show progress reports.\n"
              "    --xml                Write results in xml format to error stream (stderr).\n"
              "\n"
              "Example usage:\n"
              "  # Recursively check the current folder. Print the progress on the screen and\n"
              "  # write errors to a file:\n"
              "  tscancode . 2> err.txt\n"
              "\n"
              "  # Recursively check ../myproject/ and don't print progress:\n"
              "  tscancode --quiet ../myproject/\n"
              "\n"
              "  # Check test.cpp, enable all checks:\n"
              "  tscancode --enable=all test.cpp\n"
              "\n"
              "  # Check f.cpp and search include files from inc1/ and inc2/:\n"
              "  tscancode -I inc1/ -I inc2/ f.cpp\n"
              "\n";
}
