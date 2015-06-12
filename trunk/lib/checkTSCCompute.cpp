/*
* Tencent is pleased to support the open source community by making TscanCode available.
* Copyright (C) 2015 THL A29 Limited, a Tencent company. All rights reserved.
* Licensed under the GNU General Public License as published by the Free Software Foundation, version 3 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
* http://www.gnu.org/licenses/gpl.html
* TscanCode is free software: you can redistribute it and/or modify it under the terms of License.    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR * A PARTICULAR PURPOSE.  See the GNU General Public License for more details. 
*/


//---------------------------------------------------------------------------
#include "checkTSCCompute.h"
#include "symboldatabase.h"


//---------------------------------------------------------------------------

namespace {
	CheckTSCCompute instance;
}



// TSC:from  20130708
void CheckTSCCompute::checkPreciseComparison()
{
  const SymbolDatabase* symbolDatabase = _tokenizer->getSymbolDatabase();

    for (const Token *tok = _tokenizer->tokens(); tok; tok = tok->next())
	{
		if(tok->type()==Token::eVariable)
		{
			if (tok->varId())
			{
				const Variable *var = symbolDatabase->getVariableFromVarId(tok->varId());
				//ADD FLOAT type declaration 20150109
				if (var && (
					strcmp(var->typeEndToken()->str().c_str(),"float")==0 ||
					strcmp(var->typeEndToken()->str().c_str(),"FLOAT")==0||
					strcmp(var->typeEndToken()->str().c_str(),"double")==0 ) )	
				{
					if(tok->previous()->str()=="==" || tok->previous()->str()=="!=")	// var==float
					PreciseComparisonError(tok);
					if(tok->next())
					{
						if(tok->next()->str()=="=="  || tok->next()->str()=="!=")			// float ==var 
							PreciseComparisonError(tok);
					}
			   }
			}
		}
	}
}

void CheckTSCCompute::PreciseComparisonError(const Token *tok)
{
	reportError(tok, Severity::compute, "compute","PreciseComparison","The == or != operator is used to compare floating point numbers.It's probably better to use a comparison with defined precision: fabs(A - B) <Epsilon or fabs(A - B) > Epsilon.");
}
//  TSC:from  20130708
//	 eg: int a=0;
//  code  "a=+10" may be a mistake instead of a+=10;
void CheckTSCCompute::checkAssignAndCompute()
{
    for (const Token *tok = _tokenizer->tokens(); tok; tok = tok->next())
	{
		if(tok && Token::Match(tok,"= +") )
		//from  20140718  false positive bug fixed  
		{
			bool flag=true;
			const Token *toktmp=tok->next();
			while(toktmp && toktmp->str()!=";")
			{
				toktmp=toktmp->next();
				if(toktmp!=NULL && (toktmp->str()=="+"|| toktmp->str()=="-" || toktmp->str()=="*" ||toktmp->str()=="/" ||toktmp->str()=="%"))
				{
					flag=false;
					break;
				}
			}
			if(flag==true)
			{
				AssignAndComputeError(tok);
			}
		}
	}
}

void CheckTSCCompute::AssignAndComputeError(const Token *tok)
{
	reportError(tok, Severity::compute, "compute","AssignAndCompute","The expression of the ' A=+B' kind is utilized.It is possible that 'A+=B' was meant.");
}


void CheckTSCCompute::checkUnsignedlessthanzero()
{
	const SymbolDatabase *symbolDatabase = _tokenizer->getSymbolDatabase();
	for (const Token* tok = _tokenizer->tokens(); tok; tok = tok->next()) {
		if (Token::Match(tok, "%var% < %num%"))
		{
			const Variable *var = symbolDatabase->getVariableFromVarId(tok->varId());
			if ( var != NULL )
			{
				if (var->typeStartToken()->isUnsigned())
				{
					if (MathLib::isLessEqual(tok->strAt(2),"0"))
					{
						UnsignedlessthanzeroError(tok);
					}
				}
			}
		}
		else if (Token::Match(tok, "%var% <= %num%"))
		{
			const Variable *var = symbolDatabase->getVariableFromVarId(tok->varId());
			if ( var != NULL )
			{
				if (var->typeStartToken()->isUnsigned())
				{
					if (MathLib::isLess(tok->strAt(2),"0"))
					{
						UnsignedlessthanzeroError(tok);
					}
				}
			}
		}
		else if (Token::Match(tok, "%var% <=|< %var%"))
		{
			const Variable *var = symbolDatabase->getVariableFromVarId(tok->tokAt(2)->varId());
			if ( var != NULL )
			{
				const Token* typetoken=var->typeStartToken();

				if ( typetoken != NULL && typetoken->previous() != NULL && typetoken->previous()->str()=="const")
				{
					if (var->nameToken()->next() != NULL && var->nameToken()->next()->str() == "=")
					{
						const Token* valuetok=var->nameToken()->next()->next();
						if (valuetok != NULL && valuetok->str() != "0")
						{
							if ( MathLib::isLess(valuetok->str(),"0"))
							{
								UnsignedlessthanzeroError(tok);
							}
						}
						else
						{
							if (tok->strAt(1) == "<")
							{
								UnsignedlessthanzeroError(tok);
							}
						}
					}
				}
			}
		}
	}
}
void CheckTSCCompute::UnsignedlessthanzeroError( const Token *tok )
{
	reportError(tok, Severity::compute,"compute", "Unsignedlessthanzero",tok->str()+"'s type is unsigned,can not less than 0");
}

