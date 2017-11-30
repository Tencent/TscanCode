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

//---------------------------------------------------------------------------
// Invalid variable arguments check
//---------------------------------------------------------------------------

#include "checktscinvalidvarargs.h"
#include "symboldatabase.h"


#include <list>
#include <string>
#include <map>

#ifdef UNREFERENCED_PARAMETER
#undef UNREFERENCED_PARAMETER
#endif
#define UNREFERENCED_PARAMETER(a) ((void)a);
//---------------------------------------------------------------------------


namespace {
	static CheckTSCInvalidVarArgs instance;
}


using namespace std;


unsigned int static GetVarID(const Variable * const v)
{
	if (v == nullptr)
	{
		return 0;
	}
	if (v->nameToken() == nullptr)
	{
		return 0;
	}

	return v->nameToken()->varId();
}


map<std::string, int> CheckTSCInvalidVarArgs::s_funcMap;

void CheckTSCInvalidVarArgs::runChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
{
		(void)tokenizer;
		(void)settings;
		(void)errorLogger;
}

void CheckTSCInvalidVarArgs::runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
{
	CheckTSCInvalidVarArgs checkInvalidVarArgs(tokenizer, settings, errorLogger);
	checkInvalidVarArgs.CheckVarArgs_new();
}

void CheckTSCInvalidVarArgs::getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const
{
	UNREFERENCED_PARAMETER(settings);
	UNREFERENCED_PARAMETER(errorLogger);
}

void CheckTSCInvalidVarArgs::InitFuncMap()
{
	if (s_funcMap.size() == 0)
	{
		s_funcMap["printf"]		= 0;
		s_funcMap["wprintf"]	= 0;
		s_funcMap["sprintf"]	= 1;
		s_funcMap["swprintf"]	= 1;
		s_funcMap["fprintf"]	= 1;
		s_funcMap["fwprintf"]	= 1;

		s_funcMap["scanf"]		= 0;
		s_funcMap["wscanf"]		= 0;
		s_funcMap["fwscanf"]	= 1;
		s_funcMap["wscanf"]		= 1;
		s_funcMap["sscanf"]		= 1;
		s_funcMap["swscanf"]	= 1;

	}
}


static bool inline IsNameLikeFormatFunction(const std::string& name)
{
	std::string upper = name;
	std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
	
	return upper.find("LOG") != std::string::npos;
}

void CheckTSCInvalidVarArgs::CheckVarArgs_new()
{
	const SymbolDatabase *symbolDatabase = _tokenizer->getSymbolDatabase();
	const std::size_t functionCount = symbolDatabase->functionScopes.size();
	std::map<std::string, int>::iterator iter;
	std::map<std::string, int>::iterator end = s_funcMap.end();
	for (std::size_t ii = 0; ii < functionCount; ++ii)
	{
		const Scope * scope = symbolDatabase->functionScopes[ii];
		if (!scope || !scope->classStart)
		{
			continue;
		}
		for (const Token *tok = scope->classStart->next(); tok && tok != scope->classEnd; tok = tok->next())
		{
			if (tok->str() != "(" 
				|| (tok->previous()->tokType() != Token::eName && tok->previous()->tokType() != Token::eVariable)
				|| !Token::Match(tok->tokAt(-2), "[=;{)]"))
			{
				continue;
			}
			iter = s_funcMap.find(tok->previous()->str());
			if (iter != end) // std functions
			{
				const Token *pTokVar = tok->tokAt(1 + 2 * iter->second);
				if (!pTokVar)
				{
					continue;
				}
				// ingore pattern: printf("RoleID:(%"PRIu64"),%d", 10, 20);
				if (!pTokVar->next() || (pTokVar->next()->str() != "," && pTokVar->next()->str() != ")"))
				{
					tok = pTokVar;
					continue;
				}
				if (pTokVar->tokType() == Token::eString)
				{
					const std::string& strFormatted = pTokVar->str();

					if (strFormatted.empty())
						continue;

					tok = CheckParamCount(strFormatted, pTokVar, iter->first, false, iter->first.find("printf") != std::string::npos);
				}
				// char* or wchar_t* string
				else if (pTokVar->tokType() == Token::eVariable)
				{
					const Variable* pVar = symbolDatabase->getVariableFromVarId(pTokVar->varId());
					if (!pVar)
						continue;

					if (pVar->isLocal())
					{
						const Token * tokEnd = tok;
						while (tokEnd && tokEnd->str() != ";")
						{
							tokEnd = tokEnd->previous();
						}
						if (tokEnd)
						{
							std::string strFormatted;
							bool inconclusive = false;
							CheckScope(pVar->scope()->classStart, tokEnd, pVar->name(), ::GetVarID(pVar), strFormatted, inconclusive);
							if (strFormatted.empty())
							{
								continue;
							}
							tok = CheckParamCount(strFormatted, pTokVar, iter->first, inconclusive, iter->first.find("printf") != std::string::npos);
						}
					}
				}
			}
			else
			{
				const Token *tokFormatStr = tok->next();
				if (!tokFormatStr 
					|| tokFormatStr->tokType() != Token::eString 
					|| !tokFormatStr->next() 
					|| !IsNameLikeFormatFunction(tok->previous()->str())
					)
				{
					continue;
				}
				int nFormatParam = CountFormatStrParam(tokFormatStr->str(), false, false);
				if (nFormatParam <= 0)
				{
					continue;
				}

				int realParam = 0;
				const Token *tokParent = tokFormatStr->astParent();
				if (!tokParent)
				{
					continue;
				}
				if (tokParent->str() == ",")
				{
					tokParent = tokParent->astParent();
					while (tokParent && tokParent->str() == ",")
					{
						++realParam;
						tokParent = tokParent->astParent();
					}
					if (tokParent)
						++realParam;
				}
				
				//report only less
				if (realParam < nFormatParam && nFormatParam - realParam < 3)
				{
					ReportErrorParamDismatch(tok, tok->previous()->str());
				}
			}
		}
	}
}

void CheckTSCInvalidVarArgs::ReportErrorParamDismatch(const Token *tok, const string& funcName/*, bool inconclusive*/ /*= false*/)
{
	char szMsg[0x100] = {0};
	sprintf(szMsg, "The count of parameters mismatches the format string in %s", funcName.c_str());
	
	reportError(tok, Severity::error, ErrorType::Logic, "InvalidVarArgs", szMsg, ErrorLogger::GenWebIdentity((funcName)));
}

void CheckTSCInvalidVarArgs::CheckScope(const Token *tokBegin, const Token *tokEnd, const std::string &varname, unsigned int varid, std::string& strFormatted, bool& inconclusive)
{
	UNREFERENCED_PARAMETER(varname);
	strFormatted.clear();
	std::stack<const Token *> assignStack;

	Token *tok = GetCode(tokBegin, tokEnd, assignStack, varid);

	std::stack<std::string> useStack;

	for (const Token* tok2 = tok; tok2 ; tok2 = tok2->next())
	{
		if (tok2->str() == "use" || tok2->str() == "assign")
		{
			useStack.push(tok2->str());
		}
	}

	bool bUsed = false;
	bool bBlankStr = false;
	while(!useStack.empty())
	{
		std::string useType = useStack.top();
		useStack.pop();
		if (useType == "use")
		{
			bUsed = true;
		}
		else if (useType == "assign")
		{
			const Token* pTokBack = assignStack.top();
			assignStack.pop();

			if (pTokBack && pTokBack->tokType() == Token::eString)
			{
				bBlankStr = true;
				strFormatted = pTokBack->str();
				break;
			}
		}
	}
	inconclusive = bUsed;

	TokenList::deleteTokens(tok);
}

void CheckTSCInvalidVarArgs::addtoken(Token **rettail, const Token *tok, const std::string &str)
{
	(*rettail)->insertToken(str);
	(*rettail) = (*rettail)->next();
	(*rettail)->linenr(tok->linenr());
	(*rettail)->fileIndex(tok->fileIndex());
}

Token* CheckTSCInvalidVarArgs::GetCode(const Token *tokBegin, const Token *tokEnd, std::stack<const Token *>& assignStack, const unsigned int varid)
{
	const Token* tok = tokBegin;
	if(tok == nullptr )
	{
		return nullptr;
	}
	// The first token should be ";"
	Token* rethead = new Token(0);
	rethead->str(";");
	rethead->linenr(tok->linenr());  
	rethead->fileIndex(tok->fileIndex());

	Token* rettail = rethead;
	int indentlevel = 0;
	int parlevel = 0;

	for (; tok ; tok = tok->next()) 
	{
		if(tok == tokEnd)
			break;

		if (tok->str() == "{") 
		{
			addtoken(&rettail, tok, "{");
			++indentlevel;
		} 
		else if (tok->str() == "}") 
		{
			addtoken(&rettail, tok, "}");
			--indentlevel;
			if (indentlevel <= 0)
				break;

		}
		else if (tok->str() == "(")
			++parlevel;
		else if (tok->str() == ")")
			--parlevel;

		if (parlevel == 0 && tok->str() == ";")
			addtoken(&rettail, tok, ";");

		// Start of new statement.. check if the statement has anything interesting
		if (varid > 0 && parlevel == 0 && Token::Match(tok, "[;{}]")) 
		{
			if (Token::Match(tok->next(), "[{};]"))
				continue;

			// function calls are interesting..
			const Token *tok2 = tok;
			while (Token::Match(tok2->next(), "%var% ."))
				tok2 = tok2->tokAt(2);
			if (Token::Match(tok2->next(), "%var% ("))
			{
			}
			else if (Token::Match(tok->next(), "continue|break|return|throw|goto|do|else"))
			{
			}
			else 
			{
				const Token *skipToToken = 0;
				// scan statement for interesting keywords / varid
				for (tok2 = tok->next(); tok2; tok2 = tok2->next()) 
				{
					if (tok2->str() == ";") 
					{
						// nothing interesting found => skip this statement
						skipToToken = tok2->previous();
						break;
					}

					if (tok2->varId() == varid || tok2->str() == ":" || tok2->str() == "{" || tok2->str() == "}") 
					{
						break;
					}
				}

				if (skipToToken) 
				{
					tok = skipToToken;
					continue;
				}
			}
		}

		// p = p + 1;
		// use -> ; -> assign

		// is the pointer in rhs?
		bool rhs = false;
		const Token *tok2 = NULL;
		for (tok2 = tok->next(); tok2; tok2 = tok2->next()) 
		{
			if (tok2->str() == ";") 
			{
				break;
			}

			if (Token::Match(tok2, "[=+(,] %varid%", varid)) 
			{
				if (!rhs && Token::Match(tok2, "[(,]")) {
					addtoken(&rettail, tok, "use");
					addtoken(&rettail, tok, ";");
				}
				rhs = true;
			}
		}

		if (Token::Match(tok, "[(;{}] %varid% =", varid)) 
		{
			addtoken(&rettail, tok, "assign");
			assignStack.push(tok->tokAt(3));
		}
		if(tok2 != NULL)
		{
			tok = tok2->previous();
		}
	}

	return rethead;
}



const Token * CheckTSCInvalidVarArgs::CheckParamCount(const string &strFormatted, const Token * tok, const std::string& strFunc, bool inconclusive, bool bPrintf)
{
	// traverse formatted string to find out the number of parameters
	int paramCount = CountFormatStrParam(strFormatted, true, bPrintf);

	const Token* tokFormatted = tok;	
	const Token* tokParent = tokFormatted->astParent();

	if (!tokParent)
		return tokFormatted->next();

	bool isLeft = (tokFormatted->astParent()->astOperand1() == tokFormatted);
	int realParam = 0;


	if (tokParent->str() == "(")
	{
		realParam = 0;
	}
	else if (tokParent->str() == ",")
	{
		const Token* tok2 = tokParent->astParent();
		while(tok2 && tok2->str() == ",")
		{
			++realParam;
			tok2 = tok2->astParent();
		}
		if (tok2)
			++realParam;

		if (!isLeft)
			--realParam;
	}
	if (realParam != paramCount)
	{
		ReportErrorParamDismatch(tokFormatted, strFunc/*, inconclusive*/);
	}


	return tok;
}

int CheckTSCInvalidVarArgs::CountFormatStrParam(const std::string &strFormatted, bool bFormatString, bool bPrintf) const
{
	static const char* szFormatChars = "bdiuoxXfFeEgGaAcsSpnlLzhtIwW"; //add  20151217%I64u %64d, 2016/5/4, add %ws   
	int paramCount = 0;
	size_t index =  1;
	const std::size_t nSize = strFormatted.size() - 1;
	while (index < nSize)
	{
		if (strFormatted[index] == '%')
		{
			char nextChar = strFormatted[++index];
			if (nextChar == '%')
			{
				++index;
				continue;
			}

			if (bPrintf)
			{
				while (!isalpha(nextChar) && (index < nSize + 1))
				{
					if (nextChar == '*')
					{
						++paramCount;
					}
					nextChar = strFormatted[++index];
				}

				if (strchr(szFormatChars, nextChar))
				{
					++paramCount;
				}
			}
			else
			{
				if (nextChar == '*')
				{
					++index;
					continue;;
				}
				
				if (isalpha(nextChar) && !strchr(szFormatChars, nextChar))
				{
					return -1;
				}
				++paramCount;
			}
		}
		++index;
	}
	if (!bFormatString && paramCount == 0)
	{
		return -1;
	}
	return paramCount;
}
