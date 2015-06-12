/*
* Tencent is pleased to support the open source community by making TscanCode available.
* Copyright (C) 2015 THL A29 Limited, a Tencent company. All rights reserved.
* Licensed under the GNU General Public License as published by the Free Software Foundation, version 3 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
* http://www.gnu.org/licenses/gpl.html
* TscanCode is free software: you can redistribute it and/or modify it under the terms of License.    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR * A PARTICULAR PURPOSE.  See the GNU General Public License for more details. 
*/


//---------------------------------------------------------------------------
// Invalid variable arguments check
//---------------------------------------------------------------------------

#include "checkTSCInvalidVarArgs.h"
#include "symboldatabase.h"

#include <list>
#include <string>

//---------------------------------------------------------------------------


// Register this check class into cppcheck by creating a static instance of it..
namespace {
    static CheckTSCInvalidVarArgs instance;
}

map<std::string, int> CheckTSCInvalidVarArgs::s_funcMap;

void CheckTSCInvalidVarArgs::runChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
{
	if(getCheckConfig()->logic)
	{
		if(getCheckConfig()->InvalidVarArgs)
		{
			(void)tokenizer;
			(void)settings;
			(void)errorLogger;
		}
	}
}

void CheckTSCInvalidVarArgs::runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
{
	if(getCheckConfig()->logic)
	{
		if(getCheckConfig()->InvalidVarArgs)
		{
			CheckTSCInvalidVarArgs checkInvalidVarArgs(tokenizer, settings, errorLogger);
			checkInvalidVarArgs.CheckVarArgs();
		}
	}
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

void CheckTSCInvalidVarArgs::CheckVarArgs()
{
	InitFuncMap();

	const SymbolDatabase *symbolDatabase = _tokenizer->getSymbolDatabase();
	const std::size_t functions = symbolDatabase->functionScopes.size();

	std::map<std::string, int>::iterator iter = s_funcMap.begin();
	while(iter != s_funcMap.end())
	{
		std::string strFunc = iter->first;
		bool bPrintf = (strFunc.find("printf") != std::string::npos);
		int posIndex = iter->second;

		std::ostringstream streamPattern;
		streamPattern << "[=;{)] ";
		streamPattern << strFunc;
		streamPattern << " (";
		for (int i=0; i<posIndex; i++)
		{
			streamPattern << " %var% ,";
		}

		for (std::size_t i = 0; i < functions; ++i) 
		{
			const Scope * scope = symbolDatabase->functionScopes[i];
			if(!scope)
				continue;
			for (const Token *tok = scope->classStart; tok && tok != scope->classEnd; tok = tok->next()) 
			{

				if (Token::Match(tok, streamPattern.str().c_str()))
				{
					const Token *pTokVar = tok->tokAt(3 + 2 * posIndex);
					if (!pTokVar)
						break;

					// ingore pattern: printf("RoleID:(%"PRIu64"),%d", 10, 20);
					if (!pTokVar->next() || (pTokVar->next()->str() != "," && pTokVar->next()->str() != ")"))
					{
						tok = pTokVar;
						continue;
					}

					string strFormatted;
					bool inconclusive = false;
					// blank string
					if (pTokVar->type() == Token::eString)
					{
						strFormatted = pTokVar->str();

						if (strFormatted.empty())
							continue;

						tok = CheckParamCount(strFormatted, pTokVar, strFunc, false, bPrintf);
					}
					// char* or wchar_t* string
					else if (pTokVar->type() == Token::eVariable)
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
								CheckScope(pVar->scope()->classStart, tokEnd, pVar->name(), pVar->varId(), strFormatted, inconclusive);
								if (strFormatted.empty())
									continue;	
								tok = CheckParamCount(strFormatted, pTokVar, strFunc, inconclusive, bPrintf);
							}
						}
					}
				}
			}
		}
		iter++;
	}
	
}

void CheckTSCInvalidVarArgs::ReportErrorParamDismatch(const Token *tok, const string& funcName, bool inconclusive /*= false*/)
{
	char szMsg[0x100] = {0};
	sprintf(szMsg, "The count of parameters mismatches the formatted string in %s", funcName.c_str());
	if (!inconclusive) 
		reportError(tok, Severity::error, "logic", "InvalidVarArgs", szMsg, false);
	else 
		reportError(tok, Severity::error, "logic", "InvalidVarArgs", szMsg, true);
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
			
			if (pTokBack && pTokBack->type() == Token::eString)
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
	//add nullcheck by TSC 20141128
	if(tok == NULL )
	{
		return NULL;
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
		//add nullcheck by TSC 20141128
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
	int paramCount = 0;
	size_t index = 0;
	while(index < strFormatted.size() - 1)
	{
		if (strFormatted[index] == '%')
		{
			char nextChar = strFormatted[index + 1];
			if(nextChar != '%')
			{
				// handle %m
				if (nextChar == 'm')
				{
				}
				// handle %*
				else if (nextChar == '*')
				{
					if (bPrintf)
					{
						// handle %*.*
						// handle %*-*
						if (index + 3 < strFormatted.size())
						{
							if (strFormatted[index + 2] == '.' || strFormatted[index + 2] == '-')
							{
								if (strFormatted[index + 3] == '*')
								{
									paramCount ++;
								}
							}
						}
						paramCount += 2;
					}
				}
				// handle %-*
				// handle %.*
				else if (nextChar == '-' || nextChar == '.')
				{
					if (index + 2 < strFormatted.size())
					{
						if (strFormatted[index + 2] == '*')
						{
							paramCount += 2;
						}
						else
						{
							paramCount++;
						}
					}
				}
				else
				{
					paramCount++;
				}
			}
			else
			{
				index++;
			}
		}
		index++;
	}

	const Token* tokFormatted = tok;

	int brace = 0;
	while (tok)
	{
		const Token* tok2 = Token::findmatch(tok, "[,;()]");
		if (!tok2)
			break;
		if (tok2->str() == ";")
		{
			if (paramCount != 0)
			{
				ReportErrorParamDismatch(tokFormatted, strFunc, inconclusive);
			}
			tok = tok2->previous();
			break;
		}
		else if (tok2->str() == ",")
		{
			if (brace == 0)
			{
				//from TSC 20141027 bug fixed. printf("",xx,xx,xx,);
				if(!Token::Match(tok2,", ) ;"))
				{
					paramCount--;
				}
			}
			tok = tok2->next();
		}
		else if (tok2->str() == "(")
		{
			brace++;
			tok = tok2->next();
		}
		else if (tok2->str() == ")")
		{
			if (brace > 0)
			{
				brace--;
			}
			tok = tok2->next();
		}
	}		
	return tok;
}
