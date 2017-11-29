#include "checkusercustom.h"
#include "symboldatabase.h"
#include "globaltokenizer.h"

// Register this check class (by creating a static instance of it)
namespace {
	CheckUserCustom checkUserCustom;
}


std::set<std::string> CheckUserCustom::s_nonThreadSafeFunc = make_container<std::set<std::string> >()
<< "asctime"
<< "basename"
<< "ctime"
<< "gethostbyaddr"
<< "gethostbyname"
<< "tmpnam"
<< "strtok"
<< "rand"
<< "localtime"
<< "gmtime"
<< "getspnam"
<< "getservbyname"
<< "getservbyport";


bool static IsStandardType(const Variable *v)
{
	const Token* tok = v->typeStartToken();
	const Token * end = v->typeEndToken();
	for (; tok && tok != end->next(); tok = tok->next())
	{
		if (tok->isStandardType())
		{
			return true;
		}
	}
	return false;
}

bool static GetFuncParamTok(std::list<const Token*>& paramList, const Token *paramtok)
{
	if (paramtok->astParent() && paramtok->astParent()->str() == "(")
	{
		const Token* tokInvoke = paramtok->astParent();
		const Token* tok = tokInvoke->astOperand2();
		while (tok && tok->str() == ",")
		{
			if (tok->astOperand2())
			{
				paramList.push_front(tok->astOperand2());
			}
			tok = tok->astOperand1();
		}
		if (tok)
		{
			paramList.push_front(tok);
		}
		return true;
	}
	return false;
}


int tryGetSizeByName(const Token* tok)
{
	int s = -1;
	if (tok->isName())
	{
		if (tok->str().find("32") != std::string::npos)
		{
			s = 4;
		}
		else if (tok->str().find("16") != std::string::npos)
		{
			s = 2;
		}
		else if (tok->str().find("8") != std::string::npos)
		{
			s = 1;
		}
		else if (tok->str().find("64") != std::string::npos)
		{
			s = 16;
		}
	}
	// array
	else if (tok->str() == "[" && tok->astOperand1())
	{
		s = tryGetSizeByName(tok->astOperand1());
	}
	else if (tok->str() == "." && tok->astOperand2())
	{
		const Token* tok2 = tok->astOperand2();
		s = tryGetSizeByName(tok2);
	}
	else if (tok->str() == "(" && iscast(tok) && tok->astOperand1())
	{
		s = tryGetSizeByName(tok->astOperand1());
	}

	return s;
}

void CheckUserCustom::checkWeShoot_AssignTypeSizeUnmatch()
{
	const std::vector<const Scope *>& funcs = _tokenizer->getSymbolDatabase()->functionScopes;

	for (std::vector<const Scope *>::const_iterator I = funcs.begin(), E = funcs.end(); I != E; ++I)
	{
		const Scope* scope = *I;

		for (const Token* tok = scope->classStart; tok && tok != scope->classEnd; tok = tok->next())
		{
			if (tok->str() == "=" && tok->astOperand1() && tok->astOperand2())
			{
				const Token* tok1 = tok->astOperand1();
				const Token* tok2 = tok->astOperand2();

				int size1 = tryGetExprSize(tok1);
				int size2 = tryGetExprSize(tok2);

				if (size1 > 0 && size2 > 0)
				{
					if (size1 < size2)
					{
						checkWeShoot_AssignTypeSizeUnmatch_Error(tok, iscast(tok2), false);
					}
				}
				else
				{
					size1 = tryGetSizeByName(tok1);
					size2 = tryGetSizeByName(tok2);
					if (size1 > 0 && size2 > 0)
					{
						if (size1 < size2)
						{
							checkWeShoot_AssignTypeSizeUnmatch_Error(tok, iscast(tok2), true);
						}
					}
				}
				
			}
		}
	}
}

void CheckUserCustom::checkWeShoot_AssignTypeSizeUnmatch_Error(const Token* tok, bool isCast, bool bPossible)
{
	std::string rule = "WeShoot_SizeUnmatch";
	if (bPossible)
	{
		rule = "WeShoot_SizePossibleUnmatch";
	}
	std::stringstream ss;
	ss << "Size of [" << tok->astOperand2()->astString2() << "] is greater than [" << tok->astOperand1()->astString2() << "], this may cause overflow.";
	reportError(tok, Severity::error, ErrorType::UserCustom, rule, ss.str(), tok->str());
}

void CheckUserCustom::checkSGame_PointerMember()
{
    const std::vector<const Scope *>& classVec = _tokenizer->getSymbolDatabase()->classAndStructScopes;
    
    for(std::vector<const Scope *>::const_iterator I = classVec.begin(), E = classVec.end(); I != E; ++I)
    {
        const Scope* sClass = *I;
        
        const std::list<Variable>& varList = sClass->varlist;
        
        for (std::list<Variable>::const_iterator I2 = varList.begin(), E2 = varList.end(); I2 != E2; ++I2) {
            const Variable& var = *I2;
            
            if (var.isPointer()) {
                checkSGame_PointerMember_Error(var.nameToken(), false);
            }
            else
            {
                const Token* tok = Token::findsimplematch(var.typeStartToken(), "<", var.typeEndToken());
                if (tok && tok->link()) {
                    const Token* tokPointer = Token::findsimplematch(tok, "*", tok->link());
                    if (tokPointer) {
                        checkSGame_PointerMember_Error(var.nameToken(), true);
                    }
                }
            }
        }
    }
}

void CheckUserCustom::checkSGame_PointerMember_Error(const Token* tok, bool bTemplate)
{
    std::string rule = "SGame_PointerMember";
    if (bTemplate) {
        rule = "SGame_PointerMemberTemplate";
    }
    std::stringstream ss;
    ss << "[SGame] Pointer member: " << tok->str();
    reportError(tok, Severity::error, ErrorType::UserCustom, rule, ss.str(), tok->str());
}


static void FindComparedVar(const Token *tok, std::set<unsigned int> &checkedVar, std::set<std::string> &checkName)
{
	if (!tok || tok->str() != ".")
	{
		return;
	}
	
	if (tok->astOperand1() && tok->astOperand1()->str().find("Req") && tok->astOperand2())
	{
		if (tok->astOperand2()->varId() == 0)
		{
			checkName.insert(tok->astOperand1()->str() + "." + tok->astOperand2()->str());
			
		}
		else
		{
			checkedVar.insert(tok->astOperand2()->varId());
		}
	}
}

static bool IsVarTypeMap(const Variable *var)
{
	for (const Token *tok = var->typeStartToken(); tok && tok != var->typeEndToken(); tok = tok->next())
	{
		if (Token::Match(tok, "map|multimap"))
		{
			return true;
		}
	}
	return false;
}

void CheckUserCustom::CheckSSK_UnCheckedIndex()
{
	if (!_settings->IsCheckIdOpened(ErrorType::ToString(ErrorType::UserCustom).c_str(), "ssk_UncheckedIndex"))
	{
		return;
	}
	const SymbolDatabase *base = _tokenizer->getSymbolDatabase();
	std::set<unsigned int> checkVar;
	std::set<std::string> checkName;
	for (std::size_t ii = 0, nSize = base->functionScopes.size(); ii < nSize; ++ii)
	{
		checkVar.clear();
		checkName.clear();
		for (const Token *tok = base->functionScopes[ii]->classStart, *end = base->functionScopes[ii]->classEnd;
		tok && tok != end; tok = tok->next())
		{
			if (Token::Match(tok, ">=|>|<|<=|==|!="))
			{
				FindComparedVar(tok->astOperand1(), checkVar, checkName);
				FindComparedVar(tok->astOperand2(), checkVar, checkName);
			}
			else if (tok->str() == "[")
			{
				if (tok->previous()->variable() && IsVarTypeMap(tok->previous()->variable()))
				{
					continue;
				}
				if (tok->previous()->str().find("map") != std::string::npos)
				{
					continue;
				}
				const Token *child = tok->astOperand2();
				if (child && child->str() == "." && child->previous()->str().find("Req") != std::string::npos)
				{
					child = child->astOperand2();
					if (child  && ((child->varId() == 0 && 0 == checkName.count(child->strAt(-2) + "." + child->str()))
						|| (child->varId() > 0 && 0 == checkVar.count(child->varId()))))
					{
						std::string str;
						for (const Token *t = tok->next(), *link = tok->link(); t && t != link; t = t->next())
						{
							str += t->str();
						}
						reportError(tok, Severity::error, ErrorType::UserCustom, "ssk_UncheckedIndex", str + " is used as index without any check", ErrorLogger::GenWebIdentity(str));
					}
				}
			}
		}
	}
}

void CheckUserCustom::NonThreadSafeFuncError(const Token* tok)
{
	std::stringstream ss;
	ss << "Func [" << tok->str() << "] is not thread-safe, use [" << tok->str() << "_r] instead.";
	reportError(tok, Severity::error, ErrorType::UserCustom, "NonThreadSafeFunc", ss.str(), tok->str());
}


void CheckUserCustom::checkNonThreadSafeFunc()
{
	const std::vector<const Scope *>& funcs = _tokenizer->getSymbolDatabase()->functionScopes;

	for (std::vector<const Scope *>::const_iterator I = funcs.begin(), E = funcs.end(); I != E; ++I)
	{
		const Scope* scope = *I;

		for (const Token* tok = scope->classStart; tok && tok != scope->classEnd; tok = tok->next())
		{
			if (!s_nonThreadSafeFunc.count(tok->str()))
			{
				continue;
			}

			if (tok->next() && tok->next()->str() == "(")
			{
				NonThreadSafeFuncError(tok);
			}
		}
	}
}

void CheckUserCustom::SGameClientCheck()
{
	if (_settings->IsCheckIdOpened(ErrorType::ToString(ErrorType::Unity).c_str(), "unityFloatVariables"))
		SGameClient_FloatCheck();
	if (_settings->IsCheckIdOpened(ErrorType::ToString(ErrorType::Unity).c_str(), "unityRandomFunc"))
		SGameClient_RandomCheck();
	if (_settings->IsCheckIdOpened(ErrorType::ToString(ErrorType::Unity).c_str(), "unityUnstableSortFunc"))
		SGameClient_SortCheck();
	if (_settings->IsCheckIdOpened(ErrorType::ToString(ErrorType::Unity).c_str(), "sgameStructAlignError"))
		SGameStructAlignCheck();
}


void CheckUserCustom::SGameStructAlignCheck()
{
	const SymbolDatabase* symbolDB = _tokenizer->getSymbolDatabase();
	const std::set<std::string>& riskTypes = CGlobalTokenizer::Instance()->GetRiskTypes();

	std::vector<const Scope*>::const_iterator I = symbolDB->classAndStructScopes.begin();
	std::vector<const Scope*>::const_iterator E = symbolDB->classAndStructScopes.end();

	for (; I != E; ++I)
	{

		bool bOneByte = false;
		const Scope* typeScope = *I;

		bool bRisk = riskTypes.count(typeScope->className) != 0;

		std::list<Variable>::const_iterator I2 = typeScope->varlist.begin();
		std::list<Variable>::const_iterator E2 = typeScope->varlist.end();
		for (; I2 != E2; ++I2)
		{
			const Variable& var = *I2;
			if (var.isIntegralType() && !var.isArrayOrPointer())
			{
				if (Token::Match(var.typeStartToken(), "bool|char|byte"))
				{
					bOneByte = !bOneByte;
				}
			}

			if (var.isFloatingType() || (var.isIntegralType() && var.typeStartToken()->isLong()))
			{
				if (bOneByte && bRisk)
				{
					reportError(var.typeStartToken(), Severity::error,
						ErrorType::None, "sgameStructAlignError", "Risk Types!", ErrorLogger::GenWebIdentity(var.typeStartToken()->str()));
					continue;
				}
			}

			bool bRiskType = false;
			for (const Token* tok = var.typeStartToken(); tok != var.typeEndToken(); tok = tok->next())//ignore TSC
			{
				if (riskTypes.count(tok->str()))
				{
					bRiskType = true;
					break;
				}
			}

			if (bRiskType && bOneByte)
			{
				reportError(var.typeStartToken(), Severity::error,
					ErrorType::None, "sgameStructAlignError", "Risk Types!", ErrorLogger::GenWebIdentity(var.typeStartToken()->str()));
				continue;
			}

		}
	}

}

void CheckUserCustom::SGameClient_FloatCheck()
{
	for (const Token *tok = _tokenizer->tokens(); tok; tok = tok->next())
	{
		if (Token::Match(tok, "float|double %var%"))
		{
			std::stringstream ss;
			ss << "float variable [" << tok->str() << "] may cause sync issues.";
			reportError(tok, Severity::warning, ErrorType::Unity, "unityFloatVariables", ss.str().c_str(), ErrorLogger::GenWebIdentity(tok->str()));
		}
	}
}

void CheckUserCustom::SGameClient_RandomCheck()
{
	for (const Token *tok = _tokenizer->tokens(); tok; tok = tok->next())
	{
		if (Token::Match(tok, "rand|InRange ("))
		{
			std::stringstream ss;
			ss << tok->str() << "() may cause sync problems.";
			reportError(tok, Severity::warning, ErrorType::Unity, "unityRandomFunc", ss.str().c_str(), ErrorLogger::GenWebIdentity(tok->str()));
		}
	}
}

void CheckUserCustom::SGameClient_SortCheck()
{
	for (const Token *tok = _tokenizer->tokens(); tok; tok = tok->next())
	{
		if (Token::Match(tok, "sort|Sort|SortEx ("))
		{
			std::stringstream ss;
			ss << tok->str() << "() may cause sync problems.";
			reportError(tok, Severity::warning, ErrorType::Unity, "unityUnstableSortFunc", ss.str().c_str(), ErrorLogger::GenWebIdentity(tok->str()));
		}
	}
}



void CheckUserCustom::checkSgameAssignZeroContinuously()
{
	const SymbolDatabase* symbolDatabase = _tokenizer->getSymbolDatabase();
	const std::vector<const Scope*>& func_list = symbolDatabase->functionScopes;

	for (std::vector<const Scope*>::const_iterator I = func_list.begin(), E = func_list.end(); I != E; ++I)
	{
		const Scope* pFunc = *I;

		const Scope* type_scope = pFunc->functionOf;
		std::map < std::string, int> var_index;

		std::vector < std::string > zero_list;
		std::vector < int > size_list;

		int count = 0;
		const int threshold = 16;
		if (pFunc)
		{
			for (const Token* tok = pFunc->classStart, *tok_end = pFunc->classEnd; tok && tok != tok_end; tok = tok->next())
			{

				if (Token::Match(tok, "%var% . Clear"))
				{
					if (const Variable* pvar = tok->variable())
					{
						if (Token::findsimplematch(pvar->typeStartToken(), "TArray", pvar->typeEndToken()))
						{
							reportError(tok, Severity::style, ErrorType::Logic, "warning_sgameAssignZero_TArray",
								"Assign zero successively",
								pFunc->className);
						}
					}

				}
				else if (Token::Match(tok, "%var% = 0|false ;"))
				{
					int size_sum = 0;
					int size = 0;
					bool has8Byte = false;

					zero_list.clear();
					size_list.clear();

					zero_list.push_back(tok->str());

					const Variable* pvar = tok->variable();
					if (pvar)
					{
						size = GetStandardTypeSize(pvar->typeStartToken(), pvar->typeEndToken());
						if (size == 0)
						{
							break;
						}
					}
					else
					{
						size = 4;
					}
					if (size >= 8)
					{
						has8Byte = true;
					}
					size_list.push_back(size);
					size_sum += size;

					const Token* tok2 = tok->tokAt(4);

					while (size_sum < threshold && Token::Match(tok2, "%var% = 0|false ;"))
					{
						zero_list.push_back(tok2->str());
						const Variable* pvar2 = tok2->variable();
						if (pvar2)
						{
							size = GetStandardTypeSize(pvar2->typeStartToken(), pvar2->typeEndToken());
							if (size == 0)
							{
								break;
							}
						}
						else
						{
							size = 4;
						}
						if (size >= 8)
						{
							has8Byte = true;
						}
						size_list.push_back(size);
						size_sum += size;
						tok2 = tok2->tokAt(4);
					}

					if (size_sum >= threshold)
					{
						if (type_scope)
						{
							const std::list<Variable>& var_list = type_scope->varlist;

							if (var_index.empty())
							{
								int index = 0;
								for (std::list<Variable>::const_iterator I2 = var_list.begin(), E2 = var_list.end(); I2 != E2; ++I2)
								{
									var_index[I2->name()] = index++;
								}
							}

							std::set<int> index_set;
							for (std::vector < std::string >::const_iterator I2 = zero_list.begin(), E2 = zero_list.end(); I2 != E2; ++I2)
							{
								if (var_index.count(*I2))
								{
									index_set.insert(var_index[*I2]);
								}
							}


							bool is_continious = true;
							int last_index = -1;
							for (std::set<int>::const_iterator I3 = index_set.begin(), E3 = index_set.end(); I3 != E3; ++I3)
							{
								int index = *I3;
								if (I3 == index_set.begin())
								{
									last_index = index;
								}
								else
								{
									if (index - last_index != 1)
									{
										is_continious = false;
										break;
									}
									last_index = index;
								}
							}


							if (is_continious)
							{
								std::string rule;
								if (has8Byte)
								{
									rule = "warning_sgameAssignZero_64bit";
								}
								else
								{
									if (IsDerivedFromPooledObject(type_scope->definedType))
									{
										rule = "warning_sgameAssignZero";
									}
									else
									{
										rule = "info_sgameAssignZero";
									}
								}

								reportError(tok, Severity::style, ErrorType::Logic, rule,
									"Assign zero successively",
									pFunc->className);

								break;
							}

						}
						else
						{
							reportError(tok, Severity::style, ErrorType::Logic, "info_sgamePossibleAssignZero",
								"Assign zero successively",
								pFunc->className);

							break;
						}
					}
					tok = tok2;
					count = 0;
				}
			}
		}
	}
}


void CheckUserCustom::SGameServerCheck()
{
	//SGameForLoopVarType();
	SGameArgumentCountOfTlog_log();
	//SGamePlusOperatorTypeCheck();
}

void CheckUserCustom::SGameForLoopVarType()
{
	for (const Token *tok = _tokenizer->tokens(); tok; tok = tok->next())
	{
		//Todo: check compatibility
		if (Token::Match(tok, "for (") && tok->linkAt(1))
		{
			const Token* tokVar = tok->tokAt(2);
			const Token* tokEnd = tok->linkAt(1);

			for (; tokVar && tokVar->str() != ";" && tokVar != tokEnd; tokVar = tokVar->next())//ignore TSC
			{
				if (tokVar->varId())
				{
					if (const Variable* pVar = tokVar->variable())
					{
						unsigned sizeVar = GetStandardTypeSize(pVar->typeStartToken(), pVar->typeEndToken());
						if (sizeVar == 0)
						{
							continue;
						}
						if (sizeVar < 4)
						{
							std::stringstream ss;
							ss << "The type of " << pVar->name() << " is " << pVar->typeStartToken()->str() << ".";
							reportError(tok, Severity::warning, ErrorType::Logic, "SGameHighRisk", ss.str(), ErrorLogger::GenWebIdentity(tok->str()));
						}
					}
					break;
				}
			}
		}
	}
}


void CheckUserCustom::SGamePlusOperatorTypeCheck()
{
	for (const Token *tok = _tokenizer->tokens(); tok; tok = tok->next())
	{
		if (tok->str() == "+" /*|| tok->str() == "-"*/)
		{
			if (tok->astOperand1() && tok->astOperand2())
			{
				bool bUnsigned1 = false;
				bool bUnsigned2 = false;
				if (TryGetVarUnsigned(tok->astOperand1(), bUnsigned1) && TryGetVarUnsigned(tok->astOperand2(), bUnsigned2))
				{
					if (bUnsigned1 ^ bUnsigned2)
					{
						std::stringstream ss;
						ss << "One side of '+' is unsigned, the other side is signed.";
						reportError(tok, Severity::warning, ErrorType::Logic, "SGameHighRisk", ss.str(), ErrorLogger::GenWebIdentity(tok->str()));
					}
				}
			}
		}
	}
}


bool CheckUserCustom::TryGetVarUnsigned(const Token* tokVar, bool& bUnsigned)
{
	//changed to suppress warning C4706: assignment within conditional expression
	const Variable* pVar = tokVar->variable();
	if (pVar)
	{
	}
	else if (tokVar->str() == ".")
	{
		if (const Token* tok2 = tokVar->astOperand2())
		{
			pVar = tok2->variable();
		}
	}

	if (pVar)
	{
		if (::IsStandardType(pVar))
		{
			unsigned size = GetStandardTypeSize(pVar->typeStartToken(), pVar->typeEndToken());
			if (size <8 && size >0)
			{
				if (!pVar->isPointer() && !pVar->isArray())
				{
					bUnsigned = pVar->typeStartToken()->isUnsigned();
					return true;
				}
			}
		}
	}

	return false;
}



void CheckUserCustom::SGameCheckCond()
{
	for (const Token *tok = _tokenizer->tokens(); tok; tok = tok->next())
	{
		if (tok->str() == "for")
		{
			const Token* tokStart = tok->linkAt(1)->next();
			if (tokStart->str() == "{" && tokStart->link())
			{
				if (tok->astParent())
				{
					const Token* tok2 = tok->astParent()->astOperand2()->astOperand2();
					if (tok2 && tok2->str() == ";")
					{
						const Token* tok3 = tok2->astOperand1();
						SGameCheckCondInternal(tok3, tokStart);
					}
				}
			}
		}
	}
}

void CheckUserCustom::SGameCheckCondInternal(const Token* tokCond, const Token* tokStart)
{
	if (tokCond == NULL)
	{
		return;
	}
	//Todo: check compatibility
	if (Token::Match(tokCond, "&&|%oror%"))
	{
		SGameCheckCondInternal(tokCond->astOperand1(), tokStart);
		SGameCheckCondInternal(tokCond->astOperand2(), tokStart);
	}
	else if (tokCond->str() == "<")
	{
		const Token* tokVar = tokCond->astOperand2();
		if (tokVar && !tokVar->isName())
		{
			std::string sVar = tokVar->astString();
			for (const Token* tok = tokStart; tok != tokStart->link(); tok = tok->next())//ignore TSC
			{
				if (tok->str() == "++" || tok->str() == "--")
				{
					if (tok->astOperand1() && tok->astOperand1()->astString() == sVar)
					{
						reportError(tok, Severity::warning, ErrorType::Logic, "SGameHighRisk",
							"High risk code for buffer overrun.", ErrorLogger::GenWebIdentity(tok->str()));
						break;
					}
					else if (tok->astOperand2() && tok->astOperand2()->astString() == sVar)
					{
						reportError(tok, Severity::warning, ErrorType::Logic, "SGameHighRisk",
							"High risk code for buffer overrun.", ErrorLogger::GenWebIdentity(tok->str()));
						break;
					}
				}
				else if (tok->str() == "=")
				{
					if (tok->astOperand1() && tok->astOperand1()->astString() == sVar)
					{
						reportError(tok, Severity::warning, ErrorType::Logic, "SGameHighRisk",
							"High risk code for buffer overrun.", ErrorLogger::GenWebIdentity(tok->str()));
						break;
					}
				}
			}
		}
	}
}



void CheckUserCustom::SGameArgumentCountOfTlog_log()
{
	if (!_settings->isEnabled("warning"))
		return;

	for (const Token *tok = _tokenizer->tokens(); tok; tok = tok->next())
	{
		if (tok->str() == "tlog_category_logv")
		{
			std::list<const Token*> paramList;
			if (GetFuncParamTok(paramList, tok))
			{
				if (paramList.size() < 3)
				{
					std::stringstream ss;
					ss << "The parameter count of tlog_log is " << paramList.size() + 1 << " which should be at least 4.";
					reportError(tok, Severity::warning, ErrorType::Logic, "SGameHighRisk", ss.str(), ErrorLogger::GenWebIdentity(tok->str()));
				}

				unsigned index = 0;
				for (std::list<const Token*>::iterator I = paramList.begin(), E = paramList.end(); I != E; ++I)
				{
					if (index == 2)
					{
						if (!(*I)->isLiteral())
						{
							std::stringstream ss;
							ss << "Suspicious parameters of tlog_XXX(...)";
							reportError(tok, Severity::warning, ErrorType::Logic, "SGameHighRisk", ss.str(), ErrorLogger::GenWebIdentity(tok->str()));
						}
					}
					++index;
				}


			}
		}
	}
}




void CheckUserCustom::checkStructSpaceWasted()
{
	const SymbolDatabase * const symbolDatabase = _tokenizer->getSymbolDatabase();

	std::map<std::string, std::set<SPack1Scope> >& pack1 = CGlobalTokenizer::Instance()->GetPack1Scope();

	std::vector<const Scope *>::const_iterator I = symbolDatabase->classAndStructScopes.begin();
	std::vector<const Scope *>::const_iterator E = symbolDatabase->classAndStructScopes.end();
	for (; I != E; ++I)
	{
		const Scope* typeScope = *I;
		const std::string& sFilePath = _tokenizer->list.file(typeScope->classDef);
		const unsigned lineno = typeScope->classDef->linenr();

		bool is_pack1 = false;
		if (pack1.count(sFilePath))
		{
			const std::set<SPack1Scope>& packSet = pack1[sFilePath];
			for (std::set<SPack1Scope>::const_iterator I2 = packSet.begin(), E2 = packSet.end(); I2 != E2; ++I2)
			{
				if (lineno > I2->StartLine && lineno < I2->EndLine)
				{
					is_pack1 = true;
					break;
				}
			}
		}

		if (is_pack1)
		{
			continue;
		}

		std::list<Variable>::const_iterator I3 = typeScope->varlist.begin();
		std::list<Variable>::const_iterator E3 = typeScope->varlist.end();

		int structSize = 0;
		int gap_sum = 0;
		int gap = 0;
		std::vector<int> gap_list;

		for (; I3 != E3; ++I3)
		{
			const Variable& var = *I3;

			if (var.isStatic())
			{
				continue;
			}

			int size = GetStandardTypeSize(var.typeStartToken(), var.typeEndToken());

			if (size == 0)
				size = 4;

			if (gap != 0 && size < 4 && gap >= size)
			{
				gap -= size;
			}
			else
			{
				int minSize = structSize + size;
				structSize = structSize + ((size - 1) / 4 + 1) * 4;

				if (structSize > minSize)
				{
					gap = structSize - minSize;
				}
				else
				{
					if (gap > 0)
					{
						std::string rule = emptyString;
						if (IsDerivedFromPooledObject(typeScope->definedType))
						{
							rule = "warning_SgameStructArrangement";
						}
						else
						{
							rule = "info_SgameStructArrangement";
						}

						reportError(typeScope->classDef, Severity::style, ErrorType::Logic, rule,
							"bool/short types in the middle of type's field list.",
							typeScope->className);

						gap_list.push_back(gap);
						gap_sum += gap;
						gap = 0;
					}
				}
			}
		}

		if (gap > 0)
		{
			gap_sum += gap;
			gap = 0;
		}

		if (gap_sum >= 4)
		{
			reportError(typeScope->classDef, Severity::style, ErrorType::UserCustom, "structSpaceWasted",
				"Adjust fields of type [" + typeScope->className + "] can make higher memory usage.",
				typeScope->className);
		}
	}
}

bool CheckUserCustom::IsDerivedFromPooledObject(const Type* pType)
{
	if (pType)
	{
		if (pType->name() == "PooledObject")
		{
			return true;
		}

		const std::vector<Type::BaseInfo>& base_list = pType->derivedFrom;

		for (std::vector<Type::BaseInfo>::const_iterator I = base_list.begin(), E = base_list.end(); I != E; ++I)
		{
			Type::BaseInfo base_info = *I;
			if (base_info.name == "PooledObject")
			{
				return true;
			}
			else if (IsDerivedFromPooledObject(base_info.type))
			{
				return true;
			}
		}
	}

	return false;
}

