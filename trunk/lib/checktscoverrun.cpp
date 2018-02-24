#include "check.h"
#include "checktscoverrun.h"
#include "symboldatabase.h"
#include "globaltokenizer.h"

std::map<std::string, int> CheckTSCOverrun::s_returnLengthFunc = make_container<std::map<std::string, int> >()
<< std::make_pair("read", 1);

void CheckTSCOverrun::checkFuncRetLengthUsage()
{
	const std::vector<const Scope *>& funcs = _tokenizer->getSymbolDatabase()->functionScopes;

	for (std::vector<const Scope *>::const_iterator I = funcs.begin(), E = funcs.end(); I != E; ++I)
	{
		const Scope* scope = *I;

		for (const Token* tok = scope->classStart; tok && tok != scope->classEnd; tok = tok->next())
		{
			if (s_returnLengthFunc.count(tok->str()))
			{
				const Token* tok2 = tok->next();
				if (tok2 && tok2->str() == "(")
				{
					if (tok2->astParent() && tok2->astParent()->str() == "=")
					{
						if (const Token* tokLen = tok2->astParent()->astOperand1())
						{
							if (tokLen && tokLen->varId())
							{
								const unsigned varid = tokLen->varId();
								int paramIndex = s_returnLengthFunc[tok->str()];

								std::vector<const Token*> params;
								getParamsbyAst(tok2, params);
								if (params.size() > (unsigned)paramIndex)
								{

									const Token* tokBuf = params[paramIndex];

									int index = 0;

									for (const Token* tok3 = tok->next(); tok3 && tok3 != scope->classEnd && index >= 0; tok3 = tok3->next())
									{
										if (tok3->varId() == varid)
										{
											if (Token::Match(tok3->previous(), "[,(&]|++|--"))
											{
												break;
											}
											else if (Token::Match(tok3->next(), "=|++|--"))
											{
												break;
											}
											else
											{
												const Token* tok4 = tok3->astParent();
												if (tok4 && tok4->str() == "[")
												{
													const Token* tok5 = tok4->astOperand1();

													if (tok5 && tok5->astString() == tokBuf->astString() && tok5->varId() == tokBuf->varId())
													{
														FuncRetLengthUsageError(tok3, tok->str());
													}
													break;
												}
											}


										}
										else if (tok->str() == "{")
										{
											++index;
										}
										else if (tok->str() == "}")
										{
											--index;
										}
									}
								}
							}
						}
					}
				}

			}
		}
	}
}

void CheckTSCOverrun::checkIndexPlusOneInFor()
{
	const std::vector<const Scope *>& funcs = _tokenizer->getSymbolDatabase()->functionScopes;

	for (std::vector<const Scope *>::const_iterator I = funcs.begin(), E = funcs.end(); I != E; ++I)
	{
		const Scope* scope = *I;
		checkIndexPlusOneInForByScope(scope);
	}
}

void CheckTSCOverrun::checkIndexBeforeCheck()
{
	const std::vector<const Scope *>& funcs = _tokenizer->getSymbolDatabase()->functionScopes;

	for (std::vector<const Scope *>::const_iterator I = funcs.begin(), E = funcs.end(); I != E; ++I)
	{
		const Scope* scope = *I;

		for (const Token* tok = scope->classStart; tok && tok != scope->classEnd; tok = tok->next())
		{

			if (tok->str() != "&&" && tok->str() != "||")
			{
				continue;
			}

			const Token* tok1 = tok->astOperand1();
			const Token* tok2 = tok->astOperand2();



			if (tok1 && tok2)
			{
				const Token* tok1s = tok1;
				while (tok1s->astOperand1())
				{
					tok1s = tok1s->astOperand1();
				}

				const Token* tokArr = Token::findmatch(tok1s, "%name% [ %var% ]", tok);


				if (tokArr)
				{
					const Token* tokIndex = tokArr->tokAt(2);
					if (tok2->str() == "<")
					{
						const Token* tok21 = tok2->astOperand1();
						const Token* tok22 = tok2->astOperand2();

						if (tok21->varId() == tokIndex->varId() && tok21->str() == tokIndex->str())
						{
							if (tok22->isNumber() || (tok22->isName() && !tok22->variable() && !Token::Match(tok22->next(), "(|.|[")))
							{
								IndexBeforeCheckError(tokIndex);
							}
						}
					}
				}
			}
		}
	}
}

extern bool iscast(const Token *tok);



void CheckTSCOverrun::checkIndexCheckDefect()
{
	const std::vector<const Scope *>& funcs = _tokenizer->getSymbolDatabase()->functionScopes;

	for (std::vector<const Scope *>::const_iterator I = funcs.begin(), E = funcs.end(); I != E; ++I)
	{
		const Scope* scope = *I;

		std::set<SExpr> checkedVar;
		for (const Token* tok = scope->classStart; tok && tok != scope->classEnd; tok = tok->next())
		{
			if (tok->str() != "[")
			{
				continue;
			}

			const Token* tok1 = tok->astOperand1();
			const Token* tok2 = tok->astOperand2();
			if (!tok1 || !tok2)
			{
				continue;
			}
			if (iscast(tok1))
			{
				continue;
			}
			SExprLocation elArray = CreateSExprByExprTok(tok1);
			SExprLocation elIndex = CreateSExprByExprTok(tok2);

			if (elArray.Empty() || elIndex.Empty())
			{
				continue;
			}

			if (checkedVar.count(elArray.Expr))
			{
				continue;
			}

			const Variable* varArr = elArray.GetVariable();


			gt::CField::Dimension d1;
			if (varArr)
			{
				if (varArr->nameToken() == elArray.TokTop)
				{
					continue;
				}
				if (varArr->isArray() && varArr->dimensions().size() == 1)
				{
					d1.Init(varArr->dimensions()[0]);
				}
			}
			else
			{
				const gt::CField* gtF = CGlobalTokenizer::Instance()->GetFieldInfoByToken(tok->previous());

				if (gtF && gtF->GetDimensions().size() == 1)
				{
					d1 = gtF->GetDimensions()[0];
				}
			}

			checkedVar.insert(elArray.Expr);

			if (!d1.IsKnown())
			{
				continue;
			}
			int layer1 = 0;
			int layer2 = 0;
			for (const Token* tok3 = elArray.Begin->previous(); tok3 && tok3 != scope->classStart; tok3 = tok3->previous())
			{
				if (tok3->str() == "}")
				{
					++layer2;
					tok3 = tok3->link();
					continue;
				}
				else if (tok3->str() == "{")
				{
					++layer1;
				}
				else if (CheckIndexTheSame(elIndex, tok3))
				{
					if (Token::Match(tok3->next(), " %op% %any%"))
					{
						const Token* tokOp = tok3->next();
						const Token* tokNum = tok3->tokAt(2);

						if (Token::Match(tokOp, "++|--"))
						{
							break;
						}

						if (tokOp->astOperand2() != tokNum)
						{
							continue;
						}

						if (tokNum->variable())
						{
							break;
						}

						gt::CField::Dimension d2;

						if (tokNum->isNumber())
						{
							if (!tokNum->values.empty())
							{
								d2.iNum = (int)tokNum->values.front().intvalue;
							}
						}
						else
						{
							d2.sNum = tokNum->str();
						}

						if (!d2.IsKnown())
						{
							continue;
						}

						if (d1.IsNum() ^ d2.IsNum())
						{
							continue;
						}


						const Token* tokTop = tokOp->astTop();
						if (!tokTop || tokTop->str() != "(")
						{
							continue;
						}

						const Token* tokCond = tokTop->astOperand1();
						if (!tokCond || (tokCond->str() != "for" && tokCond->str() != "if"))
						{
							continue;
						}

						bool bOK = false;
						bool bRet = false;
						if (tokCond->str() == "if" && layer1 == 0 && layer2 > 0)
						{
							if (tokOp->str() == ">")
							{
								if (d1.IsNum())
								{
									if (d1.iNum <= d2.iNum)
									{
										bOK = true;
									}
								}
								else
								{
									if (d1.sNum == d2.sNum)
									{
										bOK = true;
									}
								}
							}
							else if (tokOp->str() == ">=")
							{
								if (d1.IsNum())
								{
									if (d1.iNum < d2.iNum)
									{
										bOK = true;
									}
								}
							}
							else if (Token::Match(tokOp, "<|<="))
							{
								continue;
							}

							if (bOK)
							{
								bRet = IsScopeReturn(tokCond);
							}
						}
						else if ((tokCond->str() == "if" && layer1 > 0) || (tokCond->str() == "for" && layer1 > 0))
						{
							if (tokOp->str() == "<=")
							{
								if (d1.IsNum())
								{
									if (d1.iNum <= d2.iNum)
									{
										bOK = true;
									}
								}
								else
								{
									if (d1.sNum == d2.sNum)
									{
										bOK = true;
									}
								}
							}
							else if (tokOp->str() == "<")
							{
								if (d1.IsNum())
								{
									if (d1.iNum < d2.iNum)
									{
										bOK = true;
									}
								}
							}

							bRet = bOK;

						}

						if (bOK)
						{
							IndexCheckDefectError(elIndex, elArray, d1, tokCond);
						}

						break;
					}
				}
			}
		}
	}
}

bool CheckTSCOverrun::CheckIndexTheSame(SExprLocation elIndex, const Token* tok)
{
	if (tok->varId() != 0)
	{
		return elIndex.Expr.VarId == tok->varId();
	}
	else
	{
		const Token* tmp = elIndex.LastToken();
		const Token* end = elIndex.Begin->previous();
		while (tmp != end)
		{
			if (tmp->str() == tok->str())
			{
				tmp = tmp->previous();
				tok = tok->previous();

				if (tmp && tok)
				{
					continue;
				}
			}
			return false;
		}
		return true;
	}
}

void CheckTSCOverrun::checkIndexPlusOneInForByScope(const Scope* scope)
{

	const std::list<Scope*>& subScopes = scope->nestedList;
	for (std::list<Scope*>::const_iterator I2 = subScopes.begin(), E2 = subScopes.end(); I2 != E2; ++I2)
	{
		checkIndexPlusOneInForByScope(*I2);
	}

	const Scope* sFor = scope;
	if (sFor->GetScopeType() != Scope::eFor)
	{
		return;
	}

	const Token* tokFor = sFor->classDef;
	const Token* tokParenthesis = tokFor->astParent();
	if (tokParenthesis && tokParenthesis->str() == "(" && tokParenthesis->link())
	{
		const Token* tokParenthesis2 = tokParenthesis->link();
		const Token* sc1 = tokParenthesis->astOperand2();
		if (sc1 && sc1->str() == ";")
		{
			const Token* sc2 = sc1->astOperand2();
			if (sc2 && sc2->str() == ";")
			{
				// ast is intact, then go forward

				const Token* tokIndex = nullptr;
				const Token* tokVar = nullptr;
				// first check if there exists 'array[i + 1]'
				for (const Token* tok = sFor->classStart->next(); tok && tok != sFor->classEnd; tok = tok->next())
				{
					if (Token::Match(tok, "goto|return|continue|break"))
					{
						break;
					}
					else if (Token::Match(tok, "[ %name% + 1 ]"))
					{
						if (tok->next()->varId())
						{

							const Scope* s = tok->scope();
							bool bBreak = false;
							while (s && s != sFor)
							{
								if (s->GetScopeType() == Scope::eIf)
								{
									const Token* tok2 = Token::findmatch(s->classDef, "%name% <", s->classStart);
									if (tok2 && (tok2->str() == tok->next()->str()))
									{
										bBreak = true;
										break;
									}
                                    
                                    tok2 = Token::findmatch(s->classDef, "%name% + 1 <", s->classStart);
                                    if (tok2 && (tok2->str() == tok->next()->str()))
                                    {
                                        bBreak = true;
                                        break;
                                    }
								}
								s = s->nestedIn;
							}
							if (bBreak)
								break;

							tokIndex = tok->next();
							tokVar = tok->previous();


							break;
						}
					}
				}

				if (!tokIndex || !tokVar)
				{
					return;
				}

				std::string sLength;

				if (const Variable* pVar = tokVar->variable())
				{
					if (!pVar->dimensions().empty())
					{
						const Dimension& d = pVar->dimensions()[0];
						if (d.start && d.start == d.end)
						{
							sLength = d.start->str();
						}
					}
				}
				else
				{
					const gt::CField* gtF = CGlobalTokenizer::Instance()->GetFieldInfoByToken(tokVar);
					if (gtF && !gtF->GetDimensions().empty())
					{
						const gt::CField::Dimension d = gtF->GetDimensions()[0];
						sLength = d.Output();
					}
				}


				if (sLength.empty())
					return;


				//const Token* tokIndex = nullptr;
				// second check the index var
				bool bMatch = false;
				for (const Token* tok = sc2; tok && tok != tokParenthesis2; tok = tok->next())
				{
					if (Token::Match(tok, "++ %name%") && tok->next()->varId() == tokIndex->varId())
					{
						bMatch = true;
						break;
					}
					else if (Token::Match(tok, "%name% ++") && tok->varId() == tokIndex->varId())
					{
						bMatch = true;
						break;
					}
				}

				if (!bMatch)
				{
					return;
				}

				// second check if there has 'i < LENGTH'
				for (const Token* tok = sc1; tok && tok != sc2; tok = tok->next())
				{
					if (Token::Match(tok, "%varid% < %any% ;|&&", tokIndex->varId()) && tok->strAt(2) == sLength)
					{
						IndexPlusOneInForError(tokIndex);
					}
				}
			}
		}
	}
}

void CheckTSCOverrun::FuncRetLengthUsageError(const Token * tok, const std::string& func)
{
	std::stringstream ss;
	ss << "Index [" << tok->str() << "] which is returned form [" << func << "] may cause index out of bound.";
	reportError(tok, Severity::error, ErrorType::BufferOverrun, "funcRetLengthAsIndex", ss.str(), ErrorLogger::GenWebIdentity(tok->str()) );
}

void CheckTSCOverrun::IndexPlusOneInForError(const Token * tok)
{
	std::stringstream ss;
	ss << "Index [" << tok->str() << tok->next()->str() << tok->strAt(2) << "] may cause index out of bound.";
	reportError(tok, Severity::error, ErrorType::BufferOverrun, "arrayIndexOutOfBounds", ss.str(), ErrorLogger::GenWebIdentity(tok->str()));
}

void CheckTSCOverrun::IndexBeforeCheckError(const Token* tok)
{
	std::stringstream ss;
	ss << "Array index '" + tok->str() + "' is used before limits check.";
	reportError(tok, Severity::error, ErrorType::BufferOverrun, "arrayIndexThenCheck", ss.str(), ErrorLogger::GenWebIdentity(tok->str()));
}

void CheckTSCOverrun::MemcpyError(const Token* tokFunc, const Token* tokVar, const Token* tokStr)
{
	std::stringstream ss;
	ss << "[" << tokStr->str() << "] is greater or equal than size of [" << tokVar->astString2()<< "] which the buffer may be full filled then no space for the terminating null character.";
	reportError(tokFunc, Severity::error, ErrorType::BufferOverrun, "bufferAccessOutOfBounds", ss.str(), ErrorLogger::GenWebIdentity(tokVar->str()));
}

void CheckTSCOverrun::StrncatError(const Token* tokVar)
{
	std::stringstream ss;
	ss << "Calling strncat in for-loop may cause [" << tokVar->astString2() << "] overrun";
	reportError(tokVar, Severity::error, ErrorType::BufferOverrun, "bufferAccessOutOfBounds", ss.str(), ErrorLogger::GenWebIdentity(tokVar->astString()));
}

void CheckTSCOverrun::IndexCheckDefectError(const SExprLocation& elIndex, const SExprLocation& elArray, gt::CField::Dimension d, const Token* tokCond)
{
	if (m_reportedError)
	{
		m_reportedError->insert(elIndex.TokTop->astParent());
	}
    
	std::stringstream ss;
	ss << "Array " << elArray.Expr.ExprStr << "[" << d.Output() << "] is accessed at index [" << elIndex.ToTypeString() << "] may out of bound, as the index range check defect at line " << tokCond->linenr() << " may be wrong, mind the boundary value.";
	reportError(elIndex.TokTop, Severity::error, ErrorType::BufferOverrun, "arrayIndexCheckDefect", ss.str(), ErrorLogger::GenWebIdentity(elIndex.ToTypeString()));
}

bool CheckTSCOverrun::IsScopeReturn(const Token* tok)
{
	if (tok)
	{
		if (const Token* tok2 = tok->linkAt(1))
		{
			if (const Token* tok3 = tok2->linkAt(1))
			{

				const Token* tokRet = Token::findmatch(tok2->tokAt(2), "return|goto|throw", tok3);
				return tokRet != nullptr;
			}
		}
	}
	return false;
}

gt::CField::Dimension CheckTSCOverrun::GetDimensionInfo(const Token* tok)
{
	gt::CField::Dimension d1;
	const Variable* varArr = tok->variable();

	if (!varArr && tok->str() == "." && tok->astOperand2())
	{
		tok = tok->astOperand2();
		varArr = tok->variable();
	}

	if (varArr)
	{
		if (varArr->nameToken() != tok)
		{
			if (varArr->isArray() && varArr->dimensions().size() == 1)
			{
				d1.Init(varArr->dimensions()[0]);
			}
		}
	}
	else
	{
		const gt::CField* gtF = CGlobalTokenizer::Instance()->GetFieldInfoByToken(tok);

		if (gtF && gtF->GetDimensions().size() == 1)
		{
			d1 = gtF->GetDimensions()[0];
		}
	}
	return d1;
}

void CheckTSCOverrun::checkMemcpy()
{
	const std::vector<const Scope *>& funcs = _tokenizer->getSymbolDatabase()->functionScopes;

	for (std::vector<const Scope *>::const_iterator I = funcs.begin(), E = funcs.end(); I != E; ++I)
	{
		const Scope* scope = *I;

		std::set<SExpr> checkedVar;
		for (const Token* tok = scope->classStart; tok && tok != scope->classEnd; tok = tok->next())
		{
			if (Token::Match(tok, "memcpy|memmove ("))
			{
				const Token* tokTop = tok->next();
				std::vector<const Token*> args;
				getParamsbyAst(tokTop, args);

				if (args.size() != 3)
				{
					continue;
				}


				if (!args[1]->isString())
				{
					continue;
				}

				gt::CField::Dimension d = GetDimensionInfo(args[0]);
				if (!d.IsNum())
				{
					continue;
				}

				std::size_t strLen = Token::getStrLength(args[1]);

				if (strLen >= (std::size_t)d.iNum)
				{
					MemcpyError(tok, args[0], args[1]);
				}
			}
		}
	}
}

void CheckTSCOverrun::checkStrncat()
{
	const std::vector<const Scope *>& funcs = _tokenizer->getSymbolDatabase()->functionScopes;

	for (std::vector<const Scope *>::const_iterator I = funcs.begin(), E = funcs.end(); I != E; ++I)
	{
		const Scope* scope = *I;

		std::set<SExpr> checkedVar;
		for (const Token* tok = scope->classStart; tok && tok != scope->classEnd; tok = tok->next())
		{
			if (Token::Match(tok, "strncat ("))
			{
				std::vector<const Token*> args;
				getParamsbyAst(tok->next(), args);
				if (args.size() == 3)
				{
					const Token* tok1 = args[0];
					const Token* tok3 = args[2];
					if (tok3->str() == "(" && tok3->astOperand1() && tok3->astOperand1()->str() == "sizeof" && tok3->astOperand2())
					{
						if (tok1->astString() == tok3->astOperand2()->astString())
						{
							const Scope* scope2 = tok1->scope();
							if (scope2->GetScopeType() == Scope::eFor)
							{
								StrncatError(tok1);

								tok = scope2->classEnd;
							}
						}
					}
				}
			}
		}
	}
}

void CheckTSCOverrun::SetReportedErrors(std::set<const Token*>* reportedErrors)
{
	m_reportedError = reportedErrors;
}

