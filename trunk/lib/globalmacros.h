#pragma once

#include "config.h"
#include "tokenlist.h"
#include "token.h"


#include <vector>
#include <map>
#ifdef TSC_THREADING_MODEL_WIN
#include <windows.h>
#endif

void skipstring(const std::string &line, std::string::size_type &pos);
std::string trim(const std::string& s);
void getparams(const std::string &line,
	std::string::size_type &pos,
	std::vector<std::string> &params,
	unsigned int &numberOfNewlines,
	bool &endFound);



class CCodeFile;
class CFileDependTable;

/** @brief Class that the preprocessor uses when it expands macros. This class represents a preprocessor macro */
class PreprocessorMacro {
private:
	/** tokens of this macro */
	TokenList tokenlist;

	/** macro parameters */
	std::vector<std::string> _params;

	/** macro definition in plain text */
	const std::string _macro;

	/** does this macro take a variable number of parameters? */
	bool _variadic;

	/** The macro has parentheses but no parameters.. "AAA()" */
	bool _nopar;

	/** disabled assignment operator */
	void operator=(const PreprocessorMacro &);

	/** @brief expand inner macro */
	std::vector<std::string> expandInnerMacros(const std::vector<std::string> &params1,
		std::map<std::string, PreprocessorMacro *> &macroBuffer, CCodeFile* pCodeFile) const;


public:
	/**
	* @brief Constructor for PreprocessorMacro. This is the "setter"
	* for this class - everything is setup here.
	* @param [in] macro The code after define, until end of line,
	* e.g. "A(x) foo(x);"
	* @param [in] settings Current settings being used
	*/
	PreprocessorMacro(const std::string &macro, const Settings* settings);

	/** return tokens of this macro */
	const Token *tokens() const {
		return tokenlist.front();
	}

	/** read parameters of this macro */
	const std::vector<std::string> &params() const {
		return _params;
	}

	/** check if this is macro has a variable number of parameters */
	bool variadic() const {
		return _variadic;
	}

	/** Check if this macro has parentheses but no parameters */
	bool nopar() const {
		return _nopar;
	}

	/** name of macro */
	const std::string &name() const {
		return tokens() ? tokens()->str() : emptyString;
	}

	const std::string& macro() const { return _macro; }

	/**
	* get expanded code for this macro
	* @param params2 macro parameters
	* @param macros macro definitions (recursion)
	* @param macrocode output string
	* @return true if the expanding was successful
	*/
	bool code(const std::vector<std::string> &params2, std::map<std::string, PreprocessorMacro *> &macroBuffer, std::string &macrocode, CCodeFile* pCodeFile) const;

	/** character that is inserted in expanded macros */
	static char macroChar;
};



typedef std::map<std::string, PreprocessorMacro*> M_MAP;
typedef std::map< CCodeFile*, std::map<std::string, PreprocessorMacro*> > G_M_MAP;

class TSCANCODELIB CGlobalMacros
{
public:

	//void ClearGlobalMacros();
	static PreprocessorMacro* FindMacro(const std::string& macroName, CCodeFile* pFile, std::map<std::string, PreprocessorMacro*>& macroBuffer);

	static void Uninitialize();

	static void AddMacros(M_MAP& macroMap, CCodeFile* pFile);

	static void reportStatus(int threadIndex, bool bStart, std::size_t fileindex, std::size_t filecount, std::size_t sizedone, std::size_t sizetotal, const std::string& fileName);

	static void DumpMacros();

	static void SetFileTable(CFileDependTable* table)
	{
		s_fileDependTable = table;
	}

	static CFileDependTable* GetFileTable()
	{
		return s_fileDependTable;
	}

private:
	static G_M_MAP s_global_macros;
	static CFileDependTable* s_fileDependTable;

public:

	static TSC_LOCK MacroLock;

};

struct SGTypeDef
{
	std::string Name;
	std::vector<std::string> TypeVec;
};

typedef std::map<std::string, SGTypeDef> T_MAP;
typedef std::map< CCodeFile*, std::map<std::string, SGTypeDef> > G_T_MAP;

class TSCANCODELIB CGlobalTypedefs
{
public:
	static bool ExtractGTypeDef(TokenList& tokenList, SGTypeDef& gTypedef);

	static void AddTypedefs(T_MAP& macroMap, CCodeFile* pFile);

	static void DumpTypedef();


	static G_T_MAP s_global_typedefs;
	static TSC_LOCK TypedefLock;
};
