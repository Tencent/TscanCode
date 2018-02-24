#include "globalmacros.h"
#include "path.h"
#include "filedepend.h"
#include "tokenize.h"
#include "settings.h"
#include <fstream>

char PreprocessorMacro::macroChar = char(1);

G_M_MAP CGlobalMacros::s_global_macros;

CFileDependTable* CGlobalMacros::s_fileDependTable = nullptr;

TSC_LOCK CGlobalMacros::MacroLock;

G_T_MAP CGlobalTypedefs::s_global_typedefs;

TSC_LOCK CGlobalTypedefs::TypedefLock;

/**
* Remove heading and trailing whitespaces from the input parameter.
* @param s The string to trim.
*/
std::string trim(const std::string& s)
{
	const std::string::size_type beg = s.find_first_not_of(" \t");
	if (beg == std::string::npos)
		return s;
	const std::string::size_type end = s.find_last_not_of(" \t");
	if (end == std::string::npos)
		return s.substr(beg);
	return s.substr(beg, end - beg + 1);
}

/**
* Skip string in line. A string begins and ends with either a &quot; or a &apos;
* @param line the string
* @param pos in=start position of string, out=end position of string
*/
void skipstring(const std::string &line, std::string::size_type &pos)
{
	const char ch = line[pos];

	++pos;
	while (pos < line.size() && line[pos] != ch) {
		if (line[pos] == '\\')
			++pos;
		++pos;
	}
}

/**
* @brief get parameters from code. For example 'foo(1,2)' => '1','2'
* @param line in: The code
* @param pos  in: Position to the '('. out: Position to the ')'
* @param params out: The extracted parameters
* @param numberOfNewlines out: number of newlines in the macro call
* @param endFound out: was the end parentheses found?
*/
void getparams(const std::string &line,
	std::string::size_type &pos,
	std::vector<std::string> &params,
	unsigned int &numberOfNewlines,
	bool &endFound)
{
	params.clear();
	numberOfNewlines = 0;
	endFound = false;

	if (line[pos] == ' ')
		pos++;

	if (line[pos] != '(')
		return;

	// parentheses level
	int parlevel = 0;

	// current parameter data
	std::string par;

	// scan for parameters..
	for (; pos < line.length(); ++pos) {
		// increase parentheses level
		if (line[pos] == '(') {
			++parlevel;
			if (parlevel == 1)
				continue;
		}

		// decrease parentheses level
		else if (line[pos] == ')') {
			--parlevel;
			if (parlevel <= 0) {
				endFound = true;
				params.push_back(trim(par));
				break;
			}
		}

		// string
		else if (line[pos] == '\"' || line[pos] == '\'') {
			const std::string::size_type p = pos;
			skipstring(line, pos);
			if (pos == line.length())
				break;
			par += line.substr(p, pos + 1 - p);
			continue;
		}

		// count newlines. the expanded macro must have the same number of newlines
		else if (line[pos] == '\n') {
			++numberOfNewlines;
			continue;
		}

		// new parameter
		if (parlevel == 1 && line[pos] == ',') {
			params.push_back(trim(par));
			par = "";
		}

		// spaces are only added if needed
		else if (line[pos] == ' ') {
			// Add space only if it is needed
			if (par.size() && isalnum((unsigned char)(*par.rbegin()))) {
				par += ' ';
			}
		}

		// add character to current parameter
		else if (parlevel >= 1 && line[pos] != PreprocessorMacro::macroChar) {
			par.append(1, line[pos]);
		}
	}
}


bool CGlobalTypedefs::ExtractGTypeDef(TokenList& tokenList, SGTypeDef& gTypedef)
{
	Token* tok = tokenList.front();
	if (tok->str() != "typedef")
	{
		return false;
	}

	STypedefEntry newEntry;
	newEntry.TypeDef = tok;
	Token *tokOffset = tok->next();

	// check for invalid input
	if (!tokOffset)
	{
		return false;
	}

	if (tokOffset->str() == "::")
	{
		newEntry.TypeStart = tokOffset;
		tokOffset = tokOffset->next();

		while (Token::Match(tokOffset, "%type% ::"))
			tokOffset = tokOffset->tokAt(2);

		newEntry.TypeEnd = tokOffset;

		if (Token::Match(tokOffset, "%type%"))
			tokOffset = tokOffset->next();
	}
	else if (Token::Match(tokOffset, "%type% ::"))
	{
		newEntry.TypeStart = tokOffset;

		do {
			tokOffset = tokOffset->tokAt(2);
		} while (Token::Match(tokOffset, "%type% ::"));

		newEntry.TypeEnd = tokOffset;

		if (Token::Match(tokOffset, "%type%"))
			tokOffset = tokOffset->next();
	}
	else if (Token::Match(tokOffset, "%type%"))
	{
		newEntry.TypeStart = tokOffset;

		while (Token::Match(tokOffset, "const|struct|enum %type%") ||
			(Token::Match(tokOffset, "signed|unsigned %type%") && tokOffset->next()->isStandardType()) ||
			(tokOffset->next() && tokOffset->next()->isStandardType()))
			tokOffset = tokOffset->next();

		newEntry.TypeEnd = tokOffset;
		tokOffset = tokOffset->next();

		bool atEnd = false;
		while (!atEnd) {
			if (tokOffset && tokOffset->str() == "::") {
				newEntry.TypeEnd = tokOffset;
				tokOffset = tokOffset->next();
			}

			if (Token::Match(tokOffset, "%type%") &&
				tokOffset->next() && !Token::Match(tokOffset->next(), "[|;|,|(")) {
				newEntry.TypeEnd = tokOffset;
				tokOffset = tokOffset->next();
			}
			else if (Token::simpleMatch(tokOffset, "const (")) {
				newEntry.TypeEnd = tokOffset;
				tokOffset = tokOffset->next();
				atEnd = true;
			}
			else
				atEnd = true;
		}
	}
	else
		return false; // invalid input

					  // check for invalid input
	if (!tokOffset)
		return false;

	// check for template
	if (tokOffset->str() == "<")
	{
		newEntry.TypeEnd = tokOffset->findClosingBracket();

		while (newEntry.TypeEnd && Token::Match(newEntry.TypeEnd->next(), ":: %type%"))
			newEntry.TypeEnd = newEntry.TypeEnd->tokAt(2);

		if (!newEntry.TypeEnd) {
			// internal error
			return false;
		}

		while (Token::Match(newEntry.TypeEnd->next(), "const|volatile"))
			newEntry.TypeEnd = newEntry.TypeEnd->next();

		tok = newEntry.TypeEnd;
		tokOffset = tok->next();
	}

	// check for pointers and references
	while (Token::Match(tokOffset, "*|&|&&|const")) {
		newEntry.Pointers.push_back(tokOffset->str());
		tokOffset = tokOffset->next();
	}

	// check for invalid input
	if (!tokOffset)
		return false;

	if (Token::Match(tokOffset, "%type%"))
	{
		// found the type name
		newEntry.TypeName = tokOffset;
		tokOffset = tokOffset->next();

		// check for array
		if (tokOffset && tokOffset->str() == "[") {
			newEntry.ArrayStart = tokOffset;

			bool atEnd = false;
			while (!atEnd) {
				while (tokOffset->next() && !Token::Match(tokOffset->next(), ";|,")) {
					tokOffset = tokOffset->next();
				}

				if (!tokOffset->next())
					return false; // invalid input
				else if (tokOffset->next()->str() == ";")
					atEnd = true;
				else if (tokOffset->str() == "]")
					atEnd = true;
				else
					tokOffset = tokOffset->next();
			}

			newEntry.ArrayEnd = tokOffset;
			tokOffset = tokOffset->next();
		}

		// check for end or another
		if (Token::Match(tokOffset, ";|,"))
			tok = tokOffset;

		// or a function typedef
		else if (tokOffset && tokOffset->str() == "(")
		{
			return false;
		}

		// unhandled typedef, skip it and continue
		else {
			return false;
		}
	}
	else
	{
		// unsupported typedefs
		return false;
	}

	if (newEntry.ArrayStart || newEntry.ArrayEnd)
	{
		return false;
	}

	if (!newEntry.TypeName || !newEntry.TypeStart || !newEntry.TypeEnd)
	{
		return false;
	}

	gTypedef.Name = newEntry.TypeName->str();
	for (const Token* tok2 = newEntry.TypeStart, *tokE = newEntry.TypeEnd->next(); tok2 && tok2 != tokE; tok2 = tok2->next())
	{
		gTypedef.TypeVec.push_back(tok2->str());
	}

	if (!newEntry.Pointers.empty())
	{
		gTypedef.TypeVec.insert(gTypedef.TypeVec.end(), newEntry.Pointers.begin(), newEntry.Pointers.end());
	}
	


	return true;
}

void CGlobalTypedefs::AddTypedefs(T_MAP& macroMap, CCodeFile* pFile)
{
	if (macroMap.empty())
	{
		return;
	}
	TSC_LOCK_ENTER(&TypedefLock);
	s_global_typedefs[pFile] = macroMap;
	TSC_LOCK_LEAVE(&TypedefLock);
}

void CGlobalTypedefs::DumpTypedef()
{
	std::ofstream ofs;
	std::string sPath = CFileDependTable::GetProgramDirectory();
	sPath += "log/typedefs.log";
	CFileDependTable::CreateLogDirectory();
	ofs.open(Path::toNativeSeparators(sPath).c_str(), std::ios_base::trunc);

	std::map<CCodeFile*, std::map<std::string, SGTypeDef > >::iterator iter = s_global_typedefs.begin();
	std::map<CCodeFile*, std::map<std::string, SGTypeDef > >::iterator iterEnd = s_global_typedefs.end();

	while (iter != iterEnd)
	{
		CCodeFile* pFile = iter->first;
		std::string sFile = pFile ? Path::toNativeSeparators(pFile->GetFullPath()) : "User Defined";
		ofs << sFile << std::endl;
		std::map<std::string, SGTypeDef >::iterator iterSub = iter->second.begin();
		std::map<std::string, SGTypeDef >::iterator iterSubEnd = iter->second.end();
		for (;iterSub != iterSubEnd;iterSub++)
		{
			std::string sType;

			for (std::vector<std::string>::const_iterator I = iterSub->second.TypeVec.begin(),
				E = iterSub->second.TypeVec.end(); I != E; ++I)
			{
				sType += *I + " ";
			}
			ofs << "\t\t[" << iterSub->second.Name << "] => [" << sType << "]" << std::endl;
		}
		ofs << std::endl;
		iter++;
	}

	ofs.close();
}

PreprocessorMacro* CGlobalMacros::FindMacro(const std::string& macroName, CCodeFile* pFile, std::map<std::string, PreprocessorMacro*>& macroBuffer)
{
	if (macroBuffer.count(macroName))
	{
		return macroBuffer[macroName];
	}

	if (!pFile)
	{
		return NULL;
	}
	//if macros in stdfunction ,do not expand it
	Settings* settings = Settings::Instance();
	if (settings->library.CheckIfIsLibFunction(macroName) || settings->CheckIfJumpCode(macroName))
	{
		return NULL;
	}

	std::list<CCodeFile*>& allDepends = pFile->GetAllDepends();
	std::list<CCodeFile*>::reverse_iterator iter = allDepends.rbegin();
	std::list<CCodeFile*>::reverse_iterator rend = allDepends.rend();
	++iter;

	for (;iter != rend; iter++)
	{
		if (*iter == pFile)
		{
			continue;
		}
		if (s_global_macros.count(*iter))
		{
			if (s_global_macros[*iter].count(macroName))
			{
				PreprocessorMacro* pMacro = s_global_macros[*iter][macroName];
				macroBuffer[macroName] = pMacro;
				return pMacro;
			}
		}
	}
	macroBuffer[macroName] = NULL;
	return NULL;
}

void CGlobalMacros::Uninitialize()
{
	typedef std::map< CCodeFile*, std::map<std::string, PreprocessorMacro *> >::iterator MMI;
	typedef std::map<std::string, PreprocessorMacro *>::iterator MI;
	for (MMI I = s_global_macros.begin(), E = s_global_macros.end(); I != E; ++I)
	{
		for (MI I2 = I->second.begin(), E2 = I->second.end(); I2 != E2; ++I2)
		{
			PreprocessorMacro* macro = I2->second;
			delete macro;
		}
		I->second.clear();
	}
	s_global_macros.clear();
	SetFileTable(nullptr);
}

void CGlobalMacros::AddMacros(M_MAP& macroMap, CCodeFile* pFile)
{
	TSC_LOCK_ENTER(&MacroLock);
	s_global_macros[pFile] = macroMap;
	TSC_LOCK_LEAVE(&MacroLock);
}

void CGlobalMacros::reportStatus(int threadIndex, bool bStart, std::size_t fileindex, std::size_t filecount, std::size_t sizedone, std::size_t sizetotal, const std::string& fileName)
{
	if (filecount > 0) {
		std::ostringstream oss;
		oss << "[Preprocess] [" << fileindex << '/' << filecount
			<< ", " << (sizetotal > 0 ? static_cast<long>(static_cast<long double>(sizedone) / sizetotal * 100) : 0) << "%] " << "[" << threadIndex << "] " << (bStart ? "[Start] " : "[Done] " ) << fileName;
		std::cout << oss.str() << std::endl;
	}
}

void CGlobalMacros::DumpMacros()
{
	std::ofstream ofs;
	std::string sPath = CFileDependTable::GetProgramDirectory();
	sPath += "log/macros.log";
	CFileDependTable::CreateLogDirectory();
	ofs.open(Path::toNativeSeparators(sPath).c_str(), std::ios_base::trunc);

	std::map<CCodeFile*, std::map<std::string, PreprocessorMacro *> >::iterator iter = s_global_macros.begin();
	std::map<CCodeFile*, std::map<std::string, PreprocessorMacro *> >::iterator iterEnd = s_global_macros.end();

	while (iter != iterEnd)
	{
		CCodeFile* pFile = iter->first;
		std::string sFile = pFile ? Path::toNativeSeparators(pFile->GetFullPath()) : "User Defined";
		ofs << sFile << std::endl;
		std::map<std::string, PreprocessorMacro *>::iterator iterSub = iter->second.begin();
		std::map<std::string, PreprocessorMacro *>::iterator iterSubEnd = iter->second.end();
		for (;iterSub != iterSubEnd;iterSub++)
		{
			ofs << "\t\t[" << iterSub->second->macro() << "]" << std::endl;
		}
		ofs << std::endl;
		iter++;
	}

	ofs.close();
}

std::vector<std::string> PreprocessorMacro::expandInnerMacros(const std::vector<std::string> &params1, std::map<std::string, PreprocessorMacro *> &macroBuffer, CCodeFile* pCodeFile) const
{
	std::string innerMacroName;

	// Is there an inner macro..
	{
		const Token *tok = Token::findsimplematch(tokens(), ")");
		if (!Token::Match(tok, ") %name% ("))
			return params1;
		innerMacroName = tok->strAt(1);
		tok = tok->tokAt(3);
		unsigned int par = 0;
		while (Token::Match(tok, "%name% ,|)")) {
			tok = tok->tokAt(2);
			par++;
		}
		if (tok || par != params1.size())
			return params1;
	}

	std::vector<std::string> params2(params1);

	for (std::size_t ipar = 0; ipar < params1.size(); ++ipar) {
		const std::string s(innerMacroName + "(");
		const std::string param(params1[ipar]);
		if (param.compare(0, s.length(), s) == 0 && *param.rbegin() == ')') {
			std::vector<std::string> innerparams;
			std::string::size_type pos = s.length() - 1;
			unsigned int num = 0;
			bool endFound = false;
			getparams(param, pos, innerparams, num, endFound);
			if (pos == param.length() - 1 && num == 0 && endFound && innerparams.size() == params1.size()) {
				// Is inner macro defined?
				PreprocessorMacro* innerMacro = CGlobalMacros::FindMacro(innerMacroName, pCodeFile, macroBuffer);
				if (innerMacro) {
					// expand the inner macro
					std::string innercode;
					//std::map<std::string,PreprocessorMacro *> innermacros = macroBuffer;
					//innermacros.erase(innerMacroName);
					innerMacro->code(innerparams, macroBuffer, innercode, pCodeFile);//ignore TSC
					params2[ipar] = innercode;
				}
			}
		}
	}

	return params2;
}

PreprocessorMacro::PreprocessorMacro(const std::string &macro, const Settings* settings) : tokenlist(settings), _macro(macro)
{
	// Tokenize the macro to make it easier to handle
	std::istringstream istr(macro);
	tokenlist.createTokens(istr);

	// initialize parameters to default values
	_variadic = _nopar = false;

	const std::string::size_type pos = macro.find_first_of(" (");
	if (pos != std::string::npos && macro[pos] == '(') {
		// Extract macro parameters
		if (Token::Match(tokens(), "%name% ( %name%")) {
			for (const Token *tok = tokens()->tokAt(2); tok; tok = tok->next()) {
				if (tok->str() == ")")
					break;
				if (Token::simpleMatch(tok, ". . . )")) {
					if (tok->previous()->str() == ",")
						_params.push_back("__VA_ARGS__");
					_variadic = true;
					break;
				}
				if (tok->isName())
					_params.push_back(tok->str());
			}
		}

		else if (Token::Match(tokens(), "%name% ( . . . )"))
			_variadic = true;

		else if (Token::Match(tokens(), "%name% ( )"))
			_nopar = true;
	}
}

bool PreprocessorMacro::code(const std::vector<std::string> &params2, std::map<std::string, PreprocessorMacro *> &macroBuffer, std::string &macrocode, CCodeFile* pCodeFile) const
{
	if (_nopar || (_params.empty() && _variadic)) {
		macrocode = _macro.substr(1 + _macro.find(')'));
		if (macrocode.empty())
			return true;

		std::string::size_type pos = 0;
		// Remove leading spaces
		if ((pos = macrocode.find_first_not_of(" ")) > 0)
			macrocode.erase(0, pos);
		// Remove ending newline
		if ((pos = macrocode.find_first_of("\r\n")) != std::string::npos)
			macrocode.erase(pos);

		// Replace "__VA_ARGS__" with parameters
		if (!_nopar) {
			std::string s;
			for (std::size_t i = 0; i < params2.size(); ++i) {
				if (i > 0)
					s += ",";
				s += params2[i];
			}

			pos = 0;
			while ((pos = macrocode.find("__VA_ARGS__", pos)) != std::string::npos) {
				macrocode.erase(pos, 11);
				macrocode.insert(pos, s);
				pos += s.length();
			}
		}
	}

	else if (_params.empty()) {
		std::string::size_type pos = _macro.find_first_of(" \"");
		if (pos == std::string::npos)
			macrocode = "";
		else {
			if (_macro[pos] == ' ')
				pos++;
			macrocode = _macro.substr(pos);
			if ((pos = macrocode.find_first_of("\r\n")) != std::string::npos)
				macrocode.erase(pos);
		}
	}

	else {
		const std::vector<std::string> givenparams = expandInnerMacros(params2, macroBuffer, pCodeFile);

		const Token *tok = tokens();
		while (tok && tok->str() != ")")
			tok = tok->next();
		if (tok) {
			bool optcomma = false;
			while (nullptr != (tok = tok->next())) {
				std::string str = tok->str();
				if (str[0] == '#' || tok->isName()) {
					if (str == "##")
						continue;

					const bool stringify(str[0] == '#');
					if (stringify) {
						str = str.erase(0, 1);
					}
					for (std::size_t i = 0; i < _params.size(); ++i) {
						if (str == _params[i]) {
							if (_variadic &&
								(i == _params.size() - 1 ||
								(givenparams.size() + 2 == _params.size() && i + 1 == _params.size() - 1))) {
								str = "";
								for (std::size_t j = _params.size() - 1; j < givenparams.size(); ++j) {
									if (optcomma || j > _params.size() - 1)
										str += ",";
									optcomma = false;
									str += givenparams[j];
								}
								if (stringify)
								{
									str = "\"" + str + "\"";
								}
							}
							else if (i >= givenparams.size()) {
								// Macro had more parameters than caller used.
								macrocode = "";
								return false;
							}
							else if (stringify) {
								const std::string &s(givenparams[i]);
								std::ostringstream ostr;
								ostr << "\"";
								for (std::string::size_type j = 0; j < s.size(); ++j) {
									if (s[j] == '\\' || s[j] == '\"')
										ostr << '\\';
									ostr << s[j];
								}
								str = ostr.str() + "\"";
							}
							else
								str = givenparams[i];

							break;
						}
					}

					// expand nopar macro
					if (tok->strAt(-1) != "##") {
						PreprocessorMacro* macro = CGlobalMacros::FindMacro(str, pCodeFile, macroBuffer);
						if (macro && macro->_macro.find('(') == std::string::npos) {
							str = macro->_macro;
							if (str.find(' ') != std::string::npos)
								str.erase(0, str.find(' '));
							else
								str = "";
						}
					}
				}
				if (_variadic && tok->str() == "," && tok->next() && tok->next()->str() == "##") {
					optcomma = true;
					continue;
				}
				optcomma = false;
				macrocode += str;
				if (Token::Match(tok, "%name% %name%|%num%") ||
					Token::Match(tok, "%num% %name%") ||
					Token::simpleMatch(tok, "> >"))
					macrocode += " ";
			}
		}
	}

	return true;
}
