/*
* TscanCode - A tool for static C/C++ code analysis
* Copyright (C) 2012-2013 tencent TSC TEAM.
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
#include "checktsccompute.h"
#include "symboldatabase.h"



namespace {
	CheckTSCCompute instance;
}


void CheckTSCCompute::checkPreciseComparison()
{
	if (!_settings->isEnabled("warning"))
		return;

	const SymbolDatabase* symbolDatabase = _tokenizer->getSymbolDatabase();

	for (const Token *tok = _tokenizer->tokens(); tok; tok = tok->next())
	{
		if (tok->tokType() == Token::eVariable)
		{
			if (tok->varId())
			{
				const Variable *var = symbolDatabase->getVariableFromVarId(tok->varId());
				if (var && (strcmp(var->typeEndToken()->str().c_str(), "float") == 0 || strcmp(var->typeEndToken()->str().c_str(), "double") == 0))
				{
					if (tok->previous()->str() == "==" || tok->previous()->str() == "!=")
						PreciseComparisonError(tok);
					if (tok->next())
					{
						if (tok->next()->str() == "==" || tok->next()->str() == "!=")
							PreciseComparisonError(tok);
					}
				}
			}
		}
	}
}

void CheckTSCCompute::PreciseComparisonError(const Token *tok)
{
	reportError(tok, Severity::warning, ErrorType::Compute, "PreciseComparison", "The == or != operator is used to compare floating point numbers.It's probably better to use a comparison with defined precision: fabs(A - B) <Epsilon or fabs(A - B) > Epsilon.", ErrorLogger::GenWebIdentity(tok->str()));
}

void CheckTSCCompute::checkAssignAndCompute()
{
	if (!_settings->isEnabled("warning"))
		return;

	for (const Token *tok = _tokenizer->tokens(); tok; tok = tok->next())
	{
		//if(tok && (Token::Match(tok,"= +") || Token::Match(tok,"= -")))
		if (tok && Token::Match(tok, "= +"))
		{
			bool flag = true;
			const Token *toktmp = tok->next();
			while (toktmp && toktmp->str() != ";")
			{
				toktmp = toktmp->next();
				if (toktmp != NULL && (toktmp->str() == "+" || toktmp->str() == "-" || toktmp->str() == "*" || toktmp->str() == "/" || toktmp->str() == "%"))
				{
					flag = false;
					break;
				}

			}
			if (flag == true)
			{
				AssignAndComputeError(tok);
			}
		}
	}
}

void CheckTSCCompute::AssignAndComputeError(const Token *tok)
{
	reportError(tok, Severity::warning, ErrorType::Compute, "AssignAndCompute", "The expression of the ' A=+B' kind is utilized.It is possible that 'A+=B' was meant.", ErrorLogger::GenWebIdentity(tok->previous()->str()));
}


void CheckTSCCompute::checkUnsignedlessthanzero()
{
	if (!_settings->isEnabled("warning"))
		return;

	const SymbolDatabase *symbolDatabase = _tokenizer->getSymbolDatabase();
	for (const Token* tok = _tokenizer->tokens(); tok; tok = tok->next()) {
		if (Token::Match(tok, "%var% < %num%"))
		{
			const Variable *var = symbolDatabase->getVariableFromVarId(tok->varId());
			if (var != NULL)
			{
				if (var->typeStartToken()->isUnsigned())
				{
					if (MathLib::isLessEqual(tok->strAt(2), "0"))
					{
						UnsignedlessthanzeroError(tok);
					}
				}
			}
		}
		else if (Token::Match(tok, "%var% <= %num%"))
		{
			const Variable *var = symbolDatabase->getVariableFromVarId(tok->varId());
			if (var != NULL)
			{
				if (var->typeStartToken()->isUnsigned())
				{
					if (MathLib::isLess(tok->strAt(2), "0"))
					{
						UnsignedlessthanzeroError(tok);
					}
				}
			}
		}
		else if (Token::Match(tok, "%var% <=|< %var%"))
		{
			const Variable *var = symbolDatabase->getVariableFromVarId(tok->tokAt(2)->varId());
			if (var != NULL)
			{
				const Token* typetoken = var->typeStartToken();

				if (typetoken != NULL && typetoken->previous() != NULL && typetoken->previous()->str() == "const")
				{
					if (var->nameToken()->next() != NULL && var->nameToken()->next()->str() == "=")
					{
						const Token* valuetok = var->nameToken()->next()->next();
						if (valuetok != NULL && valuetok->str() != "0")
						{
							if (MathLib::isLess(valuetok->str(), "0"))
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
void CheckTSCCompute::UnsignedlessthanzeroError(const Token *tok)
{
	//Debug ( var-(int)var<0 );
	if (tok->tokAt(-1) && tok->tokAt(-2) && tok->strAt(-1) == ")" &&tok->strAt(-2) == "int")
	{
		return;
	}
	reportError(tok, Severity::warning, ErrorType::Compute, "Unsignedlessthanzero", tok->str() + "'s type is unsigned,can not less than 0", ErrorLogger::GenWebIdentity(tok->str()));
}

