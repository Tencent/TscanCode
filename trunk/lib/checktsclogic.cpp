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
#include "checktsclogic.h"
#include "symboldatabase.h"
#include "globaltokenizer.h"


#ifdef UNREFERENCED_PARAMETER
#undef UNREFERENCED_PARAMETER
#endif
#define UNREFERENCED_PARAMETER(P) (void)(P)


//---------------------------------------------------------------------------

namespace {
	CheckTSCLogic instance;
}


using namespace std;



void CheckTSCLogic::runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
{
	UNREFERENCED_PARAMETER(tokenizer);
	UNREFERENCED_PARAMETER(settings);
	UNREFERENCED_PARAMETER(errorLogger);

	CheckTSCLogic CheckTSCLogic(tokenizer, settings, errorLogger);
	CheckTSCLogic.CheckCompareDefectInFor();
}

void CheckTSCLogic::checkSwitchNoBreakUP()
{
	if (!(_settings->isEnabled("style")))
		return;

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
		for (const Token *tok3 = (Token*)i->classStart; tok3 != i->classEnd && tok3; tok3 = tok3->next())
		{
			if (Token::Match(tok3, "switch|for"))
			{
				int deep=0;
				while(tok3 != i->classEnd)//ignore TSC
				{
					tok3 = tok3->next();
					if ( Token::Match(tok3, "{"))
					{
						deep = 1;
						break;
					}
				}
				while(tok3 != i->classEnd)//ignore TSC
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
				if (Token::Match(tok3, "case") 
					&& tok3->tokAt(2)  
					&& !Token::Match(tok3->tokAt(2), ": ; case") 
					&& !Token::Match(tok3->tokAt(2), ": ; abort|exit|continue|break|goto|ExitProcess") 
					&& !Token::Match(tok3->tokAt(2), ": ; { abort|exit|continue|break|goto|ExitProcess }"))
				{
					iscase=true;
					casecount++;
				}
				if(Token::Match(tok3, "default :"))
				{
					iscase=false;
				}
				if (CGlobalTokenizer::Instance()->CheckIfReturn(tok3) && iscase)
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
				while(tok3 != i->classEnd)//ignore TSC
				{
					tok3 = tok3->next();
					if ( Token::Match(tok3, "{"))
					{
						deep = 1;
						break;
					}
				}
				while(tok3 != i->classEnd)//ignore TSC
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
						std::string jump = "";/*=getCheckConfig()->JUMPSTR;*/
						if (CGlobalTokenizer::Instance()->CheckIfReturn(tok3->tokAt(3)) || tok3->tokAt(3) == i->classEnd)
						{
							isneedbreak=false;
						}
					}
					if ( isfist >1 && isneedbreak )
					{
						if ( !Token::Match(tok3->tokAt(-2),":") && tok3->tokAt(-4) && !Token::Match(tok3->tokAt(-4),": ; { }"))
						{
							VerrorTok.push_back(toktemp);
						}
						isneedbreak = false;
					}		
					isneedbreak = true;
					toktemp = tok3;
				}
				if (CGlobalTokenizer::Instance()->CheckIfReturn(tok3) 
					|| _settings->CheckIfJumpCode(tok3->str()))
				{
					isneedbreak = false;
				}

			}
		}
		std::vector<Token*>::iterator itr;
		for (itr=VerrorTok.begin();itr!=VerrorTok.end();itr++)
		{
			SwitchNoBreakUPError(*itr);
		}
	}
}

void CheckTSCLogic::SwitchNoBreakUPError(const Token *tok)
{
	//skip empty default
	if (tok->tokAt(4)->str() == "{" && Token::Match(tok->tokAt(4)->link(), "} default : ; { break|return|exit|ExitProcess|assert"))
	{
		return;
	}
	//skip empty case
	const Token* tmp = tok;
	while (tmp && tmp->str() != ":")
	{
		if (Token::Match(tmp,"(|[|<|{"))
		{
			tmp = tmp->link();
		}
		else if (tmp->str() == "?")
		{
			while (tmp && tmp->str() != ":")
				tmp = tmp->next();
			if (tmp)
				tmp = tmp->next();
		}
		else
			tmp = tmp->next();
	}
	if (!Token::Match(tmp, ": ;| case|default"))
	{
		//
		const Token* nextCase = tmp;
		while (nextCase && nextCase->str() != "case" && nextCase->str() != "default")
		{
			if (Token::Match(nextCase, "(|[|<|{"))
			{
				nextCase = nextCase->link();
			}
			else
				nextCase = nextCase->next();
		}
		if (nextCase)
		{
			std::ostringstream errmsg;
			errmsg << "Switch falls through case without comment. 'break;' missing? Case start at line :"
				<< tok->linenr() << ".";
			reportError(nextCase, Severity::style,
				ErrorType::Logic, "SwitchNoBreakUP", errmsg.str(), ErrorLogger::GenWebIdentity(nextCase->str()));
		}

	}
}

void CheckTSCLogic::checkSwitchNoDefault()
{
	if (!(_settings->isEnabled("style")))
		return;

	const SymbolDatabase* const symbolDatabase = _tokenizer->getSymbolDatabase();

	for (std::list<Scope>::const_iterator i = symbolDatabase->scopeList.begin(); i != symbolDatabase->scopeList.end(); ++i) {
		if (i->type != Scope::eSwitch || !i->classStart)
			continue;

		int defaultcount = 0;
		Token *tokbegin= (Token*)i->classDef;
		//some lib uses switch case to implement yield
		//see https://coolshell.cn/articles/10975.html
		if (Token::Match(i->classStart->next(), "for") && i->classStart->next()->isExpandedMacro())
		{
			continue;
		}
		for (const Token *tok3 = i->classStart; tok3 != i->classEnd && tok3; tok3 = tok3->next())
		{
			if ( Token::Match(tok3, "switch|for"))
			{				
				int deep=0;
				while(tok3 != i->classEnd)//ignore TSC
				{
					tok3 = tok3->next();
					if ( Token::Match(tok3, "{"))
					{
						deep = 1;
						break;
					}
				}
				while(tok3 != i->classEnd)//ignore TSC
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
	reportError(tok, Severity::style,
		ErrorType::Logic, "SwitchNoDefault", "Is 'default' missing in this switch?", ErrorLogger::GenWebIdentity(tok->str()));
}

void CheckTSCLogic::SwitchMoreDefaultError(const Token *tok)
{
	reportError(tok, Severity::style,
		ErrorType::Logic, "SwitchNoDefault", "Is 'Default' too much in this switch?", ErrorLogger::GenWebIdentity(tok->str()));

}

void CheckTSCLogic::checkNoFirstCase()
{
	if (!_settings->isEnabled("style"))
		return;

    const SymbolDatabase *symbolDatabase = _tokenizer->getSymbolDatabase();

    for (std::list<Scope>::const_iterator scope = symbolDatabase->scopeList.begin(); scope != symbolDatabase->scopeList.end(); ++scope) {
        const Token* const toke = scope->classDef;

		if (scope->type == Scope::eSwitch && toke) {
			const Token* tok = scope->classStart;
			tok=tok->next();
			if(tok && !Token::Match(tok,"case") &&
				//some lib uses switch case to implement yield
				//see https://coolshell.cn/articles/10975.html
				!(tok->str() == "for" && tok->isExpandedMacro()) &&
				tok->str() != "using"
				)
			{
				if (tok->str().find("BOOST") == 0)
				{
					continue;
				}
				//some project put default at first,such as TPS 
				//one more { in switch eg. switch(c){ {case c: xx;break;}}
				//there is no logic in switch eg. switch(c){}
				if(!Token::Match(tok,"while|default|{|}"))
					NoFirstCaseError(tok->next());
			}
            
		}
	}

}
void CheckTSCLogic::NoFirstCaseError(const Token *tok)
{
	reportError(tok, Severity::warning,
		ErrorType::Logic, "NoFirstCase", "It's possible that the first 'case' operator is missing.", ErrorLogger::GenWebIdentity(tok->str()));

}

void CheckTSCLogic::checkRecursiveFunc()
{
	if (!_settings->isEnabled("style"))
		return;

	const SymbolDatabase* symbolDatabase = _tokenizer->getSymbolDatabase();

	for (const Token *tok = _tokenizer->tokens(); tok; tok = tok->next())
	{

		if ( tok->scope() && tok->scope()->type == Scope::eFunction && strcmp(tok->str().c_str(), tok->scope()->className.c_str()) == 0)
		{

			int argnum=-1;
			vector<std::string> argname1;
			if(  tok->scope()->function)
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
										//check force type translate debug  调用处 (char*)data 重载函数递归  不应该使用定义处的Type  误报 20151022; 
										Token *tCheckType=toktemp->tokAt(-1);
										if  (tCheckType&& tCheckType->str()==")")
										{
											Token *tCheckTypeLink=tCheckType->link();
											std::string types2="";
											if (tCheckTypeLink)
											{
													tCheckTypeLink=tCheckTypeLink->next();
											}
											for(;tCheckTypeLink!=NULL && tCheckTypeLink!=tCheckType;tCheckTypeLink=tCheckTypeLink->next())
											{
											 	types2=types2+ tCheckTypeLink->str();
											}
											argname2.push_back(types2);
										}
										else
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
						if (tok2!=NULL&&tok2==tok->link())
						{
							Token* toktemps=tok2->previous();
							if(toktemps)
							{
								const Variable *var = symbolDatabase->getVariableFromVarId(toktemps->varId());
								if (var !=NULL )
								{
									//check force type translate debug  调用处 (char*)data 重载函数递归  不应该使用定义处的Type  误报 20151022; 
									Token *tCheckType = toktemps->tokAt(-1);
									if (tCheckType&& tCheckType->str() == ")")
									{
										Token *tCheckTypeLink = tCheckType->link();
										std::string types2 = "";
										if (tCheckTypeLink)
										{
											tCheckTypeLink = tCheckTypeLink->next();
										}
										for (; tCheckTypeLink != NULL && tCheckTypeLink != tCheckType; tCheckTypeLink = tCheckTypeLink->next())
										{
											types2 = types2 + tCheckTypeLink->str();
										}
										argname2.push_back(types2);
									}
									else
									{
										std::string types2 = var->typeStartToken()->str();
										types2 = types2 + var->typeEndToken()->str();
										argname2.push_back(types2);
									}
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
	reportError(tok, Severity::warning, ErrorType::Logic, "RecursiveFunc",
		"A recursive function is inefficient or means to call other functions.", ErrorLogger::GenWebIdentity(tok->str()));
}

void CheckTSCLogic::CheckCompareDefectInFor()
{
	if (!_settings->isEnabled("warning"))
		return;

	for (const Token *tok = _tokenizer->tokens(); tok; tok = tok->next())
	{
		//Todo: check compatibility
		if (Token::Match(tok, "for (") && tok->linkAt(1))
		{
			const Token* tokStart = tok->next();
			const Token* tokEnd = tok->linkAt(1);
			for (const Token* tokInner = tokStart; tokInner && tokInner != tokEnd; tokInner = tokInner->next())
			{

				if (Token::Match(tokInner, "%var% <|<= "))
				{
					if(const Variable* pVar = tokInner->variable())
					{
						bool bIngore = true;//for循环中自增变量
						const Token* tok2 = tokStart;
						if (tok2->astOperand2() && tok2->astOperand2()->str() == ";")
						{
							tok2 = tok2->astOperand2();
							if (tok2->astOperand2() && tok2->astOperand2()->str() == ";")
							{
								tok2 = tok2->astOperand2()->next();
								for (; tok2 != tokEnd; tok2 = tok2->next())
								{
									if (tok2->varId() == pVar->declarationId())
									{
										bIngore = false;
										break;
									}
								}
							}
						}

						if(bIngore)
							continue;

						unsigned sizeVar = GetStandardTypeSize(pVar->typeStartToken(), pVar->typeEndToken());
						if (sizeVar == 0 || sizeVar >= 4)
							continue;
						if (Token::Match(tokInner, "%var% <|<= "))
						{
							// no need to handle the cast condition, which is defaut as right
							if (tokInner->tokAt(2) && tokInner->tokAt(2)->isCast())
								continue;
						}
						tok2 = tokInner->next()->astOperand2();
						if(!tok2)
							continue;
						if (tok2->isNumber())
						{
							MathLib::bigint max = ((MathLib::bigint)1) << (sizeVar * 8);
							if (const ValueFlow::Value* maxValue = tok2->getMaxValue(false))
							{
								if (maxValue->intvalue >=max)
								{
									stringstream ss;
									ss << maxValue->intvalue << " is greater than the ceiling of variable " << pVar->name() << " (" << max << ").";
									reportError(tok, Severity::warning, ErrorType::Logic, "CompareDefectInFor", ss.str(), ErrorLogger::GenWebIdentity(pVar->name()));
								}
							}
						}
						else if (const Variable* pVar2 = tok2->variable())
						{
							unsigned sizeVar2 = GetStandardTypeSize(pVar2->typeStartToken(), pVar2->typeEndToken());
							if (sizeVar2 == 0 || tok2->isCast())
							{
								continue;
							}
							if (sizeVar != sizeVar2)
							{
								stringstream ss;
								ss << "The type of " << pVar->name() << " doesn't match " << pVar2->name() << ", this may cause unexpected errors.";
								reportError(tok, Severity::warning, ErrorType::Logic, "CompareDefectInFor", ss.str(), ErrorLogger::GenWebIdentity(tok->str()));
							}
						}
					}
				}
			}
		}
	}
}



void CheckTSCLogic::CheckUnintentionalOverflow()
{
	for (const Token *tok = _tokenizer->tokens(); tok; tok = tok->next())
	{
		if (Token::Match(tok, "%var% ="))
		{
			if (const Variable* pVar = tok->variable())
			{
				unsigned sizeVar = GetStandardTypeSize(pVar->typeStartToken(), pVar->typeEndToken());
				if (sizeVar)
				{
					const Token* tokEnd = Token::findmatch(tok->tokAt(2), ";|{");
					if (tokEnd)
					{
						unsigned size  = 0;
						const Token* flag = NULL;
						if (GetExprSize(tok->tokAt(2), tokEnd, size, flag))
						{
							if (size < sizeVar)
							{
								UnintentionalOverflowError(flag, size, sizeVar);
							}
						}

					}
				}

			}
		}
		else if (tok->str() == "return")
		{
			const Scope* scope = tok->scope();
			while(scope && scope->type != Scope::eFunction)
			{
				scope = scope->nestedIn;
			}

			if (scope)
			{

				const Token* tokBegin = scope->classDef->previous();
				const Token* tokEnd = tokBegin;
				while(tokBegin)
				{
					if (tokBegin->str() == "::")
					{
						tokBegin = tokBegin->tokAt(-2);
						tokEnd = tokBegin;
						continue;
					}
					if (!tokBegin->isStandardType())
					{
						tokBegin = tokBegin->next();
						break;
					}

					if (!tokBegin->previous())
					{
						break;
					}
					tokBegin = tokBegin->previous();
				}

				if (tokBegin)
				{
					unsigned sizeVar = GetStandardTypeSize(tokBegin, tokEnd);
					if (sizeVar)
					{
						tokEnd = Token::findmatch(tok->tokAt(2), ";|{");
						if (tokEnd)
						{
							unsigned size  = 0;
							const Token* flag = NULL;
							if (GetExprSize(tok->next(), tokEnd, size, flag))
							{
								if (size < sizeVar)
								{
									UnintentionalOverflowError(flag, size, sizeVar);
								}
							}
						}
					}
				}
			}
		}
	}
}

bool CheckTSCLogic::GetExprSize(const Token* tokStart, const Token* tokEnd, unsigned& size, const Token*& tokFlag)
{
	size = 0;

	if (tokStart == tokStart->previous()->astOperand2())
	{
		return false;
	}
	for (const Token* tok = tokStart; tok && tok != tokEnd; tok = tok->next())
	{
		if (tok->str() == "(")
		{

			if((tok->previous()->isName() && (tok->previous()->str() != "return")) || (tok->previous()->str() ==">" && tok->previous()->link()))
			{
				return false;
			}

			if (!tok->link())
			{
				return false;
			}
			bool bCast = true;
			for (const Token* tok2 = tok->next(); tok2 && tok2 != tok->link(); tok2 = tok2->next())//ignore TSC
			{
				if (!tok2->isName())
				{
					bCast = false;
					break;
				}
			}
			if (bCast)
			{
				return false;
			}
		}
	}

	bool bVar = false;
	for (const Token* tok = tokStart; tok && tok != tokEnd; tok = tok->next())
	{
		if (tok->isNumber())
		{
			if (strstr(tok->str().c_str(), "L") || strstr(tok->str().c_str(), "l"))
			{
				size = size > 8 ? size : 8;
			}
			else
			{
				size = size > 4 ? size : 4;
			}
		}
		else if (tok->varId())
		{
			bVar = true;
			const Variable* pVar = tok->variable();
			if (pVar)
			{
				if (pVar->isConst())
				{
					return false;
				}
				unsigned temp = GetStandardTypeSize(pVar->typeStartToken(), pVar->typeEndToken());
				if (temp == 0)
				{
					return false;
				}
				size = size > temp ? size : temp;
			}
			else
				return false;
		}
		else if (tok->str() == "*" || tok->str() == "<<")
		{
			if (Token::Match(tok, "*|<< %num%"))
			{
				std::string numstr = tok->tokAt(1)->str();
				double num = atof(numstr.c_str());
				if (num <= 1 && num>0)
				{
					return false;
				}
			}
			if (!Token::Match(tok->previous(), "%var%|)") || tok->previous()->str()== "return")
			{
				return false;
			}
			if (!tokFlag)
			{
				tokFlag = tok;
			}
		}
	}

	return (tokFlag != NULL) && bVar;
}

void CheckTSCLogic::UnintentionalOverflowError(const Token *tok, unsigned size, unsigned sizeVar)
{
	//Potentially overflowing expression map_id << 24 with type int (32 bits, signed) is evaluated using 32-bit arithmetic, and then used in a context that expects an expression of type uint64 (64 bits, unsigned).
	stringstream ss;
	ss << "Potentially overflowing expression around operator [ " << tok->str() << " ], which is evaluated using " << size << " byte arithmetic, and then used in a context that expects an expression of " << sizeVar << " bytes type.";
	reportError(tok, Severity::warning, ErrorType::Logic, "UnintentionalOverflow", ss.str(), ErrorLogger::GenWebIdentity(tok->astString()) );
}

void CheckTSCLogic::CheckReferenceParam()
{
	if (!_settings->IsCheckIdOpened(ErrorType::ToString(ErrorType::UserCustom).c_str(), "ReferenceParamError"))
	{
		return;
	}


	std::list<Variable>::iterator itr;
	std::list<std::string> templateTypes;//template names should not be too many;
	std::list<std::string>::iterator endTempType = templateTypes.end();
	const SymbolDatabase * const symbolDatabase = _tokenizer->getSymbolDatabase();
	const std::size_t functions = symbolDatabase->functionScopes.size();
	for (std::size_t i = 0; i < functions; ++i) {
		const Scope * scope = symbolDatabase->functionScopes[i];
		if (!scope || scope->function == 0 || !scope->function->hasBody())// We only look for functions with a body
			continue;
		templateTypes.clear();
		if (Token::simpleMatch(scope->function->retDef, "template <"))
		{
			const Token *tTemp = scope->function->retDef->next()->next();
			const Token *tEnd = scope->function->retDef->next()->link();
			if (tEnd == nullptr)
			{
				tEnd = scope->function->tokenDef;
			}
			for (const Token *t = tTemp; t && t != tEnd; t = t->next())
			{
				if (Token::Match(t, "class|typename %name%"))
				{
					templateTypes.push_back(t->next()->str());
					t = t->next();
				}
				else if (t->str() == "{" || t->str() == ";")
				{
					break;
				}
			}
		}
		for (itr = scope->function->argumentList.begin(); itr != scope->function->argumentList.end(); itr++)
		{
			//empty struct
			if (itr->typeScope() 
				&& itr->typeScope()->classStart 
				&& itr->typeScope()->classStart->next() == itr->typeScope()->classEnd)
			{
				continue;
			}

			const Token* typeStartTok = itr->typeStartToken();
			const Token* typeEndTok = itr->typeEndToken();
		
			//type dummy = 0
			if (!typeEndTok 
				|| (typeEndTok->next() 
				&& typeEndTok->next()->isName() 
				&& Token::Match(typeEndTok->next()->next(), "= %any%")
				&& typeEndTok->next()->next()->next()->isLiteral()
				&& !typeEndTok->next()->next()->next()->isString()))
			{
				continue;
			}
			//T<A> ar[]
			if (Token::Match(typeEndTok->next(), "%name% ["))
			{
				continue;
			}
			
			if (!templateTypes.empty() && endTempType != std::find(templateTypes.begin(), endTempType, typeEndTok->str()))
			{
				continue;
			}

			if (Token::Match(typeStartTok, "std| ::| union|enum|unique_ptr|shared_ptr|auto_ptr|smart_ptr|weak_ptr") 
				|| (typeStartTok->isStandardType() || _settings->library.podtype(typeStartTok->str())))
			{
				continue;
			}

			string typeEndStr = typeEndTok->str();
			if (typeEndTok->str() == ">" && typeEndTok->link())
			{
				typeEndStr = typeEndTok->link()->previous()->str();
			}

			transform(typeEndStr.begin(), typeEndStr.end(), typeEndStr.begin(), ::tolower);
			if (strstr(typeEndStr.c_str(), "HANDLE")||strstr(typeEndStr.c_str(), "$") 
				|| strstr(typeEndStr.c_str(), "8") || strstr(typeEndStr.c_str(), "16") 
				|| strstr(typeEndStr.c_str(), "32") || strstr(typeEndStr.c_str(), "64") 
				|| strstr(typeEndStr.c_str(), "short") || strstr(typeEndStr.c_str(), "word")  
				|| strstr(typeEndStr.c_str(), "float") || strstr(typeEndStr.c_str(), "double") 
				|| strstr(typeEndStr.c_str(), "byte") || strstr(typeEndStr.c_str(), "char") 
				|| strstr(typeEndStr.c_str(), "int") || strstr(typeEndStr.c_str(), "long") 
				|| strncmp(typeEndStr.c_str(), "lp", strlen("lp")) == 0 
				|| strncmp(typeEndStr.c_str(), "enm", strlen("enm")) == 0 
				|| strncmp(typeEndStr.c_str(), "tagenm", strlen("tagenm")) == 0)
			{
				continue;
			}
	
			if (Token::Match(typeEndTok, "const_iterator|iterator|reverse_iterator|const_reverse_iterator|*|&|(|shared_ptr|BUF_TYPE|ENV_TYPE|m3e_Sound_Handle|T1|T2|T3|T4|T5|FT_F26Dot6|FT_Error|lua_Number|lua_CFunction|size_type|long|tdr_longlong|tdr_ulonglong|tdr_wchar_t|tdr_date_t|tdr_time_t|tdr_datetime_t|tdr_ip_t|byte|typename|T|charType|__in_opt|__inout_opt|__out|__in|__in_ecount|ArrayIndex|apr_int32_t|HWND") 
				|| (typeStartTok->isStandardType() 
				|| _settings->library.podtype(typeStartTok->str())))
			{
				continue;
			}
			if (_settings->_NonReferenceType.find(typeEndStr) != _settings->_NonReferenceType.end()) {
				continue;
			}


			if (typeEndTok->str() == ">" && !Token::Match(typeEndTok, "> &|*|."))
			{
				reportError(typeEndTok, Severity::performance, ErrorType::UserCustom, "ReferenceParamError", "Passing parameter of type [" + typeEndTok->link()->previous()->str() + "] by value.Copying large values is inefficient, consider passing by reference.", ErrorLogger::GenWebIdentity(typeEndTok->link()->previous()->str()));
			}
			else if(!Token::Match(typeEndTok, ".|iterator|unique_ptr|shared_ptr|auto_ptr|smart_ptr|weak_ptr") && !Token::Match(typeEndTok->previous(),"*|&"))
			{
				reportError(typeEndTok, Severity::performance, ErrorType::UserCustom, "ReferenceParamError", "Passing parameter of type [" + typeEndTok->str() + "] by value.Copying large values is inefficient, consider passing by reference.", ErrorLogger::GenWebIdentity(typeEndTok->str()));
			}
		}
	}
}



void CheckTSCLogic::checkSTLFind()
{
	const SymbolDatabase* symbolDatabase = _tokenizer->getSymbolDatabase();
	for (const Token *tok = _tokenizer->tokens(); tok; tok = tok->next())
	{
		if (Token::Match(tok, ". find ("))
		{
			bool isEString = tok->tokAt(3)->tokType() == Token::eString;
			bool isEChar = tok->tokAt(3)->tokType() == Token::eChar;
			bool isStr = false;
			bool isStr2 = false;
			if (tok->tokAt(-1)->varId() != 0)
			{

				const Variable *var1 = symbolDatabase->getVariableFromVarId(tok->tokAt(-1)->varId());
				if (var1 && Token::Match(var1->typeStartToken(), "std| ::| string"))//must be string ,char* not has find()
				{
					isStr = true;
				}
			}
			if (tok->tokAt(3)->varId() != 0)
			{

				const Variable *var2 = symbolDatabase->getVariableFromVarId(tok->tokAt(3)->varId());
				if (var2 && (Token::Match(var2->typeStartToken(), "std| ::| string") || Token::Match(var2->typeStartToken(), "char *|")))//string ,char* char[]
				{
					isStr2 = true;
				}
			}

			if (isEString || isEChar || isStr || isStr2)
			{

				if (Token::Match(tok->astParent(), "("))
				{
					const Token* tmp = tok->astParent()->astParent();
					if (tmp && tmp->str() == "=")
					{
						tmp = tmp->astOperand1();
						if (tmp)
						{
							const Variable *var = symbolDatabase->getVariableFromVarId(tmp->varId());
							if (var)
							{
								const Token* typeTok = var->typeStartToken();

								if (typeTok && Token::Match(typeTok, "int|long|char|byte") && !Token::Match(typeTok, "int|long|char|byte *") && typeTok->originalName() != "size_t" && typeTok->originalName() != "std::size_t")
								{
									std::stringstream ss;
									ss << "Using " << typeTok->str() << " as return type of string::find is dangerous, it should use size_t instead.";
									reportError(tok, Severity::warning, ErrorType::Logic, "STLFindError", ss.str().c_str(), ErrorLogger::GenWebIdentity(tok->str()));
								}

							}
						}

					}
					else if (tmp && Token::Match(tmp, ">|>=|<|<="))
					{
						std::stringstream ss;
						ss << "Use " << tmp->str() << " to check find() is dangerous.";
						reportError(tmp, Severity::warning, ErrorType::Logic, "STLFindError", ss.str().c_str(), ErrorLogger::GenWebIdentity(tmp->str()));

					}
				}
			}
		}
	}
}


void CheckTSCLogic::checkSignedUnsignedMixed()
{
	const SymbolDatabase* symbolDatabase = _tokenizer->getSymbolDatabase();
	for (const Token *tok = _tokenizer->tokens(); tok; tok = tok->next())
	{
		if (tok && tok->str() == "=")
		{
			const Token* lTok = tok->astOperand1();
			const Token* rTok = tok->astOperand2();

			if (!lTok || !rTok || lTok->isExpandedMacro() || rTok->isExpandedMacro())
			{
				continue;
			}
		

			const Variable *lvar = symbolDatabase->getVariableFromVarId(lTok->varId());
			const Variable *rvar = symbolDatabase->getVariableFromVarId(rTok->varId());
			if (lvar && rvar && Token::Match(lvar->typeStartToken(),"char|int|short|long") && Token::Match(rvar->typeStartToken(),"char|int|short|long"))
			{
				if (rTok->values.size() > 0)
				{
					const ValueFlow::Value* value = &rTok->values.front();
					//a = b;report only when b positive
					if (value && value->intvalue >= 0)
					{
						continue;
					}
				}

				if (tok->isExpandedMacro())
				{
					continue;
				}
				if ((lvar->typeStartToken()->isUnsigned() && !rvar->typeStartToken()->isUnsigned()) || (!lvar->typeStartToken()->isUnsigned() && rvar->typeStartToken()->isUnsigned()))
				{
					std::stringstream ss;
					ss << "Unsigned to signed assignment occurs.";
					reportError(tok, Severity::warning, ErrorType::Logic, "SignedUnsignedMixed", ss.str().c_str(), ErrorLogger::GenWebIdentity(tok->str()));
				}
			}
		}
	}
}





