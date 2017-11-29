#ifndef CMDLINE_PARSER_H
#define CMDLINE_PARSER_H

#include <vector>
#include <string>

class Settings;

/// @addtogroup CLI
/// @{

/**
 * @brief The command line parser.
 * The command line parser parses options and parameters user gives to
 * tsancode command line.
 *
 * The parser takes a pointer to Settings instance which it will update
 * based on options user has given. Couple of options are handled as
 * class internal options.
 */
class CmdLineParser {
public:
    /**
     * The constructor.
     * @param settings Settings instance that will be modified according to
     * options user has given.
     */
    explicit CmdLineParser(Settings *settings);

    /**
     * Parse given command line.
     * @return true if command line was ok, false if there was an error.
     */
    bool ParseFromArgs(int argc, const char* const argv[]);

    /**
     * Return if user wanted to see program version.
     */
    bool GetShowVersion() const {
        return _showVersion;
    }

    /**
     * Return if user wanted to see list of error messages.
     */
    bool GetShowErrorMessages() const {
        return _showErrorMessages;
    }

    /**
     * Return the path names user gave to command line.
     */
    const std::vector<std::string>& GetPathNames() const {
        return _pathnames;
    }

    /**
     * Return if help is shown to user.
     */
    bool GetShowHelp() const {
        return _showHelp;
    }

	/**
	* Return if help is shown to user.
	*/
	bool GetMergeCfg() const {
		return _mergecfg;
	}

	const std::string& GetNewCfgPath() const 
	{
		return _newcfg;
	}

	const std::string& GetOldCfgPath() const
	{
		return _oldcfg;
	}

    /**
     * Return if we should exit after printing version, help etc.
     */
    bool ExitAfterPrinting() const {
        return _exitAfterPrint;
    }

    /**
     * Return a list of paths user wants to ignore.
     */
    const std::vector<std::string>& GetIgnoredPaths() const {
        return _ignoredPaths;
    }

protected:

    /**
     * Print help text to the console.
     */
    static void PrintHelp();

    /**
     * Print message (to console?).
     */
    static void PrintMessage(const std::string &message);

private:
    std::vector<std::string> _pathnames;
    std::vector<std::string> _ignoredPaths;
    Settings *_settings;
    bool _showHelp;
    bool _showVersion;
    bool _showErrorMessages;
    bool _exitAfterPrint;

	bool _mergecfg;
	std::string _newcfg;
	std::string _oldcfg;
};

/// @}

#endif // CMDLINE_PARSER_H
