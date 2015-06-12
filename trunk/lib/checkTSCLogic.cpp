/*
* Tencent is pleased to support the open source community by making TscanCode available.
* Copyright (C) 2015 THL A29 Limited, a Tencent company. All rights reserved.
* Licensed under the GNU General Public License as published by the Free Software Foundation, version 3 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
* http://www.gnu.org/licenses/gpl.html
* TscanCode is free software: you can redistribute it and/or modify it under the terms of License.    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR * A PARTICULAR PURPOSE.  See the GNU General Public License for more details. 
*/


//---------------------------------------------------------------------------
#include "checkTSCLogic.h"
#include "symboldatabase.h"


//---------------------------------------------------------------------------

namespace {
	CheckTSCLogic instance;
}

void CheckTSCLogic::checkSwitchNoBreakUP()
{
	const SymbolDatabase* const symbolDatabase = _tokenizer->getSymbolDatabase();
	std::vector<Token*> VerrorTok;
	for (std::list<Scope>::const_iterator i = symbolDatabase->scopeList.begin(); i != symbolDatabase->scopeList.end(); ++i) 
	{
		if (i->type != Scope::eSwitch || !i->classStart) // Find the beginning of a switch
			continue;
		int  isfist = 0;
		bool isneedbreak = false;
		Token *toktemp= NULL;
		int breakcount=0;
		int casecount=0;
		bool iscase=false;
		VerrorTok.clear();
		//check all case missbreak;
		for (Token *tok3 = (Token*)i->classStart; tok3 != i->classEnd && tok3; tok3 = tok3->next())
		{
			if (Token::Match(tok3, "switch|for"))
			{
				int deep=0;
				while(tok3 != i->classEnd)
				{
					tok3 = tok3->next();
					if ( Token::Match(tok3, "{"))
					{
						deep = 1;
						break;
					}
				}
				while(tok3 != i->classEnd)
				{
					tok3 = tok3->next();
					if ( Token::Match(tok3, "{"))
						deep++;

					if ( Token::Match(tok3, "}"))
						deep--;

					if(deep == 0)
						break;
				}
			}
			if ( tok3 != i->classEnd )
			{
				if (Token::Match(tok3, "case"))
				{
					iscase=false;
				}
				if (Token::Match(tok3, "case") && tok3->tokAt(2)  &&!Token::Match(tok3->tokAt(2),": ; case") && !Token::Match(tok3->tokAt(2), ": ; abort|exit|continue|break|goto|ExitProcess") && !Token::Match(tok3->tokAt(2), ": ; { abort|exit|continue|break|goto|ExitProcess }"))
				{
					iscase=true;
					casecount++;
				}
				if(Token::Match(tok3, "default :"))
				{
					iscase=false;
				}
				if (Token::Match(tok3,JUMPSTR.c_str()) && iscase)
				{
					breakcount++;
					iscase=false;
				}
			}
		}
		if(breakcount==0)
		{
			continue;
		}
		float per=1-(breakcount/(float)casecount);
		if (casecount>=5&&per>=0.6)
		{
			continue;
		}
		for (Token *tok3 = (Token*)i->classStart; tok3 != i->classEnd && tok3; tok3 = tok3->next())
		{
			if (Token::Match(tok3, "switch|for"))
			{				
				int deep=0;
				while(tok3 != i->classEnd)
				{
					tok3 = tok3->next();
					if ( Token::Match(tok3, "{"))
					{
						deep = 1;
						break;
					}
				}
				while(tok3 != i->classEnd)
				{
					tok3 = tok3->next();
					if ( Token::Match(tok3, "{"))
						deep++;

					if ( Token::Match(tok3, "}"))
						deep--;

					if(deep == 0)
						break;
				}
			}
			if ( tok3 != i->classEnd )
			{
				if (Token::Match(tok3, "case") || Token::Match(tok3, "default :"))
				{
					isfist++;		
					if (Token::Match(tok3, "default :"))
					{
						//modify by TSC 20141013 JUMPSTR
						if (Token::Match(tok3->tokAt(3),JUMPSTR.c_str()) || tok3->tokAt(3) == i->classEnd )	
						{
							isneedbreak=false;
						}
					}
					if ( isfist >1 && isneedbreak )
					{
						if ( !Token::Match(tok3->tokAt(-2),":") && tok3->tokAt(-4) && !Token::Match(tok3->tokAt(-4),": ; { }"))
						{
							//SwitchNoBreakUPError(toktemp);
							VerrorTok.push_back(toktemp);
						}
						isneedbreak = false;
					}
					isneedbreak = true;
					toktemp = tok3;
				}
				if (Token::Match(tok3,JUMPSTR.c_str()))
					isneedbreak=false;

			}
		}
		vector<Token*>::iterator itr;
		for (itr=VerrorTok.begin();itr!=VerrorTok.end();itr++)
		{
			SwitchNoBreakUPError(*itr);
		}
	}
}

void CheckTSCLogic::SwitchNoBreakUPError(const Token *tok)
{
	reportError(tok, Severity::error,
		"logic", "SwitchNoBreakUP","this case  'break;' missing?");
}

void CheckTSCLogic::checkSwitchNoDefault()
{
	const SymbolDatabase* const symbolDatabase = _tokenizer->getSymbolDatabase();

	for (std::list<Scope>::const_iterator i = symbolDatabase->scopeList.begin(); i != symbolDatabase->scopeList.end(); ++i) {
		if (i->type != Scope::eSwitch || !i->classStart)
			continue;

		int defaultcount = 0;
		Token *tokbegin= (Token*)i->classDef;	
		for (Token *tok3 = (Token*)i->classStart; tok3 != i->classEnd && tok3; tok3 = tok3->next())
		{
			if ( Token::Match(tok3, "switch|for"))
			{				
				int deep=0;
				while(tok3 != i->classEnd)
				{
					tok3 = tok3->next();
					if ( Token::Match(tok3, "{"))
					{
						deep = 1;
						break;
					}
				}
				while(tok3 != i->classEnd)
				{
					tok3 = tok3->next();
					if ( Token::Match(tok3, "{"))
						deep++;

					if ( Token::Match(tok3, "}"))
						deep--;

					if(deep == 0)
						break;
				}
			}
			if ( tok3 != i->classEnd )
			{
				if (Token::Match(tok3, "default"))
					defaultcount++;

				if((tok3->next()) == (i->classEnd))
				{
					if (defaultcount == 0)
						SwitchNoDefaultError(tokbegin);

					if (defaultcount >1)
					{
						SwitchMoreDefaultError(tokbegin);
					}
				}
			}
		}
	}
}

void CheckTSCLogic::SwitchNoDefaultError(const Token *tok)
{
	reportError(tok, Severity::error,
		"logic", "SwitchNoDefault","this Switch 'Default' missing?");
}

void CheckTSCLogic::SwitchMoreDefaultError(const Token *tok)
{
	reportError(tok, Severity::error,
		"logic", "SwitchNoDefault","this Switch 'Default' is too much?");

}

// TSC:from  20130708
void CheckTSCLogic::checkNoFirstCase()
{
    const SymbolDatabase *symbolDatabase = _tokenizer->getSymbolDatabase();
    for (std::list<Scope>::const_iterator scope = symbolDatabase->scopeList.begin(); scope != symbolDatabase->scopeList.end(); ++scope) {
        const Token* const toke = scope->classDef;
		if (scope->type == Scope::eSwitch && toke) {
			const Token* tok = scope->classStart;
			tok=tok->next();
			if(tok && !Token::Match(tok,"case"))
			{
				//some project put default at first
				//one more { in switch eg. switch(c){ {case c: xx;break;}}
				//there is no logic in switch eg. switch(c){}
				if(!Token::Match(tok,"default|{|}"))
					NoFirstCaseError(tok->next());
			}
            
		}
	}

}
void CheckTSCLogic::NoFirstCaseError(const Token *tok)
{

	reportError(tok, Severity::error, 
		"logic","NoFirstCase","It's possible that the first 'case' operator is missing.");

}

// TSC:from  20130709 
void CheckTSCLogic::checkRecursiveFunc()
{
	const SymbolDatabase* symbolDatabase = _tokenizer->getSymbolDatabase();

	for (const Token *tok = _tokenizer->tokens(); tok; tok = tok->next())
	{

		if(tok && tok->scope()->type==Scope::eFunction && strcmp(tok->str().c_str(),tok->scope()->className.c_str())==0)
		{

			int argnum=-1;
			vector<std::string> argname1;
			if(tok->scope() && tok->scope()->function)
			{
				argnum=tok->scope()->function->argumentList.size();
				std::list<Variable>::iterator itr;
				for (itr=tok->scope()->function->argumentList.begin();itr!=tok->scope()->function->argumentList.end();itr++)
				{

					std::string types=itr->typeStartToken()->str();
					types=types+itr->typeEndToken()->str();
					argname1.push_back(types);
				}

			}
			vector<std::string> argname2;
			if((strcmp(tok->previous()->str().c_str(),";")==0 || strcmp(tok->previous()->str().c_str(),"{")==0 || strcmp(tok->previous()->str().c_str(),"}")==0 ) && tok->next() && strcmp(tok->next()->str().c_str(),"(")==0)
			{
				int argcount=0;

				if(strcmp(tok->next()->str().c_str(),"(")==0)
				{
					tok=tok->next();//tok (
					if(tok->next() && strcmp(tok->next()->str().c_str(),")")==0)//foo() no argument
					{
						argcount=0;
					}
					else
					{
						const Token *tok2;
						for (tok2=tok; tok2 && (tok2!=tok->link()); tok2 = tok2->next())
						{
							if(Token::Match(tok2,","))
							{
								Token* toktemp=tok2->previous();
								if(toktemp)
								{
									const Variable *var = symbolDatabase->getVariableFromVarId(toktemp->varId());
									if (var !=NULL )
									{
										std::string types2=var->typeStartToken()->str();
										types2=types2+var->typeEndToken()->str();
										argname2.push_back(types2);
									}
								}
								argcount++;
							}
						}
						if (tok2!=NULL&&tok2==tok->link())
						{
							Token* toktemps=tok2->previous();
							if(toktemps)
							{
								const Variable *var = symbolDatabase->getVariableFromVarId(toktemps->varId());
								if (var !=NULL )
								{
									std::string types2=var->typeStartToken()->str();
									types2=types2+var->typeEndToken()->str();
									argname2.push_back(types2);
								}
							}
						}
						argcount++;
					}
				}		
				if(argcount==argnum)
				{
					vector<std::string>::iterator itr1;
					vector<std::string>::iterator itr2;
					itr2=argname2.begin();
					int samecount=0;
					for (itr1=argname1.begin();itr1!=argname1.end()&&itr2!=argname2.end();itr1++)
					{
						std::string a=*itr1;
						if ((*itr1)==(*itr2))
						{
							samecount++;
						}
						itr2++;
					}
					if (samecount==argcount)
					{
						RecursiveFuncError(tok);
					}				
				}			
			}
		}
	}
}

void CheckTSCLogic::RecursiveFuncError(const Token *tok)
{
	reportError(tok, Severity::error, "logic","RecursiveFunc",
		"Recursive function may be inefficent or that means to call other function.");
}


