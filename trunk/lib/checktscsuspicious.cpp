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
#include "checktscsuspicious.h"
#include "symboldatabase.h"
#include <string>
#include <cassert>
//---------------------------------------------------------------------------

namespace {
	CheckTSCSuspicious instance;
}


using namespace std;


//only for 4 args ( variable args to do) 
void CheckTSCSuspicious::formatbufoverrun()
{
	for (const Token* tok = _tokenizer->tokens(); tok; tok = tok->next()) {
		if (Token::Match(tok, "sprintf_s ("))
		{
			Token* mytok=tok->next();
			int argcnt=1;
			int arg1len=0,arg2=0,arg4len=0;

			while(tok && tok!=mytok->link())
			{
				if(tok->str()==",")
				{
					argcnt++;

					if(argcnt==2)
					{
						Token* arg1tok=tok->previous();
						if(arg1tok)
						{
							const Variable * var = _tokenizer->getSymbolDatabase()->getVariableFromVarId(arg1tok->varId());
							if(var)
							{
							const Token* toktmp=var->nameToken();
							if(toktmp && toktmp->next()->str()=="[")
							{
								toktmp=toktmp->next();
								if(toktmp && toktmp->next() && toktmp->next()->tokType()==Token::eNumber)
								{
									arg1len=atoi(toktmp->next()->str().c_str());
								}
							}
							}
						}
					}
					if(argcnt==3)
					{
						Token* arg2tok=tok->previous();
						if(arg2tok && arg2tok->tokType()==Token::eNumber)
						{
							arg2=atoi(arg2tok->str().c_str());
						}
					}
					if(argcnt==4)
					{
						Token* arg4tok=tok->next();
						if(arg4tok && arg4tok->next()==mytok->link())
						{
							arg4len=strlen(arg4tok->str().c_str());
						}
					}
				}

				tok=tok->next();
			}
			if(arg1len>0 && arg2>0 && arg4len>0)
			{
				if(arg2>arg1len)
				{
					formatbufoverrunError(mytok->previous());
				}
				if(arg2<=arg4len)
				{
					formatbufoverrunError(mytok->previous());
				}
			}

		}

	}

}
void CheckTSCSuspicious::formatbufoverrunError(const Token *tok)
{
	reportError(tok, Severity::warning,
		ErrorType::BufferOverrun, "formatbufoverrun", "Array index " + tok->str() + " may be out of range.",
		ErrorLogger::GenWebIdentity(tok->str()));
}

//add for nested loop check
void CheckTSCSuspicious::checkNestedLoop()
{
	const SymbolDatabase *symbolDatabase = _tokenizer->getSymbolDatabase();
	std::list<Scope>::const_iterator scope;
	std::list<std::string> LoopVars;
	std::list<std::string>::iterator it;
	std::string tempstr;

	for (scope = symbolDatabase->scopeList.begin(); scope != symbolDatabase->scopeList.end(); ++scope) 
	{
		Token *tok;
		bool flag = false;
		if (scope->type == Scope::eFor)
		{
			flag = true;
			int denum = 0;
			tok = (Token*)scope->classDef;
			for ( ; tok && tok != scope->classStart; tok = tok->next())
			{
				if(tok->str() == ";")
				{
					denum++;
				}
				if (Token::Match(tok,"%any% <|>|<=|>=|!= %any%") && denum == 1)
				{
					LoopVars.push_back(tok->str());
				}
			}
		}
		else if(scope->type == Scope::eWhile)
		{
			flag = true;
			tok = (Token*)scope->classDef;
			for ( ; tok != NULL &&tok != scope->classStart; tok = tok->next())
			{
				if (Token::Match(tok,"%name% <|>|<=|>=|!= %any%") || Token::Match(tok, "%name% ++|-- <|>|<=|>=|!= %any%"))
				{
					LoopVars.push_back(tok->str());
				}
			}
		}
		else
		{
			continue;
		}
		if (flag)
		{
			while (tok != NULL && tok != scope->classEnd)
			{
				if (tok->str()=="while")
				{
					Token *temptok1 = tok->next()->link();			//find ")",the loop control end
					if(temptok1==NULL)
						continue;
					Token *temptok2 = temptok1->next()->link();		//find "}",the for or while end
					while (tok!=temptok1)//ignore TSC
					{
						if (Token::Match(tok,"%name% <|>|<=|>=|!= %any%") || Token::Match(tok, "%name% ++|-- <|>|<=|>=|!= %any%"))
						{
							for (it=LoopVars.begin();it!=LoopVars.end();it++)
							{
								if (it->c_str() == tok->str())
								{
									tempstr = tok->str();
								}
							}
							if (!tempstr.empty())
							{
								for (Token *temptok = temptok1->link();temptok && temptok != temptok2; temptok = temptok->next())
								{
									if ((temptok->str() == tempstr)&&(Token::Match(temptok,"%any% = %any%")||Token::Match(temptok,"%any% ++|+=|--|-=")||Token::Match(temptok,"%any% = %any% +|-")||Token::Match(temptok,"--|++ %any%")))
									{
										NestedLoopError(tok);
									}
								}
								tempstr.clear();
							}
						}
						tok = tok->next();
					} 
				}
				else if (tok && tok->str()=="for")
				{
					int seminum = 0;
					Token *temptok1 = tok->next()->link();			//find ")",the loop control end
					if(temptok1)
					{
					Token *temptok2 = temptok1->next()->link();		//find "}",the for or while end
					while (tok && tok!=temptok1)//ignore TSC
					{
						if (tok->str() == ";")
						{
							seminum++;
						}
						if (Token::Match(tok,"%any% <|>|<=|>=|!= %any%")&&seminum==1)
						{
							for (it=LoopVars.begin();it!=LoopVars.end();it++)
							{
								if (it->c_str() == tok->str())
								{
									tempstr = tok->str();
								}
							}
							if (!tempstr.empty())
							{
								for (Token *temptok = temptok1->link();temptok != temptok2; temptok = temptok->next())
								{
									if ((temptok->str() == tempstr)&&(Token::Match(temptok,"%any% = %any%")||Token::Match(temptok,"%any% ++|+=|--|-=")||Token::Match(temptok,"%any% = %any% +|-")||Token::Match(temptok,"--|++ %any%")))
									{
										NestedLoopError(tok);
									}
								}
								tempstr.clear();
							}
						}
						tok = tok->next();
					} 
					}
				}
				tok = tok->next();
			}
			LoopVars.clear();
		}
	}
}
void CheckTSCSuspicious::NestedLoopError(const Token *tok)
{
	//not report error in macro
	if (!tok || tok->isExpandedMacro())
	{
		return;
	}
	reportError(tok, Severity::style,
		ErrorType::Suspicious, "NestedLoop",
		"Using  same loop index variable [" + tok->str() + "]  in a nested loop can cause loop confusion or an infinite loop.", 
		ErrorLogger::GenWebIdentity(tok->str()));
}
void CheckTSCSuspicious::suspiciousArrayIndex()
{
	const SymbolDatabase* const symbolDatabase = _tokenizer->getSymbolDatabase();

	for (std::list<Scope>::const_iterator i = symbolDatabase->scopeList.begin(); i != symbolDatabase->scopeList.end(); ++i) 
	{
		if ( i->type == Scope::eFor )
		{
			std::set<unsigned int> vars;
			vars.insert(0);
			unsigned int index_varid=0;
			unsigned int wrongindex_varid=0;
			for (Token *tok = (Token*)i->classDef; tok&&tok != i->classStart; tok = tok->next())
			{
					if(i->nestedIn->type == Scope::eFor )
					{
						break;//for nested 
					}
				
					if ( Token::Match(tok,"; %var% <|<= %var% ; ++ %var%"))
					{
						Token *index=tok->tokAt(6);
						index_varid=index->varId();
						if(tok->tokAt(1)->varId()==index_varid)
						{
							wrongindex_varid=tok->tokAt(3)->varId();
							break;
						}

					}
					if ( Token::Match(tok,"; %var% <|<= %var% ; %var% ++"))
					{
						Token *index=tok->tokAt(5);
						index_varid=index->varId();
						if(tok->tokAt(1)->varId()==index_varid)
						{
							wrongindex_varid=tok->tokAt(3)->varId();
							break;
						}

					}
			}

			//
			for (Token *tok = (Token*)i->classStart; tok&&tok != i->classEnd; tok = tok->next())
			{

				if (wrongindex_varid && 
					(Token::Match(tok, "%varid% --", wrongindex_varid) || Token::Match(tok, "%varid% =", wrongindex_varid)))
				{
					break;
				}
				if ( Token::Match(tok,"[ %var% ]"))
				{
					// ignore "static int array[i];"
					const Token* tokR = tok->previous();
					bool isStatic = false;
					while (tokR && !Token::Match(tokR, ";|}|)"))
					{
						if (tokR->str() == "static")
						{
							isStatic = true;
							break;
						}
						tokR = tokR->previous();
					}

					if (isStatic)
					{
						continue;
					}


					tok=tok->tokAt(1);
					if(tok->varId()==wrongindex_varid && wrongindex_varid!=0 )
					{
						//eg.arry[list_num]=atoi(sztemp); for(int i=0;i<list_num;i++)    arry[i]=arry[list_num];
						const Token* tokBack=_tokenizer->list.back();
						Token* pTok=tok;
						bool flag=true;
						if(tokBack->str()=="}" && tokBack->link())
						{
							while(pTok && pTok!=tokBack->link())
							{
								if(pTok->linenr()!=tok->linenr() && Token::Match(pTok,"%var% ++|--|=") && pTok->varId()==wrongindex_varid)
								{
									break;
								}
								if(pTok->linenr()!=tok->linenr() && Token::Match(pTok,"[ %var% ]") && pTok->tokAt(1)->varId()==wrongindex_varid)
								{
									flag=false;
									break;
								}
								pTok=pTok->previous();
							}
						}
						if(flag)
							suspiciousArrayIndexError(tok);
					}
				}
			}
			
		}
	}
}
void CheckTSCSuspicious::suspiciousArrayIndexError(const Token *tok)
{
	reportError(tok, Severity::warning,
		ErrorType::Suspicious, "suspiciousArrayIndex", "Array index " + tok->str() + " may be out of range.",
		ErrorLogger::GenWebIdentity(tok->str()));
}

typedef std::map<std::string,std::string>::value_type valType;
void CheckTSCSuspicious::unsafeFunctionUsage()
{
	std::map<std::string,std::string> _funcLists;

	_funcLists.insert(valType("gets","fgets"));
	_funcLists.insert(valType("_getws","fgetws"));
	_funcLists.insert(valType("_getts","fgetts"));
	_funcLists.insert(valType("strcpy","strncpy"));
	_funcLists.insert(valType("lstrcpy","lstrcpyn"));
	_funcLists.insert(valType("lstrcpyA","lstrcpyn"));
	_funcLists.insert(valType("lstrcpyW","wcsncpy"));
	_funcLists.insert(valType("wcscpy","wcsncpy"));
	_funcLists.insert(valType("_tcscpy","tcsncpy"));
	_funcLists.insert(valType("_ftcscpy","_ftcsncpy"));
	_funcLists.insert(valType("strcat","strncat"));
	_funcLists.insert(valType("wcscat","wcsncat"));
	_funcLists.insert(valType("_mbscat","mbsncat"));
	_funcLists.insert(valType("_tcscat","_tcsncat"));
	_funcLists.insert(valType("sprintf","_snprintf"));
	_funcLists.insert(valType("wsprintf","wnsprintf"));
	_funcLists.insert(valType("vswprintf","_vsnwprintf"));
	_funcLists.insert(valType("vsprintf","_vsnprintf"));
	_funcLists.insert(valType("_stprintf","_sntprintf"));
	_funcLists.insert(valType("swprintf","_snwprintf"));

	for(std::map<std::string,std::string>::const_iterator func_it=_funcLists.begin();func_it!=_funcLists.end();++func_it)
	{
		for(const Token *tok=_tokenizer->tokens();tok;tok=tok->next())
		{
			if(! Token::Match(tok,func_it->first.c_str()))
				continue;
			const std::string& unsafeFunc = tok->str();
			const std::string safeFunc=func_it->second;
			unsafeFunctionUsageError(tok,unsafeFunc,safeFunc);
		}
	}
}

void CheckTSCSuspicious::unsafeFunctionUsageError(const Token *tok, const std::string& unsafefuncname,const std::string& safefuncname)
{
	reportError(tok, Severity::information, ErrorType::UnsafeFunc, "unsafeFunctionUsage", "High Risk to  use " + unsafefuncname + "(). It must be replaced by " + safefuncname, ErrorLogger::GenWebIdentity(unsafefuncname));
}


void CheckTSCSuspicious::checkUnconditionalBreakinLoop()
{
    const SymbolDatabase* symbolDatabase = _tokenizer->getSymbolDatabase();

	  for (std::list<Scope>::const_iterator scope = symbolDatabase->scopeList.begin(); scope != symbolDatabase->scopeList.end(); ++scope)
	  {
        const Token* const toke = scope->classDef;
		

		if (toke && (scope->type == Scope::eFor || scope->type == Scope::eWhile || scope->type == Scope::eDo))
		{
			//except for(;;){xxx;break;}
			if(Token::Match(toke,"for ( ; ; )"))
			{
				continue;
			}
			bool flag=true;
			for (Token *tok = (Token*)scope->classStart; tok != scope->classEnd && tok; tok = tok->next())
			{
				if (Token::Match(tok, "switch (") && tok->next()->link())
				{
					if (Token::Match(tok->next()->link(), ") {"))
					{
						tok = tok->next()->link()->next()->link();
					}
				}
				 if (Token::Match(tok, "break|continue|return|goto"))
				 {
					 if(tok->scope()->type==Scope::eFor|| tok->scope()->type==Scope::eWhile|| tok->scope()->type==Scope::eDo)
					{
			
						if(flag && !Token::Match(tok, "continue"))
						{
							//fix bug:ignore the "return" without condition in do{...}while(0)
							const Token* toke_end = tok->scope()->classEnd;
							if(tok->scope()->type==Scope::eDo)
							{
								if(!Token::simpleMatch(toke_end, "} while ( 0 )"))
									UnconditionalBreakinLoopError(tok->next());
							}
							else
								UnconditionalBreakinLoopError(tok->next());
						}
					}
					 else
					 {
						 flag=false;
					 }
				 }
			}
		}
	 }
}
void CheckTSCSuspicious::UnconditionalBreakinLoopError(const Token *tok)
{
	//not report error in macro
	if (!tok || tok->isExpandedMacro())
	{
		return;
	}
	reportError(tok, Severity::warning, ErrorType::Suspicious,
		"unConditionalBreakinLoop", "An unconditional 'break/return/goto' within a loop.It may be a mistake.",
		ErrorLogger::GenWebIdentity(tok->str()));
}

void CheckTSCSuspicious::checkSuspiciousPriority()
{
	int flag = 0;
	for (const Token *tok = _tokenizer->tokens(); tok; tok = tok->next())
	{
		if (tok && Token::Match(tok, "("))
		{
			flag = 1;
			continue;
		}
		if (tok && Token::Match(tok, ")"))
		{
			flag = 0;
			continue;
		}
		if (tok && tok->str() == "||")
		{
			if (flag == 1)
			{
				flag = 2;
				continue;
			}
		}
		if (tok && tok->str() == "||")
		{
			if (flag == 1)
			{
				flag = 2;
				continue;
			}
		}
		if (tok && Token::Match(tok, "&&"))
		{
			if (flag == 2)
			{
				SuspiciousPriorityError(tok);
				flag = 0;
				continue;
			}
		}

	}
}

void CheckTSCSuspicious::SuspiciousPriorityError(const Token *tok)
{

	reportError(tok, Severity::style, ErrorType::Suspicious, 
		"SuspiciousPriority", 
		"Priority of the '&&' operation is higher than that of the '||' operation.It's possible that parentheses should be used in the expression.",
		0U, "&&,||", ErrorLogger::GenWebIdentity(tok->str()));
}

void CheckTSCSuspicious::checkSuspiciousPriority2()
{
    int flag=0;
    int count=0;
    for (const Token *tok = _tokenizer->tokens(); tok; tok = tok->next())
	{
		//eg.dst[j++] = _base64_decode_map[src[i]] << 2 | _base64_decode_map[src[i + 1]] >> 4;
		if(Token::Match(tok,"[") && tok->link())
		{
			tok=tok->link();
		}
		if(tok && Token::Match(tok,"cout") )
		{
			 for (; tok; tok = tok->next())
			 {
				 if(Token::Match(tok,";") )
				 {
					 break;
				 }
			 }
			 if (tok)
				 continue;
			 else
				 return;
		}

		if(tok && Token::Match(tok,"<<") )
		{
			//False Positive fixed  std::stringstream ss; ss << "\n" + _title << " ---\n";
			count=count+1;
			if(tok->tokAt(-2) && Token::Match(tok->tokAt(-2),";|{|}|,"))
			{
				const Token* tok1=tok->previous();
				const Variable * var = _tokenizer->getSymbolDatabase()->getVariableFromVarId(tok1->varId());
		
				if(var)
				{
					const Token* myToken=var->nameToken();

					if (myToken && myToken->previous() && myToken->previous()->str() == "&")
					{
						myToken = myToken->previous();
					}
					if(myToken && myToken->tokAt(-1) 
						&& Token::Match(myToken->tokAt(-1),"ofstream|std::ofstream|ifstream|std::ifstream|fstream|std::fstream|ostringstream|std::ostringstream|istringstream|std::istringstream|stringstream|std::stringstream"))
					{
						flag=0;
						continue;
					}
				}
			}
			if(count<2)
			{
			 flag=1;
			 continue;
			}
		}
		if(tok && (Token::Match(tok,")")  || Token::Match(tok,"(") || Token::Match(tok,";")|| Token::Match(tok,",") ))
		{
			if(Token::Match(tok,";"))
			{
				count=0;
			}
			 flag=0;
			 continue;
		}
		if(tok && tok->tokType()== Token::Type::eString)
		{
			 flag=0;
			 continue;
		}
		if(tok && Token::Match(tok,"+") )
		{
			if(flag==1)
			 {
				 SuspiciousPriorityError2(tok);
				 flag=0;
				 continue;
			}
		}
	}
}

void CheckTSCSuspicious::SuspiciousPriorityError2(const Token *tok)
{
	reportError(tok, Severity::style, ErrorType::Suspicious,
		"SuspiciousPriority","The priority of the '+' operation is higher than that of the '<<' operation. It's possible that parentheses should be used in the expression.",\
		0U, "+,<<");
}


struct forinfo
{
	Token* tok;
	unsigned int forid;
	std::string onestrs;
	std::string twostrs;
};

//Loop StrStream buf.str();
void CheckTSCSuspicious::checksuspiciousStrErrorInLoop()
{
	const SymbolDatabase* const symbolDatabase = _tokenizer->getSymbolDatabase();

	for (std::list<Scope>::const_iterator i = symbolDatabase->scopeList.begin(); i != symbolDatabase->scopeList.end(); ++i) 
	{
		if ( i->type == Scope::eWhile )
		{
			std::set<const Token* > vars;
			std::set<unsigned int> protectedvars;
			for (Token *tok = (Token*)i->classStart; tok&&tok != i->classEnd; tok = tok->next())
			{
				if (tok&&tok->varId()>0)
				{
					 const Variable *var = symbolDatabase->getVariableFromVarId(tok->varId());
					 if (var==NULL)
					 {
						 continue;
					 }
					 const Token* myToken=var->nameToken();

					 if (myToken && myToken->previous() && myToken->previous()->str() == "&")
					 {
						 myToken = myToken->previous();
					 }
					 if(myToken &&myToken->varId()&&myToken->varId()>0&& myToken->tokAt(-1) 
						 && Token::Match(myToken->tokAt(-1),"ofstream|std::ofstream|ifstream|std::ifstream|fstream|std::fstream|ostringstream|std::ostringstream|istringstream|std::istringstream|stringstream|std::stringstream"))
					 {
						 if (protectedvars.find(myToken->varId())==protectedvars.end()) // not in protected
						 {
							 	vars.insert(myToken);
						 }
						 if (Token::Match(tok,"%var% . str"))
						 {
							protectedvars.insert(myToken->varId());
						 }
					 }
				}
			}
			//Find Error and report
			set<const Token *>::iterator tempitor; //定义前向迭代器 

			for(tempitor=vars.begin();tempitor!=vars.end();tempitor++) 
			{
				if (*tempitor)
				{
					const Token *tCompre=*tempitor;
					if ( tCompre&&tCompre->varId()>0&& (protectedvars.find(tCompre->varId())==protectedvars.end()  ) )
					{
						suspiciousWhileStrError(tCompre);
					}
				}
			}
		}
	}
}


void CheckTSCSuspicious::checksuspiciousfor()
{
	const SymbolDatabase* const symbolDatabase = _tokenizer->getSymbolDatabase();

	for (std::list<Scope>::const_iterator i = symbolDatabase->scopeList.begin(); i != symbolDatabase->scopeList.end(); ++i) 
	{
		if ( i->type == Scope::eFor )
		{
			std::vector<forinfo> myinfo;
			int denum=0;
			int ischeck=1;
			for (Token *tok = (Token*)i->classDef; tok != i->classStart && tok; tok = tok->next())
			{
				if(tok->str() == ";")
				{
					denum++;
				}
				if ( Token::Match(tok,"%any% = %any%") && denum == 0 )
				{
					forinfo tempinfo;
					tempinfo.tok=tok;
					tempinfo.forid= tok->varId();
					tempinfo.onestrs="";
					tempinfo.twostrs="";
					if(tempinfo.forid != 0 )
					{
						myinfo.push_back(tempinfo);
					}
				}
				if (denum ==1 && tok->str()==";" )
				{
					Token* temptok=tok->next();
					while (temptok && temptok->str()!=";")
					{
						if (Token::Match(temptok,"&&"))
						{
							ischeck=0;
							break;
						}
						temptok=temptok->next();
					}
				}
				
				if (Token::Match(tok,"%any% = %any%") && denum == 1)
				{
					std::vector<forinfo>::iterator infoitr1;
					for (infoitr1 = myinfo.begin();infoitr1!=myinfo.end(); infoitr1++)
					{
						if (infoitr1->forid==tok->varId())
						{
							//(p=Macro(p,ss))!=null
							if  ( tok->tokAt(3)&&(tok->strAt(3)=="(")&&tok->tokAt(2)->isExpandedMacro() )
							{
								continue;
							}
							//eg.for(i=1,c=z[0];c!=']' && c=z[i]!=0;i++) or for (string::size_type pos = 0; (pos = ans.find("&", pos)) != string::npos; )   assign and check
							//eg. for (i = 0, unit_str = unit_words_table[i].word;        unit_str = unit_words_table[i].word, unit_str != NULL; i++)
							else if ((tok->tokAt(1)->astOperand2() && tok->tokAt(1)->astOperand2()->tokType() == Token::eComparisonOp) || (tok->tokAt(1)->astParent() && (tok->tokAt(1)->astParent()->tokType() == Token::eComparisonOp || tok->tokAt(1)->astParent()->str() == ",")))
							{
								continue;
							}
							else
							{
								suspiciousforError(tok);
							}
						}					
					}		
				}
				if (ischeck==1)
				{
					if (Token::Match(tok,"%any% <|>|<=|>= %any%") && denum == 1)
					{
						std::vector<forinfo>::iterator itr;
						for (itr=myinfo.begin();itr!=myinfo.end();itr++)
						{
							if (tok->varId() == itr->forid)
							{
								itr->onestrs=tok->next()->str();
							}
							if (tok->tokAt(2)->varId() == itr->forid)
							{
								if (Token::Match(tok->tokAt(1),"<|<="))
								{
									itr->onestrs=">";	
								}
							}
						}					
					}
					if (Token::Match(tok,"%any% ++|+=") && denum == 2)
					{
						std::vector<forinfo>::iterator itr;
						for (itr=myinfo.begin();itr!=myinfo.end();itr++)
						{
							if (tok->varId() == itr->forid)
							{
								itr->twostrs="+";
							}	
						}
					}
					if (Token::Match(tok,"%any% --|-=") && denum == 2)
					{
						std::vector<forinfo>::iterator itr;
						for (itr=myinfo.begin();itr!=myinfo.end();itr++)
						{
							if (tok->varId() == itr->forid)
							{
								itr->twostrs="-";
							}	
						}
					}
					if (Token::Match(tok,"++ %any%") && denum == 2)
					{
						std::vector<forinfo>::iterator itr;
						for (itr=myinfo.begin();itr!=myinfo.end();itr++)
						{
							if (tok->next()->varId() == itr->forid)
							{
								itr->twostrs="+";
							}	
						}
					}
					if (Token::Match(tok,"-- %any%") && denum == 2)
					{
						std::vector<forinfo>::iterator itr;
						for (itr=myinfo.begin();itr!=myinfo.end();itr++)
						{
							if (tok->next()->varId() == itr->forid)
							{
								itr->twostrs="-";
							}	
						}
					}
					if (Token::Match(tok,"%any% = %any% +") && denum == 2)
					{
						std::vector<forinfo>::iterator itr;
						for (itr=myinfo.begin();itr!=myinfo.end();itr++)
						{
							if (tok->varId() == itr->forid && tok->tokAt(2)->varId() == itr->forid)
							{
								itr->twostrs="+";
							}	
						}
					}
					if (Token::Match(tok,"%any% = %any% -") && denum == 2)
					{
						std::vector<forinfo>::iterator itr;
						for (itr=myinfo.begin();itr!=myinfo.end();itr++)
						{
							if (tok->varId() == itr->forid && tok->tokAt(2)->varId() == itr->forid)
							{
								itr->twostrs="-";
							}	
						}
					}
				}		
			}
			std::vector<forinfo>::iterator infoitr;
			for (infoitr = myinfo.begin();infoitr!=myinfo.end(); infoitr++)
			{
				if ((infoitr->onestrs == "<"|| infoitr->onestrs == "<=") && infoitr->twostrs == "-")
				{
					suspiciousforError(infoitr->tok);
				}
				if ((infoitr->onestrs == ">"|| infoitr->onestrs == ">=") && infoitr->twostrs == "+")
				{
					suspiciousforError(infoitr->tok);
				}	
			}
			myinfo.clear();
		}
	}
}

void CheckTSCSuspicious::suspiciousforError( const Token *tok )
{
	reportError(tok, Severity::warning,
		ErrorType::Suspicious, "suspiciousfor", "suspicious this 'for' has error.", ErrorLogger::GenWebIdentity(tok->str()));
}


void CheckTSCSuspicious::suspiciousWhileStrError( const Token *tok )
{
	reportError(tok, Severity::warning,
		ErrorType::Suspicious, "suspiciouswhilestrerror", "suspicious string error in loop;Do you forget to set string empty?", ErrorLogger::GenWebIdentity(tok->str()));
}

void CheckTSCSuspicious::checkwrongvarinfor()
{
	const SymbolDatabase* const symbolDatabase = _tokenizer->getSymbolDatabase();

	for (std::list<Scope>::const_iterator i = symbolDatabase->scopeList.begin(); i != symbolDatabase->scopeList.end(); ++i) 
	{
		if ( i->type == Scope::eFor )
		{
			std::set<unsigned int> vars;
			int denum=0;
			//bool nofirst=false;
			bool ischeck=false;
			bool isdef=false;
			unsigned int tmpvarid=0;
			Token *errortok = (Token*)i->classDef;
			for (Token *tok = (Token*)i->classDef; tok && tok != i->classStart; tok = tok->next())
			{
				if(tok->str() == ";")
				{
					denum++;
				}	
				if(Token::Match(tok,"for ( ;"))
				{
					//nofirst=true;
				}
				if ( Token::Match(tok,"%var% ;|(") && denum == 0 )
				{
					vars.insert(tok->varId());
				}	
				if ( Token::Match(tok,"%var% = %any%") && denum == 0 )
				{
					vars.insert(tok->varId());
				}	
				if ( Token::Match(tok,"%var% +=|-= %any%") && denum == 0 )
				{
					vars.insert(tok->varId());
				}	
				if (Token::Match(tok,"%any% ;|&&|OR") && denum == 1)
				{
					
					if(tok->tokType()==Token::eVariable)
					{  
						vars.insert(tok->varId());
						errortok = tok;
						isdef =true;
					}
				}
				if (Token::Match(tok,"%any% [ %var% ] ;") && denum == 1)
				{
					if(tok->previous()->str()==";")
					{
						vars.insert(tok->next()->next()->varId());
						errortok = tok->next()->next();
						isdef =true;
					}

					//  }
				}
				//*p==null;

				if (Token::Match(tok,"%any% +|- <|>|<=|>=|!=|== %any%") && denum == 1)
				{
					// if(nofirst)    //maybe for(i=0;buf;buf++),so buf insert
					//  {
					if(tok->tokType()==Token::eVariable)
					{  
						vars.insert(tok->varId());
						errortok = tok;
						isdef =true;
					}
					//  }
				}
				//eg: for (; l->name; l++) 
				//eg:  for (; (node = cache_obj_pool_.GetNextNode(node)) != NULL; ++i)
				if (Token::Match(tok, "%var% .") && tok->tokAt(-1)->str()!="=" && denum == 1)
				{
					
					if (tok->tokType() == Token::eVariable)
					{
						vars.insert(tok->varId());
						errortok = tok;
						isdef = true;
					}
			
				}
		
				if (Token::Match(tok,"%any% <|>|<=|>=|!=|== %any%") && denum == 1)
				{
					//  if(nofirst)
					//   {
					//for(;i>0;i--) ->tokenize for(;0<i;i00(

					if (Token::Match(tok,"%any% != * %any%") )
					{
						if(tok->tokAt(3)->tokType()==Token::eVariable)
						{  
							vars.insert(tok->tokAt(3)->varId());
							errortok = tok->tokAt(3);
							isdef =true;
						}
					}
					else
					{
						if(tok->tokType()==Token::eVariable)
						{  
							vars.insert(tok->varId());
							errortok = tok;
							isdef =true;
						}
						//false positive bug fixed eg. for (; section_ir_end != section_ir; ++ section_ir)
						//else if (tok->next()->next()->type() == Token::eVariable)
						if(tok->tokAt(2)->tokType()== Token::eVariable)
						{	
							vars.insert(tok->tokAt(2)->varId());
							errortok = tok->tokAt(2);
							isdef =true;
						}
					}

					tmpvarid=tok->varId();
					tok=tok->tokAt(2);
					while(tok && tok->str()!=";" && tok->str() !="&&" && tok->str() !="||")
					{
						if (tok->str() == "?")
						{
							break;
						}
						if(tok->tokType() == Token::eVariable && tok->varId()==tmpvarid && (vars.find(tok->varId())!=vars.end()))
						{
							wrongvarinforError2(tok);
						}
						tok=tok->next();
					}
					if(tok && tok->str()==";")
					{
						denum++;
					}
					if (!tok)
						break;

				}

				if (Token::Match(tok, "%var% ++|+=|--|-=|=") && denum == 1)//for (n = info->dlpi_phnum; --n >= 0; phdr++) the second term varies
				{
					if (tok->varId() != 0 && vars.find(tok->varId()) != vars.end())
					{
						ischeck = true;
						break;
					}
			
					//for(; (p=p->m_next)!=NULL;);   p=p->next means p is changed
					if ((Token::Match(tok, "%var% = %var% . %var%"))
						&& (tok->tokAt(2))
						&& (tok->varId() == tok->tokAt(2)->varId()))
					{
						Token *checkTok = tok->tokAt(4);
						if (checkTok && ((checkTok->str() == "next") || (checkTok->str() == "Next")))
						{
							ischeck = true;
							break;
						}
					}

				}

				if (Token::Match(tok, "++|-- %var%") && denum == 1)//for (n = info->dlpi_phnum; --n >= 0; phdr++) the second term varies
				{
					if (tok->next()->varId() != 0 && vars.find(tok->next()->varId()) != vars.end())
					{
						ischeck = true;
						break;
					}

				}

				//eg: for (it->SeekToFirst(); it->Valid(); it->Next())
				if (Token::Match(tok, "%var% .") && denum == 2)
				{
					if (tok->varId() != 0 && vars.find(tok->varId()) != vars.end())
					{
						ischeck = true;
						break;
					}

				}

				if (denum == 2 && tok && tok->str() == ";"&& tok->tokAt(2) && tok->tokAt(2)==i->classStart )
				{
					ischeck=true;
					break;
				}
				if ( denum == 2 )
				{
					int flagScopeInFunction=0;
					for (Token* tempTok=tok;tempTok&&tempTok != i->classEnd;tempTok=tempTok->next())
					{
						//eg.for (int i = 0; i <= iLen - 3; i += 3) because += is changed to i=i+3 after tokenizer
						//if (Token::Match(tempTok,"%var% ++|+=|--|-="))
						//Debug ++i match
						if (tempTok==i->classStart)
						{
							flagScopeInFunction=1;
						}

						 if (tempTok->tokAt(-1)&&(Token::Match(tempTok->tokAt(-1),"++ %name%")))
						 {
							 if(tempTok->varId()!=0 && vars.find(tempTok->varId())!=vars.end())
							 {
								 ischeck=true;
								 break;
							 }
							 //solve varid=0 because of syntax error like register i;
							 if ((tempTok->varId() == 0) && (flagScopeInFunction == 0))
							 {
								 ischeck = true;
								 break;
							 }
						 }
						if (Token::Match(tempTok,"%name% )| ++|+=|--|-=|=" ))
						{
							if(tempTok->varId()!=0 && vars.find(tempTok->varId())!=vars.end())
							{
								ischeck=true;
								break;
							}
							//solve varid=0 because of syntax error like register i;
							if ((tempTok->varId()==0)&&(flagScopeInFunction==0))
							{
								ischeck=true;
								break;
							}
							//20151022  Debug 误报:for(; NULL != p->m_next; p=p->m_next);   p=p->next means p is changed
							if (  (Token::Match (tempTok,"%var% = %var% . %var%") ) 
								&& (tempTok->tokAt(2))
								&&( tempTok->varId() ==tempTok->tokAt(2)->varId() ) ) 
							{
								Token *checkTok=tempTok->tokAt(4);
								if  ( checkTok && ( (checkTok->str() =="next") || ( checkTok->str() =="Next") ) )
								{
										ischeck=true;
										break;
								}
							}

						}
					}
				}
				if (ischeck)
				{
					break;
				}
				if ( denum == 2 )
				{
					int flagScopeInFunction=0;
					for (Token* tempTok=tok;tempTok&&tempTok != i->classEnd;tempTok=tempTok->next())
					{
						if (tempTok==i->classStart)
						{
							flagScopeInFunction=1; 
						}
						if (Token::Match(tempTok,"++|-- %var%"))
						{
							if(tempTok->tokAt(1)->varId()!=0 && vars.find(tempTok->tokAt(1)->varId())!=vars.end())
							{
								ischeck=true;
							}
							//solve varid=0 because of syntax error like register i;
							if( ( tempTok->tokAt(1)->varId()==0)&&(flagScopeInFunction==0))
							{
								ischeck=true;
							}
						}
					}
				}
				if (ischeck)
				{
					break;
				}
				if ( denum == 2 )
				{
					for (Token* tempTok=tok;tempTok&&tempTok != i->classStart;tempTok=tempTok->next())
					{
						if (Token::Match(tempTok,"%any% = "))
						{
							if(tempTok->varId()!=0 && vars.find(tempTok->varId())!=vars.end())
							{
								ischeck=true;
							}
							//solve varid=0 because of syntax error like register i;
							if(tempTok->varId()==0)
							{
								ischeck=true;
							}

						}
					}
				}
				if (ischeck)
				{
					break;
				}

			}
			if(!ischeck && isdef )
			{
				wrongvarinforError(errortok);
			}
		}

	}
}

void CheckTSCSuspicious::wrongvarinforError( const Token *tok )
{
	reportError(tok, Severity::warning,
		ErrorType::Suspicious, "wrongvarinfor", 
		"Invalid use of index["+tok->str()+"] in 'for' statement leads to dead loop.", ErrorLogger::GenWebIdentity(tok->str()));

}

void CheckTSCSuspicious::wrongvarinforError2( const Token *tok )
{
	reportError(tok, Severity::warning,
		ErrorType::Suspicious, 
		"wrongvarinfor", "Invalid use of index[" + tok->str() + "] in 'for' statement leads to dead loop. The condition in 'for statement' like 'i<i+2' or 'i<a[i].length' may be an error. ", 
		ErrorLogger::GenWebIdentity(tok->str()));

}


void CheckTSCSuspicious::checkFuncReturn()
{
	const SymbolDatabase* const symbolDatabase = _tokenizer->getSymbolDatabase();
	std::vector<STfunclist> funlist;
	std::vector<const Scope *> scope=symbolDatabase->functionScopes;
	bool hasreturn;
	for ( std::vector<const Scope *>::const_iterator i = scope.begin(); i != scope.end(); ++i )
	{
			STfunclist func;
			hasreturn=false;
			const Token* nametok=(*i)->classDef;
			if(!nametok)
				continue;
			std::string  tokname=nametok->str();
			func.funcname=tokname;
			if ( nametok->previous()!=NULL)
			{
				if(nametok->previous()->str() != "::")
				{
					if (nametok->previous()->str()!="void" && nametok->previous()->str() != "~")
					{
						hasreturn=true;
					}
				}
				else
				{
					if (nametok->tokAt(-2)!=NULL&&nametok->tokAt(-2)->str()==tokname)
					{
						hasreturn=false;
					}
					else
					{
						Token * dedeTok=nametok->previous();
						if (dedeTok && dedeTok->previous()!=NULL && dedeTok->previous()->str()==">")
						{
							dedeTok=dedeTok->previous()->link();
							if (dedeTok != NULL && dedeTok->tokAt(-2) !=NULL && dedeTok->tokAt(-2)->str() != "void")
							{
								hasreturn=true;
							}	
						}
						else if (nametok->tokAt(-3) != NULL && nametok->tokAt(-3)->str() == "::")
						{
							if (nametok->tokAt(-5) != NULL&&nametok->tokAt(-5)->str() != "::"&& nametok->tokAt(-5)->str() != "void")
							{
								hasreturn=true;
							}
						}
						else
						{
							if (nametok->tokAt(-3) != NULL&&nametok->tokAt(-3)->str() != "void")
							{
								hasreturn = true;
							}
						}

					}
				}
			}
			if ( hasreturn )
			{
				std::vector<std::string> type;
				if ( nametok->next()!=NULL && nametok->next()->str() == "(")
				{
					Token *start=nametok->next();
					if(!start)
					return;
					Token *end=start->link();
					int argcount=0;

					for (Token* temp=start;temp != end && temp; temp=temp->next())
					{
						if (temp->str() == "<" && temp->link() != NULL )
						{
							temp=temp->link();
							continue;
						}		
						if (temp->str() ==",")
						{
							argcount++;
							if (temp->next()!=NULL)
							{
								if(temp->next()->str() != "const")
								{
									type.push_back(temp->next()->str());
								}
								else
								{
									if (temp->next()->next()!=NULL)
									{
										type.push_back(temp->tokAt(2)->str());
									}
								}
							}
						}
					}
					if (argcount != 0)
					{
						if (nametok->tokAt(2)->str() != "const")
						{
							type.insert(type.begin(),nametok->tokAt(2)->str());
						}
						else
						{
							type.insert(type.begin(),nametok->tokAt(3)->str());
						}
						argcount=argcount+1;
					}
					else
					{
						if (start->next()!=end)
						{
							if (nametok->tokAt(2)->str() != "const")
							{
								type.push_back(nametok->tokAt(2)->str());
							}
							else
							{
								type.push_back(nametok->tokAt(3)->str());
							}
							argcount=1;
						}	
					}
					func.argType=type;
					func.argNum=argcount;
				}	
				funlist.push_back(func);
			}
			
	}
	for ( std::vector<const Scope *>::const_iterator i = scope.begin(); i != scope.end(); ++i )
	{
		for (const Token* tok=(*i)->classStart;tok != (*i)->classEnd; tok =tok->next())
		{
			if (tok && Token::Match(tok,"%any% (") )
			{
				if (tok->next()->link()->next() != NULL &&Token::Match(tok->next()->link()->next(),"."))
				{
					continue;
				}
				//----------
				//if (!Token::Match(tok,"if|while|switch|=|for|return"))
				if (tok->tokType()==Token::eFunction&&!Token::Match(tok,"return|printf|assert"))
				{
					if (tok->next()->link() && Token::simpleMatch(tok->next()->link()->next(), "{") && Token::Match(tok->previous(), ";|{|public:|private:|protected:"))
					{
						continue;
					}
					bool isfind=false;
					Token* tmp = tok->next()->link()->next();
					if (Token::Match(tmp, "=|[|+|-|++|--|<|>|==|!=|<=|>=|!|<<|&&|%oror%"))
					{
						isfind = true;
					}
					for(const Token* temp=tok;temp!=NULL&&!Token::Match(temp,";|}|{");temp=temp->previous())
					{
						if (Token::Match(temp,"=|if|while|for|switch|return|(|[|+|-|++|--|<|>|==|!=|<=|>=|!|<<|&&|%oror%"))
						{
							isfind=true;
							break;
						}
					}
					if (!isfind)
					{
						STfunclist nowfunc;
						nowfunc.funcname=tok->str();
						Token *fstart=tok->next();
						if(!fstart)
							continue;
						Token *fend=fstart->link();
						nowfunc.argNum=0;
						if ( fstart && fend && fstart->next() != fend )
						{
							for ( Token* ftemp=fstart; ftemp !=fend && ftemp; ftemp=ftemp->next() )
							{
								if ( ftemp->str() == "<" && ftemp->link() !=NULL)
								{
									ftemp=ftemp->link();
									continue;
								}
								if ( ftemp->str() =="," )
								{
									 Token *mytok=ftemp->previous();
									 if(mytok!=NULL)
									{
									 const Variable *var = symbolDatabase->getVariableFromVarId(mytok->varId());
									 if(var != NULL)
									 {
										 nowfunc.argType.push_back(var->typeStartToken()->str());
									 }
									 else
									 {
										 nowfunc.argType.push_back("UNKOWN");
									 }
									 nowfunc.argNum++;
									 }
								}
							}
							if ( nowfunc.argNum >0 )
							{
								nowfunc.argNum++;
								Token *mytok=fend->previous();
								if(mytok!=NULL)
								{
								 const Variable *var = symbolDatabase->getVariableFromVarId(mytok->varId());
								 if(var != NULL)
								 {
									 nowfunc.argType.push_back(var->typeStartToken()->str());
								 }
								 else
								 {
									 nowfunc.argType.push_back("UNKOWN");
								 }
								 }
							}
							else
							{
								nowfunc.argNum=1;
								Token *mytok=fend->previous();
								if(mytok!=NULL)
								{
								const Variable *var = symbolDatabase->getVariableFromVarId(mytok->varId());
								if(var != NULL)
								{
									nowfunc.argType.push_back(var->typeStartToken()->str());
								}
								else
								{
									nowfunc.argType.push_back("UNKOWN");
								}
								}
							}
						}
						std::vector<STfunclist>::iterator itrfunlist;
						for (itrfunlist=funlist.begin();itrfunlist!=funlist.end();itrfunlist++)
						{
							if (itrfunlist->argNum==nowfunc.argNum&&itrfunlist->funcname==nowfunc.funcname)
							{
								std::vector<string>::iterator itrtype;
								int i2=0;
								bool isok=true;
								for ( itrtype= itrfunlist->argType.begin();itrtype != itrfunlist->argType.end();itrtype++)
								{
									if (*itrtype != nowfunc.argType[i2]&&nowfunc.argType[i2] !="UNKOWN")
									{
										isok=false;
										break;
									}
									i2++;
								}
								if (isok)
								{
									FuncReturnError(tok);
								}
							}
						}
					}
					
				}
			}		
		}	
	}
}

void CheckTSCSuspicious::FuncReturnError( const Token* tok )
{
	reportError(tok, Severity::style,
		ErrorType::Suspicious, "FuncReturn", "The return value of function [" + tok->str() + "] is not used.", ErrorLogger::GenWebIdentity(tok->str()));
}


void CheckTSCSuspicious::checkboolFuncReturn()
{
	bool hasreturn;
	const SymbolDatabase *symbolDatabase = _tokenizer->getSymbolDatabase();
	for ( std::vector<const Scope *>::const_iterator i = symbolDatabase->functionScopes.begin(); i != symbolDatabase->functionScopes.end(); ++i)
	{
		hasreturn=false;
		const Token* nametok=(*i)->classDef;
		const Token* classsbegin=(*i)->classStart;
		if(!nametok)
			continue;
		if(nametok->strAt(-1) != "::")
		{
			if (nametok->strAt(-1)=="bool")
			{
				hasreturn=true;
			}
		}
		else
		{
			if ( nametok->strAt(-3) =="bool" )
			{
				hasreturn=true;
			}
		}

		if ( hasreturn )
		{
			for ( const Token* tok=classsbegin; tok && tok != (*i)->classEnd; tok=tok->next() )
			{
				// inner class
				if (Token::Match(tok, "class|struct"))
				{
					const Token* tok2 = Token::findmatch(tok, "{|;");
					if (tok2)
					{
						if (tok2->link())
						{
							tok = tok2->link();
							continue;
						}
					}
				}
				// lambda expression
				else if (Token::Match(tok, "= [") && tok->tokAt(1)->link())
				{
					const Token* tok2 = Token::findmatch(tok, "{|;");
					if (tok2)
					{
						if (tok2->link())
						{
							tok = tok2->link();
							continue;
						}
					}
				}
				else if (Token::Match(tok,"return %num% ;"))
				{
					if (tok->strAt(1) != "0" && tok->strAt(1) != "1")
					{
						boolFuncReturnError(tok);
					}
				}
			}
		}
	}
}
void CheckTSCSuspicious::boolFuncReturnError( const Token *tok )
{
	if(tok)
		reportError(tok, Severity::style,
			ErrorType::Suspicious, "BoolFuncReturn", "Returning type of this function is expected to be bool,but non-bool variable [" + tok->str() + "] is returned.",
			ErrorLogger::GenWebIdentity(tok->str()));
}

void CheckTSCSuspicious::checkIfCondition()
{
	const SymbolDatabase* const symbolDatabase = _tokenizer->getSymbolDatabase();
	int isNum;

	for (std::list<Scope>::const_iterator i = symbolDatabase->scopeList.begin(); i != symbolDatabase->scopeList.end(); ++i) 
	{
		isNum=0;
		//Todo: checkif "else if {" is treated as Scope::eIf
		if ( i->type == Scope::eIf /*|| i->type == Scope::eElseIf*/ || i->type == Scope::eWhile )
		{
			for (Token *tok3 = (Token*)i->classDef->tokAt(2); tok3 != i->classStart && tok3; tok3 = tok3->next())
			{
				
				if (tok3->str() == "(")
				{
					tok3 = tok3->link();
				}
				//comma expression if (vbits=0, bitbuf & 255) dcr_derror(p);
				if (Token::Match(tok3, "%any% = %any%") && tok3->tokAt(3) && tok3->tokAt(3)->str() != ",")
				{
					Token *valueTok = tok3->tokAt(2);				
					if ( valueTok && valueTok->isNumber() && valueTok->next()->tokType()!=Token::eArithmeticalOp)
					{
						isNum = 1;
					}
					const Variable *var = symbolDatabase->getVariableFromVarId(tok3->varId());					
					if (var != NULL )
					{
						string a = var->typeStartToken()->str();
						if (var->isPointer() || a == "bool")
						{
							continue;
						}				
					}				
					Token *downtok1 =tok3->tokAt(1);
					Token *uptok2 =tok3->tokAt(1);
					bool bLogicalOp = true;
					//bool bDeng = false;
					while(!Token::Match(downtok1,"||")&&!Token::Match(downtok1,"&&")&&downtok1 != i->classStart)
					{
						if ( Token::Match(downtok1,"!=|==|>|<|>=|<=") )
						{
							bLogicalOp = false;
							break;
						}	
						downtok1=downtok1->next();
					}
					while(!Token::Match(uptok2,"||")&&!Token::Match(uptok2,"&&")&&uptok2 != i->classDef&&bLogicalOp)
					{
						if ( Token::Match(uptok2,"!|!=|==|>|<|>=|<=") )
						{
							bLogicalOp = false;
							break;
						}
						uptok2=uptok2->previous();
					}
					if (bLogicalOp&&isNum==1)
					{
						IfConditionError(tok3);
					}									
				}
			}
		}
	}
}

void CheckTSCSuspicious::IfConditionError(const Token *tok)
{
	if (!tok || tok->isExpandedMacro())
	{
		return;
	}
	reportError(tok, Severity::warning,
		ErrorType::Suspicious, "IfCondition", "this Condition Contain '=' and Condition is not boolean ?", 
		ErrorLogger::GenWebIdentity(tok->str()));

}

void CheckTSCSuspicious::checkRenameLocalVariable()
{
	// only check functions
	const SymbolDatabase *symbolDatabase = _tokenizer->getSymbolDatabase();
	const std::size_t functions = symbolDatabase->functionScopes.size();
	for (std::size_t i = 0; i < functions; ++i) {
		const Scope * scope = symbolDatabase->functionScopes[i];

		std::map<int, std::string> varnameMap;
		for (const Token *tok = scope->classDef->next(); tok != scope->classEnd; tok = tok->next()) {
			int varid = tok->varId();
			std::string varname = tok->str();
			if (varid>0 && tok->tokAt(1)->str() != "."&& tok->tokAt(-1)->str() != ".")
			{
				std::map<int, std::string>::iterator iter, iter2;
				iter = varnameMap.find(varid);
				if (iter == varnameMap.end())//not find the variable with the same id
				{
					for (iter2 = varnameMap.begin(); iter2 != varnameMap.end(); ++iter2)
					{
						if (iter2->second == varname)
						{
							RenameLocalVariableError(tok);
						}
					}
					varnameMap[varid] = varname;
				}
			}
		}
	}
}

void CheckTSCSuspicious::RenameLocalVariableError(const Token *tok)
{
	reportError(tok, Severity::warning,
		ErrorType::Suspicious, "RenameLocalVariable", "Local variable ["+tok->str()+"] is renamed in current function.",
		ErrorLogger::GenWebIdentity(tok->str()));
}


