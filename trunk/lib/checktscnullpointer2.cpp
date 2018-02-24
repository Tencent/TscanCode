//
//  checkTSCNullPointer2.cpp
//
//
//

#include "tokenex.h"
#include "checktscnullpointer2.h"
#include "symboldatabase.h"
#include "globaltokenizer.h"
#include <fstream>

extern bool iscast(const Token *tok);


// Register CheckTSCNullPointer2..
namespace {
	CheckTSCNullPointer2 instance;
}

CExprValue CExprValue::EmptyEV;

const CValueInfo CValueInfo::PossibleNull(0, true, true);
const CValueInfo CValueInfo::Unknown;

bool CheckExprIsSubField(const SExpr &expr, const SExpr& sub)
{
	const std::string& subStr = sub.ExprStr;
	const std::string& s = expr.ExprStr;

	if (subStr.size() <= s.size())
	{
		return false;
	}

	std::string::size_type offset = subStr.find(s);
	if (offset == 0)
	{
		char flag = subStr[s.length()];
		if (flag == '.' || flag == '[')
		{
			return true;
		}
	}
	return false;
}

bool CheckTSCNullPointer2::CanBeNull(const SExprLocation& sExprLoc, CExprValue& ev)
{
	if (sExprLoc.Empty())
	{
		return false;
	}

	if (const Variable* pVar = sExprLoc.GetVariable())
	{
		if (pVar->isIntegralType() && !pVar->isPointer())
		{
			if (!(pVar->isArgument() && pVar->isArray()))
			{
				return false;
			}
		}
			
	}

	MEV_CI I = m_localVar.PreCond.find(sExprLoc.Expr);
	if(I != m_localVar.PreCond.end())
	{
		bool bNull = I->second.CanBeNull();
		if (bNull)
		{
			ev = I->second;
		}
		return bNull;
	}
	
	return false;
}

bool CheckTSCNullPointer2::CanNotBeNull(const SExprLocation& sExprLoc, CExprValue& ev)
{
	if (sExprLoc.Empty())
	{
		return false;
	}

	if (const Variable* pVar = sExprLoc.GetVariable())
	{
		if (pVar->isIntegralType() && !pVar->isPointer())
		{
			if (!(pVar->isArgument() && pVar->isArray()))
			{
				return true;
			}
		}

	}

	MEV_CI I = m_localVar.PreCond.find(sExprLoc.Expr);
	if (I != m_localVar.PreCond.end())
	{
		bool bNull = I->second.CanNotBeNull();
		if (bNull)
		{
			ev = I->second;
		}
		return bNull;
	}
	
	return false;
}


bool CheckTSCNullPointer2::CheckSpecialCheckNull(const SExpr& expr, const Token* tok, bool& bSafe, CExprValue& ouyputEV)
{
	if (!tok)
		return false;

	const Token* tokP = tok->astParent();
	if (!tokP)
		return false;

	if (tokP->str() == "&&" || tokP->str() == "||")
	{
		bool bAnd = (tokP->str() == "&&");
		std::shared_ptr<CCompoundExpr> spCond(new CCompoundExpr);
		if (ExtractCondFromToken(tok, spCond.get()))
		{
			if (!spCond->IsUnknownOrEmpty())
			{

				std::vector<CCompoundExpr*> listCE;
				spCond->ConvertToList(listCE);

				for (std::vector<CCompoundExpr*>::iterator it = listCE.begin(); it != listCE.end(); it++)
				{

					CCompoundExpr* ce = *it;
					if (ce->GetExprValue().GetExpr() == expr)
					{
						const CExprValue& ev = ce->GetExprValue();
						if (ev.IsCertain())
						{
							if (ev.GetValueInfo().IsNotEqualZero())
							{
								ouyputEV = ev;
								bSafe = bAnd;
								return true;
							}
							else if (ev.GetValueInfo().IsEqualZero())
							{
								ouyputEV = ev;
								bSafe = !bAnd;
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


bool CheckTSCNullPointer2::HasSpecialCheckNull(const SExprLocation& sExprLoc, bool& bSafe, CExprValue& outputEV)
{
	bool bLeft = true;
	const Token* pTok = sExprLoc.TokTop;
	while (pTok)
	{
		SExprLocation exprLoc_l = SExprLocation::EmptyExprLoc;
		SExprLocation exprLoc_r = SExprLocation::EmptyExprLoc;
		if (pTok->str() == "&&")
		{
			while (pTok && pTok->str() == "&&")
			{
				if (!bLeft)
				{
					if (CheckSpecialCheckNull(sExprLoc.Expr, pTok->astOperand1(), bSafe, outputEV))
					{
						return true;
					}
				}

				const Token* temp = pTok->astParent();
				if (temp)
				{
					bLeft = (temp->astOperand1() == pTok);
				}
				pTok = temp;
			}
		}
		//handle NULL==p || 2==p->xx
		else if (pTok->str() == "||")
		{
			while (pTok && pTok->str() == "||")
			{
				if (!bLeft)
				{
					if (CheckSpecialCheckNull(sExprLoc.Expr, pTok->astOperand1(), bSafe, outputEV))
					{
						return true;
					}
				}

				const Token* temp = pTok->astParent();
				if (temp)
				{
					bLeft = (temp->astOperand1() == pTok);
				}
				pTok = temp;
			}
		}
		//handle top ? top->valuetype : nullptr
		else if (pTok->str() == ":")
		{
			const Token* tok2 = pTok->astParent();
			if (!tok2)
			{
				return false;
			}
			if (tok2->str() == "?")
			{
				if (const Token* tok3 = tok2->astOperand1())
				{
					std::shared_ptr<CCompoundExpr> spCond(new CCompoundExpr);
					if (ExtractCondFromToken(tok3, spCond.get()))
					{
						if (!spCond->IsUnknownOrEmpty())
						{
							CCompoundExpr* ce = nullptr;
							if (spCond->HasSExpr(sExprLoc.Expr, &ce))
							{
								const CExprValue& ev = ce->GetExprValue();
								if (ev.IsCertain())
								{
									if (ev.GetValueInfo().IsEqualZero())
									{
										outputEV = ev;
										bSafe = !bLeft;
										return true;
									}
									else if (ev.GetValueInfo().IsNotEqualZero())
									{
										outputEV = ev;
										bSafe = bLeft;
										return true;
									}
								}
							}
						}
					}
				}
			}
			const Token* temp = pTok->astParent();
			if (temp)
			{
				bLeft = (temp->astOperand1() == pTok);
			}
			pTok = temp;
		}
		else
		{
			const Token* temp = pTok->astParent();
			if (temp)
			{
				bLeft = (temp->astOperand1() == pTok);
			}
			pTok = temp;
		}
	}
	return false;
}

bool CheckTSCNullPointer2::HasSpecialCheckNull2(const SExprLocation& sExprLoc, bool& bSafe, CExprValue& outputEV)
{
	bool bRight = true;
	const Token* pTok = sExprLoc.TokTop;
	//not handle , exp 
	/*eg. crash bug 
		#define CHECK_CHAR( d )  ( temp = (FT_Char)d, temp == d )
		#define CHECK_BYTE( d )  ( temp = (FT_Byte)d, temp == d )
		void test266(A* bitmap)
		{

			if ( !CHECK_BYTE( bitmap->rows  )     ||
				!CHECK_BYTE( bitmap->width )     ||
				!CHECK_CHAR( bitmap->pitch )
				)
				;
		}
	*/
	while (pTok && pTok->str()!=",")//not handle , exp 
	{
		SExprLocation exprLoc_l = SExprLocation::EmptyExprLoc;
		SExprLocation exprLoc_r = SExprLocation::EmptyExprLoc;
		if (pTok->str() == "&&")
		{
			while (pTok && pTok->str() == "&&")
			{
				if (!bRight)
				
				{
					std::shared_ptr<CCompoundExpr> spCond(new CCompoundExpr);
					if (ExtractCondFromToken(pTok->astOperand2(), spCond.get()))
					{
						if (spCond->GetExprValue().GetExpr() == sExprLoc.Expr && (spCond->GetExprValue().GetValueInfo().IsEqualZero() || spCond->GetExprValue().GetValueInfo().IsNotEqualZero()))
						{
							return true;
						}
					}
				}

				const Token* temp = pTok->astParent();
				if (temp)
				{
					bRight = (temp->astOperand2() == pTok);
				}
				pTok = temp;
			}
		}
		//handle NULL==p || 2==p->xx
		else if (pTok->str() == "||")
		{
			while (pTok && pTok->str() == "||")
			{
				if (!bRight)
				{
					std::shared_ptr<CCompoundExpr> spCond(new CCompoundExpr);
					if (ExtractCondFromToken(pTok->astOperand2(), spCond.get()))
					{
						if (spCond->GetExprValue().GetExpr() == sExprLoc.Expr && (spCond->GetExprValue().GetValueInfo().IsEqualZero() || spCond->GetExprValue().GetValueInfo().IsNotEqualZero()))
						{
							return true;
						}
					}
				}

				const Token* temp = pTok->astParent();
				if (temp)
				{
					bRight = (temp->astOperand2() == pTok);
				}
				pTok = temp;
			}
		}
		else
		{
			const Token* temp = pTok->astParent();
			if (temp)
			{
				bRight = (temp->astOperand2() == pTok);
			}
			pTok = temp;
		}
	}
	return false;
}

bool CheckTSCNullPointer2::IsDerefTok(const Token* tok)
{
	//todo: char *a[10]; for "*"
	if (tok->str() == "." && tok->originalName() == "->" && !tok->astUnderSizeof())
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
	if (tok->str() == "[" && !tok->astUnderSizeof()) //maybe array definition
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

bool CheckTSCNullPointer2::checkVarId(const SExprLocation& sExprLoc, const CExprValue& ev)
{

	//varid不为0，且不相同时return true，其余return false;


	const Token* tok1 = sExprLoc.Begin;
	const Token* tok2 = ev.GetExprLoc().Begin;
	while (tok1 && tok2 && tok1 != sExprLoc.End && tok2 != ev.GetExprLoc().End)
	{
		if (tok1->varId() != 0 && tok2->varId() != 0 && tok1->varId() != tok2->varId())
		{
			return true;
		}

		tok1 = tok1->next();
		tok2 = tok2->next();
	}
	return false;
}

void CheckTSCNullPointer2::HandleNullPointerError(const SExprLocation& sExprLoc, const CExprValue& ev)
{
	//处理同名变量，varid不相同 
	if (checkVarId(sExprLoc, ev))//varid不一样，返回true
	{
		return;
	}
	if (ev.IsCertainAndEqual() && ev.IsCondIf())
	{
		//dereferenceIfNull has  the highest priority
		if (m_ifNullreportedErrorSet.find(sExprLoc.Expr) == m_ifNullreportedErrorSet.end())
		{
			dereferenceIfNullError(sExprLoc, ev);
			m_reportedErrorSet.insert(sExprLoc.Expr);
			m_ifNullreportedErrorSet.insert(sExprLoc.Expr);
			m_funcRetNullErrors.erase(sExprLoc.Expr);
		}
	}
	else
	{		
		//handle redundant error for the same variable
		if (m_reportedErrorSet.find(sExprLoc.Expr) == m_reportedErrorSet.end())
		{
			// 如果是FuncRetNull，先存起来不报
			if (ev.IsCondFuncRet() || ev.IsCondDynamic())
			{
				if (!m_funcRetNullErrors.count(sExprLoc.Expr))
				{
					m_funcRetNullErrors[sExprLoc.Expr] = SErrorBak(sExprLoc, ev);
				}
				m_localVar.AddDerefExpr(sExprLoc);
			}
			else
			{
				if (nullPointerErrorFilter(sExprLoc, ev))
				{
					nullPointerError(sExprLoc, ev);
				}
				m_reportedErrorSet.insert(sExprLoc.Expr);
				m_funcRetNullErrors.erase(sExprLoc.Expr);
			}
		}
	}
}

void CheckTSCNullPointer2::HandleArrayDef(const Token *tok)
{
	//T t[10]; not null
	//T *t[10]; not null 
	//T (*t)[10]; null
	//T t[10][10];
	//todo: T (*t[2])[10];
	
	const Token *pToken = tok;
	if (tok->astTop())
	{
		pToken = tok->astTop();
	}
	if (pToken->astOperand1() && pToken->astOperand1()->str() == "(")
	{
		return;
	}
	//char *p[1] = {"123"}
	//char p[10] = "";
	if (pToken && pToken->str() == "=" && pToken->astOperand2() && (pToken->astOperand2()->str() == "{" || pToken->astOperand2()->tokType() == Token::eString))
	{
		if (pToken->astOperand1() && pToken->astOperand1()->str() != "[")
		{
			pToken = pToken->astOperand1();
		}
	}
	if (pToken->str() != "[")//find variable location
	{
		if (pToken->astOperand1() && pToken->astOperand1()->str() == "[")
		{
			pToken = pToken->astOperand1();
		}
		else if (pToken->astOperand2() && pToken->astOperand2()->str() == "[")
		{
			pToken = pToken->astOperand2();
		}
	}
	while (pToken->str() == "[" && pToken->astOperand1())
	{
		pToken = pToken->astOperand1();
	}
	if (pToken->isName())
	{
		SExprLocation loc = CreateSExprByExprTok(tok->astOperand1());
		CExprValue ev2(loc, CValueInfo(0, false));
		ev2.SetCondInit();
		m_localVar.AddPreCond(ev2, true);
		m_localVar.AddInitStatus(ev2, true);
	}
}

void CheckTSCNullPointer2::HanldeDotOperator(const Token* tok)
{
	if (tok->astOperand1())
	{
		SExprLocation exprLoc = CreateSExprByExprTok(tok->astOperand1());
		if (!exprLoc.Empty())
		{
			if (m_localVar.HasPreCond(exprLoc.Expr))
			{
				const CExprValue& ev = m_localVar.PreCond[exprLoc.Expr];
				if (ev.GetValueInfo().IsEqualZero())
				{
					if (Token::Match(tok, ". %any% (|<") && !Token::Match(tok->next(), "swap|reset"))
					{
						m_localVar.ResetExpr(exprLoc.Expr);
						CExprValue ev2(exprLoc, CValueInfo(0, false));
						ev2.SetCondInit();
						m_localVar.AddPreCond(ev2, true);
						m_localVar.AddInitStatus(ev2, true);
					}
				}
			}
		}
	}
}

void CheckTSCNullPointer2::HandleJumpCode(const Token* tok)
{
	if (const Token* tokP = tok->astParent())
	{
		if (tokP->str() == "(")
		{
			const std::set<int> & checkIndex = _settings->_jumpCode.find(tok->str())->second;
			if (!checkIndex.empty())
			{
				std::vector<const Token*> args;
				getParamsbyAst(tokP, args);
				for (size_t index = 0, size = args.size(); index < size; ++index)
				{
					if (checkIndex.find(index + 1) != checkIndex.end())
					{
						bool bHandled = false;
						const Token* tokAtIndex = args[index];
						if (Token::Match(tokAtIndex, "0|false"))
						{
							HandleReturnToken(tok);
							bHandled = true;
						}
						else if (tokAtIndex->str() == "!")
						{
							// check(!"asdf");
							if (const Token* tok2 = tokAtIndex->astOperand1())
							{
								if (Token::Match(tok2, "1|true") || tok2->tokType() == Token::eString )
								{
									HandleReturnToken(tok);
									bHandled = true;
								}
								
							}
						}
						
						if (!bHandled)
						{
							std::shared_ptr<CCompoundExpr> spCE(new CCompoundExpr);
							ExtractCondFromToken(tokAtIndex, spCE.get());
							if (!spCE->IsUnknownOrEmpty())
							{
								CastIfCondToPreCond(spCE.get(), CT_LOGIC);
							}
						}
					}
				}
			}
			else
			{
				HandleReturnToken(tok);
			}
		}
		//todo: add else
	}
	// simply something like THROW;
	else
	{
		HandleReturnToken(tok);
	}
	
}

bool CheckTSCNullPointer2::CheckIfCondAlwaysTrueOrFalse(SLocalVar& localVar, CCompoundExpr* ifCond, bool& bAlwaysTrue)
{
	bAlwaysTrue = false;

	if (!ifCond || ifCond->IsUnknownOrEmpty())
		return false;

	if (localVar.GetIfReturn())
	{
		return false;
	}

	std::vector<CCompoundExpr*> listCE;
	ifCond->ConvertToList(listCE, false);

	for (std::vector<CCompoundExpr*>::iterator I = listCE.begin(), E = listCE.end(); I != E; ++I)
	{
		CCompoundExpr* ce = *I;
		const CExprValue& ev = ce->GetExprValue();
		const SExpr& expr = ev.GetExpr();
		bool bAnd, bOr;
		ce->GetCondjStatus(bAnd, bOr);
		if (localVar.HasPreCond(expr))
		{
			const CExprValue& ev2 = localVar.PreCond[expr];
			if (ev2.IsCertain())
			{
				if (!bAnd)
				{
					if (ev2 == ev)
					{
						bAlwaysTrue = true;
						return true;
					}
				}
				else if (!bOr)
				{
					if (CExprValue::IsExprValueOpposite(ev, ev2))
					{
						bAlwaysTrue = false;
						return true;
					}
				}
			}
		}
	}

	return false;
}


bool CheckTSCNullPointer2::SpecialHandleForCond(const Token* tokFor)
{
	const Token* start = tokFor->next()->astOperand2();
	const Token* tokEnd = start->astOperand2();

	for (const Token* tok = start->next(); tok && tok != tokEnd; tok = tok->next())
	{
		if (Token::Match(tok, "< %var%"))
		{
			SExprLocation exprLoc = CreateSExprByExprTok(tok->next());
			if (!exprLoc.Empty())
			{
				bool bHandled = false;
				if (!bHandled)
				{
					if (m_localVar.HasInitStatus(exprLoc.Expr) && !m_localVar.HasPreCond(exprLoc.Expr))
					{
						CExprValue initEV = m_localVar.InitStatus[exprLoc.Expr];
						if (initEV.IsCertainAndEqual() && !initEV.GetValueInfo().IsStrValue())
						{
							CExprValue negEV = initEV;
							negEV.NegationValue();

							for (MMEC_I I = m_localVar.EqualCond.find(exprLoc.Expr); I != m_localVar.EqualCond.end() && I->first == exprLoc.Expr; ++I)
							{
								SEqualCond& ec = I->second;

								if (ec.EV == negEV)
								{
									// only one cond
									if (ec.EqualCE->GetConj() == CCompoundExpr::None)
									{
										CCompoundExpr* ifcond = m_localVar.GetIfCondPtr();
										CCompoundExpr* newCond = ifcond->DeepCopy();
										CCompoundExpr* newIf = new CCompoundExpr;
										newIf->SetConj(CCompoundExpr::Equal);
										newIf->SetLeft(newCond);
										newIf->SetRight(ec.EqualCE->DeepCopy());

										std::shared_ptr<CCompoundExpr> spNewIf(newIf);
										m_localVar.SetIfCond(spNewIf);
										bHandled = true;
									}
								}
							}
						}
					}
				}

				if (!bHandled)
				{
					//todo:consider float a < int b
					//float < int not allowed
					if ((tok->next()->variable() && tok->next()->variable()->isIntegralType())
						|| (!tok->next()->variable() && tok->previous() && tok->previous()->variable() && tok->previous()->variable()->isIntegralType()))
					{
						CExprValue newEV(exprLoc, CValueInfo(0, false));
						for (MMEC_I I = m_localVar.EqualCond.find(exprLoc.Expr); I != m_localVar.EqualCond.end() && I->first == exprLoc.Expr; ++I)
						{
							SEqualCond& ec = I->second;

							if (ec.EV == newEV)
							{
								// only one cond
								if (ec.EqualCE->GetConj() == CCompoundExpr::None)
								{
									CCompoundExpr* ifcond = m_localVar.GetIfCondPtr();
									CCompoundExpr* newCond = ifcond->DeepCopy();
									CCompoundExpr* newIf = new CCompoundExpr;
									newIf->SetConj(CCompoundExpr::Equal);
									newIf->SetLeft(newCond);
									newIf->SetRight(ec.EqualCE->DeepCopy());

									std::shared_ptr<CCompoundExpr> spNewIf(newIf);
									m_localVar.SetIfCond(spNewIf);
									bHandled = true;
								}
							}
						}
					}
				}
			}
		}
	}
	return false;
}

bool CheckTSCNullPointer2::CheckIfDelete(const Token *tok, const CExprValue & ev) const
{
	if (ev.GetValueInfo().IsNotEqualZero())
	{
		//todo: if (p){xxx; delete p;}
		//check if : *p; if | while | for (p){delete p;}
		//strict to if (p) not if (!p)
		const Token *pToken = tok->astParent();
		while (pToken && pToken->str() != "(")
		{
			pToken = pToken->astParent();
		}
		if (pToken && pToken->str() == "(")
		{
			if (pToken->link())
			{
				pToken = pToken->link();
				if (pToken->next() && pToken->next()->str() == "{")
				{
					const Token *pTokenAt2 = pToken->tokAt(2);
					if (pToken->tokAt(2))
					{
						if (pTokenAt2->str() == "delete")
						{
							const Token* tok2 = pTokenAt2->astOperand1();
							if (tok2)
							{
								SExprLocation sLoc = CreateSExprByExprTok(tok2);
								if (sLoc.Expr == ev.GetExpr())
								{
									return true;
								}
							}
							else
							{
								std::string exprStr;
								for (const Token *tokLoop = pTokenAt2->next(); tokLoop; tokLoop = tokLoop->next())
								{
									if (tokLoop->str() == ";")
									{
										break;
									}
									exprStr += tokLoop->str();
								}
								if (exprStr == ev.GetExpr().ExprStr)
								{
									return true;
								}
							}
						}
						else if (pTokenAt2->str() == "free")
						{
							std::string strDeref = "";
							if (pTokenAt2->next() && pTokenAt2->next()->str() == "(")
							{
								if (pTokenAt2->next()->astOperand2())
								{
									SExprLocation sLoc = CreateSExprByExprTok(pTokenAt2->next()->astOperand2());
									if (sLoc.Expr == ev.GetExpr())
									{
										return true;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return false;
}

bool CheckTSCNullPointer2::IsEmbeddedClass(const Token* tok)
{
	if (Token::Match(tok, "class|struct %name% {"))
	{
		const Token* tok2 = tok->tokAt(2);
		if (tok2->link() && Token::simpleMatch(tok2->link(), "} ;"))
		{
			return true;
		}
	}
	return false;
}

const Token* CheckTSCNullPointer2::HandleSmartPointer(const Token *tok)
{
	if (!tok->astParent())
	{
		return tok;
	}
	
	//new operator in function parameter is not simplified
	//smartptr.swap|reset(new p)
	if (tok->astParent()->str() == ".")
	{
		//left of . must be a variable
		//todo: check variable type
		/*if (!tok->tokAt(2) && tok->tokAt(2)->tokType() != Token::eVariable)
		{
			return;
		}*/
		//reset does not affect value of parameter
		const Token *pTokenTop = tok->astParent()->astParent();
		bool bLeft = (pTokenTop->astOperand1() == tok->astParent());
		if (!bLeft)
		{
			return tok;
		}
		if (!pTokenTop || pTokenTop->str() != "(")
		{
			return tok;
		}
		if (!pTokenTop->astOperand2())
		{
			return tok;
		}
		/////////////////////////////////
		// a.reset(param)              //
		//       (                     //      
		//      /  \                   //
		//    .      param             //
		//    /\                       //    
		//   a  reset                  //
		//                             //
		// or                          //
		//                             //
		//p.a.reset(param)             //
		//                 (           //
		//                 /\          //
		//                .  param     //
		//               /\            //
		//              .  reset       //
		//             / \             //
		//            p   a            //
		if (!pTokenTop->astOperand1() || !pTokenTop->astOperand1()->astOperand1())
		{
			return tok;
		}
		SExprLocation loc = CreateSExprByExprTok(pTokenTop->astOperand1()->astOperand1());
		//location must be of form A::B.c.d, A<T>::a.b notsupport
		if (loc.Empty())
		{
			return tok;
		}
		if (!loc.Begin->isName())
		{
			return tok;
		}
		for (const Token *pTmp = loc.Begin->next(); pTmp && pTmp != loc.End;)
		{
			if (!Token::Match(pTmp, ".|::"))
			{
				return tok;
			}
			if (!pTmp || pTmp->next() == loc.End)
			{
				return tok;
			}
			if (!pTmp->next()->isName())
			{
				return tok;
			}
			pTmp = pTmp->tokAt(2);
		}

		const Token *tokEnd = pTokenTop->link();
		if (!tokEnd)
		{
			return tok;
		}
		if (!tokEnd->next())
		{
			return tok;
		}
		if (tokEnd->next()->str() != ";")
		{
			return tok; 
		}

		for (const Token *tokLoop = pTokenTop->next(); tokLoop && tokLoop != tokEnd; tokLoop = tokLoop->next())
		{
			tokLoop = HandleOneToken(tokLoop);
		}

		const Token *tokParam = pTokenTop->astOperand2();
		if (tok->str() == "reset")
		{
			if (tokParam->str() != ",")
			{
				CExprValue curPre;
				HandleAssign(pTokenTop, loc, curPre);
			}
			//todo: smartptr.reset(p, dtor, alloc)
		}
		//swap value of param and ptr
		else if (tok->str() == "swap")
		{
			SExprLocation loc2 = CreateSExprByExprTok(tokParam);
			if (loc2.Empty())
			{
				return tok;
			}
			if (!loc2.Begin->isName())
			{
				return tok;
			}
			for (const Token *pTmp = loc2.Begin->next(); pTmp && pTmp != loc2.End;)
			{
				if (!Token::Match(pTmp, ".|::"))
				{
					return tok;
				}
				if (!pTmp || pTmp->next() == loc2.End)
				{
					return tok;
				}
				if (!pTmp->next()->isName())
				{
					return tok;
				}
				pTmp = pTmp->tokAt(2);
			}
			//todo: s.swap(std::shared_ptr<int>(new int));
			//s@1 . swap ( std :: shared_ptr < int > ( new int ) ) ;
			m_localVar.SwapCondition(loc, loc2);
			return tokEnd;
		}
	}
	//todo: swap(sp1, sp2)
	//c++11 support this type of swap
	else
	{

	}
	return tok;
}

void CheckTSCNullPointer2::HandleSelfOperator(const Token *tok)
{
	if (tok->astOperand1())
	{
		SExprLocation exprLoc = CreateSExprByExprTok(tok->astOperand1());

		if (!exprLoc.Empty())
		{
			if (m_localVar.HasPreCond(exprLoc.Expr))
			{
				CExprValue& ev = m_localVar.PreCond[exprLoc.Expr];
				if (ev.CanBeNull())
				{
					CExprValue newEv(exprLoc, CValueInfo(0, false));
					m_localVar.ResetExpr(exprLoc.Expr);
					m_localVar.AddPreCond(newEv);
				}
			}
		}
	}
}

const Token* CheckTSCNullPointer2::HandleTryCatch(const Token* tok)
{
	if ( Token::Match(tok, "catch ("))
	{
		if (const Token* tok2 = tok->linkAt(1))
		{
			if (tok2->next() && tok2->next()->str() == "{" && tok2->next()->link())
			{
				return tok2->next()->link();
			}
		}
	}
	return tok;
	
}

const Token* CheckTSCNullPointer2::HandleCase(const Token* tokCase)
{
	if (tokCase)
	{
		if ( const Token* tok2 = Token::findsimplematch(tokCase, ": ;") )
		{
			m_localVar = m_stackLocalVars.top();
			return tok2->next();
		}
	}
	return tokCase;
}

const void CheckTSCNullPointer2::CheckFuncCallToUpdateVarStatus(const Token* tokFunc, const Token* tok)
{
	if (tokFunc && tok)
	{
		// no args
		if (Token::Match(tok, "( void| )")) 
		{
			if (const Token* tok1 = tok->astOperand1())
			{
				if (Token::Match(tok1, "::|."))
				{
					if (const Token* tok2 = tok1->astOperand1())
					{
						SExprLocation exprLoc = CreateSExprByExprTok(tok2);
						if (!exprLoc.Empty())
						{
							std::string sExpr = exprLoc.Expr.ExprStr + tok1->str();
							
							for (MEV_I I = m_localVar.PreCond.begin(), E = m_localVar.PreCond.end();I != E;++I)
							{
								if (I->second.GetValueInfo().IsEqualZero())
								{
									SExpr expr = I->first;
									if (expr.ExprStr.find(sExpr) != std::string::npos)
									{
										CExprValue newEV = I->second;
										newEV.NegationValue();
										newEV.SetCondLogic();

										m_localVar.ResetExpr(expr);
										m_localVar.EraseInitStatus(expr);
	
										m_localVar.AddPreCond(newEV, true);
										break;
									}
								}
							}

							for (MDEREF_I I = m_localVar.DerefedMap.begin(), E = m_localVar.DerefedMap.end(); I != E;)
							{
								const SExpr& expr = I->first;
								if (expr.ExprStr.find(sExpr) != std::string::npos)
								{
									m_localVar.DerefedMap.erase(I++);
									continue;
								}
								++I;
							}
						}
					}
				}
			}
			
		}
		
	}
}


bool CheckTSCNullPointer2::CheckIfDirectFuncNotRetNull(const SExprLocation& expr) const
{
	const Token *end = expr.End;
	if (!end)
	{
		return false;
	}
	while (end && end->str() != ")" && end != expr.Begin)
	{
		end = end->previous();
	}

	if (!end || end->str() != ")" || !end->link())
	{
		return false;
	}

	const Token *link = end->link();
	if (!link->previous())
	{
		return false;
	}
	if (link->previous()->str() == ">")
	{
		link = link->previous()->link();
	}
	if (!link || !link->previous() || !link->previous()->isName())
	{
		return false;
	}
	
	return CGlobalTokenizer::Instance()->CheckIfMatchSignature(link->previous(),
		_settings->_functionNotRetNull, _settings->_equalTypeForFuncSig)
		//|| CheckIfLibFunctionReturnNull(link->previous())
		|| CGlobalTokenizer::Instance()->CheckFuncReturnNull(link->previous()) == gt::CFuncData::RetNullFlag::fNotNull;
}



void CheckTSCNullPointer2::CheckDerefBeforeCondError(CCompoundExpr* ifCond, const Token *tok, const Token* tokKey)
{
	if (!ifCond)
	{
		return;
	}

	if (m_localVar.DerefedMap.empty())
	{
		return;
	}

	std::vector<CCompoundExpr*> listCE;
	ifCond->ConvertToList(listCE);
	for (std::vector<CCompoundExpr*>::iterator I = listCE.begin(), E = listCE.end(); I != E; ++I)
	{
		CCompoundExpr* ce = *I;
		const CExprValue& ev = ce->GetExprValue();
		const SExpr& expr = ev.GetExpr();
		if (!ev.GetPossible() && (!ev.GetValueInfo().IsStrValue()) && (ev.GetValueInfo().GetValue() == 0))
		{
			if (!m_localVar.HasPreCond(expr) || m_localVar.PreCond[expr].CanReportBeforeCheckError())
			{
				if (m_localVar.HasDerefedExpr(expr))
				{
					if (m_reportedErrorSet.find(expr) == m_reportedErrorSet.end())
					{	
						if (CheckIfDelete(tok, ev))
						{
							return;
						}
						const SExprLocation& exprLoc = m_localVar.DerefedMap[expr].ExprLoc;
						if (nullPointerErrorFilter(exprLoc, ev))
						{
							bool bSafe = false;
							CExprValue ev2;
							bool isSpecialCheck = HasSpecialCheckNull(exprLoc, bSafe, ev2);
							if (!isSpecialCheck || (isSpecialCheck && !bSafe))
							{
								//except while (nullptr != (tok = tok->next()))
								if (Token::Match(tokKey, "while|if") && exprLoc.TokTop->linenr() == ev.GetExprLoc().TokTop->linenr())
								{
									return;
								}
								else
								{
									//different var with same name
									if (checkVarId(exprLoc, ev))
									{
										return;
									}
									derefBeforeCheckError(exprLoc, ev);
									m_reportedErrorSet.insert(expr);
									m_funcRetNullErrors.erase(expr);
									m_localVar.EraseDerefedStatus(expr);
								}
							}
						}
					}
				}
			}
		}
	}
}

void CheckTSCNullPointer2::HandleDerefTok(const Token* tok)
{
	SExprLocation exprLoc;
	if (TryGetDerefedExprLoc(tok, exprLoc, _tokenizer->getSymbolDatabase()))
	{
		HandleDeref(exprLoc);
	}
}

void CheckTSCNullPointer2::HandleDeref(const SExprLocation& exprLoc)
{
	if (!exprLoc.Empty())
	{
		bool bSafe = false;
		bool bSafe2 = false;
		CExprValue ev,ev2;
		if (HasSpecialCheckNull(exprLoc, bSafe, ev))
		{
			if (!bSafe)
			{
				if (m_reportedErrorSet.find(exprLoc.Expr) == m_reportedErrorSet.end())
				{
					checkNullDefectError(exprLoc);
					m_reportedErrorSet.insert(exprLoc.Expr);
					m_ifNullreportedErrorSet.insert(exprLoc.Expr);
				}
	
			}
		}
		//handle if (*p == 1 && p)
		else if (HasSpecialCheckNull2(exprLoc, bSafe2, ev2))
		{
			if (m_reportedErrorSet.find(exprLoc.Expr) == m_reportedErrorSet.end())
			{
			
				CExprValue evNull;
			    // handle if(data->a &&data) 漏报，canbeNULL返回false ,use CanNotBeNULL instead CanBeNULL
			
				if (CanNotBeNull(exprLoc, evNull))
				{
					return;
				}
				//don't call nullPointerErrorFilter because ev is not handle in HasSpecialCheckNull2()
				if (exprLoc.Expr.ExprStr == "this")
				{
					return;
				}

				for (const Token* tok = exprLoc.Begin; tok && tok != exprLoc.End; tok = tok->next())
				{
					if (tok->isExpandedMacro())
					{
						return;
					}
				}

				
				if (CheckIfExprLocIsArray(exprLoc))
				{
					return;
				}
				if (exprLoc.TokTop == NULL)
				{
					return;
				}

				if (m_reportedErrorSet.find(exprLoc.Expr) == m_reportedErrorSet.end())
				{
					checkNullDefectError(exprLoc);
					m_reportedErrorSet.insert(exprLoc.Expr);
					m_ifNullreportedErrorSet.insert(exprLoc.Expr);
				}
			}
			m_localVar.EraseDerefedStatus(exprLoc.Expr);
			

		}
		
		else
		{
			if (!m_localVar.HasPreCond(exprLoc.Expr) /*|| m_localVar.PreCond[exprLoc.Expr].CanReportBeforeCheckError()*/)
			{
				HandleNullPointerPossibleError(exprLoc);
				m_localVar.AddDerefExpr(exprLoc);
			}
			else
			{
				CExprValue evNull;
				if (CanBeNull(exprLoc, evNull))
				{
					HandleNullPointerError(exprLoc, evNull);
				}
			}
		}
	}
}

bool CheckTSCNullPointer2::HandleEqualExpr(const Token* tok, const Token*& tokExpr, CValueInfo& info, bool& bIterator)
{
	bool bEqual = (tok->str() == "==");
	const Token* tok1 = tok->astOperand1();
	const Token* tok2 = tok->astOperand2();
	if (tok1 && tok2)
	{
		const ValueFlow::Value* value = nullptr;
		const Token* tokValue = nullptr;

		bool bHandled = false;

		if (!bHandled)
		{
			if (tok1->isNumber())
			{
				if (tok1->values.size() == 1)
				{
					value = &tok1->values.front();
					//make sure value is certain and not string,value->tokvalue==true means it's string
					if (value->isKnown() && !value->tokvalue)
					{
						tokExpr = tok2;
						bHandled = true;
					}
					else
						value = nullptr;
				}
			}
			else if (tok2->isNumber())
			{
				if (tok2->values.size() == 1)
				{
					value = &tok2->values.front();
					if (value->isKnown() && !value->tokvalue)
					{
						tokExpr = tok1;
						bHandled = true;
					}
					else
						value = nullptr;
				}
			}
		}
		

		if (!bHandled)
		{
			//左操作数有明确值，取右操作数为tokExpr
			if (tok1->values.size() == 1)
			{
				value = &tok1->values.front();
				//make sure value is certain and not string,value->tokvalue==true means it's string
				if (value->isKnown() && !value->tokvalue)
				{
					tokExpr = tok2;
					bHandled = true;
				}
				else
					value = nullptr;
			}
		}
		//右操作数有明确值，取左操作数为tokExpr
		if (!bHandled)
		{
			if (tok2->values.size() == 1)
			{
				value = &tok2->values.front();
				if (value->isKnown() && !value->tokvalue)
				{
					tokExpr = tok1;
					bHandled = true;
				}
				else
					value = nullptr;
			}
		}

		if (!bHandled)
		{
			// if one size has just one token and it's varid == 0, assume it's a enum or macro which hasn't be expanded.
			if (tok1 == tok->previous() && tok1->varId() == 0 && (!tok1->astOperand1() && !tok1->astOperand2()))
			{

				tokExpr = tok2;
				tokValue = tok1;
				bHandled = true;
			}
			else if (tok2 == tok->next() && tok2->varId() == 0 && (!tok2->astOperand1() && !tok2->astOperand2()))
			{
				tokExpr = tok1;
				tokValue = tok2;
				bHandled = true;
			}
		}

		// handle iterator
		if (!bHandled)
		{
			if (IsIterator(tok1) && IsIteratorBeginOrEnd(tok2, false))
			{
				bIterator = true;
				tokExpr = tok1;
				bHandled = true;
			}
			else if (IsIterator(tok2) && IsIteratorBeginOrEnd(tok1, false))
			{
				bIterator = true;
				tokExpr = tok2;
				bHandled = true;
			}
		}


		if (bHandled)
		{
			if (value)
			{
				info = CValueInfo((int)value->intvalue, bEqual);
				
				return true;
				//bRet = AddEVToCompoundExpr(tokExpr, info, compExpr);
			}
			else if (tokValue)
			{
				info = CValueInfo(tokValue->str(), bEqual);
				return true;
				//bRet = AddEVToCompoundExpr(tokExpr, info, compExpr);
			}
			// for iterators
			else if (bIterator)
			{
				info = CValueInfo(0, bEqual);
				bIterator = true;
				return true;
				//bRet = AddEVToCompoundExpr(tokExpr, info, compExpr, true);
			}
		}
	}

	return false;
}

bool CheckTSCNullPointer2::IsDerefedTokPossibleBeNull(const SExprLocation& exprLoc)
{
	if (const Variable* var = exprLoc.GetVariable())
	{
		return !var->isArray();
	}
	else if (exprLoc.TokTop->str() == "(")
	{
		const Token* tokFunc = exprLoc.TokTop->previous();

		if (CGlobalTokenizer::Instance()->CheckIfMatchSignature(tokFunc,
			_settings->_functionNotRetNull, _settings->_equalTypeForFuncSig))
		{
			return false;
		}
		else
		{
			if (CheckIfLibFunctionReturnNull(tokFunc)
				|| CGlobalTokenizer::Instance()->CheckFuncReturnNull(tokFunc) != gt::CFuncData::RetNullFlag::fNotNull)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
	}

	return true;

}

const Token* CheckTSCNullPointer2::HandleCheckNull(const Token* tok)
{
	if (tok->str() == "for")
	{
		if (const Token* tok1 = tok->next()->astOperand2())
		{
			//for (auto & x : wt->db->members)
			if (tok1->str() == ";")
			{
				// part 1
				for (const Token* tok4 = tok->tokAt(2); tok4 && tok4 != tok1; tok4 = tok4->next())
				{
					tok4 = HandleOneToken(tok4);
				}

				if (const Token* tok2 = tok1->astOperand2())
				{
					if (const Token* tok3 = tok2->astOperand1())
					{
						UpdateIfCondFromCondition(tok3, tok);
					}
				}

				//part2
				const Token* tokend2 = tok1->astOperand2();
				for (const Token* tok5 = tok1->next(); tok5 && tokend2 && tok5 != tokend2; tok5 = tok5->next())
				{
					tok5 = HandleOneToken_deref(tok5);
				}

				return tok->next()->link();
			}
		}
	}
	else if (tok->str() == "do")
	{
		if (const Token* tok1 = tok->next()->link())
		{
			if (const Token* tok2 = tok1->tokAt(2))
			{
				if (const Token* tok3 = tok2->astOperand2())
				{
					UpdateIfCondFromCondition(tok3, tok);
				}
			}
		}
	}
	else
	{
		if ((tok->next()->str() == "(") && (tok->astParent() == tok->next()))
		{
			// ignore empty if checking null at the begin of function
			//if (tok->str() == "if")
			//{
			//	if (const Token* tok1 = tok->previous())
			//	{
			//		if (tok1->str() == "{" && tok1->scope() && tok1->scope()->isFunction())
			//		{
			//			if (const Token* tok2 = tok->next()->link())
			//			{
			//				if (const Token* tok3 = tok2->next())
			//				{
			//					if (tok3->link() == tok3->next())
			//					{
			//						return tok3->next();
			//					}
			//				}
			//			}
			//		}
			//	}
			//}

			UpdateIfCondFromCondition(tok->next()->astOperand2(), tok);
		}
		else
		{
			debugError(tok, "unexpected token list!");
		}
	}

	return tok;
}

std::string& CheckTSCNullPointer2::FindValueReturnFrom(const Token *tok, std::string &retFrom) const
{
	retFrom.clear();
	if (!tok)
	{
		return retFrom;
	}
	const Token *tokTop = tok->astTop();
	if (!tok)
	{
		return retFrom;
	}

	const Token *tokOper2 = tokTop->astOperand2();
	if (!tokOper2 || tokOper2->str() != "(")
	{
		return retFrom;
	}
	
	if (iscast(tokOper2))
	{
		tokOper2 = tokOper2->astOperand1();
		if (!tokOper2)
		{
			return retFrom;
		}
	}

	const Token *fooName = tokOper2->astOperand1();
	if (!fooName)
	{
		return retFrom;
	}
	while (fooName->astOperand2())
	{
		fooName = fooName->astOperand2();
	}
	retFrom = fooName->str();
	return retFrom;
}

const std::string CheckTSCNullPointer2::FindValueReturnFrom(const Token *tokBegin, const Token *tokEnd) const
{
	if (!tokBegin || tokBegin == tokEnd)
	{
		return std::string();
	}

	for (const Token *tok = tokBegin->next(); tok && tok != tokEnd; tok = tok->next())
	{
		if (tok->str() == "(")
		{
			if ((tok->previous()->tokType() == Token::eName || tok->previous()->tokType() == Token::eFunction))
			{
				return tok->previous()->str();
			}
			else if (tok->previous()->str() == ">" && tok->previous()->link())
			{
				const Token *nameToken = tok->previous()->link();
				if (nameToken->tokType() == Token::eName || nameToken->tokType() == Token::eFunction)
				{
					return nameToken->str();
				}
			}
		}
	}
	return std::string();
}

void CheckTSCNullPointer2::FindFuncDrefIn(const Token *tok, std::string &funcName) const
{
	if (!tok || !tok->astParent())
	{
		return ;
	}
	if (Token::Match(tok->astParent(), ".|[|*|>>|<<"))
	{
		return ;
	}

	const Token *top = tok->astTop();
	if (!top)
	{
		return ;
	}
	
	//possible function
	if (top->str() == "(")
	{
		if (top->previous() && top->previous()->str() == ">")
		{
			top = top->previous()->link();
		}
		if (top->previous() && top->previous()->isName() && !top->previous()->variable())
		{
			if (Token::Match(top->previous(), "if|for|while|switch"))
			{
				return;
			}
			funcName = top->previous()->str();
		}
	}
}


void CheckTSCNullPointer2::GetDerefLocStr(const Token *tok, std::string &strLoc) const
{
	std::string strFuncName;
	FindFuncDrefIn(tok, strFuncName);
	if (!strFuncName.empty())
	{
		strLoc = " in function[" + strFuncName + "]";
	}
}

void CheckTSCNullPointer2::debugError(const Token* tok, const char* errMsg)
{
	reportError(tok, Severity::debug, ErrorType::None, "NP_DEBUG", errMsg, ErrorLogger::GenWebIdentity(tok->str()));
}


void CheckTSCNullPointer2::nullPointerError(const SExprLocation& sExprLoc, const CExprValue& ev)
{
	std::stringstream ss;
	std::string strDerefLoc = "here";
	GetDerefLocStr(sExprLoc.TokTop, strDerefLoc);
	if (ev.IsCondIterator())
	{
		ss << "Iterator [" << sExprLoc.Expr.ExprStr << "] may be invalid here.";
		reportError(sExprLoc.Begin, Severity::error, ErrorType::NullPointer, "invalidDereferenceIterator", ss.str().c_str(),
			ErrorLogger::GenWebIdentity(sExprLoc.Expr.ExprStr));
	}
	else if (ev.IsCondCheckNull())
	{
		ss << "Comparing [" << sExprLoc.Expr.ExprStr << "] to null at line " << ev.GetExprLoc().TokTop->linenr() << " implies that [" << sExprLoc.Expr.ExprStr << " ] might be null.Dereferencing null pointer [" << sExprLoc.Expr.ExprStr << "].";
		if (strstr(sExprLoc.Expr.ExprStr.c_str(), "("))
		{
			if (CheckIfDirectFuncNotRetNull(sExprLoc))
			{
				return;
			}
			
			reportError(sExprLoc.Begin, Severity::error, ErrorType::NullPointer, "funcDereferenceAfterCheck", ss.str().c_str(), ErrorLogger::GenWebIdentity(sExprLoc.Expr.ExprStr));
		}
		else if (strstr(sExprLoc.Expr.ExprStr.c_str(), "["))
		{
			
			reportError(sExprLoc.Begin, Severity::error, ErrorType::NullPointer, "arrayDereferenceAfterCheck", ss.str().c_str(), ErrorLogger::GenWebIdentity(sExprLoc.Expr.ExprStr));
		}
		else
		{
			if (CheckIfExprLocIsArray(sExprLoc))
			{
				return;
			}
			
			reportError(sExprLoc.Begin, Severity::error, ErrorType::NullPointer, "dereferenceAfterCheck", ss.str().c_str(), ErrorLogger::GenWebIdentity(sExprLoc.Expr.ExprStr));
		}
	}
	else if (ev.IsCondFuncRet())
	{
		std::string strRetFrom;
		FindValueReturnFrom(ev.GetExprLoc().TokTop, strRetFrom);//ignore TSC
		ss << "Dereferencing a null pointer [" << sExprLoc.Expr.ExprStr << "] which is assigned with null return value from [" << strRetFrom << "] at line " <<ev.GetExprLoc().TokTop->linenr() << ".";
		if (strstr(sExprLoc.Expr.ExprStr.c_str(), "("))
		{
			
			reportError(sExprLoc.Begin, Severity::error, ErrorType::NullPointer, "funcFuncRetNull", ss.str().c_str(), ErrorLogger::GenWebIdentity(sExprLoc.Expr.ExprStr, strRetFrom));
		}
		else if (strstr(sExprLoc.Expr.ExprStr.c_str(), "["))
		{
			
			reportError(sExprLoc.Begin, Severity::error, ErrorType::NullPointer, "arrayFuncRetNull", ss.str().c_str(), ErrorLogger::GenWebIdentity(sExprLoc.Expr.ExprStr, strRetFrom));
		}
		else
		{
			if (CheckIfExprLocIsArray(sExprLoc))
			{
				return;
			}
			
			reportError(sExprLoc.Begin, Severity::error, ErrorType::NullPointer, "funcRetNull", ss.str().c_str(), ErrorLogger::GenWebIdentity(sExprLoc.Expr.ExprStr, strRetFrom));
		}
	}
	else if (ev.IsCertainAndEqual())
	{
		if (FilterExplicitNullDereferenceError(sExprLoc, ev))
		{
			//ss << "[" << sExprLoc.Expr.ExprStr << "] is dereferenced " << strDerefLoc << ". But it can be set null in a branch at line " << ev.GetExprLoc().TokTop->linenr();
			ss << "Dereferencing null pointer [" << sExprLoc.Expr.ExprStr << "], [" << sExprLoc.Expr.ExprStr << "] is assigned with null at line " << ev.GetExprLoc().TokTop->linenr() << ".";
			reportError(sExprLoc.Begin, Severity::error, ErrorType::NullPointer, "explicitNullDereference", ss.str().c_str(),
				ErrorLogger::GenWebIdentity(sExprLoc.Expr.ExprStr));
		}
	}
	else
	{
		if (CheckIfExprLocIsArray(sExprLoc))
		{
			return;
		}
		reportError(sExprLoc.Begin, Severity::error, ErrorType::NullPointer, "possibleDereferenceAfterCheck", "Possible null pointer dereference:" + sExprLoc.Expr.ExprStr,
			ErrorLogger::GenWebIdentity(sExprLoc.Expr.ExprStr));
	}
}

void CheckTSCNullPointer2::derefBeforeCheckError(const SExprLocation& exprLoc, const CExprValue& ev)
{
	std::string strDerefLoc = "";
	GetDerefLocStr(exprLoc.TokTop, strDerefLoc);
	if (ev.IsCondIterator())
	{
		std::stringstream ss;
		ss << "Iterator [" << exprLoc.Expr.ExprStr << "] is dereferenced "<< strDerefLoc <<"at line " << exprLoc.TokTop->linenr() << " before validating here.";
		reportError(ev.GetExprLoc().TokTop, Severity::error, ErrorType::NullPointer, "invalidDereferenceIterator", ss.str().c_str(), ErrorLogger::GenWebIdentity(exprLoc.Expr.ExprStr));
	}
	else
	{
		std::stringstream ss;
		ss << "Null - checking [" << exprLoc.Expr.ExprStr << "] suggests that it may be null, but it has already been dereferenced at line " << exprLoc.TokTop->linenr() <<strDerefLoc << ".";
		if (strstr(exprLoc.Expr.ExprStr.c_str(), "("))
		{
			if (CheckIfDirectFuncNotRetNull(exprLoc))
			{
				return;
			}
			
			reportError(ev.GetExprLoc().TokTop, Severity::error, ErrorType::NullPointer, "funcDereferenceBeforeCheck", ss.str().c_str(), ErrorLogger::GenWebIdentity(exprLoc.Expr.ExprStr));
		}
		else if (strstr(exprLoc.Expr.ExprStr.c_str(), "["))
		{
	
			reportError(ev.GetExprLoc().TokTop, Severity::error, ErrorType::NullPointer, "arrayDereferenceBeforeCheck", ss.str().c_str(), ErrorLogger::GenWebIdentity(exprLoc.Expr.ExprStr));
		}
		else
		{
			if (CheckIfExprLocIsArray(exprLoc))
			{
				return;
			}
	
			reportError(ev.GetExprLoc().TokTop, Severity::error, ErrorType::NullPointer, "dereferenceBeforeCheck", ss.str().c_str(), ErrorLogger::GenWebIdentity(exprLoc.Expr.ExprStr));
		}
	}
}

void CheckTSCNullPointer2::dereferenceIfNullError(const SExprLocation& exprLoc, const CExprValue& ev)
{
	std::stringstream ss;
	std::string strDerefLoc = "here";
	GetDerefLocStr(exprLoc.TokTop, strDerefLoc);
	if (strstr(exprLoc.Expr.ExprStr.c_str(), "("))
	{
		if (CheckIfDirectFuncNotRetNull(exprLoc))
		{
			return;
		}
		ss << "[" << exprLoc.Expr.ExprStr << "] is null dereferenced "<< strDerefLoc <<", as codes at line " << ev.GetExprLoc().TokTop->linenr() << " make it a null pointer.";
		reportError(exprLoc.TokTop, Severity::error, ErrorType::NullPointer, "funcDereferenceIfNull", ss.str().c_str(), ErrorLogger::GenWebIdentity(exprLoc.Expr.ExprStr));
	}
	else if (strstr(exprLoc.Expr.ExprStr.c_str(), "["))
	{
		ss << "[" << exprLoc.Expr.ExprStr << "] is null dereferenced " << strDerefLoc << ", as codes at line " << ev.GetExprLoc().TokTop->linenr() << " make it a null pointer.";
		reportError(exprLoc.TokTop, Severity::error, ErrorType::NullPointer, "arrayDereferenceIfNull", ss.str().c_str(), ErrorLogger::GenWebIdentity(exprLoc.Expr.ExprStr));
	}
	else
	{
		if (CheckIfExprLocIsArray(exprLoc))
		{
			return;
		}
		ss << "[" << exprLoc.Expr.ExprStr << "] is null dereferenced " << strDerefLoc << ", as codes at line " << ev.GetExprLoc().TokTop->linenr() << " make it a null pointer.";
		reportError(exprLoc.TokTop, Severity::error, ErrorType::NullPointer, "dereferenceIfNull", ss.str().c_str(), ErrorLogger::GenWebIdentity(exprLoc.Expr.ExprStr));
	}
}


void CheckTSCNullPointer2::HandleNullPointerPossibleError(const SExprLocation& sExprLoc)
{

	if (!_settings->isEnabled("warning"))
	{
		return;
	}

	// this 不报错
	if (sExprLoc.Expr.ExprStr == "this")
	{
		return;
	}

	if (m_possibleReportedErrorSet.find(sExprLoc.Expr) == m_possibleReportedErrorSet.end())
	{
		if (IsDerefedTokPossibleBeNull(sExprLoc))
		{
			if (sExprLoc.TokTop->str() == "(")
			{
				directFuncPossibleError(sExprLoc);
			}
			else if (sExprLoc.TokTop->str() == "[")
			{
				arrayPossibleError(sExprLoc);
			}
			else if (IsParameter(sExprLoc.TokTop))
			{
				parameterPossibleError(sExprLoc);
			}
			else if (IsClassMember(sExprLoc.TokTop))
			{
				classMemberPossibleError(sExprLoc);
			}
			else
			{
				possibleNullPointerError(sExprLoc);
			}
		}
		m_possibleReportedErrorSet.insert(sExprLoc.Expr);
	}
}

void CheckTSCNullPointer2::possibleNullPointerError(const SExprLocation& exprLoc)
{
	if (CheckIfExprLocIsArray(exprLoc))
	{
		return;
	}
	reportError(exprLoc.Begin, Severity::error, ErrorType::NullPointer, "possibleNullDereferenced", "Possible null pointer dereference:" + exprLoc.Expr.ExprStr, ErrorLogger::GenWebIdentity(exprLoc.Expr.ExprStr));
}

bool CheckTSCNullPointer2::nullPointerErrorFilter(const SExprLocation& sExprLoc, const CExprValue& ev)
{
	// this 不报错
	if (sExprLoc.Expr.ExprStr == "this")
	{
		return  false;
	}

	int macroLine1	= 0;
	int macroLine2 = 0;

	for (const Token* tok = sExprLoc.Begin; tok && tok != sExprLoc.End; tok = tok->next())
	{
		if (tok->isExpandedMacro())
		{
			macroLine1 = tok->linenr();
			break;
		}
	}

	for (const Token* tok = ev.GetExprLoc().Begin; tok && tok != ev.GetExprLoc().End; tok = tok->next())
	{
		if (tok->isExpandedMacro())
		{
			macroLine2 = tok->linenr();
			break;
		}
	}

	if (macroLine1 && (macroLine1 == macroLine2))
	{
		return false;
	}

	return true;
}

void CheckTSCNullPointer2::arrayPossibleError(const SExprLocation& exprLoc)
{
	if (CheckIfExprLocIsArray(exprLoc))
	{
		return;
	}
	reportError(exprLoc.Begin, Severity::error, ErrorType::NullPointer, "nullPointerArray", "Possible null pointer dereference:" + exprLoc.Expr.ExprStr, ErrorLogger::GenWebIdentity(exprLoc.Expr.ExprStr));
}

void CheckTSCNullPointer2::parameterPossibleError(const SExprLocation& exprLoc)
{
	reportError(exprLoc.Begin, Severity::error, ErrorType::NullPointer, "nullPointerArg", "Possible null pointer dereference:" + exprLoc.Expr.ExprStr, ErrorLogger::GenWebIdentity(exprLoc.Expr.ExprStr));
}

void CheckTSCNullPointer2::classMemberPossibleError(const SExprLocation& exprLoc)
{
	reportError(exprLoc.Begin, Severity::error, ErrorType::NullPointer, "nullPointerClass", "Possible null pointer dereference:" + exprLoc.Expr.ExprStr, ErrorLogger::GenWebIdentity(exprLoc.Expr.ExprStr));
}

void CheckTSCNullPointer2::directFuncPossibleError(const SExprLocation& exprLoc)
{
	reportError(exprLoc.Begin, Severity::error, ErrorType::NullPointer, "directFuncPossibleRetNull", "Possible null pointer dereference:" + exprLoc.Expr.ExprStr, ErrorLogger::GenWebIdentity(exprLoc.Expr.ExprStr, FindValueReturnFrom(exprLoc.Begin, exprLoc.End)));
}


void CheckTSCNullPointer2::GetForLoopIndexByLoopTok(const Token* tokLoop, std::vector<SExprLocation>& indexList)
{
	if (!tokLoop)
	{
		return;
	}
	if (Token::Match(tokLoop, "++|--"))
	{
		if (const Token* tokExpr = tokLoop->astOperand1())
		{
			SExprLocation exprLoc = CreateSExprByExprTok(tokExpr);
			if (!exprLoc.Empty())
			{
				indexList.push_back(exprLoc);
			}
		}
	}
	else if (tokLoop->str() == ",")
	{
		GetForLoopIndexByLoopTok(tokLoop->astOperand1(), indexList);
		GetForLoopIndexByLoopTok(tokLoop->astOperand2(), indexList);
	}
}

void CheckTSCNullPointer2::TSCNullPointerCheck2()
{
	OpenDumpFile();

	InitNameVarMap();

	const SymbolDatabase *symbolDatabase = _tokenizer->getSymbolDatabase();

	const std::size_t functions = symbolDatabase->functionScopes.size();
	for (std::size_t i = 0; i < functions; ++i) 
	{
		const Scope * scope = symbolDatabase->functionScopes[i];
		//if (scope->function == 0 || !scope->function->hasBody()) // We only look for functions with a body
		//	continue;

		const Token *tok = scope->classStart;

		const Token* end = scope->classEnd;
		if (tok && end)
		{
			end = end->next();
			for (; tok && tok != end; tok = tok->next())
			{
				tok = HandleOneToken(tok);
			}
			ReportFuncRetNullErrors();

			Clear();
		}
	}

	CloseDumpFile();
}

void CheckTSCNullPointer2::HandleReturnToken(const Token * tok)
{
	m_localVar.SetReturnTok(tok);
	m_localVar.SetReturn(true);
}

void CheckTSCNullPointer2::UpdateIfCondFromCondition(const Token* tok, const Token* tokKey)
{
	CCompoundExpr* newIfCond = new CCompoundExpr;
	ExtractCondFromToken(tok, newIfCond, true);//ignore TSC
	SimplifyIfCond(newIfCond, newIfCond);//ignore TSC
	SimplifyIfCond2(newIfCond, newIfCond);//ignore TSC
	SimplifyIfCond3(newIfCond, newIfCond);//ignore TSC
	SimplifyIfCond4(newIfCond, newIfCond);//ignore TSC
	SimplifyCondWithUpperCond(newIfCond);//ignore TSC

	std::shared_ptr<CCompoundExpr> spNewIfCond(newIfCond);
	m_localVar.SetIfCond(spNewIfCond);
	ExtractDerefedExpr(tok, tokKey, m_localVar.GetIfCondDerefed());
	

	// for,while,if认为是有效判空
	CheckDerefBeforeCondError(m_localVar.GetIfCondPtr(), tok,tokKey);
	
	if (tokKey->str() == "for")
	{
		SpecialHandleForCond(tokKey);//ignore TSC
	}
	
	return;
}

bool CheckTSCNullPointer2::AddEVToCompoundExpr(const Token* tok, const CValueInfo& info, CCompoundExpr* compExpr, bool bIterator)
{
	if (tok->str() == "=")
	{
		SExprLocation exprLoc1 = CreateSExprByExprTok(tok->astOperand1());
		SExprLocation exprLoc2 = CreateSExprByExprTok(tok->astOperand2());

		/*if (!exprLoc1.Empty() && !exprLoc2.Empty())
		{
			compExpr->SetConj(CCompoundExpr::Equal);
			CCompoundExpr* ceLeft = compExpr->CreateLeft();
			CCompoundExpr* ceRight = compExpr->CreateRight();
			
			CExprValue ev1(exprLoc1, info);
			ceLeft->SetExprValue(ev1);
			CExprValue ev2(exprLoc2, info);
			ceRight->SetExprValue(ev2);

			return true;
		}
		else */if (!exprLoc1.Empty())
		{
			CExprValue ev(exprLoc1, info);
			compExpr->SetExprValue(ev);
			return true;
		}
		else if (!exprLoc2.Empty())
		{
			//eg.if (AreaObject* ao=dynamic_cast<AreaObject*>(object))  AST bug: ='s left is * ,not ao
			if (exprLoc2.Begin->isDynamicCast())
			{
				return false;
			}
			CExprValue ev(exprLoc2, info);
			compExpr->SetExprValue(ev);
			return true;
		}
		else
			return false;

	}
	else if (Token::Match(tok, "==|!="))
	{
		if (info.IsStrValue())
		{
			return false;
		}

		bool needNegative = info.IsEqualZero() || (!info.GetEqual() && (info.GetValue() != 0));

		const Token* tokExpr = nullptr;
		bIterator = false;
		CValueInfo info2;
		if (HandleEqualExpr(tok, tokExpr, info2, bIterator))
		{
			if (needNegative)
			{
				info2.Negation();
			}
			return AddEVToCompoundExpr(tokExpr, info2, compExpr, bIterator);
		}
	}
	else
	{
		SExprLocation exprLoc = CreateSExprByExprTok(tok);
		if (exprLoc.Empty())
		{
			return false;
		}
		if (exprLoc.Begin->isDynamicCast())
		{
			return false;
		}
		CExprValue ev(exprLoc, info);
		if (bIterator)
		{
			ev.SetCondIterator();
		}
		compExpr->SetExprValue(ev);
		return true;
	}
	return false;
}

void CheckTSCNullPointer2::IfToPreInCond()
{
	// think about how to handle conflicts between IfCond and PreCond.
	// IfCond has higher priority, looks like OK
	CCompoundExpr* ifCond = m_localVar.GetIfCondPtr();
	if (ifCond)
	{
		CastIfCondToPreCond(ifCond, CT_IF);
		CastIfCondDerefedToPreCond(ifCond);
	}
	m_localVar.ClearIfCond();
}

void CheckTSCNullPointer2::CastIfCondDerefedToPreCond(CCompoundExpr* ifCond)
{
	const std::vector<SExprLocation>& ifDerefed = m_localVar.GetIfCondDerefed();
	for (std::vector<SExprLocation>::const_iterator CI = ifDerefed.begin(), CE = ifDerefed.end(); CI != CE; ++CI)
	{
		CExprValue newEV(*CI, CValueInfo(0, false));
		//newEV.SetCondLogic();
		newEV.SetCondNone(); //to prevent if (p->q) => p!=0 ascend to uppper scope
		newEV.SetCondIfDeref();

		CCompoundExpr* findCE = nullptr;
		if (ifCond->HasSExpr(CI->Expr, &findCE))
		{
			if (findCE->GetExprValue().IsCertainAndEqual())
			{
				continue;
			}
		}

		m_localVar.AddPreCond(newEV, true);
	}
}

void CheckTSCNullPointer2::CastIfCondToPreCond(CCompoundExpr* ifCond, ECondType condType)
{
	m_localVar.GetIfCondExpanded().reset();

	if (ifCond && !ifCond->IsUnknownOrEmpty())
	{
		std::shared_ptr<CCompoundExpr> ifCondExpanded(ExpandIfCondWithEqualCond(ifCond, m_localVar));
		m_localVar.SetIfCondExpanded(ifCondExpanded);

		bool bAnd = false;
		bool bOr = false;
		std::vector<CCompoundExpr*> listCE;
		ifCondExpanded->ConvertToList(listCE);

		for (std::vector<CCompoundExpr*>::iterator I = listCE.begin(), E = listCE.end(); I != E; ++I)
		{
			CCompoundExpr* ce = *I;

			if (ce->IsExpanded())
			{
				if (m_localVar.HasPreCond(ce->GetExprValue().GetExpr()))
				{
					const CExprValue& ev = m_localVar.PreCond[ce->GetExprValue().GetExpr()];
					if (ev.IsCertain() && !ev.IsCondLogic())
					{
						continue;
					}
				}
			}

			ce->GetCondjStatus(bAnd, bOr);

			if (bAnd && bOr)
			{
				continue;
			}

			CExprValue ev = ce->GetExprValue();

			if (condType == CT_IF)
			{
				ev.SetCondIf();
				ev.SetCondCheckNull();
			}
			else if (condType == CT_LOGIC)
			{
				ev.SetCondLogic();
			}
			
			if (!bOr)// p!=NULL && q!=NULL 或 p!=NULL 或 p==NULL
			{
				// as the equal cond is not reliable, don't force to update the pre cond if it's from equal cond
				m_localVar.AddPreCond(ev, true);
			}
			else if (!bAnd)// 条件一定是bOr==true, p==NULL || q==NULL
			{
				ev.SetPossible(true);
				m_localVar.AddPreCond(ev, false);//若前置条件更强(certain),则以前置条件为准
			}
		}


		// hanlde if(p1 == 0 && p2 == 0) {} else if(p1 == 0) {} else if (p2 == 0) {}
		if (listCE.size() == 2) 
		{
			CCompoundExpr* ce1 = listCE[0];
			CCompoundExpr* ce2 = listCE[1];
			//if (ce1->GetExprValue().GetValueInfo().IsNotEqualZero() && ce2->GetExprValue().GetValueInfo().IsNotEqualZero())
			{
				ce1->GetCondjStatus(bAnd, bOr);
				if (bOr && !bAnd)
				{
					CExprValue newEV1 = ce1->GetExprValue();
					newEV1.NegationValue();
					std::shared_ptr<CCompoundExpr> spCE1(ce2->DeepCopy());
					newEV1.SetCondForward();
					m_localVar.AddEqualCond(newEV1, spCE1, true);

					CExprValue newEV2 = ce2->GetExprValue();
					newEV2.NegationValue();
					std::shared_ptr<CCompoundExpr> spCE2(ce1->DeepCopy());
					newEV2.SetCondForward();
					m_localVar.AddEqualCond(newEV2, spCE2, true);
				}
			}
		}

	}
}

void CheckTSCNullPointer2::IfCondElse()
{
	CCompoundExpr* ifCond = m_localVar.GetIfCondPtr();
	if (!ifCond)
	{
		return;
	}
	ifCond->Negation();
}

void CheckTSCNullPointer2::IfCondEnd(const Token* tok, bool bReturn, bool bHasElse, SLocalVar& localvar, SLocalVar& topLocalVar)
{
	CCompoundExpr* ifCond = topLocalVar.GetIfCondPtr();
	if (!ifCond || ifCond->IsUnknownOrEmpty())
	{
		return;
	}

	bool bAnd = false;
	bool bOr = false;
	std::vector<CCompoundExpr*> listCE;
	ifCond->ConvertToList(listCE, false);

	for (std::vector<CCompoundExpr*>::iterator I = listCE.begin(), E = listCE.end(); I != E; ++I)
	{
		CCompoundExpr* ce = *I;
		ce->GetCondjStatus(bAnd, bOr);

		if (ce->IsEmpty())
		{
			continue;
		}

		CExprValue ev = ce->GetExprValue();
		ev.SetCondIf();
		ev.SetCondCheckNull();

		if (!bAnd)
		{
			if (bReturn)
			{
				// set cond type as CT_LOGIC
				ev.NegationValue();
                ev.SetCondLogic();
				
				// handle conflicts
				const SExpr& expr = ev.GetExpr();
				if (topLocalVar.HasPreCond(expr))
				{
					const CExprValue& ev2 = topLocalVar.PreCond[expr];
					// p != 0 and p == 0
					if (CExprValue::IsExprValueOpposite(ev, ev2))
					{
						//topLocalVar.ClearSExpr(expr);
						topLocalVar.AddPreCond(ev, true);
						// try expand
						ExpandPreCondWithEqualCond(ev, topLocalVar);
					}
					else
					{
						topLocalVar.AddPreCond(ev, true);
						// try expand
						ExpandPreCondWithEqualCond(ev, topLocalVar);
					}
				}
				else
				{
					topLocalVar.AddPreCond(ev, true);
					// try expand
					ExpandPreCondWithEqualCond(ev, topLocalVar);
				}
			}
			else
			{
				ev.SetPossible(true);

				// 1. if( s->a == null) { s = xxx;  }
				// 2. if(!p) { p != 0,L,V  }
				// 对于以上两种情况，不添加该前置条件
				bool bIgnore = false;
				if (ev.GetValueInfo().GetEqual())
				{
					for (MEV_I I2 = m_localVar.InitStatus_temp.begin(), E2 = m_localVar.InitStatus_temp.end(); I2 != E2; ++I2)
					{
						if (CheckExprIsSubField(I2->first, ev.GetExpr()))
						{
							bIgnore = true;
							break;
						}
						else if (I2->first == ev.GetExpr() && I2->second.GetValueInfo().IsNotEqualZero() && I2->second.IsInitVar())
						{
							bIgnore = true;

							//if (bHasElse)
							//{
							//	// 如果有else那么precond里面暂时还没有p!=0,L,V条件（else合并后才有），此时如果有"p!=0,L,V",则该条件是前面产生的，应该干掉。
							//	if (topLocalVar.HasPreCond(ev.GetExpr()))
							//	{
							//		const CExprValue& evPre = topLocalVar.PreCond[ev.GetExpr()];
							//		if (evPre.GetValueInfo().IsNotEqualZero() && evPre.IsInitVar())
							//		{
							//			topLocalVar.ErasePreCond(ev.GetExpr());
							//		}
							//	}
							//}

							break;
						}
					}

					if (bIgnore)
					{
						continue;
					}
				}

				topLocalVar.AddPreCond(ev, false);
			}
		}
		else if (!bOr)	//actually only and
		{	
			bool bHandled = false;
			if (!bHandled)
			{
				if (ev.CanBeNull())
				{
					bool bLeft = (ce->GetParent()->GetLeft() == ce);
					CCompoundExpr* ce2 = nullptr;
					if (bLeft)
						ce2 = ce->GetParent()->GetRight();
					else
						ce2 = ce->GetParent()->GetLeft();

					// two scenarios	
					// if (!pstAttr && GetShm2((void**)&pstAttr, ATTR_SHM_ID, getShmSize(), 0666) < 0) return -1;
					// if( !(iMatchFlag & V3_BIT_LANGUAGE) &&  sCodeBitMap == NULL ) return -1;
					if (ce2->IsUnknownOrEmpty())
					{
						ev.NegationValue();
						ev.SetCondLogic();

						topLocalVar.AddPreCond(ev, true);
						// try expand
						ExpandPreCondWithEqualCond(ev, topLocalVar);

						bHandled = true;
					}

				}
			}

			if (!bHandled)
			{
				// 处理如下场景
				//if (!s_instance || !s_instance->GetHWND())
				//{
				//	s_instance = std::make_shared<UserInfoWindow>(hParentWnd, true, diable_send_message);
				//}
				//else {
				//	s_instance->StopCheckHideTipsTimer();
				//}

				if (!ev.GetValueInfo().GetEqual())
				{
					if (topLocalVar.HasPreCond(ev.GetExpr()))
					{
						const CExprValue& topEV = topLocalVar.PreCond[ev.GetExpr()];
						if (topEV.GetValueInfo().IsNotEqualZero() && topEV.IsInitVar())
						{
							bHandled = true;
						}
					}
				}

			}
		
			if (!bHandled)
			{
				ev.SetPossible(true);

				// if( s->a == null) { s = xxx;  }
				bool bParentInited = false;
				if (ev.GetValueInfo().GetEqual())
				{
					for (MEV_I I2 = m_localVar.InitStatus_temp.begin(), E2 = m_localVar.InitStatus_temp.end(); I2 != E2; ++I2)
					{
						if (CheckExprIsSubField(I2->first, ev.GetExpr()))
						{
							bParentInited = true;
							break;
						}
					}

					if (bParentInited)
					{
						continue;
					}
				}

				topLocalVar.AddPreCond(ev, false);
				bHandled = true;
			}

			
		}
	}
}

void CheckTSCNullPointer2::IfToPreAfterCond(const Token* tok, bool bReturn, bool bHasElse, SLocalVar& localvar, SLocalVar& topLocalvar)
{
	if (!tok->scope())
	{
		debugError(tok, "Tok's scope should not be null.");
		return;
	}

	IfCondEnd(tok, bReturn, bHasElse, localvar, topLocalvar);
	
	//Scope::ScopeType scopeType = tok->scope()->type;
	//switch (scopeType)
	//{
	//case Scope::eFor:
	//	//fall through
	//case Scope::eWhile:
	//	//fall through
	//case Scope::eDo:
	//	//fall through
	//case Scope::eElse:
	//	//fall through
	//case Scope::eIf:
	//	IfCondEnd(bReturn, bHasElse, localvar, topLocalvar);		//根据是否有return添加前置条件
	//	break;
	//case Scope::eSwitch:
	//case Scope::eUnconditional:
	//case Scope::eTry:
	//case Scope::eLambda:
	//	// do nothing
	//	break;
	//default:
	//	break;
	//}
}

void CheckTSCNullPointer2::UpdateLocalVarWhenScopeBegin()
{
	m_stackLocalVars.push(m_localVar);
	m_localVar.UpdateStatusWhenStepIntoScope();
	IfToPreInCond();
}

void AddEqualCond(MMEC& temp, const CExprValue& ev, std::shared_ptr<CCompoundExpr>& ifcond)
{
	temp.insert(PEC(ev.GetExpr(), SEqualCond(ev, ifcond)));
}

void CheckTSCNullPointer2::UpdateLocalVarWhenScopeEnd(const Token* tok)
{
	SLocalVar& topLocalVar = m_stackLocalVars.top();

	//handle unconditional scope
	if (topLocalVar.GetIfCond() == nullptr)
	{
		if (!topLocalVar.InitStatus_temp.empty())
		{
			m_localVar.InitStatus_temp.insert(topLocalVar.InitStatus_temp.begin(), topLocalVar.InitStatus_temp.end());
		}
		
		if (topLocalVar.GetReturn() && !m_localVar.GetReturn())
		{
			m_localVar.SetReturn(true);
		}
		m_stackLocalVars.pop();
		return;
	}

	bool bIf2Pre = true;

	bool isElse = false;
	bool hasElse = false;
	if (tok->next() && tok->next()->str() == "else")
	{
		hasElse = true;
	}
	if (tok->link() && tok->link()->previous() && tok->link()->previous()->str() == "else")
	{
		isElse = true;
	}

	const Token* returnTok = m_localVar.GetReturnTok();
 
	topLocalVar.SetReturnTok(returnTok);
	bool bReturn = m_localVar.GetReturn();

	AscendDerefedMap(topLocalVar);
	
	// update init status 
	UpdateAssignedPreCond(topLocalVar);

	auto ifCond = topLocalVar.GetIfCond();
	if (ifCond && !ifCond->IsEmpty())
	{
		bool bAlwaysTrue;
		if (CheckIfCondAlwaysTrueOrFalse(topLocalVar, ifCond.get(), bAlwaysTrue))
		{
			if (hasElse)
			{
				topLocalVar.SetIfReturn(bReturn);
			}

			if (topLocalVar.GetIfReturn() && bReturn)
			{
				// if-else都是return，那么清空所有状态。
				topLocalVar.Clear();
				topLocalVar.SetReturn(true);
			}

			if (bAlwaysTrue)
			{
				HandleAlwaysTrueCond(bReturn, topLocalVar);
				// if(p == NULL) { p != NULL; }else { return; } 
				// 对于这种场景，不需要执行IfToPreAfterCond函数
				if (isElse && !hasElse)
				{
					bIf2Pre = false;
				}
			}
			else
			{
				// do nothing
			}
		}
		else
		{
			MMEC tempEqualCond;
			MEV tempPre;

			// ascend equal condition
			AscendEqualCond(topLocalVar, ifCond, tempEqualCond, bReturn);

			if (!bReturn)
			{
				ExtractEqualCond(ifCond, topLocalVar, tempEqualCond, hasElse);
				//ExtractDerefedEqualCond(ifCond, topLocalVar, tempEqualCond, hasElse);
				ExtractPreCond(tempPre, topLocalVar);
			}
			else
			{
				ExtractEqualCondReturn(tok, ifCond, topLocalVar, tempEqualCond);
				
			}

			if (hasElse)
			{
				topLocalVar.SetIfReturn(bReturn);
			}

			if (hasElse)
			{
				if (!tempEqualCond.empty())
				{
					//if (!p)
					//	p = getp();

					//if (!p)
					//{
					//	p = getp();
					//}
					//else
					//{
					//	dosth();
					//}
					topLocalVar.EqualCond_if = tempEqualCond;
				}
			}
			else
			{
				MergeEqualCond(tempEqualCond, bReturn, topLocalVar);
			}

			if (hasElse)
			{
				if (!tempPre.empty())
				{
					topLocalVar.PreCond_if = tempPre;
				}
			}
			else
			{
				MergePreCond(tempPre, bReturn, topLocalVar);
			}
		}
	
		if (bIf2Pre)
		{
			IfToPreAfterCond(tok, bReturn, hasElse, m_localVar, topLocalVar);
		}
	}
	if (!hasElse)
	{
		topLocalVar.ClearIfCond();
	}

	m_localVar = m_stackLocalVars.top();
	m_stackLocalVars.pop();
}

void CheckTSCNullPointer2::AscendDerefedMap(SLocalVar &topLocalVar)
{
	for (MDEREF_I I = m_localVar.DerefedMap.begin(), E = m_localVar.DerefedMap.end(); I != E; ++I)
	{
		const SExpr& expr = I->first;
		MDEREF_I I2 = topLocalVar.DerefedMap.find(expr);
		if (I2 == topLocalVar.DerefedMap.end())
		{
			topLocalVar.DerefedMap[expr] = I->second;
		}
	}
}

void CheckTSCNullPointer2::HandleAlwaysTrueCond(bool bReturn, SLocalVar &topLocalVar)
{
	if (!bReturn)
	{
		for (MEV_CI I = m_localVar.InitStatus_temp.begin(), E = m_localVar.InitStatus_temp.end(); I != E; ++I)
		{
			const SExpr& expr = I->first;
			const CExprValue& evAssigned = I->second;
			if (evAssigned.IsCertain())
			{
				if (m_localVar.HasPreCond(expr))
				{
					const CExprValue& ev2 = m_localVar.PreCond[expr];
					if (ev2 == evAssigned)
					{
						CExprValue newEV = evAssigned;
						newEV.SetCondLogic();
						topLocalVar.AddPreCond(newEV, true);
					}
				}
			}
		}
	}
}

void CheckTSCNullPointer2::Clear()
{
	if (m_stackLocalVars.size()!= 0)
	{
		debugError(nullptr, "stack of SLocalVar isn't correct!");
	}
	m_localVar.Clear();
	m_reportedErrorSet.clear();
	m_possibleReportedErrorSet.clear();
	m_ifNullreportedErrorSet.clear();
	m_funcRetNullErrors.clear();

}

bool CheckTSCNullPointer2::HandleRetParamRelation(const SExprLocation& exprLoc, const SExprLocation& exprLoc2, const gt::CFunction *gtFunc)
{
	//ret = foo(&p); ret != 0 => p != null
	const std::set<gt::CFuncData::ParamRetRelation> *pRetParam = CGlobalTokenizer::Instance()->CheckRetParamRelation(gtFunc);
	if (!pRetParam)
	{
		return false;
	}
	CExprValue ev2(exprLoc, CValueInfo::Unknown);
	ev2.SetCondFuncRet();
	m_localVar.AddPreCond(ev2, true);
	m_localVar.AddInitStatus(ev2, true);

	std::vector<const Token*> args;
	getParamsbyAst(exprLoc2.TokTop, args);
	bool bHandled = false;
	std::map<gt::CFuncData::ParamRetRelation, std::vector<SExprLocation> > mapRetMultiParam;
	bool bSetNeverNull = false;
	//flag = foo(&p); if (p) xxx
	for (std::set<gt::CFuncData::ParamRetRelation>::const_iterator iter = pRetParam[0].begin(), end = pRetParam[0].end(); iter != end; ++iter)
	{
		if ((size_t)iter->nIndex < args.size())
		{
			const Token *tokArg = args[iter->nIndex];
			if (!tokArg)
			{
				continue;
			}
			bool bCanAssigned = false;
			if (tokArg->str() == "&")
			{
				bCanAssigned = true;
				if (!tokArg->astOperand1())
				{
					continue;
				}
				tokArg = tokArg->astOperand1();
			}
			const SExprLocation paramLoc = CreateSExprByExprTok(tokArg);
			if (paramLoc.Empty())
			{
				continue;
			}

			//m_localVar.ResetExpr(paramLoc.Expr);
			CExprValue evLeft;
			if (iter->bStr)
			{
				evLeft = CExprValue(exprLoc, CValueInfo(iter->str, iter->bEqual));
			}
			else
			{
				evLeft = CExprValue(exprLoc, CValueInfo(iter->nValue, iter->bEqual));
			}
			if (!bCanAssigned)
			{
				const std::vector<gt::CVariable>& sigArgs = gtFunc->GetArgs();
				if ((size_t)iter->nIndex < sigArgs.size())
				{
					const std::string typeStr = sigArgs[iter->nIndex].GetTypeStr();
					if (typeStr.find('*') != std::string::npos && typeStr.find('&') != std::string::npos)
					{
						bCanAssigned = true;
					}
				}
			}
			CExprValue ev;
			if (bCanAssigned &&
				(iter->bNerverNull || (CanBeNull(paramLoc, ev) && ev.IsCondInit() && ev.IsCertainAndEqual())))
			{
				CExprValue evParam(paramLoc, CValueInfo(0, false));
				evParam.SetCondInit();
				m_localVar.AddPreCond(evParam, true);
				m_localVar.AddInitStatus(evParam, true);
				bSetNeverNull = true;
			}
			else
			{
				gt::CFuncData::ParamRetRelation prr = *iter;
				prr.nIndex = -1;
				mapRetMultiParam[prr].push_back(paramLoc);
			}
			bHandled = true;
		}
		
	}
	if (bHandled && !bSetNeverNull)
	{
		for (std::map<gt::CFuncData::ParamRetRelation, std::vector<SExprLocation> >::iterator iter = mapRetMultiParam.begin(), end = mapRetMultiParam.end();
		iter != end; ++iter)
		{
			CExprValue evLeft;
			if (iter->first.bStr)
			{
				evLeft = CExprValue(exprLoc, CValueInfo(iter->first.str, iter->first.bEqual));
			}
			else
			{
				evLeft = CExprValue(exprLoc, CValueInfo(iter->first.nValue, iter->first.bEqual));
			}
			if (iter->second.empty())
			{
				continue;
			}

			CExprValue evRight(iter->second[0], CValueInfo(0, iter->first.bNull));
			CCompoundExpr *ce = new CCompoundExpr;
			ce->SetExprValue(evRight);

			for (int nIndex = 1, nSize = iter->second.size(); nIndex < nSize; ++nIndex)
			{
				CCompoundExpr * conj(CCompoundExpr::CreateConjNode(CCompoundExpr::And));
				conj->SetLeft(ce);

				CExprValue evRight2(iter->second[nIndex], CValueInfo(0, iter->first.bNull));
				CCompoundExpr *ceRight = conj->CreateRight();
				ceRight->SetExprValue(evRight2);
				conj->SetRight(ceRight);
				ce = conj;
			}
			std::shared_ptr<CCompoundExpr> sce(ce);
			m_localVar.AddEqualCond(evLeft, sce);
		}
		return true;
	}
	//p = foo(&flag); if (flag) xxx;
	for (std::set<gt::CFuncData::ParamRetRelation>::const_iterator iter = pRetParam[1].begin(), end = pRetParam[1].end(); iter != end; ++iter)
	{
		if ((size_t)iter->nIndex < args.size())
		{
			const Token *tokArg = args[iter->nIndex];
			if (!tokArg)
			{
				continue;
			}
			if (tokArg->str() == "&")
			{
				if (!tokArg->astOperand1())
				{
					continue;
				}
				tokArg = tokArg->astOperand1();
			}
			const SExprLocation paramLoc = CreateSExprByExprTok(tokArg);
			if (paramLoc.Empty())
			{
				continue;
			}

			//m_localVar.ResetExpr(paramLoc.Expr);
			CExprValue evLeft(exprLoc, CValueInfo(0, iter->bNull));
			if (iter->bStr)
			{
				evLeft = CExprValue(exprLoc, CValueInfo(iter->str, iter->bEqual));
			}
			else
			{
				evLeft = CExprValue(exprLoc, CValueInfo(iter->nValue, iter->bEqual));
			}
			CExprValue evParam(paramLoc, CValueInfo(iter->nValue, iter->bEqual));
			std::shared_ptr<CCompoundExpr> ce = std::make_shared<CCompoundExpr>();
			ce->SetExprValue(evLeft);
			m_localVar.AddEqualCond(evParam, ce, false);
			bHandled = true;
		}
	}
	return bHandled;
}

void CheckTSCNullPointer2::HandleAssign(const Token * const tok, const SExprLocation& exprLoc, const CExprValue& curPre)
{
	bool handled = false;
	const Token* tokValue = tok->astOperand2();
	if (!tokValue)
	{
		return;
	}
	//str = NULL;
	//string str(NULL)
	if (!handled && !tok->astOperand1()->isExpandedMacro()
		&& ((tok->astOperand1()->variable()
		&& Token::Match(tok->astOperand1()->variable()->typeEndToken(), "std| ::| string|wstring"))
			|| Token::Match(tok->tokAt(-2), "wstring|string %any% (")
			)
		)
	{
		if (tok->astOperand2()->str() == "0")
		{
			reportError(tok, Severity::error, ErrorType::UserCustom, "nullToString",
				"Passing nullptr or 0 to std::string may lead to crash or undefined behavior.",
				ErrorLogger::GenWebIdentity(tok->previous()->str()));
		}
		else
		{
			SExprLocation rightLoc = CreateSExprByExprTok(tok->astOperand2());
			CExprValue rightVal;
			if (CanBeNull(rightLoc, rightVal) && (rightVal.IsCertainAndEqual() || rightVal.IsCondCheckNull()))
			{
				reportError(tok, Severity::error, ErrorType::UserCustom, "nullToString",
					"[" + rightLoc.Expr.ExprStr + "] is possible to be nullptr. Passing nullptr to std::string may lead to crash or undefined behavior." ,
					ErrorLogger::GenWebIdentity(tok->previous()->str()));
			}
		}
		handled = true;
	}

	if (tokValue->values.size() == 1)
	{
		const ValueFlow::Value& value = tokValue->values.front();
		if (value.isKnown() && value.tokvalue == nullptr)
		{
			CValueInfo newValue((int)value.intvalue, true);
			CExprValue ev(exprLoc, newValue);
			ev.SetCondInit(true);
			m_localVar.AddPreCond(ev, true);
			m_localVar.AddInitStatus(ev, true);
			handled = true;
		}
	}

	//handle operator new
	//T * p = new T;
	if (!handled)
	{
		if (tokValue->str() == "new")
		{
			CExprValue ev2(exprLoc, CValueInfo(0, false));
			ev2.SetCondInit(true);
			m_localVar.AddPreCond(ev2, true);
			m_localVar.AddInitStatus(ev2, true);
			handled = true;
		}
	}

	//handle char *p = "abc";
	if (!handled)
	{
		if (tokValue->tokType() == Token::eString)
		{
			CExprValue ev2(exprLoc, CValueInfo(0, false));
			ev2.SetCondInit(true);
			m_localVar.AddPreCond(ev2, true);
			m_localVar.AddInitStatus(ev2, true);
			handled = true;
		}
	}

	//handle iter = vec.begin();
	if (!handled)
	{
		if (IsIteratorBeginOrEnd(tokValue, true))
		{
			// maybe
			if (tok->astParent() && Token::Match(tok->astParent(), "[,;]"))
			{
				CExprValue ev2(exprLoc, CValueInfo::Unknown);
				ev2.SetCondInit();
				m_localVar.AddInitStatus(ev2, true);
			}
			else
			{
				CExprValue ev2(exprLoc, CValueInfo(0, false));
				ev2.SetCondInit();
				m_localVar.AddPreCond(ev2, true);
				m_localVar.AddInitStatus(ev2, true);
			}
			handled = true;
		}
		else if (IsIteratorBeginOrEnd(tokValue, false))
		{
			CExprValue ev2(exprLoc, CValueInfo(0, true));
			ev2.SetCondInit();
			ev2.SetCondIterator();
			m_localVar.AddPreCond(ev2, true);
			m_localVar.AddInitStatus(ev2, true);
			handled = true;
		}
	}

	if (!handled)
	{
		// if(!p) { p = func(); }
		// =>
		// p != 0, L
		CCompoundExpr* ifCond = m_stackLocalVars.top().GetIfCondPtr();
		if (ifCond)
		{
			CCompoundExpr* ce = nullptr;
			if (ifCond->HasSExpr(exprLoc.Expr, &ce))
			{
				if (ce->GetExprValue().IsCertainAndEqual())
				{
					CValueInfo vi(ce->GetExprValue().GetValueInfo().GetValue(), false);
					CExprValue ev(exprLoc, vi);
					ev.SetCondLogic();
					ev.SetInitVar();//标记为经验性推断，precond覆盖时判断

					m_localVar.AddPreCond(ev, true);
					m_localVar.AddInitStatus(ev, true);
					handled = true;
				}
			}
		}
	}

	if (!handled)
	{
		// if(!p) { if(other) { p = func(); } else { } }
		if (!curPre.Empty())
		{
			if (curPre.IsCertainAndEqual() && curPre.IsCondIf())
			{
				CValueInfo vi(curPre.GetValueInfo().GetValue(), false);
				CExprValue ev(exprLoc, vi);
				ev.SetCondLogic();

				m_localVar.AddPreCond(ev, true);
				m_localVar.AddInitStatus(ev, true);
				handled = true;
			}

		}
	}

	if (!handled)
	{
		if (tokValue->isDynamicCast() || tok->next()->isDynamicCast())
		{
			CExprValue ev2(exprLoc, CValueInfo(0, true, true));
			//ev2.SetCondInit();
			ev2.SetCondDynamic();
			m_localVar.AddPreCond(ev2, true);
			m_localVar.AddInitStatus(ev2, true);
			handled = true;
		}
	}

	if (!handled)
	{
		// p = &q;  ==> p != 0
		// if q == 0 then q != 0

		if (tokValue->str() == "&")
		{
			CExprValue ev2(exprLoc, CValueInfo(0, false));
			ev2.SetCondInit(true);
			m_localVar.AddPreCond(ev2, true);
			m_localVar.AddInitStatus(ev2, true);

			if (tokValue->astOperand1())
			{
				SExprLocation exprLoc2 = CreateSExprByExprTok(tokValue->astOperand1());
				if (!exprLoc2.Empty())
				{
					if (m_localVar.HasPreCond(exprLoc2.Expr))
					{
						const CExprValue& ev = m_localVar.PreCond[exprLoc2.Expr];
						if (ev.GetValueInfo().IsEqualZero())
						{
							m_localVar.ResetExpr(exprLoc2.Expr);
							m_localVar.EraseInitStatus(exprLoc2.Expr);
							CExprValue ev3(exprLoc2, CValueInfo(0, false));
							ev3.SetCondLogic();
							m_localVar.AddPreCond(ev3, true);
						}
					}
				}
			}
			
			handled = true;
		}
	}

	if (!handled)
	{
		// nomal type cast is OK, such as p = (int*)q;
		// dynamic_cast may have problem
		//if (!tokValue->isCast())
		{
			SExprLocation exprLoc2 = CreateSExprByExprTok(tokValue);

			if (!exprLoc2.Empty())
			{
				if (m_localVar.HasPreCond(exprLoc2.Expr))
				{
					CExprValue& ev = m_localVar.PreCond[exprLoc2.Expr];
					CExprValue ev2(exprLoc, ev.GetValueInfo());
					ev2.SetCondInit();
					//ev2.SyncCondErrorType(ev);
					m_localVar.AddPreCond(ev2, true);
					m_localVar.AddInitStatus(ev2, true);
					handled = true;
				}
				// if it's a function call
				else if (exprLoc2.TokTop->str() == "(")
				{
					const Token* tokFunc = exprLoc2.TokTop->previous();
					const gt::CFunction *gtFunc = CGlobalTokenizer::Instance()->FindFunctionData(tokFunc);
					if (HandleRetParamRelation(exprLoc, exprLoc2, gtFunc))
					{
						handled = true;
					}
					//make function impossible to return null if configured in cfg.xml
					else if (CGlobalTokenizer::Instance()->CheckIfMatchSignature(tokFunc, 
						_settings->_functionNotRetNull, _settings->_equalTypeForFuncSig) ||
						CGlobalTokenizer::Instance()->CheckFuncReturnNull(tokFunc) == gt::CFuncData::RetNullFlag::fNotNull)
					{
						CExprValue ev2(exprLoc, CValueInfo(0, false));
						ev2.SetCondInit();
						m_localVar.AddPreCond(ev2, true);
						m_localVar.AddInitStatus(ev2, true);
						handled = true;
					}
					else
					{
						//find if function shall return null from cfg file first
						if (CheckIfLibFunctionReturnNull(tokFunc)
							|| CGlobalTokenizer::Instance()->CheckFuncReturnNull(tokFunc) == gt::CFuncData::RetNullFlag::fNull)
						{
							// special handling
							// if in for-scope and the function arg is the loop index, consider the func not returning null;
							const Token* tokFuncTop = exprLoc2.TokTop;

							if (!CheckIsLoopIndexCall(tokFuncTop))
							{
								CExprValue ev2(exprLoc, CValueInfo(0, true, true));
								ev2.SetCondFuncRet();
								//ev2.SetCondInit();
								m_localVar.AddPreCond(ev2, true);
								m_localVar.AddInitStatus(ev2, true);
								handled = true;
							}
						}
					}
				}
			}
		}
	}

	if (!handled)
	{
		if (Token::Match(tokValue, "[+-/]")|| (tokValue->str()=="*" && tokValue->astOperand1() && tokValue->astOperand2()))
		{
			if (const Variable* pVar = exprLoc.GetVariable())
			{
				if (pVar->isPointer())
				{
					CExprValue newEv(exprLoc, CValueInfo(0, false));
					m_localVar.AddPreCond(newEv);
					handled = true;
				}
			}
		}
	}

	if (!handled)
	{
		// consider tokValue as a enum value which hasn't been expanded.
		if (tokValue == tok->next() && tokValue->varId() == 0 && (!tokValue->astOperand1() && !tokValue->astOperand2()))
		{
			CValueInfo newValue(tokValue->str(), true);
			CExprValue ev(exprLoc, newValue);
			ev.SetCondInit();
			m_localVar.AddPreCond(ev, true);
			m_localVar.AddInitStatus(ev, true);
			handled = true;
		}
	}

	if (!handled)
	{
		// p = NULL; if(XXX) { p = xxx(); }
		if (m_localVar.HasInitStatus(exprLoc.Expr) && !m_localVar.HasTempInit(exprLoc.Expr))
		{
			const CExprValue& ev = m_localVar.InitStatus[exprLoc.Expr];
			if (ev.IsCertainAndEqual())
			{
				CValueInfo vi;
				if (ev.GetValueInfo().IsStrValue())
				{
					vi = CValueInfo(ev.GetValueInfo().GetStrValue(), false);
				}
				else
				{
					vi = CValueInfo(ev.GetValueInfo().GetValue(), false);
				}
				CExprValue ev2(exprLoc, vi);
				ev2.SetCondLogic();
				ev2.SetInitVar();//p  = NULL;if(xx){ p=xx; if(p){}}
				m_localVar.AddPreCond(ev2, true);
				m_localVar.AddInitStatus(ev2, true);
				handled = true;
			}
		}
	}

	// 
	if (!handled)
	{
		MEV mev;
		CCompoundExpr* pCE = new CCompoundExpr;
		ExtractCondFromToken(tokValue, pCE);
		if (pCE && !pCE->IsUnknownOrEmpty())
		{
			m_localVar.AddInitStatus(CExprValue(exprLoc, CValueInfo::Unknown), true);

			// if the expression isn't just a function, then construct equal conds;
			// for "pDanceroom = dyanmic<DanceRoom*>(pRoom)", no extraction
			if (tokValue->str() != "(" && !tok->next()->isCast())
			{
				CExprValue ev(exprLoc, CValueInfo(0, false));

				// b = b && p != 0;
				CCompoundExpr* ceLeft = nullptr;
				if (pCE->HasSExpr(exprLoc.Expr, &ceLeft))
				{
					bool bAnd, bOr;
					ceLeft->GetCondjStatus(bAnd, bOr);
					if (ceLeft->GetExprValue() == ev && bAnd && !bOr)
					{
						CCompoundExpr::DeleteSelf(ceLeft, pCE);
					}
				}

				std::shared_ptr<CCompoundExpr> compExpr(pCE);

				CExprValue ev2 = ev;
				ev2.NegationValue();
				std::shared_ptr<CCompoundExpr> compExpr2(compExpr->DeepCopy());  //use deepcopy to avoid wild pointer
				compExpr2->Negation();
				m_localVar.AddEqualCond(ev, compExpr);
				m_localVar.AddEqualCond(ev2, compExpr2);
				handled = true;
			}
		}
	}

	if (!handled)
	{
		CExprValue ev(exprLoc, CValueInfo::Unknown);
		m_localVar.AddInitStatus(ev, true);
		handled = true;
	}

	//only "=" need to do this
	if (tok->str() == "=")
	{
		// if s is assigned, then s->a should be erased.
		if (handled)
		{
			m_localVar.EraseSubFiled(exprLoc.Expr);
		}

		//ResetReportedError(exprLoc.Expr);
	}
}

const Token* CheckTSCNullPointer2::HandleAssign(const Token * tok)
{
	const Token* tokExpr = tok->astOperand1();
	SExprLocation exprLoc = CreateSExprByExprTok(tokExpr);
	if (exprLoc.Empty())
	{
		return tok;
	}

	const Token* tokEnd = nullptr;
	if (tok->str() == "=")
	{
		tokEnd = tok->next();
		while (tokEnd)
		{
			if (Token::Match(tokEnd, ";|{|}|=|,"))
			{
				break;
			}
			else if (tokEnd->link() )
			{
				if (Token::Match(tokEnd, "[|(|<"))
				{
					tokEnd = tokEnd->link()->next();
					continue;
				}
				else
					break;

			}
			tokEnd = tokEnd->next();
		}
		//tokEnd = Token::findmatch(tok->next(), ";|{|}|=");
	}
	// A *p (new a); => p = new a;
	else if (tok->str() == "(")
	{
		if (tok->link())
		{
			tokEnd = tok->link()->next();
		}
		//else do nothing
	}
	
	if (tokEnd)
	{
		if (tokEnd->str() != ";")
		{
			tokEnd = tokEnd->previous();
		}
		if (tokEnd != tok)
		{
			for (const Token* tok2 = tok->next(); tok2 && tok2 != tokEnd; tok2 = tok2->next())
			{
				HandleOneToken(tok2);//ignore TSC
			}
		}
		else
		{
			tokEnd = nullptr;
		}
	}

	CExprValue curPre;
	if (tok->str() == "=")
	{
		if (m_localVar.HasPreCond(exprLoc.Expr))
		{
			curPre = m_localVar.PreCond[exprLoc.Expr];
		}
		m_localVar.ResetExpr(exprLoc.Expr);
	}

	HandleAssign(tok, exprLoc, curPre);
	
	return (tokEnd != nullptr) ? tokEnd->previous() : tok;
}


const Token* CheckTSCNullPointer2::HandleAssignOperator(const Token* tok)
{
	if (!tok || tok->str() != "=")
	{
		return tok;
	}

	if (!tok->astOperand1() || !tok->astOperand2())
	{
		return tok;
	}


	return HandleAssign(tok);
}

bool CheckTSCNullPointer2::CheckIsLoopIndexCall(const Token* tokFuncTop)
{
	std::vector<const Token*> args;
	getParamsbyAst(tokFuncTop, args);
	
	for (std::vector<const Token*>::iterator I = args.begin(), E = args.end(); I != E;++I)
	{
		const Token* tokArg = *I;
		if (tokArg->isNumber())
		{
			return true;
		}
		else
		{
			SExprLocation exprLoc = CreateSExprByExprTok(tokArg);
			if (!exprLoc.Empty())
			{
				if (const Scope* scope = tokFuncTop->scope())
				{
					while (scope->type > Scope::eFunction && scope->type != Scope::eFor)
					{
						scope = scope->nestedIn;
					}
					if (scope && scope->type == Scope::eFor)
					{
						std::vector<SExprLocation> loopList;
						GetForLoopIndex(scope->classDef, loopList);
						if (!loopList.empty())
						{
							for (std::vector<SExprLocation>::iterator I2 = loopList.begin(), E2 = loopList.end(); I2 != E2; ++I2)
							{
								if (I2->Expr == exprLoc.Expr)
								{
									return true;
								}
							}
						}
					}
				}
			}
		}
	}

	return false;
}

void CheckTSCNullPointer2::HandleFuncParamAssign(const Token* tok, const Token *tokFunc)
{
	if (!tok)
		return;

	//exclude r = foo(&p); if (r) xxx;
	//const Token *tokTop = tok->astTop();
	//std::vector<std::pair<unsigned char, unsigned char> > rp;
	//if (/*tokTop && tokTop->str() == "=" && */CGlobalTokenizer::Instance()->CheckRetParamRelation(tokFunc))
	//{
	//	return;//directly return for it is handled in handle assign
	//}

	SLocalVar& topLocalVar = m_stackLocalVars.top();
	bool handled = false;
	if (tok->str() == "&")
	{
		const Token* tokExpr = tok->astOperand1();
		SExprLocation exprLoc = CreateSExprByExprTok(tokExpr);
		if (exprLoc.Empty())
		{
			return;
		}

		const SExpr expr = exprLoc.Expr;

		bool bNotZero = false;
		bool bZero = false;
		if (m_localVar.HasPreCond(expr))
		{
			const CExprValue& ev = m_localVar.PreCond[expr];
			if (ev.GetValueInfo().IsNotEqualZero())
			{
				bNotZero = true;
			}
			else if (ev.GetValueInfo().IsEqualZero())
			{
				bZero = true;
			}
		}

		m_localVar.ResetExpr(expr);

		if (bNotZero)
		{
			CExprValue ev(exprLoc, CValueInfo(0, false));
			ev.SetCondInit();
			m_localVar.AddPreCond(ev, true);
			m_localVar.AddInitStatus(ev, true);
			handled = true;
		}

		if (!handled)
		{
			// if(!p) { func(&p); }
			// =>
			// p != 0, L
			CCompoundExpr* ifCond = topLocalVar.GetIfCondPtr();
			if (ifCond)
			{
				CCompoundExpr* ce = nullptr;
				if (ifCond->HasSExpr(expr, &ce))
				{
					if (ce->GetExprValue().IsCertainAndEqual())
					{
						CValueInfo vi(ce->GetExprValue().GetValueInfo().GetValue(), false);
						CExprValue ev(exprLoc, vi);
						ev.SetCondLogic();
						m_localVar.AddPreCond(ev, true);
						m_localVar.AddInitStatus(ev, true);
						handled = true;
					}
				}
			}
		}

		if (!handled)
		{
			// p = NULL; if(XXX) { xxx(&p); }
			// p = NULL; func(&p);
			if (m_localVar.HasInitStatus(expr) /*&& !m_localVar.HasTempInit(expr)*/)
			{
				const CExprValue& ev = m_localVar.InitStatus[expr];
				if (ev.IsCertainAndEqual())
				{
					CValueInfo vi(ev.GetValueInfo().GetValue(), false);
					CExprValue ev2(exprLoc, vi);
					ev2.SetCondLogic();
					m_localVar.AddPreCond(ev2, true);
					m_localVar.AddInitStatus(ev2, true);
					handled = true;
				}
			}
		}

		if (!handled)
		{
			if (bZero)
			{
				CExprValue ev(exprLoc, CValueInfo(0, false));
				ev.SetCondInit();
				m_localVar.AddPreCond(ev, true);
				m_localVar.AddInitStatus(ev, true);
				handled = true;
			}
		}

		if (!handled)
		{
			CExprValue ev(exprLoc, CValueInfo::Unknown);
			ev.SetCondInit();
			m_localVar.AddInitStatus(ev, true);
			handled = true;
		}

		m_localVar.EraseSubFiled(expr);
		//ResetReportedError(expr);
	}
	else
	{
		const Token* tokExpr = tok;
		SExprLocation exprLoc = CreateSExprByExprTok(tokExpr);
		if (exprLoc.Empty())
		{
			return;
		}

		const SExpr& expr = exprLoc.Expr;

		if (!handled)
		{
			// if(!p) { func(p); }
			// =>
			// p != 0, L
			CCompoundExpr* ifCond = topLocalVar.GetIfCondPtr();
			if (ifCond)
			{
				CCompoundExpr* ce = nullptr;
				if (ifCond->HasSExpr(exprLoc.Expr, &ce))
				{
					if (ce->GetExprValue().IsCertainAndEqual())
					{
						m_localVar.ResetExpr(exprLoc.Expr);

						CValueInfo vi(ce->GetExprValue().GetValueInfo().GetValue(), false);
						CExprValue ev(exprLoc, vi);
						ev.SetCondLogic();
						m_localVar.AddPreCond(ev, true);
						m_localVar.AddInitStatus(ev, true);				

						m_localVar.EraseSubFiled(expr);
						//ResetReportedError(expr);

						handled = true;
					}
				}
				else
				{
					// if(!p->pp) { func(p); }
					// =>
					// p != 0, L
					// p->pp !=0, L
					std::vector<CCompoundExpr*> listCE;
					ifCond->ConvertToList(listCE);
					for (std::vector<CCompoundExpr*>::iterator I = listCE.begin(), E = listCE.end(); I != E; ++I)
					{
						CCompoundExpr* ce2 = *I;
						if (CheckExprIsSubField(exprLoc.Expr, ce2->GetExprValue().GetExpr()))
						{
							if (ce2->GetExprValue().GetValueInfo().IsEqualZero())
							{
								CValueInfo vi(0, false);
								CExprValue ev(ce2->GetExprValue().GetExprLoc(), vi);
								ev.SetCondLogic();
								m_localVar.AddPreCond(ev, true);
								m_localVar.AddInitStatus(ev, true);
								break;
							}
						}
					}
				}
			}
		}

		if (!handled)
		{
			if (m_localVar.HasInitStatus(exprLoc.Expr))
			{
				CExprValue& ev = m_localVar.InitStatus[exprLoc.Expr];
				if (ev.IsCertainAndEqual())
				{
					if (m_localVar.HasPreCond(exprLoc.Expr))
					{
						CExprValue& ev2 = m_localVar.PreCond[exprLoc.Expr];
						if (ev == ev2)
						{
							// consider this expr is assigned by this func
							m_localVar.ResetExpr(exprLoc.Expr);

							CExprValue evNew(exprLoc, CValueInfo(0, false));
							/*evNew.SetCondLogic();*/
							m_localVar.AddInitStatus(evNew, true);
							m_localVar.AddPreCond(evNew, true);
							
							m_localVar.EraseSubFiled(expr);
							//ResetReportedError(expr);

							handled = true;
						}
					}
				}
			}
		}

		//foo(p), make p->xxx not null
		if (!handled)
		{
			handled = MakeSubFieldNotNull(tok);
		}
	}
}



void CheckTSCNullPointer2::ResetReportedError(const SExpr& expr)
{
	std::set<SExpr>::iterator iter = m_reportedErrorSet.find(expr);
	if (iter != m_reportedErrorSet.end())
	{
		m_reportedErrorSet.erase(iter);
	}
	/*
	iter = m_possibleReportedErrorSet.find(expr);
	if (iter != m_possibleReportedErrorSet.end())
	{
		m_possibleReportedErrorSet.erase(iter);
	}
	*/
}

bool CheckTSCNullPointer2::CanBePointer(const SExprLocation &expr) const
{
	if (Token::Match(expr.TokTop, "(|["))
	{
		return true;
	}
	if (expr.Expr.VarId == 0)
	{
		return true;
	}
	const Variable *pVar = _tokenizer->getSymbolDatabase()->getVariableFromVarId(expr.Expr.VarId);
	if (!pVar)
	{
		return true;
	}
	if (pVar->isPointer())
	{
		return true;
	}
	bool bHasUnknown = false;
	for (const Token *tokLoop = pVar->typeStartToken(); tokLoop && tokLoop != pVar->typeEndToken(); tokLoop = tokLoop->next())
	{
		if (tokLoop->str() == "::")
		{
			bHasUnknown = false;
		}
		else if (0 == Token::stdTypes.count(tokLoop->str()) && 0 == _settings->_nonPtrType.count(tokLoop->str()))
		{
			bHasUnknown = true;
		}
	}
	if (bHasUnknown)
	{
		return true;
	}
	if (pVar->typeEndToken() && 0 == Token::stdTypes.count(pVar->typeEndToken()->str()) 
		&& 0 == _settings->_nonPtrType.count(pVar->typeEndToken()->str()))
	{
		return true;
	}
	return false;
}

bool CheckTSCNullPointer2::CheckIfExprLocIsArray(const SExprLocation& exprLoc) const
{
	if (!exprLoc.TokTop || Token::Match(exprLoc.TokTop, "(|["))
	{
		return false;
	}
	if (exprLoc.Expr.VarId == 0)
	{
		return false;
	}
	const Variable *pVar = _tokenizer->getSymbolDatabase()->getVariableFromVarId(exprLoc.Expr.VarId);
	if (!pVar)
	{
		return false;
	}
	//array parameter
	return pVar->isArray() && (!pVar->isArgument());
}

void CheckTSCNullPointer2::ReportFuncRetNullErrors()
{
	for (std::map<SExpr, SErrorBak>::iterator I = m_funcRetNullErrors.begin(), E = m_funcRetNullErrors.end(); I != E; ++I)
	{
		if (nullPointerErrorFilter(I->second.ExprLoc, I->second.EV))
		{
			nullPointerError(I->second.ExprLoc, I->second.EV);
		}
	}
}

const Token* CheckTSCNullPointer2::HandleFuncParam(const Token* tok)
{
	if (!tok || tok->str() != "(")
	{
		return tok;
	}

	if (!tok->astOperand1())
	{
		return tok;
	}

	// Func([=](flaot delta) mutable { xxx; }
	if (Token::Match(tok, "( [ = ]"))
	{
		if (tok->link())
			return tok->link();
	}

	const Token* tok1 = tok->astOperand1();
	if (!tok1->isName() && tok1->str() != "." && tok1->str() != "::" && tok1->str() != "*")
	{
		return tok;
	}

	
	if (Token::Match(tok1, "if|do|while|for|switch"))
	{
		return tok;
	}

	// (*pfunc)(...)
	if (tok1->str() == "*" && tok1->astOperand1())
	{
		std::vector<const Token*> args;
		getParamsbyAst(tok, args);
		int index = 0;
		for (std::vector<const Token*>::iterator I = args.begin(), E = args.end(); I != E; ++I, ++index)
		{
			const Token* tokArg = *I;
			HandleFuncParamAssign(tokArg, nullptr);
		}
		return tok;
	}

	const Token* tokFunc = tok->previous();
	if (tokFunc)
	{
		if (tokFunc->str() == ">")
		{
			if (tokFunc->link())
			{
				tokFunc = tokFunc->link()->previous();
			}
			else
			{
				return tok;
			}
		}
		if (tokFunc->astUnderSizeof())
		{
			return tok;
		}

		
		if (tokFunc->tokType() != Token::eVariable)
		{
			if (tokFunc->isName())
			{
				bool bDefFound = false;
				const gt::CFunction *gtFunc = CGlobalTokenizer::Instance()->FindFunctionData(tokFunc);
				if (gtFunc)
				{
					const std::set<gt::SOutScopeVarState> &osv = gtFunc->GetFuncData().GetOutScopeVarState();
					for (std::set<gt::SOutScopeVarState>::const_iterator iter = osv.begin(), end = osv.end(); iter != end; ++iter)
					{
						const Variable *pVar = nullptr;
						std::map<std::string, const Variable *>::const_iterator mappedVar = m_nameVarMap.find(iter->strName);
						if (mappedVar != m_nameVarMap.end())
						{
							pVar = mappedVar->second;
						}
						if (!pVar)
						{
							continue;
						}
						const Token *nameTok = pVar->nameToken();
						SExprLocation loc(nameTok, nameTok->next(), nameTok, nameTok->varId());
						CExprValue ev(loc, CValueInfo(iter->bNull, false));
						ev.SetCondInit();
						m_localVar.AddPreCond(ev, true);
						m_localVar.AddInitStatus(ev, true);
					}

					//no parameter
					if (!tok->astOperand2())
					{
						return tok;
					}

					//check return param relation
					if (HandleRetParamRelation(tokFunc, tok, gtFunc))
					{
						return tok;
					}
					bDefFound = true;
				}

				std::set<gt::SVarEntry> derefedVar;
				std::set<int> derefArgIndex;
				//normal function call
				//check if function in library first
				//otherwise check global function prototypes
				if (GetLibNotNullParamIndexByName(derefArgIndex, tokFunc))
				{
					//intentionally left blank
				}
				else
				{
					bool bRet = CGlobalTokenizer::Instance()->CheckDerefVars(tokFunc, derefedVar);
					if (bRet)
					{
						for (std::set<gt::SVarEntry>::iterator I = derefedVar.begin(), E = derefedVar.end(); I != E; ++I)
						{
							const gt::SVarEntry& entry = *I;
							if (entry.eType == Argument)
							{
								derefArgIndex.insert(entry.iParamIndex);
							}
						}
					}
				}

				std::vector<const Token*> args;
				getParamsbyAst(tok, args);
				int index = 0;
				for (std::vector<const Token*>::iterator I = args.begin(), E = args.end(); I != E; ++I, ++index)
				{
					const Token* tokArg = *I;
					if (derefArgIndex.count(index))
					{
						
							CExprValue ev;
							SExprLocation exprLoc = CreateSExprByExprTok(tokArg);
							if (CanBePointer(exprLoc))
							{
								HandleDeref(exprLoc);
							}

						if (tokArg->str() == "&")
						{
							HandleFuncParamAssign(tokArg, tokFunc);
						}
						else
						{
							MakeSubFieldNotNull(tokArg);//ignore TSC
						}
					}
					else
					{
						HandleFuncParamAssign(tokArg, tokFunc);
					}
				}

				// a function call could change the other variable's status, so we need to check this.
				//if (!XX::Get())
				//{
				//	XX::Create();
				//	XX::Get()->Init();
				//}
				CheckFuncCallToUpdateVarStatus(tokFunc, tok);

			}
		}
		// variable initialization
		// or constructor, ifstream ifs(szName);
		else
		{
			if (tokFunc->variable())
			{
				//todo: consider more situations
				//variable declaration must conform pattern 
				//Type var(p0, p1, ...);
				if ((tokFunc->variable()->isPointer() 
					&& !Token::Match(tokFunc->previous(), ";|=|,|(|?|{|.|&&|%oror%|!"))
					|| (Token::Match(tokFunc->previous(), "string|wstring") && (tokFunc->strAt(-2) != "::" || tokFunc->strAt(-3) == "std"))
					)
				{
					return HandleAssign(tok);
				}
				// consider as function pointer
				// pfunc(a,b,c)
				else if (tokFunc->previous() && Token::Match(tokFunc->previous(), ";|=|,|(|?|{|}|.|&&|%oror%|!"))
				{
					std::vector<const Token*> args;
					getParamsbyAst(tok, args);
					int index = 0;
					for (std::vector<const Token*>::iterator I = args.begin(), E = args.end(); I != E; ++I, ++index)
					{
						const Token* tokArg = *I;
						HandleFuncParamAssign(tokArg, nullptr);
					}
					return tok;
				}
				//Ptr<XXX> pt(new XXX);
				else if (!tokFunc->variable()->isPointer() && tok->next() && tok->next()->str() == "new")
				{
					SExprLocation exprLoc = CreateSExprByExprTok(tokFunc);
					CExprValue newEv(exprLoc, CValueInfo(0, false));
					newEv.SetCondInit();
					m_localVar.AddPreCond(newEv, true);
					m_localVar.AddInitStatus(newEv, true);
				}
				// CBuffAuto< SSMSGDISP > oBuffAuto(&pSSMsgDisp);
				// CBuffAuto< SSMSGDISP > oBuffAuto(pSSMsgDisp);
				else if (const Token* tok2 = tok->astOperand2())
				{
					if (tok2->str() == "&")
					{
						tok2 = tok2->astOperand1();
					}
					if (tok2)
					{
						SExprLocation exprLoc = CreateSExprByExprTok(tok2);
						if (m_localVar.HasPreCond(exprLoc.Expr))
						{
							const CExprValue ev = m_localVar.PreCond[exprLoc.Expr];
							if (ev.GetValueInfo().IsEqualZero())
							{
								m_localVar.ResetExpr(exprLoc.Expr);
								CExprValue newEv(exprLoc, CValueInfo(0, false));
								newEv.SetCondInit();
								m_localVar.AddPreCond(newEv, true);
								m_localVar.AddInitStatus(newEv, true);
							}
						}
					}
				}
			}
			else
			{
				//todo: and unknown
			}
		}
	}

	return tok;
}

bool CheckTSCNullPointer2::HandleRetParamRelation(const Token *tokFunc, const Token *tokBracket, const gt::CFunction *gtFunc)
{
	const std::set<gt::CFuncData::ParamRetRelation> *pRetParam = CGlobalTokenizer::Instance()->CheckRetParamRelation(gtFunc);
	if (!pRetParam || pRetParam[0].empty())
	{
		return false;
	}

	//cannot be right operator of "="
	const Token *tokTop = tokFunc->astTop();
	if (tokTop && tokTop->str() == "=")
	{
		return false;
	}

	//at least one token
	SExprLocation loc2 = CreateSExprByExprTok(tokBracket);
	if (loc2.Empty())
	{
		return false;
	}
	std::vector<const Token*> args;
	getParamsbyAst(loc2.TokTop, args);
	if (args.empty())
	{
		return true;
	}
	std::map<gt::CFuncData::ParamRetRelation, std::vector<SExprLocation> > mapRetMultiParam;
	bool bSetNeverNull = false;
	//if (GetData(&p)) p->xxx
	for (std::set<gt::CFuncData::ParamRetRelation>::const_iterator iter = pRetParam[0].begin(), end = pRetParam[0].end(); iter != end; ++iter)
	{
		if ((size_t)iter->nIndex < args.size())
		{
			const Token *tokArg = args[iter->nIndex];
			if (!tokArg)
			{
				continue;
			}
			bool bCanAssigned = false;
			if (tokArg->str() == "&")
			{
				bCanAssigned = true;
				if (!tokArg->astOperand1())
				{
					continue;
				}
				tokArg = tokArg->astOperand1();
			}
			const SExprLocation paramLoc = CreateSExprByExprTok(tokArg);
			if (paramLoc.Empty())
			{
				continue;
			}
			
			if (!bCanAssigned)
			{
				const std::vector<gt::CVariable>& sigArgs =  gtFunc->GetArgs();
				if ((size_t)iter->nIndex < sigArgs.size())
				{
					const std::string typeStr = sigArgs[iter->nIndex].GetTypeStr();
					if (typeStr.find('*') != std::string::npos && typeStr.find('&') != std::string::npos)
					{
						bCanAssigned = true;
					}
				}
			}
			CExprValue ev;
			if (bCanAssigned &&
				(iter->bNerverNull
				|| (CanBeNull(paramLoc, ev) && ev.IsCondInit() && ev.IsCertainAndEqual())))
			{
				CExprValue evNotNull(paramLoc, CValueInfo(0, false));
				evNotNull.SetCondInit();
				m_localVar.AddPreCond(evNotNull, true);
				m_localVar.AddInitStatus(evNotNull, true);
				bSetNeverNull = true;
			}
			else
			{
				gt::CFuncData::ParamRetRelation prr = *iter;
				prr.nIndex = -1;
				mapRetMultiParam[prr].push_back(paramLoc);
			}
		}
	}
	if (bSetNeverNull)
	{
		return true;
	}
	for (std::map<gt::CFuncData::ParamRetRelation, std::vector<SExprLocation> >::iterator iter = mapRetMultiParam.begin(), end = mapRetMultiParam.end();
	iter != end; ++iter)
	{
		CExprValue evLeft;
		if (iter->first.bStr)
		{
			evLeft = CExprValue(loc2, CValueInfo(iter->first.str, iter->first.bEqual));
		}
		else
		{
			evLeft = CExprValue(loc2, CValueInfo(iter->first.nValue, iter->first.bEqual));
		}
		if (iter->second.empty())
		{
			continue;
		}

		CExprValue evRight(iter->second[0], CValueInfo(0, iter->first.bNull));
		CCompoundExpr *ce = new CCompoundExpr;
		ce->SetExprValue(evRight);

		for (int nIndex = 1, nSize = iter->second.size(); nIndex < nSize; ++nIndex)
		{
			CCompoundExpr * conj(CCompoundExpr::CreateConjNode(CCompoundExpr::And));
			conj->SetLeft(ce);
			
			CExprValue evRight2(iter->second[nIndex], CValueInfo(0, iter->first.bNull));
			CCompoundExpr *ceRight = conj->CreateRight();
			ceRight->SetExprValue(evRight2);
			conj->SetRight(ceRight);
			ce = conj;
		}
		std::shared_ptr<CCompoundExpr> sce(ce);
		m_localVar.AddEqualCond(evLeft, sce);
	}
	return true;
}

bool CheckTSCNullPointer2::ExtractCondFromToken(const Token* tok, CCompoundExpr* compExpr, bool bHandleComp)
{
	if (tok == nullptr)
		return false;

	if (!compExpr)
	{
		return false;
	}

	bool bRet = false;
	if (tok->str() == "&&" || tok->str() == "||")
	{
		if (tok->str() == "&&")
			compExpr->SetConj(CCompoundExpr::And);
		else
			compExpr->SetConj(CCompoundExpr::Or);
		
		bool bRet1 = ExtractCondFromToken(tok->astOperand1(), compExpr->CreateLeft());
		bool bRet2 = ExtractCondFromToken(tok->astOperand2(), compExpr->CreateRight());
		if (!bRet1 && !bRet2)
		{
			compExpr->SetUnknown();
			bRet = false;
		}
		else
			bRet = true;
	}
	else
	{
		if (tok->str() == "==" || tok->str() == "!=")
		{
			const Token* tokExpr = nullptr;
			bool bIterator = false;
			CValueInfo info;
			if (HandleEqualExpr(tok, tokExpr, info, bIterator))
			{
				bRet = AddEVToCompoundExpr(tokExpr, info, compExpr, bIterator);
			}
		}
		else if (bHandleComp && Token::Match(tok, "<|<=|>|>="))
		{
			std::string sComp = tok->str();
			const Token* tok1 = tok->astOperand1();
			const Token* tok2 = tok->astOperand2();
			if (tok1 && tok2)
			{
				const ValueFlow::Value* value = nullptr;
				const Token* tokValue = nullptr;
				const Token* tokExpr = nullptr;

				if (tok1->values.size() == 1)
				{
					value = &tok1->values.front();
					//make sure value is certain and not string,value->tokvalue==true means it's string
					if (value->isKnown() && !value->tokvalue)
					{
						tokExpr = tok2;
						tokValue = tok1;

						if (sComp == "<")
						{
							sComp = ">";
						}
						else if (sComp == "<=")
						{
							sComp = ">=";
						}
						else if (sComp == ">")
						{
							sComp = "<";
						}
						else if (sComp == ">=")
						{
							sComp = "<=";
						}	
					}
				}

				if (!tokExpr)
				{
					if (tok2->values.size() == 1)
					{
						value = &tok2->values.front();
						if (value->isKnown() && !value->tokvalue)
						{
							tokExpr = tok1;
							tokValue = tok2;
						}
					}
				}

				if (tokExpr)
				{
					int iValue = (int)value->intvalue;
					if (sComp == ">")
					{ 
						++iValue;
					}
					else if (sComp == "<")
					{
						--iValue;
					}
					CValueInfo info(iValue, true);
					SExprLocation exprLoc = CreateSExprByExprTok(tokExpr);
					if (exprLoc.Empty())
					{
						return false;
					}
					CExprValue ev(exprLoc, info);
					compExpr->SetExprValue(ev);
					bRet = true;
				}
			}
		}
		else if (tok->str() == "!")//!p
		{
			if (const Token* tokExpr = tok->astOperand1())
			{
				ExtractCondFromToken(tokExpr, compExpr);
				if (!compExpr->IsUnknownOrEmpty())
				{
					compExpr->Negation();
					bRet = true;
				}
				else
					bRet = false;
			}
		}
		else if (tok->str() == ",")
		{
			if (const Token* tokStart = tok->astOperand1())
			{
				while (tokStart->astOperand1())
				{
					tokStart = tokStart->astOperand1();
				}
				
				for (const Token* tok2 = tokStart; tok2 && tok2 != tok; tok2 = tok2->next())
				{
					if (tok2->link() && Token::Match(tok2, "{|}"))
					{
						break;
					}
					tok2 = HandleOneToken(tok2);
				}

				if (const Token* tok3 = tok->astOperand2())
				{
					ExtractCondFromToken(tok3, compExpr);
					bRet = !compExpr->IsUnknownOrEmpty();
				}
			}
		}
		else//p or other special expression
		{
			CValueInfo info(0, false);
			bRet = AddEVToCompoundExpr(tok, info, compExpr);
		}

		if (!bRet)
		{
			compExpr->SetUnknown();
		}
	}
	
	return bRet;
}

bool CheckTSCNullPointer2::SimplifyIfCond(CCompoundExpr* cond, CCompoundExpr*& top)
{
	if (!cond)
	{
		return false;
	}
	// if (!p && !(p = getp()))
	// 	[IF]	[p] == 0 && [p] == 0 ## [getp()] == 0
	if (cond->GetConj() > CCompoundExpr::Unknown)
	{

		if (SimplifyIfCond(cond->GetLeft(), top))
			return true;
		if (SimplifyIfCond(cond->GetRight(), top))
			return true;

		if (cond->GetLeft()->GetConj() == CCompoundExpr::None && cond->GetRight()->GetConj() == CCompoundExpr::None)
		{
			if (cond->GetLeft()->GetExprValue() == cond->GetRight()->GetExprValue())
			{
				if (cond == top)
				{
					CExprValue evbak = cond->GetLeft()->GetExprValue();
					cond->SetEmpty();
					cond->SetExprValue(evbak);
				}
				else
					CCompoundExpr::DeleteSelf(cond->GetRight(), top);

				return true;
			}
			else if (CExprValue::IsExprValueOpposite(cond->GetLeft()->GetExprValue(), cond->GetRight()->GetExprValue()))
			{
				if (cond->GetConj() == CCompoundExpr::And || cond->GetConj() == CCompoundExpr::Equal)
				{
					top->SetEmpty();
				}
				else if (cond->GetConj() == CCompoundExpr::Or)
				{
					if (cond == top)
					{
						top->SetEmpty();
					}
					else
					{
						CCompoundExpr* comp1 = cond->GetLeft();
						CCompoundExpr* comp2 = cond->GetRight();

						CCompoundExpr::DeleteSelf(comp1, top);
						CCompoundExpr::DeleteSelf(comp2, top);
					}
					
				}
				return true;

			}
		}
	}

	return false;
}

bool CheckTSCNullPointer2::SimplifyCondWithUpperCond(CCompoundExpr*& cond)
{
	if (cond->IsUnknownOrEmpty())
	{
		return false;
	}

	CCompoundExpr* upper = m_localVar.GetIfCondExpandedPtr();
	if (!upper)
	{
		return false;
	}

	std::vector<CCompoundExpr*> listCE1;
	std::vector<CCompoundExpr*> listCEUpper;
	cond->ConvertToList(listCE1, false);
	upper->ConvertToList(listCEUpper, false);

	if (listCEUpper.size() == 1)
	{
		//if (p2 != 0)
		//{
		//	if (NULL == p1 && p2 != NULL)
		CCompoundExpr* ce2 = listCEUpper.front();
		CExprValue& ev2 = ce2->GetExprValue();
		for (std::vector<CCompoundExpr*>::iterator I = listCE1.begin(), E = listCE1.end(); I != E; ++I)
		{
			CCompoundExpr* ce1 = *I;
			bool bAnd, bOr;
			ce1->GetCondjStatus(bAnd, bOr);
			if (bAnd && !bOr)
			{
				if (ce1->GetExprValue() == ev2)
				{
					CCompoundExpr::DeleteSelf(ce1, cond);
					return true;
				}
			}
		}
	}
	else if (listCEUpper.size() == 2)
	{
		//if (p1 != 0 || p2 != 0)
		//{
		//	if (NULL == p1 && p2 != NULL

		CCompoundExpr* ceUp1 = listCEUpper[0];
		CCompoundExpr* ceUp2 = listCEUpper[1];

		bool bAnd, bOr;
		ceUp1->GetCondjStatus(bAnd, bOr);
		if (bOr && !bAnd)
		{
			if (listCE1.size() == 2)
			{
				CCompoundExpr* ce1 = listCE1[0];
				CCompoundExpr* ce2 = listCE1[1];

				bool bAnd1, bOr1;
				ce1->GetCondjStatus(bAnd1, bOr1);
				if (bAnd1 && !bOr1)
				{
					if ((ceUp1->GetExprValue() == ce1->GetExprValue()) && 
						CExprValue::IsExprValueOpposite(ceUp2->GetExprValue(), ce2->GetExprValue()))
					{
						CCompoundExpr::DeleteSelf(ce1, cond);
						return true;
					}
					else if ((ceUp1->GetExprValue() == ce2->GetExprValue()) &&
						CExprValue::IsExprValueOpposite(ceUp2->GetExprValue(), ce1->GetExprValue()))
					{
						CCompoundExpr::DeleteSelf(ce2, cond);
						return true;
					}
					else if ((ceUp2->GetExprValue() == ce2->GetExprValue()) &&
						CExprValue::IsExprValueOpposite(ceUp1->GetExprValue(), ce1->GetExprValue()))
					{
						CCompoundExpr::DeleteSelf(ce2, cond);
						return true;
					}
					else if ((ceUp2->GetExprValue() == ce1->GetExprValue()) &&
						CExprValue::IsExprValueOpposite(ceUp1->GetExprValue(), ce2->GetExprValue()))
					{

						CCompoundExpr::DeleteSelf(ce1, cond);
						return true;
					}
				}
			}
		}
	}
	return false;
}

bool CheckTSCNullPointer2::CheckIfLibFunctionReturnNull(const Token * const pToken) const
{
	return pToken && _settings->library.CheckIfLibFunctionReturnNull(CGlobalTokenizer::FindFullFunctionName(pToken));
}


bool CheckTSCNullPointer2::GetLibNotNullParamIndexByName(std::set<int> &derefIndex, const Token *tok) const
{
	return _settings->library.GetLibNotNullParamIndexByName(derefIndex, CGlobalTokenizer::FindFullFunctionName(tok));
}


void CheckTSCNullPointer2::InitNameVarMap()
{
	const SymbolDatabase *base = _tokenizer->getSymbolDatabase();
	for (size_t nIndex = 0, nSize = base->getVariableListSize(); nIndex < nSize; ++nIndex)
	{
		const Variable *pVar = base->getVariableFromVarId(nIndex);
		if (pVar && !pVar->isLocal() && !pVar->isArgument())
		{
			std::string strScope = "";
			const Scope * pScope = pVar->scope();
			while (pScope)
			{
				strScope = pScope->className + ":" + strScope;
				pScope = pScope->nestedIn;
			}
			
			m_nameVarMap[strScope + pVar->name()] = pVar;
		}
	}
}

bool CheckTSCNullPointer2::FilterExplicitNullDereferenceError(const SExprLocation& sExprLoc, const CExprValue& ev)
{
	if (!sExprLoc.GetVariable() || !sExprLoc.GetVariable()->isLocal())
	{
		return false;
	}

	const Token* tokTop = ev.GetExprLoc().TokTop;
	if (tokTop->isExpandedMacro())
	{
		return false;
	}

	if (const Token* tokTop2 = tokTop->astParent())
	{
		if (Token::Match(tokTop2, "= 0") && tokTop2->next()->isExpandedMacro())
		{
			return false;
		}
	}
	
	// &(p->a) is ok
	// &(p->a[0]) is also ok
	if (const Token* tok2 = sExprLoc.TokTop->astParent())
	{ 
		if (tok2->str() == ".")
		{
			while (tok2->astParent() && Token::Match(tok2->astParent(), "["))
			{
				tok2 = tok2->astParent();
			}
			if (const Token* tok3 = tok2->astParent())
			{
				if (tok3->str() == "&")
				{
					return false;
				}
			}
		}
	}
	

	if (sExprLoc.TokTop->isInScope(Scope::eSwitch) && ev.GetExprLoc().TokTop->isInScope(Scope::eSwitch))
	{
		return false;
	}

	if (sExprLoc.TokTop->isInScope(Scope::eFor) || sExprLoc.TokTop->isInScope(Scope::eWhile))
	{
		return false;
	}

	return true;

}

bool CExprValue::IsExprValueMatched(const CExprValue& v1, const CExprValue& v2)
{
	if (!v1.IsCertain() || !v2.IsCertain() || (v1.GetExpr() != v2.GetExpr()))
	{
		return false;
	}

	const CValueInfo& vi1 = v1.GetValueInfo();
	const CValueInfo& vi2 = v2.GetValueInfo();

	if (vi1.IsStrValue() ^ vi2.IsStrValue())
		return false;

	bool bStrValue = vi1.IsStrValue();

	if (vi1 == vi2)
	{
		return true;
	}
	else
	{
		if (vi1.GetEqual() ^ vi2.GetEqual())
		{
			return bStrValue ? (vi1.GetStrValue() != vi2.GetStrValue()) : (vi1.GetValue() != vi2.GetValue());
		}
	}

	return false;
}

bool CExprValue::IsExprValueConflictedForEqualCond(const CExprValue& v1, const CExprValue& v2)
{
	if (v1.GetExpr() != v2.GetExpr())
	{
		return false;
	}

    if (!v1.IsCertain() || !v2.IsCertain())
    {
        return true;
    }
    
    const CValueInfo& vi1 = v1.GetValueInfo();
    const CValueInfo& vi2 = v2.GetValueInfo();
    
	if (vi1.IsStrValue() ^ vi2.IsStrValue())
		return false;

	bool bStrValue = vi1.IsStrValue();

    if(vi2.GetEqual())
    {
        if (vi1.GetEqual()) {
            return bStrValue ? (vi1.GetStrValue() == vi2.GetStrValue()) : (vi1.GetValue() == vi2.GetValue());
        }
        return false;
    }
    else
    {
        if (vi1.GetEqual()) {
            return bStrValue ? (vi1.GetStrValue() != vi2.GetStrValue()) : (vi1.GetValue() != vi2.GetValue());
        }
        return true;
    }
}

bool CExprValue::IsExprValueOpposite(const CExprValue& v1, const CExprValue& v2)
{
	if (!v1.IsCertain() || !v2.IsCertain() || (v1.GetExpr() != v2.GetExpr()))
	{
		return false;
	}
	return v1.GetValueInfo().IsOpposite(v2.GetValueInfo());
}

#pragma region DUMP LOG

std::string ExprValueToString(const CExprValue& exprValue)
{
	std::ostringstream oss;
	const CValueInfo& info = exprValue.GetValueInfo();
	oss << "[" << exprValue.GetExpr().ExprStr << "] ";

	if (info.GetUnknown())
	{
		oss << ", [Unknown]";
	}
	else
	{
		if (info.GetPossible())
		{
			oss << "<> ";
		}
		else
		{
			oss << (info.GetEqual() ? "== " : "!= ");
		}

		if (info.IsStrValue())
			oss << info.GetStrValue();
		else
			oss << info.GetValue();

		if (exprValue.IsCondIf())
		{
			oss << ", [IF]";
		}

		if (exprValue.IsCondLogic())
		{
			oss << ", [L]";
		}

		if (exprValue.IsCondNegative())
		{
			oss << ", [N]";
		}

		if (exprValue.IsCondInit())
		{
			oss << ", [A]";
		}

		if (exprValue.IsCondForward())
		{
			oss << ", [F]";
		}

		if (exprValue.IsCondCheckNull())
		{
			oss << ", [C]";
		}

		if (exprValue.IsCondFuncRet())
		{
			oss << ", [R]";
		}

		if (exprValue.IsCondDynamic())
		{
			oss << ", [D]";
		}

		if (exprValue.IsInitVar())
		{
			oss << ", [V]";
		}

		if (exprValue.IsCondIfDeref())
		{
			oss << ", [IfDeref]";
		}
	}


	return oss.str();
}

std::string CompoundExprToString(const CCompoundExpr* compExpr)
{
	if (!compExpr)
	{
		return emptyString;
	}
	std::ostringstream oss;
	if (CCompoundExpr* ce1 = compExpr->GetLeft())
	{
		oss << CompoundExprToString(ce1);
	}
	if (compExpr->IsConjNode())
	{
		CCompoundExpr::ConjType ct = compExpr->GetConj();
		std::string sConj;
		switch (ct)
		{
		case CCompoundExpr::Equal:
			sConj = "##";
			break;
		case CCompoundExpr::And:
			sConj = "&&";
			break;
		case CCompoundExpr::Or:
			sConj = "||";
			break;
		default:
			break;
		}
		oss << " " << sConj.c_str() << " ";
	}
	else if (compExpr->IsUnknown())
	{
		oss << "[Unknown]";
	}
	else 
	{
		const CExprValue& ev = compExpr->GetExprValue();
		if (!ev.Empty())
		{
			oss << ExprValueToString(ev);
		}
		else
		{
			oss << "[Unexpected empty CExprValue]";
		}
	}
	if (CCompoundExpr* ce2 = compExpr->GetRight())
	{
		oss << CompoundExprToString(ce2);
	}
	return oss.str();
}

void CheckTSCNullPointer2::OpenDumpFile()
{
	m_lineno = 0;
	m_dump = _settings->debugDumpNP;
	if (m_dump)
	{
#ifdef WIN32
		const char sep = '\\';
#else
		const char sep = '/';
#endif // WIN32
		static unsigned fileIndex = 0;
		std::string sFile = _tokenizer->list.getFiles().front();
		std::string logDir = Utility::CreateLogDirectory();
		std::ostringstream oss;
		oss << logDir << sep << fileIndex++ << "_" << "np" << "_" << sFile.substr(sFile.rfind('/') + 1);
		m_dumpFile.open(oss.str(), std::ios_base::trunc);
	}
}

void CheckTSCNullPointer2::TryDump(const Token* tok)
{
	if (!m_dump)
	{
		return;
	}
	if (tok->previous() && Token::Match(tok->previous(), "{|}"))
	{
		m_lineno = tok->previous()->linenr();
		DumpLocalVar(m_lineno);
	}
	else
	{
		if (tok->next() && tok->next()->linenr() > tok->linenr())
		{
			if (Token::Match(tok, "{|}"))
			{
				if (tok->previous() && tok->previous()->linenr() < tok->linenr())
				{
					return;
				}
			}

			m_lineno = tok->linenr();
			DumpLocalVar(m_lineno);
		}
	}
	
}

void CheckTSCNullPointer2::DumpLocalVar(unsigned lineno)
{
	if (m_localVar.Empty())
	{
		return;
	}

	m_dumpFile << "LINE[" << lineno << "]" << std::endl;

	m_dumpFile << "\t[RET]\t";
	if (m_localVar.GetReturn())
		m_dumpFile << "Return\t";
	if (m_localVar.GetIfReturn())
		m_dumpFile << "If_Return\t";
	if (m_localVar.GetReturnTok())
		m_dumpFile << m_localVar.GetReturnTok()->str() << ":" << m_localVar.GetReturnTok()->linenr();

	m_dumpFile << std::endl;

	if (!m_localVar.DerefedMap.empty())
	{
		m_dumpFile << "\t[DEREF]\t";
		for (MDEREF_I I = m_localVar.DerefedMap.begin(), E = m_localVar.DerefedMap.end(); I != E; ++I)
		{
			m_dumpFile << I->first.ExprStr << " ";
		}
		m_dumpFile << std::endl;
	}

	const std::vector<SExprLocation>& ifDerefed = m_localVar.GetIfCondDerefed();
	if (!ifDerefed.empty())
	{
		m_dumpFile << "\t[IF_DEREF]\t";
		for (std::vector<SExprLocation>::const_iterator I = ifDerefed.begin(), E = ifDerefed.end(); I != E; ++I)
		{
			m_dumpFile << I->Expr.ExprStr << " ";
		}
		m_dumpFile << std::endl;
	}

	CCompoundExpr* ce = m_localVar.GetIfCondPtr();
	if (ce && !ce->IsEmpty())
	{
		m_dumpFile << "\t[IF]\t" << CompoundExprToString(ce) << std::endl;
	}

	ce = m_localVar.GetIfCondExpandedPtr();
	if (ce && !ce->IsEmpty())
	{
		m_dumpFile << "\t[IF_EX]\t" << CompoundExprToString(ce) << std::endl;
	}
	
	for (MEV_CI I = m_localVar.PreCond.begin(), E = m_localVar.PreCond.end(); I != E; ++I)
	{
		m_dumpFile << "\t[PRE]\t" << ExprValueToString(I->second) << std::endl;
	}

	for (MEV_CI I = m_localVar.PreCond_if.begin(), E = m_localVar.PreCond_if.end(); I != E; ++I)
	{
		m_dumpFile << "\t[PRE_IF]\t" << ExprValueToString(I->second) << std::endl;
	}

	for (MEV_CI I = m_localVar.InitStatus.begin(), E = m_localVar.InitStatus.end(); I != E; ++I)
	{
		m_dumpFile << "\t[INIT]\t" << ExprValueToString(I->second) << std::endl;
	}

	for (MEV_CI I = m_localVar.InitStatus_temp.begin(), E = m_localVar.InitStatus_temp.end(); I != E; ++I)
	{
		m_dumpFile << "\t[INIT_TEMP]\t" << ExprValueToString(I->second) << std::endl;
	}

	for (MMEC_CI I = m_localVar.EqualCond.begin(), E = m_localVar.EqualCond.end(); I != E; ++I)
	{
		m_dumpFile << "\t[EQUAL]\t{ " << ExprValueToString(I->second.EV) << " } == { ";
		CCompoundExpr* ce2 = I->second.EqualCE.get();
		if (!ce2)
			continue;
		m_dumpFile << CompoundExprToString(ce2);
		m_dumpFile << " }" << std::endl;
	}

	for (MMEC_CI I = m_localVar.EqualCond_if.begin(), E = m_localVar.EqualCond_if.end(); I != E; ++I)
	{
		m_dumpFile << "\t[EQ_IF]\t{ " << ExprValueToString(I->second.EV) << " } == { ";
		CCompoundExpr* ce2 = I->second.EqualCE.get();
		if (!ce2)
			continue;
		m_dumpFile << CompoundExprToString(ce2);
		m_dumpFile << " }" << std::endl;
	}


	for (MEV_CI I = m_localVar.PossibleCond.begin(), E = m_localVar.PossibleCond.end(); I != E; ++I)
	{
		m_dumpFile << "\t[GLOBAL]\t" << ExprValueToString(I->second) << std::endl;
	}
}

void CheckTSCNullPointer2::CloseDumpFile()
{
	if (m_dumpFile.is_open())
	{
		m_dumpFile.close();
	}
}

bool CheckTSCNullPointer2::FindMostMatchedEqualCond(SLocalVar& localVar, bool bStrict, const CExprValue& ev, SEqualCond*& findEC)
{
	SEqualCond* tempEC = nullptr;
	const SExpr& expr = ev.GetExpr();
	for (MMEC_I I2 = m_localVar.EqualCond.find(expr), E2 = m_localVar.EqualCond.end(); I2 != E2 && expr == I2->first; ++I2)
	{
		SEqualCond& ec2 = I2->second;

		if (ev == ec2.EV)
		{
			findEC = &ec2;
			return true;
		}
		else if (!bStrict && CExprValue::IsExprValueMatched(ev, ec2.EV))
		{

			tempEC = &ec2;
		}
	}

	if (tempEC)
	{
		findEC = tempEC;
		return true;
	}
	return false;
}

CCompoundExpr* CheckTSCNullPointer2::ExpandIfCondWithEqualCond_Recursively(CCompoundExpr* ifCond, SLocalVar& localVar, std::set<SEqualCond*>& expandedECList, bool bStrict)
{
	std::vector<CCompoundExpr*> listCE;
	ifCond->ConvertToList(listCE);
	CCompoundExpr* topCond = ifCond->GetTop();
	for (std::vector<CCompoundExpr*>::const_iterator I = listCE.begin(), E = listCE.end(); I != E; ++I)
	{
		CCompoundExpr* ce = *I;
		if (!ce || ce->IsUnknownOrEmpty())
			continue;


		bool bAnd, bOr;
		ce->GetCondjStatus(bAnd, bOr);
		if (bAnd && bOr)
			continue;

		const CExprValue& ev = ce->GetExprValue();

		SEqualCond* pEC = nullptr;
		bool bMatch = FindMostMatchedEqualCond(localVar, bStrict, ev, pEC);

		if (bMatch && pEC)
		{
			if (expandedECList.count(pEC))
			{
				continue;
			}
		
			std::shared_ptr<CCompoundExpr>& sharedCE = pEC->EqualCE;

			// if the ifcond to be expanded have conflicts with precond, then don't expand.
			bool bExpand = true;
			std::vector<CCompoundExpr*> expandList;
			sharedCE->ConvertToList(expandList);
			for (std::vector<CCompoundExpr*>::iterator I3 = expandList.begin(), E3 = expandList.end(); I3 != E3; ++I3)
			{
				CCompoundExpr* expandCE = *I3;
				CCompoundExpr* sameCE = nullptr;
				if (topCond->HasSExpr(expandCE->GetExprValue().GetExpr(), &sameCE))
				{
					//if (sameCE->GetExprValue() == expandCE->GetExprValue())
					//{
					//	continue;
					//}
					bExpand = false;
					break;
				}
				// consider this as higher priority, so expand
				//const SExpr& expandExpr = expandCE->GetExprValue().GetExpr();
				//if (localVar.HasPreCond(expandExpr))
				//{
				//	CExprValue& preEV = localVar.PreCond[expandExpr];
				//	if (preEV.IsCertain() )
				//	{
				//		
				//		bExpand = false;
				//		break;
				//	}
				//}
			}

			if (!bExpand)
				continue;

			expandedECList.insert(pEC);

			CCompoundExpr* newNode = ce->DeepCopy();//use deepcopy to avoid wild pointer
			CCompoundExpr::ConjType conjType = CCompoundExpr::Equal;
			ce->SetConj(conjType);
			ce->SetExprValue(CExprValue::EmptyEV);

			ce->SetLeft(newNode);
			CCompoundExpr* equalCE = sharedCE->DeepCopy();//use deepcopy to avoid wild pointer
			// do expand recursively
			ce->SetRight(equalCE);
			equalCE = ExpandIfCondWithEqualCond_Recursively(equalCE, localVar, expandedECList, true);
		}
	}

	return ifCond;
}


CCompoundExpr* CheckTSCNullPointer2::ExpandIfCondWithEqualCond(CCompoundExpr* ifCond, SLocalVar& localVar)
{
	ifCond = ifCond->DeepCopy();
	MMEC& equalCond = localVar.EqualCond;
	// expand ifconds with equalconds
	if (equalCond.empty() || !ifCond || ifCond->IsUnknownOrEmpty())
		return ifCond;

	std::set<SEqualCond*> expandedECList;
	return ExpandIfCondWithEqualCond_Recursively(ifCond, localVar, expandedECList, false);
}

void CheckTSCNullPointer2::MergeEqualCond(MMEC& temp, bool bReturn, SLocalVar& topLocalVar)
{
	if (!temp.empty())
	{
		if (topLocalVar.EqualCond_if.empty())
		{
			topLocalVar.EqualCond_if = temp;
		}
		else
		{
			//topLocalVar.EqualCond_if.insert(temp.begin(), temp.end());

			for (MMEC_CI I = temp.begin(), E = temp.end(); I != E;)
			{
				const SExpr& expr = I->first;
				const SEqualCond& ec = I->second;

				// [EQUAL]	{ [p1] != 0 } == { [*p2] == 1 }
				// [EQUAL]	{ [p1] != 0 } == { [*p2] != 1 }
				bool bMerge = false;
				CCompoundExpr* result = nullptr;
				for (MMEC_I I2 = topLocalVar.EqualCond_if.find(expr), E2 = topLocalVar.EqualCond_if.end(); I2 != E2 && I2->first == expr;)
				{
					const SEqualCond& ec2 = I2->second;
					if (ec2.EV == ec.EV)
					{
						bMerge = true;
						if (!ec2.EqualCE->IsEqual(ec.EqualCE.get()))
						{
							CCompoundExpr::ConjType type = CCompoundExpr::Or;
							if (ec.EV.IsCondNegative() || ec2.EV.IsCondNegative())
							{
								type = CCompoundExpr::And;
							}
							MergeTwoIfCond(ec.EqualCE.get(), ec2.EqualCE.get(), type, result);
							topLocalVar.EqualCond_if.erase(I2);
						}
						break;
					}
					++I2;
				}

				if (bMerge)
				{
					if (result)
					{
						std::shared_ptr<CCompoundExpr> newCE;
						newCE.reset(result);
						SEqualCond newEC(ec.EV, newCE);
						topLocalVar.EqualCond_if.insert(PEC(expr, newEC));
					}
				}
				else
				{

					topLocalVar.EqualCond_if.insert(PEC(expr, ec));
				}

				++I;
			}
		}
	}
    
	MMEC cacheMMEC;
	for (MMEC_I I = topLocalVar.EqualCond_if.begin(), E = topLocalVar.EqualCond_if.end(); I != E;)
	{
		const SEqualCond& ec = I->second;

		std::vector<CCompoundExpr*> listCE;
		ec.EqualCE->ConvertToList(listCE);

		bool bErase = false;
		for (std::vector<CCompoundExpr*>::iterator I2 = listCE.begin(), E2 = listCE.end(); I2 != E2; ++I2)
		{
			CCompoundExpr* ce2 = *I2;

			bool bAnd, bOr;
			ce2->GetCondjStatus(bAnd, bOr);
			if (!bOr)
			{
				// if(!p) { p != 0, L}  
				// p== 0 => p != 0
				// erase other equalcond_if
				if (CExprValue::IsExprValueOpposite(ce2->GetExprValue(), ec.EV))
				{
					CExprValue pre = ce2->GetExprValue();
					pre.SetCondLogic();

					// 查询p是否来自经验性推断
					if (m_localVar.HasTempInit(pre.GetExpr()))
					{
						if (m_localVar.InitStatus_temp[pre.GetExpr()].IsInitVar())
						{
							if (m_localVar.HasPreCond(pre.GetExpr()) && !m_localVar.PreCond[pre.GetExpr()].IsInitVar())
							{
								
							}
							else
							{
								pre.SetInitVar();
							}
						}
					}
					topLocalVar.AddPreCond(pre, true);
					topLocalVar.EqualCond_if.clear();
					bErase = true;
					break;
				}
			}
		}

		if (bErase)
		{
			break;
		}
		++I;
	}


	// simplify equal cond
	// [EQUAL]	{ [pstAd] != 0 } == { [valid_hours] != 0 || [valid_hours] == 0 }
	// [EQUAL]  { [pstAd] == 0, [N] } == { [valid_hours] == 0 && [valid_hours] != 0 }
	for (MMEC_I I = topLocalVar.EqualCond_if.begin(), E = topLocalVar.EqualCond_if.end(); I != E;)
	{
		SEqualCond& ec = I->second;
		CCompoundExpr* pCE = ec.EqualCE->DeepCopy();

		// 表示已经简化了
		if (true == SimplifyIfCond(pCE, pCE))
		{
			ec.EqualCE.reset(pCE);
		}

		if (pCE->IsEmpty())
		{
			topLocalVar.EqualCond_if.erase(I++);
			continue;
		}

		++I;
	}

	bool bSearch = true;
	while (bSearch)
	{
		bSearch = false;
		for (MMEC_I I = topLocalVar.EqualCond_if.begin(), E = topLocalVar.EqualCond_if.end(); I != E;++I)
		{
			SEqualCond& ec1 = I->second;
			MMEC_I I2 = I;
			I2++;
			for (; I2 != E && I2->first == I->first; ++I2)
			{
				SEqualCond& ec2 = I2->second;
				if (CExprValue::IsExprValueOpposite(ec1.EV, ec2.EV))
				{
					
					if (ec1.EqualCE->IsEqual(ec2.EqualCE.get()))
					{
						topLocalVar.EqualCond_if.erase(I2);
						topLocalVar.EqualCond_if.erase(I);
						bSearch = true;
						break;
					}
				}
			}

			if (bSearch)
			{
				break;
			}
		}
	}

	// 如果等价条件右边标记的是指针不为零，那么可以merge到上层
	for (MMEC_I I = topLocalVar.EqualCond_if.begin(), E = topLocalVar.EqualCond_if.end(); I != E;)
	{
		bool bMerge = false;
		const SExpr& expr = I->first;
		const CExprValue& ev = I->second.EV;
		if (!ev.IsCondForward())
		{
			CCompoundExpr* pCE = I->second.EqualCE.get();

			if (pCE->GetConj() == CCompoundExpr::None && pCE->GetExprValue().GetValueInfo().IsNotEqualZero())
			{
				for (MMEC_I I2 = topLocalVar.EqualCond.find(expr), E2 = topLocalVar.EqualCond.end(); I2 != E2 && I2->first == expr;++I2)
				{
					if (I2->second.EV == ev)
					{
						CCompoundExpr* newCE = nullptr;
						MergeTwoIfCond(I2->second.EqualCE.get(), pCE, CCompoundExpr::And, newCE);
						I2->second.EqualCE.reset(newCE);
						bMerge = true;
					}

				}
			}
		}

		if (bMerge)
		{
			topLocalVar.EqualCond_if.erase(I++);
			continue;
		}
		
		++I;
	}

	// 如果出现等价条件前半部分冲突，应该删除旧的???
	for (MMEC_I I = topLocalVar.EqualCond_if.begin(), E = topLocalVar.EqualCond_if.end(); I != E;)
	{
		const SExpr& expr = I->first;
		const SEqualCond& ec = I->second;

		bool bEraseNew = false;
		for (MMEC_I I2 = topLocalVar.EqualCond.find(expr), E2 = topLocalVar.EqualCond.end(); I2 != E2 && I2->first == expr;)
		{
			const CExprValue& evOld = I2->second.EV;
			const CExprValue& evNew = ec.EV;
			if (evOld == evNew)
			{
				if (evNew.IsCondForward() && !evOld.IsCondForward())
				{
					bEraseNew = true;
					break;
				}
				else
				{
					topLocalVar.EqualCond.erase(I2++);
					continue;
				}
			}
			++I2;
		}
		if (bEraseNew)
		{
			topLocalVar.EqualCond_if.erase(I++);
			continue;
		}

		++I;
	}


	topLocalVar.EqualCond.insert(topLocalVar.EqualCond_if.begin(), topLocalVar.EqualCond_if.end());
	topLocalVar.EqualCond_if.clear();
}

void CheckTSCNullPointer2::MergePreCond(MEV& temp, bool bReturn, SLocalVar& topLocalVar)
{
	bool bReturnIf = topLocalVar.GetIfReturn();

	if (bReturnIf && bReturn)
	{
		// both branches return;
		topLocalVar.Clear();
		topLocalVar.SetReturn(true);
	}
	else if (bReturnIf)
	{
		for (MEV_I I = temp.begin(), E = temp.end(); I != E; ++I)
		{
			CExprValue ev = I->second;
			if (ev.IsInitVar())
			{
				ev.SetInitVar(false);
			}
			ev.SetCondLogic();
			topLocalVar.AddPreCond(ev, true);
			ExpandPreCondWithEqualCond(ev, topLocalVar);
		}

	}
	else if (bReturn)
	{
		for (MEV_I I = topLocalVar.PreCond_if.begin(), E = topLocalVar.PreCond_if.end(); I != E; ++I)
		{
			CExprValue ev = I->second;
			if (ev.IsInitVar())
			{
				ev.SetInitVar(false);
			}
			ev.SetCondLogic();
			topLocalVar.AddPreCond(ev, true);
			ExpandPreCondWithEqualCond(ev, topLocalVar);
		}
	}
	else
	{
		for (MEV_I I = temp.begin(), E = temp.end(); I != E; ++I)
		{
			//SExpr
			MEV_I I2 = topLocalVar.PreCond_if.find(I->first);
			if (I2 != topLocalVar.PreCond_if.end())
			{
				if (I2->second == I->second) 
				{
					CExprValue ev = I->second;
					if (ev.IsInitVar())
					{
						ev.SetInitVar(false);
					}
					ev.SetCondLogic();
					topLocalVar.AddPreCond(ev, true);
					ExpandPreCondWithEqualCond(ev, topLocalVar);
				}
				else if (CExprValue::IsExprValueOpposite(I2->second, I->second))
				{
					// maybe deadcode, both branches return
					topLocalVar.PreCond_if.erase(I->first);
					topLocalVar.ClearSExpr(I->first);
				}
			}
		}
	}

    if (!topLocalVar.PreCond_if.empty()) 
	{
        topLocalVar.PreCond_if.clear();
    }
	topLocalVar.SetIfReturn(false);
}

void CheckTSCNullPointer2::ExpandPreCondWithEqualCond(const CExprValue& pre, SLocalVar& localVar)
{
	const SExpr& expr = pre.GetExpr();
	for (MMEC_I I = localVar.EqualCond.find(expr), E = localVar.EqualCond.end(); I != E && expr == I->first; ++I)
	{
		if (CExprValue::IsExprValueMatched(pre, I->second.EV))
		{
			std::vector<CCompoundExpr*> listCE;
			I->second.EqualCE->ConvertToList(listCE);
			for (std::vector<CCompoundExpr*>::iterator I2 = listCE.begin(), E2 = listCE.end(); I2 != E2; ++I2)
			{
				CCompoundExpr* ce2 = *I2;
				bool bAnd, bOr;
				ce2->GetCondjStatus(bAnd, bOr);

				if (!bOr)
				{
					CExprValue ev2 = ce2->GetExprValue();
					if (pre.IsCondLogic())
					{
						ev2.SetCondLogic();
					}
					localVar.AddPreCond(ev2, true);
				}
			}
		}
	}
}

//void CheckTSCNullPointer2::MergeTwoIfCond(CCompoundExpr* ce1, CCompoundExpr* ce2, CCompoundExpr* result)
//{
//	std::vector<CCompoundExpr*> listCE1;
//	ce1->ConvertToList(listCE1);
//	std::vector<CCompoundExpr*> listCE2;
//	ce2->ConvertToList(listCE2);
//
//	for (std::vector<CCompoundExpr*>::iterator I = listCE1.begin(), E = listCE1.end(); I != E; ++I)
//	{
//		CCompoundExpr* node1 = *I;
//		bool bAnd1 = false;
//		bool bOr1 = false;
//		node1->GetCondjStatus(bAnd1, bOr1);
//
//		if (bOr1 == false)
//		{
//			for (std::vector<CCompoundExpr*>::iterator I2 = listCE2.begin(), E2 = listCE2.end(); I2 != E2; ++I2)
//			{
//				CCompoundExpr* node2 = *I2;
//				bool bAnd2 = false;
//				bool bOr2 = false;
//				node1->GetCondjStatus(bAnd2, bOr2);
//				
//				if (bOr2 == false)
//				{
//					CCompoundExpr* result_temp = nullptr;
//					MergeTwoCExprValue(node1->GetExprValue(), node2->GetExprValue(), result_temp);
//					if (result_temp != nullptr)
//					{
//						if (result == nullptr)
//						{
//							result = result_temp;
//						}
//						else
//						{
//							CCompoundExpr* nodeAnd = new CCompoundExpr;
//							nodeAnd->SetConj(CCompoundExpr::And);
//							nodeAnd->SetLeft(result);
//							nodeAnd->SetRight(result_temp);
//							result = nodeAnd;
//						}
//					}
//				}
//			}
//		}
//	}
//}

bool GetMergeFlag(CCompoundExpr::ConjType conj, bool bAnd, bool bOr)
{
	return (conj == CCompoundExpr::Or && bOr == false) ||
		(conj == CCompoundExpr::And && bAnd == false);
}

void CheckTSCNullPointer2::MergeTwoIfCond(CCompoundExpr* ce1, CCompoundExpr* ce2, CCompoundExpr::ConjType conj, CCompoundExpr*& result)
{
	ce1 = ce1->DeepCopy();
	ce2 = ce2->DeepCopy();

	std::vector<CCompoundExpr*> listCE1;
	ce1->ConvertToList(listCE1);

	std::set<CCompoundExpr*> deleteList1;
	std::set<CCompoundExpr*> deleteList2;
	for (std::vector<CCompoundExpr*>::iterator I = listCE1.begin(), E = listCE1.end(); I != E; ++I)
	{
		CCompoundExpr* node1 = *I;
		bool bAnd = false;
		bool bOr = false;
		node1->GetCondjStatus(bAnd, bOr);

		//bool checkFlag = GetMergeFlag(conj, bAnd, bOr);
		bool checkFlag = (bOr == false);

		if (checkFlag)
		{
			const CExprValue& ev1 = node1->GetExprValue();
			const SExpr& expr = ev1.GetExpr();
			CCompoundExpr* node2 = nullptr;
			if (ce2->HasSExpr(expr, &node2))
			{
				bAnd = false;
				bOr = false;
				node2->GetCondjStatus(bAnd, bOr);
				// checkFlag = GetMergeFlag(conj, bAnd, bOr);
				checkFlag = (bOr == false);
				if (checkFlag)
				{
					const CExprValue& ev2 = node2->GetExprValue();

					if (ev1 == ev2)
					{
						deleteList2.insert(node2);
					}
					else
					{
						bool del1, del2;
						MergeResult mr = CheckTwoCExprValueCompatible(ev1, ev2, conj, del1, del2);
						if (mr == MR_MERGE)
						{
							if (del1)
							{
								deleteList1.insert(node1);
							}
							if (del2)
							{
								deleteList2.insert(node2);
							}
						}
						else if (mr == MR_CONFLICT)
						{
						
						}
						else
						{
							
						}
					}
				}
			}
		}
	}

	for (std::set<CCompoundExpr*>::iterator I = deleteList1.begin(), E = deleteList1.end(); I != E; ++I)
	{
		CCompoundExpr* ce = *I;
		CCompoundExpr::DeleteSelf(ce, ce1);
	}

	for (std::set<CCompoundExpr*>::iterator I = deleteList2.begin(), E = deleteList2.end(); I != E; ++I)
	{
		CCompoundExpr* ce = *I;
		CCompoundExpr::DeleteSelf(ce, ce2);
	}

	result = nullptr;
	if (ce1->IsUnknownOrEmpty() && ce2->IsUnknownOrEmpty())
	{
		return;
	}
	else if (ce1->IsUnknownOrEmpty())
	{
		result = ce2;
	}
	else if (ce2->IsUnknownOrEmpty())
	{
		result = ce1;
	}
	else
	{
		result = new CCompoundExpr;
		result->SetConj(conj);
		result->SetLeft(ce1);
		result->SetRight(ce2);
	}
}

MergeResult CheckTSCNullPointer2::CheckTwoCExprValueCompatible(const CExprValue& ev1, const CExprValue& ev2, CCompoundExpr::ConjType conj, bool& deleteEV1, bool& deleteEV2)
{
	deleteEV1 = deleteEV2 = false;
	if (ev1.GetExpr() != ev2.GetExpr() || !ev1.IsCertain() || !ev2.IsCertain())
	{
		return MR_OK;
	}

	const CValueInfo& vi1 = ev1.GetValueInfo();
	const CValueInfo& vi2 = ev2.GetValueInfo();

	if (vi1.GetEqual() && vi2.GetEqual())
	{
		if (conj == CCompoundExpr::Or)
		{
			return MR_OK;
		}
		else if (conj == CCompoundExpr::And)
		{
			return MR_CONFLICT;
		}
	}
	else if (vi1.GetEqual() ^ vi2.GetEqual())
	{
		if (conj == CCompoundExpr::Or)
		{
			if (vi1.GetValue() != vi2.GetValue())
			{
				if (vi1.GetEqual())
				{
					deleteEV1 = true;
				}
				else 
				{
					deleteEV2 = true;
				}
				return MR_MERGE;
			}
			else
			{
				
				//deleteEV1 = true;
				//deleteEV2 = true;
				//return MR_MERGE;
				return MR_OK;
			}
		}
		else if (conj == CCompoundExpr::And)
		{
			if (vi1.GetValue() != vi2.GetValue())		// p == 3 && p !=5
			{
				if (vi1.GetEqual())
				{
					deleteEV2 = true;
				}
				else
				{
					deleteEV1 = true;
				}
				return MR_MERGE;
			}
			else
			{
				return MR_CONFLICT;
			}
		}
	}
	else
	{
		// p!= 3 && p!=5
		if (conj == CCompoundExpr::Or)
		{
			deleteEV1 = true;
			deleteEV2 = true;
			return MR_MERGE;
		}
		else if (conj == CCompoundExpr::And)
		{
			return MR_OK;
		}
	}
	return MR_OK;
}

void CheckTSCNullPointer2::AscendEqualCond(SLocalVar &topLocalVar, std::shared_ptr<CCompoundExpr>& ifCond, MMEC& tempEC, bool bReturn)
{
	if (ifCond->IsUnknownOrEmpty() || bReturn)
	{
		return;
	}

	// if(p == NULL) {  p != NULL [L]; }
	std::vector<CCompoundExpr*> listCE;
	ifCond->ConvertToList(listCE);
	for (std::vector<CCompoundExpr*>::iterator I2 = listCE.begin(), E2 = listCE.end(); I2 != E2; ++I2)
	{
		CCompoundExpr* comp = *I2;
		const CExprValue& evIf = comp->GetExprValue();
		if (evIf.GetValueInfo().IsEqualZero())
		{
			if (m_localVar.HasPreCond(evIf.GetExpr()))
			{
				CExprValue& ev2 = m_localVar.PreCond[evIf.GetExpr()];
				if (CExprValue::IsExprValueOpposite(ev2, evIf) && ev2.IsCondLogic())
				{
					return;
				}
			}
		}
	}

	for (MMEC_I I = m_localVar.EqualCond.begin(), E = m_localVar.EqualCond.end(); I != E; ++I)
	{
		//const SExpr& expr = I->first;

		if (topLocalVar.HasSameEqualCond(I->second))
		{
			continue;
		}

		SEqualCond& ec = I->second;
		CExprValue& ev = ec.EV;

		// the old version is HasInitCond
		if (topLocalVar.HasPreCond(ev.GetExpr()))
		{
			CExprValue& ev2 = topLocalVar.PreCond[ev.GetExpr()];
			if (ev == ev2)
			{
				continue;
			}
		}

		// if(i != 3){ if(i == 4) { XXX; }  }
		if (CheckIf_ElseIf(ev, ifCond.get()))
		{
			std::shared_ptr<CCompoundExpr> spCE;
			spCE.reset(ec.EqualCE->DeepCopy());
			AddEqualCond(tempEC, ev, spCE);
			continue;
		}

		if (ev.IsCondForward())
		{
			// if in if-elseif situation, not continue;
			continue;
		}

		CCompoundExpr* newCE = nullptr;
		MergeTwoIfCond(ec.EqualCE.get(), ifCond.get(), CCompoundExpr::And, newCE);
		if (newCE != nullptr)
		{
			std::shared_ptr<CCompoundExpr> spCE;
			spCE.reset(newCE);
			AddEqualCond(tempEC, ev, spCE);
		}
	}
}

bool CheckTSCNullPointer2::CheckIf_ElseIf(const CExprValue& ev, CCompoundExpr* ifCond)
{
	CCompoundExpr* ce = nullptr;
	if (ifCond->HasSExpr(ev.GetExpr(), &ce))
	{
		CExprValue& ifEV = ce->GetExprValue();
		
		if (CExprValue::IsExprValueMatched(ifEV, ev))
		{
			return true;
		}
	}

	return false;

}

void CheckTSCNullPointer2::ExtractPreCond(MEV &tempPreCond, SLocalVar &topLocalVar)
{
    for (MEV_CI I = m_localVar.PreCond.begin(), E = m_localVar.PreCond.end(); I != E; ++I) {
        const CExprValue& ev = I->second;
        if ((ev.IsCondLogic() || (ev.IsCondInit() && m_localVar.HasTempInit(ev.GetExpr()))) ) 
		{
			CCompoundExpr* ce = nullptr;
			if (topLocalVar.GetIfCond()->HasSExpr(I->first, &ce))
			{
				// for if(p1 != NULL || p2 != NULL) {  p1 != NULL, [L]; p2 != NULL, [L]; }
				bool bAnd, bOr;   
				ce->GetCondjStatus(bAnd, bOr);
				if (!bAnd && bOr)
				{
					tempPreCond[ev.GetExpr()] = ev;
				}
				/*
				// for if(a && !a->b) { a->b != 0 [L]; }
				if (bAnd && !bOr)
				{
					if (CExprValue::IsExprValueOpposite(ce->GetExprValue(), ev))
					{
						CCompoundExpr* ce2 = ce->GetParent()->GetLeft();
						if (ce2 != ce)
						{
							if (CheckExprIsSubField(ce2->GetExprValue().GetExpr(), ce->GetExprValue().GetExpr()) &&
								ce2->GetExprValue().CanNotBeNull())
							{
								tempPreCond[ev.GetExpr()] = ev;
							}
						}

					}
				}
				*/

			}
			else
			{
				tempPreCond[ev.GetExpr()] = ev;
			}
        }
    }
}

void CheckTSCNullPointer2::ExtractEqualCond(std::shared_ptr<CCompoundExpr> &ifCond, SLocalVar &topLocalVar, MMEC& tempEqualCond, bool hasElse)
{
	if (ifCond->IsUnknownOrEmpty())
	{
		return;
	}
	// extract equal cond
	//if ( p ) { flag1 = 0;flag2 = 3;} => { flag1 == 0 } == { p != NULL }, { flag2 == 3 } == { p != NULL }
	for (MEV_CI I = m_localVar.InitStatus_temp.begin(), E = m_localVar.InitStatus_temp.end(); I != E; ++I)
	{
		const SExpr& expr = I->first;
		const CExprValue& evAssigned = I->second;

		// 如果变量是推断出来"不等于一个值"，并且不是等于0，并且有else，那么这个等价条件是不可靠的，不应该提出来
		// if(xxx)
		//		flag = func();
		// else
		//		flag = 1;
		if (evAssigned.IsInitVar() && !evAssigned.GetValueInfo().IsNotEqualZero() && hasElse)
		{
			continue;
		}

		if (evAssigned.IsCertain())
		{
			// special handling, if(i == 24) { i == 42; }
			if (evAssigned.IsCertainAndEqual())
			{
				CCompoundExpr* temp_ce = nullptr;
				if (ifCond->HasSExpr(expr, &temp_ce))
				{
					if (temp_ce && temp_ce == ifCond.get())
					{
						const CExprValue& temp_ev = temp_ce->GetExprValue();
						if (temp_ev.IsCertainAndEqual() && (temp_ev.GetValueInfo() != evAssigned.GetValueInfo()))
						{
							CExprValue ev2 = temp_ev;
							ev2.NegationValue();
							topLocalVar.AddInitStatus(ev2, true, false);//ignore TSC
							ev2.SetCondLogic();
							topLocalVar.AddPreCond(ev2);
							continue;
						}
					}
				}
			}
			
			if (!ifCond->HasSExpr(expr))
			{
				//【TSC2】dereferenceIfNull误报（if(!p && !q){ ret= true; getData(&p,&q)赋值） from TSC1 (100000083)
				std::vector<CCompoundExpr*> cExprList;
				ifCond->ConvertToList(cExprList);
				bool flag = false;
				for (std::vector<CCompoundExpr*>::iterator iter = cExprList.begin(), iterEnd = cExprList.end(); iter != iterEnd; iter++)
				{
					CCompoundExpr* ce = *iter;
					const SExpr& se = ce->GetExprValue().GetExpr();
					if (m_localVar.HasTempInit(se))
					{
						flag = true;
						break;
					}
				}
				if (flag)
					continue;

				// 新的逻辑关系让旧的逻辑关系失效 【TSC2】dereferenceIfNull误报（其他变量推断） from TSC1 (100000530，272) ID： 54551141 handle 272
				/*
				    bool b =p!=NULL;
					if(b)//expr2(ifcond) means b 
					{
					}
					else
					{
					   p = &xx; //expr means p(当前equalcond)

					}
					需遍历旧的equalcond，发现正好是b和p关系的，就去掉
				*/
				for (std::vector<CCompoundExpr*>::iterator iter = cExprList.begin(), iterEnd = cExprList.end(); iter != iterEnd; iter++)//遍历ifcond
				{
					CCompoundExpr* ce = *iter;
					const SExpr& expr2 = ce->GetExprValue().GetExpr();//ifcond

					for (MMEC_I I2 = topLocalVar.EqualCond.find(expr), E2 = topLocalVar.EqualCond.end(); I2 != E2 && I2->first == expr;)//遍历上一层的equalcond
					{
						if (I2->second.EqualCE->HasSExpr(expr2))
						{
							topLocalVar.EqualCond.erase(I2++);
							
							if (topLocalVar.HasPreCond(expr))
							{
								if (topLocalVar.PreCond[expr].CanBeNull())
								{
									topLocalVar.ErasePreCond(expr);
								}
							}
							continue;
						}
						++I2;
					}

					for (MMEC_I I2 = topLocalVar.EqualCond.find(expr2), E2 = topLocalVar.EqualCond.end(); I2 != E2 && I2->first == expr2;)
					{
						if (I2->second.EqualCE->HasSExpr(expr))
						{
							topLocalVar.EqualCond.erase(I2++);
							continue;
						}
						++I2;

					}
				}

				
				if (topLocalVar.HasInitStatus(expr))
				{
					std::shared_ptr<CCompoundExpr> ifCond1(ifCond->DeepCopy());
					//topLocalVar.AddEqualCond_if(evAssigned, ifCond1);
					AddEqualCond(tempEqualCond, evAssigned, ifCond1);
				}

				// handle if(p){ p->a = 1; }, no negative equal cond
				bool isSubField = false;
				std::vector<CCompoundExpr*> listCE;
				ifCond->ConvertToList(listCE);
				for (std::vector<CCompoundExpr*>::iterator I2 = listCE.begin(), E2 = listCE.end(); I2 != E2; ++I2)
				{
					CCompoundExpr* ce = *I2;
					if (CheckExprIsSubField(ce->GetExprValue().GetExpr(), expr))
					{
						isSubField = true;
						break;
					}
				}
				if (!isSubField)
				{
					if (!(evAssigned.GetExprLoc().GetVariable() && evAssigned.GetExprLoc().GetVariable()->typeStartToken()->str() == "bool" && !evAssigned.IsInitCertain()))
					{

						CExprValue neg = evAssigned;
						neg.NegationValue();
						neg.SetCondNegative();
						std::shared_ptr<CCompoundExpr> ifCond2(ifCond->DeepCopy());
						ifCond2->Negation();
						AddEqualCond(tempEqualCond, neg, ifCond2);
					}
				}
			}
		}
		else
		{
			// q = NULL; p = get_p(); if(p) { q = get_q(); }
			// =>
			// { q != NULL } == { p != NULL }
			if (topLocalVar.HasInitStatus(I->first))
			{
				CExprValue& temp_ev = topLocalVar.InitStatus[I->first];
				if (temp_ev.IsCertain())
				{
					CExprValue ev2 = temp_ev;
					ev2.NegationValue();
					std::shared_ptr<CCompoundExpr> ifCond2(ifCond->DeepCopy());
					//topLocalVar.AddEqualCond_if(ev2, ifCond2);
					AddEqualCond(tempEqualCond, ev2, ifCond2);
				}
				else
				{

				}
			}
			else
			{

			}
		}
	}

	// extract 
	// p = NULL; if(i == 1) { p = getp(); if(!p) return; }
	// i ==1 => p != 0
	std::vector<CCompoundExpr*> listCE;
	ifCond->ConvertToList(listCE);
	for (std::vector<CCompoundExpr*>::iterator I = listCE.begin(), E = listCE.end(); I != E; ++I )
	{
		CCompoundExpr* ce = *I;

		bool bAnd, bOr;
		ce->GetCondjStatus(bAnd, bOr);
		
		if (!bAnd)
		{
			CCompoundExpr* ifcond_node = nullptr;
			for (MEV_CI I2 = m_localVar.PreCond.begin(), E2 = m_localVar.PreCond.end(); I2 != E2; ++I2)
			{
				if (I2->second.IsCondLogic() || I2->second.IsCondInit())
				{
					// If the upper scope has the same pre cond, then continue
					if (topLocalVar.HasPreCond(I2->first))
					{
						CExprValue& ev = topLocalVar.PreCond[I2->first];
						if (ev.IsCondLogic() || ev.IsCondInit())
						{
							if (ev == I2->second && !ev.IsInitVar())
							{
								continue;
							}
						}
					}

					// if(!p) { p = NULL; }
					// 对于这种场景，提前提成PreCond，不再等到最后merge equalcond的时候
					const CExprValue& ev2 = I2->second;
					if (CExprValue::IsExprValueOpposite(ce->GetExprValue(), ev2))
					{
						CExprValue newEV = ev2;
						newEV.SetCondLogic();
						topLocalVar.AddPreCond(newEV, true);
					}
					else
					{
						CCompoundExpr* node = new CCompoundExpr;
						node->SetExprValue(I2->second);
						node->GetExprValue().SetCondNone();
						node->SetConj(CCompoundExpr::None);

						if (ifcond_node == nullptr)
						{
							ifcond_node = node;
						}
						else
						{
							CCompoundExpr* equalNode = CCompoundExpr::CreateConjNode(CCompoundExpr::Equal);
							equalNode->SetLeft(ifcond_node);
							equalNode->SetRight(node);
							ifcond_node = equalNode;
						}
					}
				}
			}
			if (ifcond_node)
			{
				// Forward condition
				std::shared_ptr<CCompoundExpr> ifCond2(ifcond_node);
				CExprValue forwardEV = ce->GetExprValue();
				forwardEV.SetCondForward();
				AddEqualCond(tempEqualCond, forwardEV, ifCond2);
			}
		}
	}


	// if(bSucc && p == NULL)
	// { p!=NULL, L  }
	// bSucc != 0 => p != NULL;

	if (listCE.size() == 2)
	{
		CCompoundExpr* ce1 = listCE[0];
		CCompoundExpr* ce2 = listCE[1];
		if (ce1->GetParent()->GetConj() == CCompoundExpr::And)
		{
			if (ce1->GetExprValue().GetValueInfo().IsNotEqualZero() && ce2->GetExprValue().GetValueInfo().IsEqualZero())
			{
				if (m_localVar.HasTempInit(ce2->GetExprValue().GetExpr()))
				{
					const CExprValue& evInit = m_localVar.InitStatus_temp[ce2->GetExprValue().GetExpr()];
					if (evInit.GetValueInfo().IsNotEqualZero())
					{
						CExprValue evEq = ce1->GetExprValue();
						CCompoundExpr* ceEq = ce2->DeepCopy();
						ceEq->GetExprValue().NegationValue();
						std::shared_ptr<CCompoundExpr> sqCE(ceEq);
						SEqualCond ecEq(evEq, sqCE);
						tempEqualCond.insert(std::pair<SExpr, SEqualCond>(evEq.GetExpr(), ecEq));
					}
				}

				// for if(a && !a->b) { a->b != 0 [L]; }
				if (CheckExprIsSubField(ce1->GetExprValue().GetExpr(), ce2->GetExprValue().GetExpr()))
				{
					if (m_localVar.HasPreCond(ce2->GetExprValue().GetExpr()))
					{
						CExprValue ev3 = m_localVar.PreCond[ce2->GetExprValue().GetExpr()];

						if (ev3.GetValueInfo().IsNotEqualZero() && ev3.IsCondLogic())
						{
							CCompoundExpr* ceEq = new CCompoundExpr;
							ceEq->SetExprValue(ev3);
							std::shared_ptr<CCompoundExpr> sqCE(ceEq);
							SEqualCond ecEq(ce2->GetExprValue(), sqCE);
							tempEqualCond.insert(std::pair<SExpr, SEqualCond>(ev3.GetExpr(), ecEq));
						}
					}
				}
			}
		}
	}
}

void CheckTSCNullPointer2::ExtractEqualCondReturn(const Token* tok, std::shared_ptr<CCompoundExpr> &ifCond, SLocalVar &topLocalVar, MMEC& tempEqualCond)
{
	// 如果只有单独的if(!p && !q){ return; }
	// 才展开以下等价条件
	//bool bSingleIf = false;
	//if (const Token* tok1 = tok->link())
	//{
	//	if (const Token* tok2 = tok1->previous())
	//	{
	//		if (tok2->str() == ")")
	//		{
	//			if (const Token* tok3 = tok2->link())
	//			{
	//				if (const Token* tok4 = tok3->previous())
	//				{
	//					if (tok4->str() == "if")
	//					{
	//						if (!Token::Match(tok4->tokAt(-2), "else {"))
	//						{
	//							bSingleIf = true;
	//						}
	//					}
	//				}
	//			}
	//		}			
	//	}
	//}

	//if (!bSingleIf)
	//{
	//	return;
	//}

	std::vector<CCompoundExpr*> listCE;
	ifCond->ConvertToList(listCE);
	bool bAnd, bOr;
	// hanlde if(p1 == 0 && p2 == 0) {} else if(p1 == 0) {} else if (p2 == 0) {}
	if (listCE.size() == 2)
	{
		CCompoundExpr* ce1 = listCE[0];
		CCompoundExpr* ce2 = listCE[1];

		{
			ce1->GetCondjStatus(bAnd, bOr);
			if (!bOr && bAnd)
			{
				CExprValue newEV1 = ce1->GetExprValue();
				CCompoundExpr* newCE2 = ce2->DeepCopy();
				newCE2->GetExprValue().NegationValue();

				std::shared_ptr<CCompoundExpr> spCE2(newCE2);
				newEV1.SetCondForward();
				tempEqualCond.insert(std::pair<SExpr,SEqualCond>(newEV1.GetExpr(), SEqualCond(newEV1, spCE2)));

				CExprValue newEV2 = ce2->GetExprValue();
				CCompoundExpr* newCE1 = ce1->DeepCopy();
				newCE1->GetExprValue().NegationValue();

				std::shared_ptr<CCompoundExpr> spCE1(newCE1);
				newEV2.SetCondForward();
				tempEqualCond.insert(std::pair<SExpr, SEqualCond>(newEV2.GetExpr(), SEqualCond(newEV2, spCE1)));
			}
		}
	}

}

void CheckTSCNullPointer2::UpdateAssignedPreCond(SLocalVar &topLocalVar)
{
	for (MEV_I I = topLocalVar.PreCond.begin(), E = topLocalVar.PreCond.end(); I != E;)
	{
		const SExpr& expr = I->first;
		if (I->second.IsCondInit())
		{

			// commnet this block, as the side-effect is obvious
			// if is assigned with a certain value, should not remove the pre cond
			if (I->second.GetValueInfo().IsNotEqualZero())
			{
				++I;
				continue;
			}
			if (m_localVar.HasPreCond(expr))
			{
				const CExprValue& ev = m_localVar.PreCond[expr];
				if (ev != I->second)
				{
					topLocalVar.PreCond.erase(I++);
					continue;
				}
			}
			else// if (m_localVar.HasTempInit(expr))
			{
				topLocalVar.PreCond.erase(I++);
				continue;
			}
		}
		++I;
	}
}

void CheckTSCNullPointer2::ExtractDerefedExpr(const Token* tok,const Token* tokKey, std::vector<SExprLocation>& derefed)
{
	if (!tok)
		return;
	
	if (tok->astOperand1())
	{
		ExtractDerefedExpr(tok->astOperand1(), tokKey, derefed);
	}
	if (tok->astOperand2())
	{
		ExtractDerefedExpr(tok->astOperand2(), tokKey, derefed);
	}
	
	if (IsDerefTok(tok))
	{
		SExprLocation exprLoc;
		if (TryGetDerefedExprLoc(tok, exprLoc, _tokenizer->getSymbolDatabase()))
		{
			derefed.push_back(exprLoc);
			//try to handle deref(in if,for,while) to report error
			//if (tokKey->str() != "do")
			//{
			//	HandleDeref(exprLoc,true);
			//}

		}
	}

}

bool CheckTSCNullPointer2::TryGetDerefedExprLoc(const Token* tok, SExprLocation& exprLoc, const SymbolDatabase* symbolDB)
{
	exprLoc = SExprLocation::EmptyExprLoc;
	if (tok->str() == "[" || tok->str() == "." || tok->str() == "*")
	{
		const Token* ltok = tok->astOperand1();
		if (ltok)
		{
			//handle *values++ , *values--
			if ((ltok->str() == "++" || ltok->str() == "--") && ltok->astOperand1())
			{
				//except *++values,*--valuse
				if (ltok->next() != ltok->astOperand1())
				{
					ltok = ltok->astOperand1();
				}
			}
			//eg.LOG ( ERROR , " err(code:%s,msg:%s)\n" , string ( returnCode@7 . value ( ) ) . c_str ( ) , string ( description@10 . value ( ) ) . c_str ( ) ) ;
			exprLoc = CreateSExprByExprTok(ltok);
		}
		//ast may fail
		else if(tok->str()==".")
		{
			//ROTATE_LOG ( $g_unicom_music_log , ## $ERROR , $" err(code:%s,msg:%s)\n" $, $string ( $returnCode@7 . $value ( ) ) . $c_str ( ) , $string ( $description@10 . $value ( ) ) . $c_str ( ) ) ;
			exprLoc = CreateSExprByExprTok(tok->previous());
		}
	}
	else if (tok->str() == "<<" || tok->str() == ">>")
	{
		if (const Token* tok2 = tok->astOperand2())
		{
			exprLoc = CreateSExprByExprTok(tok2);
		}
	}
	return !exprLoc.Empty();
}

void CheckTSCNullPointer2::GetForLoopIndex(const Token* tokFor, std::vector<SExprLocation>& indexList)
{
	if (!tokFor || tokFor->str() != "for")
	{
		return;
	}

	if (const Token* tokP = tokFor->astParent())
	{
		if (tokP->str() == "(")
		{
			if (const Token* tok2 = tokP->astOperand2())
			{
				if (tok2->str() == ";")
				{
					if (const Token* tok3 = tok2->astOperand2())
					{
						if (tok3->str() == ";")
						{
							if (const Token* tokLoopIndex = tok3->astOperand2())
							{
								GetForLoopIndexByLoopTok(tokLoopIndex, indexList);
							}
						}
					}
				}
			}
		}
	}
}

void CheckTSCNullPointer2::ExtractDerefedEqualCond(std::shared_ptr<CCompoundExpr> &ifCond, SLocalVar &topLocalVar, MMEC& tempEqualCond, bool hasElse)
{
	for (MEV_CI I = m_localVar.InitStatus_temp.begin(), E = m_localVar.InitStatus_temp.end(); I != E; ++I)
	{
		SExpr expr = I->first;
		CExprValue evInit = I->second;
		if (evInit.IsCertainAndEqual())
		{
			if (!ifCond->HasSExpr(expr))
			{
				if (topLocalVar.HasInitStatus(expr))
				{
					// extract equal cond of derefed exprs
					std::vector<SExprLocation>& derefed = topLocalVar.GetIfCondDerefed();
					if (!derefed.empty())
					{
						CCompoundExpr* ifCondDerefed = nullptr;

						for (std::vector<SExprLocation>::iterator I2 = derefed.begin(), E2 = derefed.end(); I2 != E2; ++I2)
						{

							CExprValue newEV(*I2, CValueInfo(0, false));

							//if (topLocalVar.HasPreCond(newEV.GetExpr()))
							//{
							//	CExprValue topEV = topLocalVar.PreCond[newEV.GetExpr()];
							//	if (topEV.IsCertain())
							//	{
							//		continue;
							//	}
							//}
							CCompoundExpr* temp = new CCompoundExpr;
							temp->SetExprValue(newEV);
							if (ifCondDerefed == nullptr)
							{
								ifCondDerefed = temp;
							}
							else
							{
								CCompoundExpr* ceAnd = new CCompoundExpr;
								ceAnd->SetConj(CCompoundExpr::And);
								ceAnd->SetLeft(ifCondDerefed);
								ceAnd->SetRight(temp);
								ifCondDerefed = ceAnd;
							}
						}

						if (ifCondDerefed != nullptr)
						{
							
							bool merged = false;
							MMEC_I tempI = tempEqualCond.find(expr);
							while (tempI != tempEqualCond.end() && tempI->first == expr)
							{
								if (tempI->second.EV == evInit)
								{
									CCompoundExpr* ceMerged = nullptr;
									CCompoundExpr* ceOld = tempI->second.EqualCE.get();
									MergeTwoIfCond(ceOld, ifCondDerefed, CCompoundExpr::And, ceMerged);
									if (ceMerged != nullptr)
									{
										std::shared_ptr<CCompoundExpr> ifCond2(ceMerged);
										tempI->second.EqualCE = ifCond2;
										merged = true;
										break;
									}
								}
								++tempI;
							}

							if (!merged)
							{
								std::shared_ptr<CCompoundExpr> ifCond2(ifCondDerefed);
								AddEqualCond(tempEqualCond, evInit, ifCond2);
							}

						}
					}
				}
			}
		}
	}		
}

const Token* CheckTSCNullPointer2::HandleOneToken_deref(const Token * tok)
{
	TryDump(tok);
	
	// handle deref, todo, handle deref in subfunc , handle deref in sysfunc 
	if (IsDerefTok(tok))
	{
		HandleDerefTok(tok);
	}
	

	return tok;
}


const Token* CheckTSCNullPointer2::HandleOneToken(const Token * tok)
{
	TryDump(tok);
	if (tok->str() == "{")
	{
		UpdateLocalVarWhenScopeBegin();
	}
	else if (tok->str() == "}")
	{
		UpdateLocalVarWhenScopeEnd(tok);
	}
	else if (Token::Match(tok, "if|while|for|do"))
	{
		tok = HandleCheckNull(tok);
	}
	else if (tok->str() == "else")
	{
		IfCondElse();
	}
	else if (tok->str() == "=")
	{
		tok = HandleAssignOperator(tok);
	}
	else if (tok->str() == "(")
	{
		tok = HandleFuncParam(tok);
	}
	// handle return,break,continue,and so on, todo, handle user custom jump codes
	else if (CGlobalTokenizer::Instance()->CheckIfReturn(tok))
	{
		HandleReturnToken(tok);
	}
	// handle deref, todo, handle deref in subfunc , handle deref in sysfunc 
	else if (IsDerefTok(tok))
	{
		HandleDerefTok(tok);
	}
	else if (tok->str() == "." && tok->originalName().empty())
	{
		HanldeDotOperator(tok);
	}
	//else if (tok->str() == "assert")
	else if (_settings->CheckIfJumpCode(tok->str()))
	{
		HandleJumpCode(tok);
	}
	//not deref, possible array definition
	else if (tok->str() == "[")
	{
		HandleArrayDef(tok);
	}
	else if (Token::Match(tok, "swap|reset"))
	{
		tok = HandleSmartPointer(tok);
	}
	else if (tok->str() == "++" || tok->str() == "--")
	{
		HandleSelfOperator(tok);
	}
	else if (Token::simpleMatch(tok, "catch ("))
	{
		tok = HandleTryCatch(tok);
	}
	else if (IsEmbeddedClass(tok))
	{
		tok = tok->linkAt(2)->next();
	}

	//else if (Token::Match(tok, "case|default"))
	//{
	//	tok = HandleCase(tok);
	//}

	return tok;
}

bool CheckTSCNullPointer2::IsParameter(const Token* tok)
{
	if (!tok)
	{
		return false;
	}
	if (const Variable* pVar = tok->variable())
	{
		return pVar->isArgument();
	}
	return false;
}

bool CheckTSCNullPointer2::IsClassMember(const Token* tok)
{
	if (!tok)
	{
		return false;
	}
	if (const Variable* pVar = tok->variable())
	{
		return !pVar->isLocal();
	}
	return false;
}

bool CheckTSCNullPointer2::IsIterator(const Token* tok)
{
	if (!tok)
		return false;

	SExprLocation exprLoc = CreateSExprByExprTok(tok);
	if (exprLoc.Expr.VarId)
	{
		const Variable* pVar = exprLoc.GetVariable();
		if (pVar)
		{
			return (pVar->typeEndToken()->str() == "iterator" || pVar->typeEndToken()->str() == "auto");
		}
	}

	return false;
}

bool CheckTSCNullPointer2::IsIteratorBeginOrEnd(const Token* tok, bool bBegin)
{
	if (!tok)
		return false;

	if (tok->str() == "(")
	{
		if (const Token* tok2 =tok->astOperand1())
		{
			if (tok2->str() == ".")
			{
				if (const Token* tok3 = tok2->astOperand2())
				{
					if (tok3->str() == (bBegin ? "begin" : "end") || tok3->str() == (bBegin ? "rbegin" : "rend") || tok3->str() == (bBegin ? "cbegin" : "cend") || tok3->str() == (bBegin ? "crbegin" : "crend"))
					{
						if (tok2->astOperand1())
						{
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

#pragma endregion

void CCompoundExpr::ConvertToList(std::vector<CCompoundExpr*>& listCE, bool includeExpand)
{
	if (IsUnknownOrEmpty())
		return;
	
	std::stack<CCompoundExpr*> stackCE;
	stackCE.push(this);
	while (!stackCE.empty())
	{
		CCompoundExpr* ce = stackCE.top();
		if (ce->IsConjNode())
		{
			
		}
		else
		{
			if (!ce->IsUnknownOrEmpty())
				listCE.push_back(ce);
		}
		stackCE.pop();
		
		if (ce->GetConj() == ConjType::Equal && !includeExpand)
		{
		}
		else
		{
			if (ce->GetRight())
			{
				stackCE.push(ce->GetRight());
			}
		}
		
		if (ce->GetLeft())
		{
			stackCE.push(ce->GetLeft());
		}
	}
}

bool CCompoundExpr::HasSExpr(const SExpr& expr, CCompoundExpr** ce)
{
	if (IsConjNode())
	{
		if (m_left && m_left->HasSExpr(expr, ce))
		{
			return true;
		}
		else if (m_right && m_right->HasSExpr(expr, ce))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else if (IsUnknown())
	{
		return false;
	}
	else
	{
		if (m_ev.GetExpr() == expr)
		{
			if (ce != nullptr)
			{
				*ce = this;
			}
			return true;
		}
		return false;
	}
	
}

void CCompoundExpr::ReplaceSExpr(const SExpr& expr, const SExpr& newExpr)
{
	if (IsConjNode())
	{
		if (m_left)
		{
			m_left->ReplaceSExpr(expr, newExpr);
		}
		if (m_right)
		{
			m_right->ReplaceSExpr(expr, newExpr);
		}
	}
	else if (IsUnknown())
	{
	}
	else
	{
		if (m_ev.GetExpr() == expr)
		{
			m_ev.SetExpr(newExpr);
		}
	}
}


bool CCompoundExpr::IsEqual(const CCompoundExpr* ce)
{
	if (ce == nullptr)
	{
		return false;
	}
	if (ce->GetConj() != GetConj())
	{
		return false;
	}

	if (IsUnknown() ^ ce->IsUnknown())
	{
		return false;
	}
	else if (IsUnknown() && ce->IsUnknown())
	{
		return true;
	}


	if ((m_left == nullptr) ^ (ce->m_left == nullptr))
	{
		return false;
	}
	if ((m_right == nullptr) ^ (ce->m_right == nullptr))
	{
		return false;
	}

	if (IsConjNode())
	{
		if (m_left)
		{
			if (!m_left->IsEqual(ce->m_left))
			{
				return false;
			}
		}

		if (m_right)
		{
			if (!m_right->IsEqual(ce->m_right))
			{
				return false;
			}
		}
	}
	else
	{
		if (m_ev != ce->m_ev)
		{
			return false;
		}
	}

	return true;
}

bool CCompoundExpr::IsExpanded() const
{
	bool bLeft = true;
	const CCompoundExpr* temp = this;
	const CCompoundExpr* p = temp;
	bool beEqual = false;
	while (p->GetParent())
	{
		temp = p;
		p = p->GetParent();
		if (p->GetConj() == CCompoundExpr::Equal)
		{
			bLeft = (p->GetLeft() == temp);
			beEqual = true;
		}
	}

	if (beEqual && !bLeft)
		return true;

	return false;
}

void CCompoundExpr::DeleteSelf(CCompoundExpr* ce, CCompoundExpr*& top)
{
	CCompoundExpr* p = ce->GetParent();
	if (p == nullptr)
	{
		ce->SetEmpty();
	}
	else
	{
		bool bLeft = (p->GetLeft() == ce);

		CCompoundExpr* pp = p->GetParent();
		if (pp)
		{
			bool bLeft2 = (pp->GetLeft() == p);
			if (bLeft2)
			{
				if (bLeft)
				{
					pp->SetLeft(p->GetRight());
				}
				else
				{
					pp->SetLeft(p->GetLeft());
				}
			}
			else
			{
				if (bLeft)
				{
					pp->SetRight(p->GetRight());
				}
				else
				{
					pp->SetRight(p->GetLeft());
				}
			}
		}
		else
		{
			if (bLeft)
				top = top->GetRight();
			else
				top = top->GetLeft();

			top->SetParent(nullptr);
		}
		p->SetLeft(nullptr);
		p->SetRight(nullptr);
		ce->SetLeft(nullptr);
		ce->SetRight(nullptr);
		SAFE_DELETE(p);
		SAFE_DELETE(ce);
	}
}

void SLocalVar::EraseEqualCondBySExpr(const SExpr& expr)
{
	MMEC_I I = EqualCond.begin();
	while (I != EqualCond.end())
	{
		const SExpr& key = I->first;
		SEqualCond& ec = I->second;
		if (key == expr)
		{
			//I = EqualCond.erase(I);
			EqualCond.erase(I++);
			continue;
		}
		else
		{
			if (ec.EqualCE->HasSExpr(expr))
			{
				EqualCond.erase(I++);
				continue;
			}
		}
		++I;
	}
}

bool SLocalVar::MergeInitStatus(const SExpr& expr, const CExprValue& value)
{
	return false;
}


void SLocalVar::ResetExpr(const SExpr& expr)
{
	EraseDerefedStatus(expr);
	ErasePreCond(expr);
	EraseEqualCondBySExpr(expr);
}

void SLocalVar::SwapMapKeyValue(const SExprLocation& expr1, const SExprLocation& expr2, MEV &data)
{
	MEV_I iter1 = data.find(expr1.Expr);
	MEV_I iter2 = data.find(expr2.Expr);
	if (iter1 != data.end())
	{
		if (iter2 != data.end())
		{
			CExprValue v = iter2->second;
			data[expr2.Expr] = iter1->second;
			data[expr1.Expr] = v;
			CExprValue::SwapLoc(data[expr2.Expr], data[expr1.Expr]);
		}
		else
		{
			CExprValue v = iter1->second;
			v.SetExpLoc(expr2);//conflict with function name
			data.erase(iter1);
			data.insert(std::make_pair(expr2.Expr, v));
		}
	}
	else
	{
		if (iter2 != data.end())
		{
			CExprValue v = iter2->second;
			v.SetExpLoc(expr1);
			data.erase(iter2);
			data.insert(std::make_pair(expr1.Expr, v));
		}
	}
}


void SLocalVar::SwapMMapKeyValue(const SExprLocation& expr1, const SExprLocation& expr2, MMEC &data)
{
	typedef std::pair<MMEC_I, MMEC_I> mrange;
	mrange r1 = data.equal_range(expr1.Expr);
	mrange r2 = data.equal_range(expr2.Expr);

	if (r1.first == r1.second)
	{
		if (r2.first == r2.second)
		{
			//do nothing
		}
		else
		{
			std::vector<SEqualCond> v;
			for (MMEC_I iter = r2.first; iter != r2.second; ++iter)
			{
				v.push_back(iter->second);
			}
			data.erase(r2.first, r2.second);
			for (std::vector<SEqualCond>::iterator iter = v.begin(), end = v.end(); iter != end;  ++iter)
			{
				iter->EV.SetExpLoc(expr1);
				iter->EqualCE->ReplaceSExpr(expr2.Expr, expr1.Expr);
				data.insert(std::make_pair(expr1.Expr, *iter));
			}
		}
	}
	else
	{
		if (r2.first == r2.second)
		{
			std::vector<SEqualCond> v;
			for (MMEC_I iter = r1.first; iter != r1.second; ++iter)
			{
				v.push_back(iter->second);
			}
			data.erase(r1.first, r1.second);
			for (std::vector<SEqualCond>::iterator iter = v.begin(), end = v.end(); iter != end; ++iter)
			{
				iter->EV.SetExpLoc(expr2);
				iter->EqualCE->ReplaceSExpr(expr1.Expr, expr2.Expr);
				data.insert(std::make_pair(expr2.Expr, *iter));
			}
		}
		else
		{
			std::vector<SEqualCond> v1;
			for (MMEC_I iter = r1.first; iter != r1.second; ++iter)
			{
				v1.push_back(iter->second);
			}
			data.erase(r1.first, r1.second);
			mrange r3 = data.equal_range(expr2.Expr);
			std::vector<SEqualCond> v2;
			for (MMEC_I iter = r3.first; iter != r3.second; ++iter)
			{
				v2.push_back(iter->second);
			}
			data.erase(r3.first, r3.second);
			for (std::vector<SEqualCond>::iterator iter = v1.begin(), end = v1.end(); iter != end; ++iter)
			{
				iter->EV.SetExpLoc(expr2);
				iter->EqualCE->ReplaceSExpr(expr1.Expr, expr2.Expr);
				data.insert(std::make_pair(expr2.Expr, *iter));
			}
			for (std::vector<SEqualCond>::iterator iter = v2.begin(), end = v2.end(); iter != end; ++iter)
			{
				iter->EV.SetExpLoc(expr1);
				iter->EqualCE->ReplaceSExpr(expr2.Expr, expr1.Expr);
				data.insert(std::make_pair(expr1.Expr, *iter));
			}
		}
	}
}

void SLocalVar::SwapCondition(const SExprLocation& expr1, const SExprLocation& expr2)
{
	//Get all Condition of expr1
	SwapMapKeyValue(expr1, expr2, InitStatus);
	SwapMapKeyValue(expr1, expr2, InitStatus_temp);
	SwapMapKeyValue(expr1, expr2, PreCond);
	SwapMapKeyValue(expr1, expr2, PreCond_if);
	SwapMapKeyValue(expr1, expr2, PossibleCond);
	SwapMMapKeyValue(expr1, expr2, EqualCond);
	SwapMMapKeyValue(expr1, expr2, EqualCond_if);
	/*std::vector<SEqualCond> v1;
	std::vector<SEqualCond> v2;
	for (MMEC_I I = EqualCond.begin(), end = EqualCond.end(); I != end;)
	{
		const SExpr& key = I->first;
		SEqualCond& ec = I->second;
		if (key == expr1)
		{
			v1.push_back(ec);
			EqualCond.erase(I++);
		}
		else if (key == expr2)
		{
			v2.push_back(ec);
			EqualCond.erase(I++);
		}
		else
		{
			if (ec.EqualCE->HasSExpr(expr1))
			{
				ec.EqualCE->ReplaceSExpr(expr1, expr2);
			}
			else if (ec.EqualCE->HasSExpr(expr2))
			{
				ec.EqualCE->ReplaceSExpr(expr2, expr1);
			}
		}
		++I;
	}
	for (std::vector<SEqualCond>::iterator iter = v1.begin(), end = v1.end(); iter != end; ++iter)
	{
		EqualCond.insert(std::make_pair());
	}
	*/
}

void SLocalVar::AddEqualCond(const CExprValue& ev, std::shared_ptr<CCompoundExpr>& ce, bool bHandleConflict)
{
    if (bHandleConflict)
    {
        for (MMEC_I I = EqualCond.find(ev.GetExpr()); I != EqualCond.end() && I->first == ev.GetExpr();++I)
        {
            if (CExprValue::IsExprValueConflictedForEqualCond(I->second.EV, ev))
            {
                return;
            }
        }
    }

	EqualCond.insert(PEC(ev.GetExpr(), SEqualCond(ev, ce)));
}

void SLocalVar::AddEqualCond_if(const CExprValue& ev, std::shared_ptr<CCompoundExpr>& ce, bool bHandleConflict)
{
	if (bHandleConflict)
	{
		for (MMEC_I I = EqualCond_if.find(ev.GetExpr()); I != EqualCond_if.end() && I->first == ev.GetExpr(); ++I)
		{
			if (CExprValue::IsExprValueConflictedForEqualCond(I->second.EV, ev))
			{
				return;
			}

		}
	}

	EqualCond_if.insert(PEC(ev.GetExpr(), SEqualCond(ev, ce)));
}

bool SLocalVar::HasSameEqualCond(const SEqualCond& ec)
{
	const SExpr& expr = ec.EV.GetExpr();
	for (MMEC_CI I = EqualCond.find(expr), E = EqualCond.end(); I != E && I->first == expr;++I)
	{
		if (I->second.EV == ec.EV)
		{
			if (I->second.EqualCE->IsEqual(ec.EqualCE.get()))
			{
				return true;
			}
			
		}
	}
	return false;
}

void SLocalVar::AddPreCond(const CExprValue& ev, bool force /*= false*/)
{
	if (!force)
	{
		if (HasPreCond(ev.GetExpr()))
		{
			CExprValue& oldEV = PreCond[ev.GetExpr()];
			if (oldEV.IsCertain() && !oldEV.IsInitVar())//经验性IsInitVar推断出的条件可以覆盖，推断确定且不是经验性条件的不覆盖
			{
				//return false;
				return;
			}
		}
	}

	PreCond[ev.GetExpr()] = ev;
	//return true;
	return;
}

void SLocalVar::EraseSubFiled(const SExpr& expr)
{
	for (MDEREF_I I = DerefedMap.begin(), E = DerefedMap.end(); I != E;)
	{
		const SExpr& sub = I->first;
		bool bRet = CheckExprIsSubField(expr, sub);
		if (bRet)
		{
			DerefedMap.erase(I++);
			continue;
		}
		++I;
	}

	for (MEV_I I = PreCond.begin(), E = PreCond.end(); I != E;)
	{
		const SExpr& sub = I->first;
		bool bRet = CheckExprIsSubField(expr, sub);
		if (bRet)
		{
			EraseEqualCondBySExpr(sub);
			PreCond.erase(I++);
			continue;
		}
		++I;
	}

	for (MEV_I I = InitStatus.begin(), E = InitStatus.end(); I != E;)
	{
		const SExpr& sub = I->first;
		bool bRet = CheckExprIsSubField(expr, sub);
		if (bRet)
		{
			EraseEqualCondBySExpr(sub);
			InitStatus.erase(I++);
			continue;
		}
		++I;
	}

	for (MEV_I I = InitStatus_temp.begin(), E = InitStatus_temp.end(); I != E;)
	{
		const SExpr& sub = I->first;
		bool bRet = CheckExprIsSubField(expr, sub);
		if (bRet)
		{
			EraseEqualCondBySExpr(sub);
			InitStatus_temp.erase(I++);
			continue;
		}
		++I;
	}

	for (MEV_I I = PossibleCond.begin(), E = PossibleCond.end(); I != E;)
	{
		const SExpr& sub = I->first;
		bool bRet = CheckExprIsSubField(expr, sub);
		if (bRet)
		{
			EraseEqualCondBySExpr(sub);
			PossibleCond.erase(I++);
			continue;
		}
		++I;
	}
}

bool CValueInfo::IsOpposite(const CValueInfo& vi) const
{
	if (m_possible || vi.m_possible)
	{
		return false;
	}
	if (m_unknown || vi.m_unknown)
	{
		return false;
	}

	if (m_bStrValue != vi.m_bStrValue)
	{
		return false;
	}

	return (m_equal != vi.m_equal) && (m_bStrValue ? ( m_sValue == vi.m_sValue ) : ( m_value == vi.m_value ) );
}

bool CheckTSCNullPointer2::SimplifyIfCond2(CCompoundExpr* cond, CCompoundExpr*& top)
{
	//if (p != 0 || p != 0 && q)
	// 	[IF]	[p] == 0 || [p] == 0 ## [getp()] == 0
	if (cond->GetConj() == CCompoundExpr::Or && cond->GetRight()->GetConj() == CCompoundExpr::And)
	{
		if (cond->GetLeft()->GetExprValue() == cond->GetRight()->GetLeft()->GetExprValue())
		{
			CCompoundExpr::DeleteSelf(cond->GetRight(), top);
		}
		
			
	}

	return true;
}

// if(!p->pp) { func(p); }
// p->pp !=0, L
bool CheckTSCNullPointer2::MakeSubFieldNotNull(const Token *tok)
{

	bool bHandled = false;
	SExprLocation exprLoc = CreateSExprByExprTok(tok);
	if (exprLoc.Empty())
	{
		return bHandled;
	}

	for (MEV_CI iter = m_localVar.PreCond.begin(), end = m_localVar.PreCond.end(); iter != end; ++iter)
	{
		if (CheckExprIsSubField(exprLoc.Expr, iter->first))
		{
			if (iter->second.CanBeNull())
			{
				CValueInfo vi(0, false);
				CExprValue ev(iter->second.GetExprLoc(), vi);
				ev.SetCondLogic();
				m_localVar.AddPreCond(ev, true);
				m_localVar.AddInitStatus(ev, true);
				bHandled = true;
				break; // todo :please reconsider this line of code
			}
		}
	}

	return bHandled;
}




bool CheckTSCNullPointer2::SimplifyIfCond3(CCompoundExpr* cond, CCompoundExpr*& top)
{
	//handle if(p == NULL && q !=NULL && CreateCommonObject(&p))
	bool bSimp = true;
	while (bSimp)
	{
		bSimp = false;
		std::vector<CCompoundExpr*>listCE;
		top->ConvertToList(listCE);
		for (std::vector<CCompoundExpr*>::iterator I = listCE.begin(), E = listCE.end(); I != E; ++I)
		{
			CCompoundExpr* ce = *I;
			if (ce->GetExprValue().GetValueInfo().IsEqualZero())
			{
				// 从该节点开始往右边找getp(&p)
				CCompoundExpr* node = ce->GetParent();
				while (node && node->GetConj() == CCompoundExpr::And)
				{

					CCompoundExpr* findCE = SimplifyIfCond3_FindGetP(ce, node->GetRight());
					if (findCE)
					{
						ce->GetExprValue().NegationValue();
						CCompoundExpr::DeleteSelf(findCE, top);
						bSimp = true;
						break;
					}
					node = node->GetParent();
				}

				if (bSimp)
				{
					break;
				}
			}
		}
	}
	
	//if (cond->GetConj() == CCompoundExpr::And)
	//{
	//	if (cond->GetLeft()->IsUnknownOrEmpty() || cond->GetRight()->IsUnknownOrEmpty() || cond->GetLeft()->IsConjNode() || cond->GetRight()->IsConjNode())
	//	{
	//		return false;
	//	}
	//	CExprValue& lValue = cond->GetLeft()->GetExprValue();
	//	CExprValue& rValue = cond->GetRight()->GetExprValue();
	//	const Token* lTopTok = lValue.GetExprLoc().TokTop;
	//	const Token* rTopTok = rValue.GetExprLoc().TokTop;
	//	std::string lStr = lValue.GetExpr().ExprStr;
	//	std::string lStr1 = lStr + ",";
	//	std::string lStr2 = lStr + ")";
	//	std::string rStr = rValue.GetExpr().ExprStr;

	//	if (lValue.GetValueInfo().IsEqualZero() && rTopTok && rTopTok->str() == "(")
	//	{
	//		if (strstr(rStr.c_str(), lStr1.c_str()) || strstr(rStr.c_str(), lStr2.c_str()))
	//		{
	//			lValue.NegationValue();
	//			CCompoundExpr::DeleteSelf(cond->GetRight(), top);
	//		}

	//	}
	//}

	return true;
}


CCompoundExpr* CheckTSCNullPointer2::SimplifyIfCond3_FindGetP(CCompoundExpr* ce, CCompoundExpr* findNode)
{
	if (!ce || !findNode || (ce == findNode))
	{
		return nullptr;
	}

	if (findNode->IsConjNode())
	{
		if (findNode->GetConj() != CCompoundExpr::And)
		{
			return nullptr;
		}
		else
		{
			CCompoundExpr* findCE = SimplifyIfCond3_FindGetP(ce, findNode->GetLeft());
			if (findCE)
			{
				return findCE;
					 
			}
			findCE = SimplifyIfCond3_FindGetP(ce, findNode->GetRight());
			return findCE;
		}
	}
	else
	{
		const Token* rTopTok = findNode->GetExprValue().GetExprLoc().TokTop;
		if (rTopTok && rTopTok->str() == "(")
		{
			std::string rStr = findNode->GetExprValue().GetExpr().ExprStr;
			std::string lStr1 = ce->GetExprValue().GetExpr().ExprStr + ",";
			std::string lStr2 = ce->GetExprValue().GetExpr().ExprStr + ")";
			if (strstr(rStr.c_str(), lStr1.c_str()) || strstr(rStr.c_str(), lStr2.c_str()))
			{
				return findNode;
			}
		}
		return nullptr;
	}
}

bool CheckTSCNullPointer2::SimplifyIfCond4(CCompoundExpr* cond, CCompoundExpr*& top)
{
	//handle NP bug:if(!CreateCommonObject(&p) || p == NULL){ p = getp(); }
	//handle NP bug:if(CreateCommonObject(&p) || p != NULL){p->xx;}
	if (cond->GetConj() == CCompoundExpr::Or)
	{
		if (cond->GetLeft()->IsUnknownOrEmpty() || cond->GetRight()->IsUnknownOrEmpty() || cond->GetLeft()->IsConjNode() || cond->GetRight()->IsConjNode())
		{
			return false;
		}
		CExprValue& lValue = cond->GetLeft()->GetExprValue();
		CExprValue& rValue = cond->GetRight()->GetExprValue();
		const Token* lTopTok = lValue.GetExprLoc().TokTop;
		std::string lStr = lValue.GetExpr().ExprStr;
		std::string rStr = rValue.GetExpr().ExprStr;
		std::string rStr1 = rStr + ",";
		std::string rStr2 = rStr + ")";

		if (lTopTok && lTopTok->str() == "(")
		{
			if (strstr(lStr.c_str(), rStr1.c_str()) || strstr(lStr.c_str(), rStr2.c_str()))
			{
				CCompoundExpr::DeleteSelf(cond->GetLeft(), top);
			}
		}

	}

	return true;
}

void CheckTSCNullPointer2::checkNullDefectError(const SExprLocation& exprLoc)
{
	if (CheckIfExprLocIsArray(exprLoc))
	{
		return;
	}
	reportError(exprLoc.Begin, Severity::error, ErrorType::NullPointer, "checkNullDefect", 
		"It's wrong way to check if [" + exprLoc.Expr.ExprStr+ "] is null, this is usually caused by misusing && and ||, e.g. if(p != NULL || p->a == 42)" , ErrorLogger::GenWebIdentity(exprLoc.Expr.ExprStr));
}

void CheckTSCNullPointer2::checkMissingDerefOperator()
{
	if (!_settings->IsCheckIdOpened(ErrorType::ToString(ErrorType::NullPointer).c_str(), "missingDerefOperator"))
	{
		return;
	}

	for (std::vector<const Scope *>::const_iterator I = _tokenizer->getSymbolDatabase()->functionScopes.begin(),
		E = _tokenizer->getSymbolDatabase()->functionScopes.end(); I != E; ++I)
	{
		const Scope* s = *I;
		for (const Token* tok = s->classStart, *tokE = s->classEnd; tok && tok != tokE; tok = tok->next())
		{
			if (tok->str() != "=")
			{
				continue;
			}

			const Token* tokDeref = tok->astOperand1();
			if (!Token::simpleMatch(tokDeref, "*"))
			{
				continue;
			}

			const Token* tokVar = tokDeref->astOperand1();
			if (!tokVar)
			{
				continue;
			}

			SExprLocation elVar = CreateSExprByExprTok(tokVar);
			if (elVar.Empty())
			{
				continue;
			}

			// go to the end of the assignment sentence
			const Token* tok2 = Token::findsimplematch(tok, ";");
			if (tok2)
			{
				for (; tok2 && tok2 != tokE; tok2 = tok2->next())
				{
					if (Token::Match(tok2, "}|return|break|continue|goto"))
					{
						break;
					}

					if (tok2->str() != elVar.TokTop->str())
						continue;

					SExprLocation el2 = CreateSExprByExprTok(tok2);
					if (el2.Expr != elVar.Expr)
					{
						continue;
					}

					bool bCheckNull = false;
					const Token* tokP2 = el2.TokTop->astParent();
					if (tokP2 && tokP2->str() == "==")
					{
						bool bLeft = tokP2->astOperand1() == el2.TokTop;
						const Token* tokValue = bLeft ? (tokP2->astOperand2()) : (tokP2->astOperand1());
						if (tokValue && tokValue->str() == "0")
						{
							bCheckNull = true;
						}
					}

					if (!bCheckNull)
					{
						break;
					}
					else
					{
						reportMissingDerefOperatorError(elVar, el2);
					}
					
				}
			}
		}
	}

}

void CheckTSCNullPointer2::reportMissingDerefOperatorError(const SExprLocation& elAssign, const SExprLocation& elCheckNull)
{
	std::stringstream ss;
	ss << "Does [" << elCheckNull.Expr.ExprStr << "] missing dereference operator [*], cause at line [" << elAssign.TokTop->linenr()
		<< "] [ * " << elAssign.Expr.ExprStr << " ] is assigned.";
	reportError(elCheckNull.TokTop, Severity::error, ErrorType::NullPointer, "missingDerefOperator", ss.str().c_str(),
		ErrorLogger::GenWebIdentity(elCheckNull.Expr.ExprStr));

}
