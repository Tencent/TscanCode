#include "checkstatistic.h"
#include "symboldatabase.h"
#include "globaltokenizer.h"

// Register this check class (by creating a static instance of it)
namespace {
	CheckStatistic checkStatistic;
}

void CheckStatistic::checkFuncRetNull()
{
	std::map<std::string, std::map<const gt::CFunction*, std::list<FuncRetInfo> > >& threadData 
		= CGlobalStatisticData::Instance()->GetFuncRetNullThreadData(_errorLogger);

	int curFileIndex = -1;
	std::string curFile;
	
	for (const Token* tok = _tokenizer->list.front(); tok; tok = tok->next())
	{
		if ((int)tok->fileIndex() != curFileIndex)
		{
			curFileIndex = tok->fileIndex();
			curFile = _tokenizer->list.file(tok);

			std::map<std::string, std::map<const gt::CFunction*, std::list<FuncRetInfo> > >::iterator findFile = threadData.find(curFile);
			if (findFile != threadData.end())
			{
				const Token* tok2 = tok;
				while (tok2->next() && tok2->next()->fileIndex() == tok->fileIndex())
				{
					tok2 = tok2->next();
				}
				tok = tok2;
				continue;
			}
		}

		std::map<const gt::CFunction*, std::list<FuncRetInfo> >& infoList = threadData[curFile];
		
		if (tok->str() == "=" && tok->astOperand1() && tok->astOperand2() && !tok->astParent())
		{
			const Token* tokAstVar = tok->astOperand1();
			const Token* tokAstFunc = tok->astOperand2();

			if (tokAstFunc->str() == "(" && tokAstFunc->astOperand1())
			{
				// check whether var is checked-null or dereferenced


				FuncRetInfo op = checkVarUsageAfterFuncReturn(tokAstVar, tokAstFunc);

				if (op.Op == FuncRetInfo::Unknown)
				{
					continue;
				}

				const Token* tokFunc = tokAstFunc->previous();
				if (tokFunc->str() == ">")
				{
					tokFunc = tokFunc->link()->previous();
				}
				const gt::CFunction* gtFunc = CGlobalTokenizer::Instance()->FindFunctionData(tokFunc);

				if (gtFunc)
				{
					infoList[gtFunc].push_back(op);
				}
			}

		}
	}
}

bool CheckStatistic::IsDerefTok(const Token* tok, bool bLeft)
{
	//todo: char *a[10]; for "*"
	if (tok->str() == "." && tok->originalName() == "->" && !tok->astUnderSizeof() && bLeft)
	{
		return true;
	}
	if (tok->str() == "*"  && tok->astOperand2() == nullptr && !tok->astUnderSizeof())
	{
		const Token* ltok = tok->astOperand1();
		if (ltok)
		{
			//it's better to modify AST to handle  A** p; but var * *p;
			bool isDeclaration = false;
			const Token* ptok = tok->astParent();
			if (ptok && ptok->str() == "*")
			{
				const Token *varTok = ptok->astOperand2();
				//add support to Type *** p;
				while (varTok && varTok->str() == "*" && varTok->astOperand1())
				{
					varTok = varTok->astOperand1();
				}
				if (!varTok)
				{
					return false;
				}
				const Variable * var = _tokenizer->getSymbolDatabase()->getVariableFromVarId(varTok->varId());
				if (var)
				{
					const Token* tokType = ptok;
					while (tokType->astOperand1())
					{
						tokType = tokType->astOperand1();
					}
					if (var->typeStartToken() == tokType || Token::Match(var->typeStartToken(), "struct|union|class"))
					{
						isDeclaration = true;
					}
				}
			}
			if (!isDeclaration)
			{
				return true;
			}
		}
	}
	if (tok->str() == "[" && !tok->astUnderSizeof() && bLeft) //maybe array definition
	{
		// A p[10];
		// A * p[10];
		// A * p[10][10];
		// A (*p)[10][10];
		//char(*t[10])[10];
		// char *p[] =  {"123"};
		const Token *pToken = nullptr;
		const Token *top = tok->astTop();
		if (top)
		{
			if (!Token::Match(top, "*|[|="))
			{
				return true;
			}
			if (top->str() == "=")
			{
				// int* a = new int[10];
				if (tok->astParent()->str() == "new")
				{
					return false;
				}

				if (top->astOperand2() && top->astOperand2()->str() != "{" && top->astOperand2()->tokType() != Token::eString)
				{
					return true;
				}
			}

			pToken = tok->astTop();
		}
		else if (tok->astOperand1())
		{
			pToken = tok->astOperand1();
		}
		else
		{
			return true;
		}
		while (pToken && pToken->astOperand1())
		{
			pToken = pToken->astOperand1();
		}
		if (pToken)
		{
			//char * p[10];
			if (!_settings->library.podtype(tok->str()) && pToken->tokType() != Token::eType
				&& !Token::Match(pToken, "bool|char|short|int|long|float|double"))
			{
				//char p[10]
				//char is not in ast
				if (tok->previous())
				{
					if (tok->previous() && (Token::Match(tok->previous(), "{|;|(,") || !tok->previous()->isName()))
					{
						return true;
					}
				}
				else
				{
					return true;
				}
			}
		}
	}
	else if (tok->str() == "<<" || tok->str() == ">>")
	{
		bool isOutput = (tok->str() == "<<");
		if (const Token* tok2 = tok->astOperand2())
		{
			SExprLocation exprLoc = CreateSExprByExprTok(tok->astOperand2());
			if (!exprLoc.Empty())
			{
				if (exprLoc.Expr.VarId)
				{
					const Token* tokVar = tok2;
					while (tokVar->astOperand2())
					{
						tokVar = tokVar->astOperand2();
					}

					if (const Variable* pVar = tokVar->variable())
					{
						if (pVar->isPointer() && !pVar->isArray())
						{
							if (isOutput)
							{
								if (pVar->typeStartToken()->str() == "char")
								{
									return true;
								}
							}
							else
							{
								return true;
							}
						}
					}
				}
			}
		}
	}
	return false;
}

FuncRetInfo CheckStatistic::checkVarUsageAfterFuncReturn(const Token* tokVar, const Token* tokAstFunc)
{

	const Scope* scope = tokVar->GetFuncScope();
	if (!scope)
	{
		return FuncRetInfo::UnknownInfo;
	}

	SExprLocation exprVar = CreateSExprByExprTok(tokVar);
	if (exprVar.Empty())
	{
		return FuncRetInfo::UnknownInfo;
	}
	const Token* tokScopeEnd = scope->classEnd;
	
	const Token* tokStart = tokAstFunc->link();
	if (!tokStart)
	{
		return FuncRetInfo::UnknownInfo;
	}

	SExprLocation findExpr;
	for (const Token* tok = tokStart->next(); tok && tok != tokScopeEnd; tok = tok->next())
	{
		if (tok->str() == exprVar.TokTop->str())
		{
			SExprLocation temp = CreateSExprByExprTok(tok);
			if (!temp.Empty() && temp.Expr == exprVar.Expr)
			{
				findExpr = temp;
				break;
			}
		}
	}

	if (findExpr.Empty())
	{
		return FuncRetInfo::UnknownInfo;
	}
	
	const Token* tokTop = findExpr.TokTop->astTop();
	const Token* tok2 = findExpr.TokTop->astParent();
	
	if (!tok2 || !tokTop)
	{
		return FuncRetInfo::UnknownInfo;
	}
	bool bLeft = (tok2->astOperand1() == findExpr.TokTop);

	if (tokTop->str() == "(")
	{
		if (tokTop->astOperand1())
		{
			if (tokTop->astOperand1()->str() == "if" || 
				_settings->CheckIfJumpCode(tokTop->astOperand1()->str()))
			{
				if (tok2->str() == "==" || tok2->str() == "!=")
				{
					bool bCheckNull = false;
					if (bLeft && tok2->astOperand2() && tok2->astOperand2()->str() == "0")
					{
						bCheckNull = true;
					}
					else if (!bLeft && tok2->astOperand1() && tok2->astOperand1()->str() == "0")
					{
						bCheckNull = true;
					}

					if (bCheckNull)
					{
						tok2 = tok2->astParent();
						while (tok2 && (Token::Match(tok2, "&&|%or%%or%")))
						{
							tok2 = tok2->astParent();
						}
						if (tok2 == tokTop)
						{
							return FuncRetInfo(FuncRetInfo::CheckNull, findExpr.TokTop->linenr(), findExpr.Expr.ExprStr);
						}
					}
				}
				else if (tok2->str() == "!")
				{
					tok2 = tok2->astParent();
					while (tok2 && (Token::Match(tok2, "&&|%or%%or%")))
					{
						tok2 = tok2->astParent();
					}
					if (tok2 == tokTop)
					{
						return FuncRetInfo(FuncRetInfo::CheckNull, findExpr.TokTop->linenr(), findExpr.Expr.ExprStr);
					}
				}
				else
				{
					while (tok2 && (Token::Match(tok2, "&&|%or%%or%|,")))
					{
						tok2 = tok2->astParent();
					}
					if (tok2 == tokTop)
					{
						return FuncRetInfo(FuncRetInfo::CheckNull, findExpr.TokTop->linenr(), findExpr.Expr.ExprStr);
					}
				}
			}
		}
		return FuncRetInfo::UnknownInfo;
	}
	else if (IsDerefTok(tok2, bLeft))
	{

		return FuncRetInfo(FuncRetInfo::Use, findExpr.TokTop->linenr(), findExpr.Expr.ExprStr);
	}
	else
		return FuncRetInfo::UnknownInfo;
}

void CheckStatistic::checkOutOfBounds()
{
	std::map<std::string, std::list<ArrayIndexInfo> >& threadData
		= CGlobalStatisticData::Instance()->GetOutOfBoundsThreadData(_errorLogger);

	int curFileIndex = -1;
	std::string curFile;
	std::list<ArrayIndexInfo>* infoList = NULL;

	for (const Token* tok = _tokenizer->list.front(); tok; tok = tok->next())
	{
		if ((int)tok->fileIndex() != curFileIndex)
		{
			curFileIndex = tok->fileIndex();
			curFile = _tokenizer->list.file(tok);

			std::map<std::string, std::list<ArrayIndexInfo> >::iterator findFile = threadData.find(curFile);
			if (findFile != threadData.end())
			{
				const Token* tok2 = tok;
				while (tok2->next() && tok2->next()->fileIndex() == tok->fileIndex())
				{
					tok2 = tok2->next();
				}
				tok = tok2;
				continue;
			}

			infoList = &threadData[curFile];
			
		}

		if (tok->str() != "for")
		{
			continue;
		}

		SExprLocation tsIndex;
		SExprLocation tsBoundary;

		const Token* tokStart = NULL;

		bool bOK = CheckForIsValid(tok, tsIndex, tsBoundary, tokStart);
		if (bOK)
		{

			CheckForBody(tokStart, tsIndex, tsBoundary, infoList);
		}
	}
}

const bool CheckStatistic::CheckForIsValid(const Token* tokFor, SExprLocation& tsIndex, SExprLocation& tsBoundary, const Token*& tokStart)
{
	const Token* tokTop = tokFor->next();
	if (!tokTop || tokTop->str() != "(" || !tokTop->link())
	{
		return false;
	}

	tokStart = tokTop->link()->next();
	if (tokStart->str() != "{" || !tokStart->link())
	{
		return false;
	}

	const Token* firstSep = tokTop->astOperand2();
	if (!firstSep || firstSep->str() != ";")
	{
		return false;
	}

	const Token* secondSep = firstSep->astOperand2();
	if (!secondSep || secondSep->str() != ";")
	{
		return false;
	}

	const Token* firstCond = firstSep->astOperand1();
	if (!firstCond || firstCond->str() != "=")
	{
		return false;
	}

	const Token* secondCond = secondSep->astOperand1();
	if (!secondCond)
	{
		return false;
	}

	const Token* thirdCond = secondSep->astOperand2();
	if (!thirdCond)
	{
		return false;
	}

	if (firstCond->str() != "=")
	{
		return false;
	}

	SExprLocation el11 = CreateSExprByExprTok(firstCond->astOperand1());
	
	if (el11.Empty() || el11.Expr.VarId == 0 || !firstCond->astOperand2() || firstCond->astOperand2()->str() != "0")
	{
		return false;
	}

	if (secondCond->str() != "<")
	{
		return false;
	}

	SExprLocation el21 = CreateSExprByExprTok(secondCond->astOperand1());
	SExprLocation el22 = CreateSExprByExprTok(secondCond->astOperand2());

	if (el11.Expr != el21.Expr || el22.Empty())
	{
		return false;
	}

	if (thirdCond->str() != "++")
	{
		return false;
	}


	SExprLocation el31 = CreateSExprByExprTok(thirdCond->astOperand1());
	if (el31.Expr != el11.Expr)
	{
		return false;
	}

	tsIndex = el11;
	tsBoundary = el22;

	return true;

}

void CheckStatistic::CheckForBody(const Token* tokStart, const SExprLocation& tsIndex, const SExprLocation& tsBoundary, std::list<ArrayIndexInfo>* info)
{
	if (!info)
	{
		return;
	}

	if (tsBoundary.GetVariable() && (tsBoundary.GetVariable()->isLocal() || tsBoundary.GetVariable()->isArgument()))
	{
		return;
	}

	for (const Token* tok = tokStart, *tokEnd = tokStart->link(); tok && tok != tokEnd; tok = tok->next())
	{
		if (tok->str() != "[")
		{
			continue;
		}

		SExprLocation elArray = CreateSExprByExprTok(tok->astOperand1());
		SExprLocation elIndex = CreateSExprByExprTok(tok->astOperand2());

		if (elArray.Empty() || elIndex.Expr != tsIndex.Expr)
		{
			continue;
		}

		if (elArray.GetVariable() && (elArray.GetVariable()->isLocal() || elArray.GetVariable()->isArgument()))
		{
			continue;
		}

		const Token* tokFor = tokStart->previous()->link()->previous();
		bool bRet = CheckBeforeFor(tokFor, tsBoundary, elArray);

		if (!bRet)
		{
			return;
		}


		ArrayIndexInfo aii;
		aii.ArrayStr = FormatVarString(elArray.Expr.ExprStr);
		aii.ArrayLine = elArray.TokTop->linenr();
		aii.BoundLine = tsBoundary.TokTop->linenr();
		aii.BoundStr = FormatVarString(tsBoundary.Expr.ExprStr);

		aii.ArrayType = GetTypeString(elArray);
		aii.BoundType = GetTypeString(tsBoundary);

		info->push_back(aii);
	}
}

bool CheckStatistic::CheckBeforeFor(const Token* tokFor, const SExprLocation& tsBoundary, const SExprLocation& tsArray)
{
	const Scope* s = tokFor->GetFuncScope();
	if (!s)
	{
		return false;
	}
	for (const Token* tok = tokFor->previous(); tok && tok != s->classStart; tok = tok->previous() )
	{
		if (tok->str() == tsBoundary.TokTop->str())
		{
			SExprLocation elB = CreateSExprByExprTok(tok);
			if (elB.Expr == tsBoundary.Expr)
			{
				if (Token::Match(elB.TokTop->astParent(), "=|,|("))
				{
					return false;
				}
			}
		}

		if (tok->str() == tsArray.TokTop->str())
		{
			SExprLocation elA = CreateSExprByExprTok(tok);
			if (elA.Expr == tsArray.Expr)
			{
				if (Token::Match(elA.TokTop->astParent(), "=|,|(|."))
				{
					return false;
				}
			}
		}
	}

	return true;
}

std::string CheckStatistic::GetTypeString(const SExprLocation& el)
{
	std::string typeStr;
	return typeStr;
}

std::string CheckStatistic::FormatVarString(const std::string& var)
{
	static std::string s_strThis = "this.";
	std::size_t i = var.find(s_strThis);
	if (i == 0)
	{
		return var.substr(s_strThis.length());
	}

	return var;
}
