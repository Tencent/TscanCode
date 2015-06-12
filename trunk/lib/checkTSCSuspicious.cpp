/*
* Tencent is pleased to support the open source community by making TscanCode available.
* Copyright (C) 2015 THL A29 Limited, a Tencent company. All rights reserved.
* Licensed under the GNU General Public License as published by the Free Software Foundation, version 3 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
* http://www.gnu.org/licenses/gpl.html
* TscanCode is free software: you can redistribute it and/or modify it under the terms of License.    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR * A PARTICULAR PURPOSE.  See the GNU General Public License for more details. 
*/




//---------------------------------------------------------------------------
#include "checkTSCSuspicious.h"
#include "symboldatabase.h"
#include <string>

  //from TSC 20140623
struct  FUNCINFO
{
	std::string filename;
	std::string functionname;
	int startline;
	int endline;
};

std::vector<FUNCINFO> FuncInfoList;
//---------------------------------------------------------------------------

namespace {
	CheckTSCSuspicious instance;
}
//add by TSC 20140507 only for 4 args ( variable args to do) 
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
								if(toktmp && toktmp->next() && toktmp->next()->type()==Token::eNumber)
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
						if(arg2tok && arg2tok->type()==Token::eNumber)
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
		"bufoverrun", "formatbufoverrun","Array index "+tok->str()+" may be out of range.");
}

//add by TSC for nested loop check
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
				if (tok && Token::Match(tok,"%any% <|>|<=|>=|!= %any%"))
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
					while (tok!=temptok1)
					{
						if (Token::Match(tok,"%any% <|>|<=|>=|!= %any%"))
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
					while (tok && tok!=temptok1)
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
//add by TSC 2014/4/4 to be modified!!!
void CheckTSCSuspicious::NestedLoopError(const Token *tok)
{
	reportError(tok, Severity::portability,
		"Suspicious", "NestedLoop","Suspicious nested loop!\n"
		"Finding suspicious nested loop,using the same loop_variable and changed by inner loop!");
}
// TSC:from  20140408
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
			for (Token *tok = (Token*)i->classStart; tok&&tok != i->classEnd; tok = tok->next())
			{
				if ( Token::Match(tok,"[ %var% ]"))
				{
					tok=tok->tokAt(1);
					if(tok->varId()==wrongindex_varid && wrongindex_varid!=0 )
					{
						suspiciousArrayIndexError(tok);
					}
				}
			}
			
		}
	}
}
void CheckTSCSuspicious::suspiciousArrayIndexError(const Token *tok)
{
	
		reportError(tok, Severity::portability,
		"suspicious", "suspiciousArrayIndex","Array index "+tok->str()+" may be out of range.");
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
	reportError(tok, Severity::information, "unsafeFunc", "unsafeFunctionUsage","High Risk to  use " + unsafefuncname + "(). It must be replaced by " + safefuncname);
}


//TSC 20130708
void CheckTSCSuspicious::checkUnconditionalBreakinLoop()
{
    const SymbolDatabase* symbolDatabase = _tokenizer->getSymbolDatabase();
	  for (std::list<Scope>::const_iterator scope = symbolDatabase->scopeList.begin(); scope != symbolDatabase->scopeList.end(); ++scope)
	  {
        const Token* const toke = scope->classDef;

		if (toke && (scope->type == Scope::eFor || scope->type == Scope::eWhile || scope->type == Scope::eDo))
		{
			//from TSC 20141208 except for(;;){xxx;break;}
			if(Token::Match(toke,"for ( ; ; )"))
			{
				continue;
			}
			bool flag=true;
			for (Token *tok = (Token*)scope->classStart; tok != scope->classEnd && tok; tok = tok->next())
			{

				 if (Token::Match(tok, "break")||Token::Match(tok, "continue")||Token::Match(tok, "return")||Token::Match(tok, "goto"))
				 {
					 if(tok->scope()->type==Scope::eFor|| tok->scope()->type==Scope::eWhile|| tok->scope()->type==Scope::eDo)
					{
			
						if(flag && !Token::Match(tok, "continue"))
						{
							//from TSC 20140811
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
	reportError(tok, Severity::portability, "Suspicious","unConditionalBreakinLoop","An unconditional 'break/return/goto' within a loop.It may be a mistake.");
}

// TSC:from  20130709 
void CheckTSCSuspicious::checkSuspiciousPriority()
{
    int flag=0;
    for (const Token *tok = _tokenizer->tokens(); tok; tok = tok->next())
	{
		if(tok && Token::Match(tok,"(") )
		{
			 flag=1;
			 continue;
		}
		if(tok && Token::Match(tok,")") )
		{
			 flag=0;
			 continue;
		}
		if(tok && Token::Match(tok,"||") )
		{
			if(flag==1)
			 {
				 flag=2;
			 continue;
			}
		}
			if(tok && Token::Match(tok,"||") )
		{
			if(flag==1)
			 {
				 flag=2;
			 continue;
			}
		}
		if(tok && Token::Match(tok,"&&") )
		{
			if(flag==2)
			 {
				 SuspiciousPriorityError(tok);
				 flag=0;
				 continue;
			}
		}

	}
}

void CheckTSCSuspicious::SuspiciousPriorityError(const Token *tok)
{

	reportError(tok, Severity::portability, "Suspicious","SuspiciousPriority","Priority of the '&&' operation is higher than that of the '||' operation.It's possible that parentheses should be used in the expression.");
}

// TSC:from  20130709 
void CheckTSCSuspicious::checkSuspiciousPriority2()
{
    int flag=0;
    int count=0;
    for (const Token *tok = _tokenizer->tokens(); tok; tok = tok->next())
	{
		//from TSC 20141113 bug fixed.eg.dst[j++] = _base64_decode_map[src[i]] << 2 | _base64_decode_map[src[i + 1]] >> 4;
		if(Token::Match(tok,"[") && tok->link())
		{
			tok=tok->link();
		}
		if(tok && Token::Match(tok,"cout") )
		{
			 for (; tok; tok = tok->next())
			 {
				 if(tok && Token::Match(tok,";") )
				 {
					 break;
				 }
			 }
			 continue;
		}

		if(tok && Token::Match(tok,"<<") )
		{
			//from TSC 20140123 False Positive fixed  std::stringstream ss; ss << "\n" + _title << " ---\n";
			count=count+1;
			if(tok && tok->previous() && tok->previous()->previous()->str()==";")
			{
				const Token* tok1=tok->previous();
				  const Variable * var = _tokenizer->getSymbolDatabase()->getVariableFromVarId(tok1->varId());
		
				if(var)
				{
				
				const Token* myToken=var->nameToken();
				if(myToken && myToken->tokAt(-1) && Token::Match(myToken->tokAt(-1),"ostringstream|std::ostringstream|istringstream|std::istringstream|stringstream|std::stringstream"))
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
		if(tok && (Token::Match(tok,")")  || Token::Match(tok,"(") || Token::Match(tok,";") ))
		{
			if(Token::Match(tok,";"))
			{
				count=0;
			}
			 flag=0;
			 continue;
		}
		if(tok && tok->type()==5 )//eString=5
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

	reportError(tok, Severity::portability, "Suspicious","SuspiciousPriority","The priority of the '+' operation is higher than that of the '<<' operation. It's possible that parentheses should be used in the expression.");
}


struct forinfo
{
	Token* tok;
	unsigned int forid;
	std::string onestrs;
	std::string twostrs;
};
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
							suspiciousforError(tok);
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
	reportError(tok, Severity::portability,
		"Suspicious", "suspiciousfor","suspicious this 'for' has error.");
}


// TSC:from 20131024
void CheckTSCSuspicious::checkwrongvarinfor()
{
	const SymbolDatabase* const symbolDatabase = _tokenizer->getSymbolDatabase();

	for (std::list<Scope>::const_iterator i = symbolDatabase->scopeList.begin(); i != symbolDatabase->scopeList.end(); ++i) 
	{
		if ( i->type == Scope::eFor )
		{
			std::set<unsigned int> vars;
			vars.insert(0);
			int denum=0;
			bool ischeck=false;
			bool isdef=false;
			unsigned int tmpvarid=0;
			Token *errortok = (Token*)i->classDef;
			for (Token *tok = (Token*)i->classDef; tok&&tok != i->classStart; tok = tok->next())
			{
				if(tok->str() == ";")
				{
					denum++;
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
					if(tok->type()==Token::eVariable)
					{  
						vars.insert(tok->varId());
						isdef =true;
					}
				}
				if (Token::Match(tok,"%any% [ %var% ] ;") && denum == 1)
				{
					if(tok->previous()->str()==";")
					{
						vars.insert(tok->next()->next()->varId());
						isdef =true;
					}
				}
				if (Token::Match(tok,"%any% +|- <|>|<=|>=|!=|== %any%") && denum == 1)
				{
					if(tok->type()==Token::eVariable)
					{  
						vars.insert(tok->varId());
						isdef =true;
					}
				}
				if (Token::Match(tok,"%any% <|>|<=|>=|!=|== %any%") && denum == 1)
				{
					if (Token::Match(tok,"%any% != * %any%") )
					{
						if(tok->tokAt(3)->type()==Token::eVariable)
						{  
							vars.insert(tok->tokAt(3)->varId());
							isdef =true;
						}
					}
					else
					{
						if(tok->type()==Token::eVariable)
						{  
							vars.insert(tok->varId());
							isdef =true;
						}
						//from TSC 20141022 false positive bug fixed eg. for (; section_ir_end != section_ir; ++ section_ir)
						//else if (tok->next()->next()->type() == Token::eVariable)
						if(tok->tokAt(2)->type()== Token::eVariable)
						{	
							vars.insert(tok->tokAt(2)->varId());
							isdef =true;
						}
					}

					tmpvarid=tok->varId();
					tok=tok->tokAt(2);
					while(tok && tok->str()!=";" && tok->str() !="&&" && tok->str() !="||")
					{
						if(tok->type() == Token::eVariable && tok->varId()==tmpvarid && (vars.find(tok->varId())!=vars.end()))
						{
							wrongvarinforError2(tok);
						}
						tok=tok->next();
					}
					if(tok->str()==";")
					{
						denum++;
					}

				}
				if (denum == 2 &&tok->str() == ";"&& tok->tokAt(2) && tok->tokAt(2)==i->classStart )
				{
					ischeck=true;
					break;
				}
				if ( denum == 2 )
				{
					for (Token* tempTok=tok;tempTok&&tempTok != i->classStart;tempTok=tempTok->next())
					{
						//from TSC 20141022 false positive bug fixed eg.for (int i = 0; i <= iLen - 3; i += 3) because += is changed to i=i+3 after tokenizer
						//if (Token::Match(tempTok,"%var% ++|+=|--|-="))
						if (Token::Match(tempTok,"%var% ++|+=|--|-=|="))
						{
							if(tempTok->varId()!=0 && vars.find(tempTok->varId())!=vars.end())
							{
								ischeck=true;
							}
							//from TSC 20141022solve varid=0 because of syntax error like register i;
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
				if ( denum == 2 )
				{
					for (Token* tempTok=tok;tempTok&&tempTok != i->classStart;tempTok=tempTok->next())
					{
						if (Token::Match(tempTok,"++|-- %var%"))
						{
							if(tempTok->tokAt(1)->varId()!=0 && vars.find(tempTok->tokAt(1)->varId())!=vars.end())
							{
								ischeck=true;
							}
							//from TSC 20141022 solve varid=0 because of syntax error like register i;
							if(tempTok->tokAt(1)->varId()==0)
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
							//from TSC 20141022	solve varid=0 because of syntax error like register i;
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
	reportError(tok, Severity::portability,
		"Suspicious", "wrongvarinfor","Use wrong variable "+tok->str()+" in 'for statement'.");

}

void CheckTSCSuspicious::wrongvarinforError2( const Token *tok )
{
	reportError(tok, Severity::portability,
		"Suspicious", "wrongvarinfor","Use wrong variable "+tok->str()+" in 'for statement'. The condition in 'for statement' like 'i<i+2' or 'i<a[i].length' may be an error. ");

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
						else
						{
							if (nametok->tokAt(-3)!=NULL&&nametok->tokAt(-3)->str() != "void")
							{
								hasreturn=true;
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
				if (tok->type()==Token::eName&&!Token::Match(tok,"return|printf|assert"))
				{
					bool isfind=false;
					for(const Token* temp=tok;temp!=NULL&&!Token::Match(temp,";|}|{");temp=temp->previous())
					{
						if (Token::Match(temp,"=|if|while|for|switch|return|(|[|+|-|++|--|<|>|==|<=|>=|!"))
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
								int i=0;
								bool isok=true;
								for ( itrtype= itrfunlist->argType.begin();itrtype != itrfunlist->argType.end();itrtype++)
								{
									if (*itrtype != nowfunc.argType[i]&&nowfunc.argType[i] !="UNKOWN")
									{
										isok=false;
										break;
									}
									i++;
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
	reportError(tok, Severity::portability,
		"Suspicious", "FuncReturn","Function return value is not being used");
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
				if (Token::Match(tok,"return %num% ;"))
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
	reportError(tok, Severity::portability,"Suspicious", "BoolFuncReturn",tok->str()+"'s type is not bool, But this function need return bool");
}

void CheckTSCSuspicious::checkIfCondition()
{
	const SymbolDatabase* const symbolDatabase = _tokenizer->getSymbolDatabase();
	int isNum;
	for (std::list<Scope>::const_iterator i = symbolDatabase->scopeList.begin(); i != symbolDatabase->scopeList.end(); ++i) 
	{
		isNum=0;
		if ( i->type == Scope::eIf || i->type == Scope::eElseIf || i->type == Scope::eWhile )
		{
			for (Token *tok3 = (Token*)i->classDef; tok3 != i->classStart && tok3; tok3 = tok3->next())
			{
				if ( Token::Match(tok3,"%any% = %any%"))
				{
					Token *valueTok = tok3->tokAt(2);				
					if ( valueTok && valueTok->isNumber())
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
	reportError(tok, Severity::portability,
		"Suspicious", "IfCondition","this Condition Contain '=' and Condition is not boolean ?");

}

///This is not a check rule,but to record the code functions and the scope of them.
//Format: "filename#TSC#functionname#TSC#startline#TSC#endline"; 
//from TSC 20140623 
void CheckTSCSuspicious::recordfuncinfo()
{
	ofstream myFuncinfo("funcinfo.txt",ios::app);
	std::vector<FUNCINFO>::iterator it1; 
	const SymbolDatabase * const symbolDatabase = _tokenizer->getSymbolDatabase();

	const std::size_t functions = symbolDatabase->functionScopes.size();
	for (std::size_t i = 0; i < functions; ++i) {
		const Scope * scope = symbolDatabase->functionScopes[i];
		if(!scope)
			return;
		Token* tok = (Token*)scope->classStart;
		int startline=0;
		int endline=0;
		if(tok)
		{
			startline=tok->linenr();
			if(tok->link())
			{
				endline=tok->link()->linenr();
			}
			string msg=tok->getfuncinfo();
			int fileindex=-1;
			fileindex=tok->fileIndex();
			if(fileindex==0 && startline>0 && endline>0)
			{
				FUNCINFO temp;
				temp.filename=_tokenizer->getSourceFilePath();
				temp.functionname=msg;
				temp.startline=startline;
				temp.endline=endline;
				FuncInfoList.push_back(temp);
			}
		}
	}
	
	std::vector<FUNCINFO>::iterator it; 
	for(it=FuncInfoList.begin();it!=FuncInfoList.end();++it)
	{
		myFuncinfo<<(*it).filename<<"#TSC#"<<(*it).functionname<<"#TSC#"<<(*it).startline<<"#TSC#"<<(*it).endline<<endl;
	}
	FuncInfoList.clear();
		
}

