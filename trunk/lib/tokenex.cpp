#include "tokenex.h"
#include "token.h"
#include "globalsymboldatabase.h"
#include "globaltokenizer.h"
#include "settings.h"

TokenSection TokenSection::EmptySection;
SExprLocation SExprLocation::EmptyExprLoc;

const Variable* SExprLocation::GetVariable() const
{
	if (Expr.VarId)
	{
		if (TokTop->varId())
		{
			return TokTop->variable();
		}
		else if (TokTop->str() == ".")
		{
			if (const Token* tokVar = TokTop->astOperand2())
			{
				return tokVar->variable();
			}
		}
	}
	return nullptr;
}

std::string TokenSection::ToTypeString(const char* sep /*= NULL*/) const
{
	if (Empty())
		return "";

	std::string sType;
	const Token* tok = Begin;
	if (tok == NULL)
	{
		return sType;
	}
	sType += tok->str();
	tok = tok->next();
	for (; tok && tok != End; tok = tok->next())
	{
		if (sep)
			sType += sep;

		sType += tok->str();
	}
	return sType;
}

const Token* TokenSection::LastToken() const
{
	if (Empty())
		return nullptr;

	if (End)
		return End->previous();
	else
		return Begin;
}

bool TokenSection::IsOneToken() const
{
	return !Empty() && Begin->next() == End;
}

bool iscast(const Token *tok)
{
	if (!Token::Match(tok, "( %name%"))
		return false;

	if (tok->previous() && tok->previous()->isName() && tok->previous()->str() != "return")
		return false;
	//template <>
	//std::string Com::tostr<bool>(const bool &t)
	if (Token::simpleMatch(tok->previous(), ">") && tok->previous()->link())
		return false;

	if (Token::Match(tok, "( (| typeof (") && Token::Match(tok->link(), ") %num%"))
		return true;

	bool type = false;
	for (const Token *tok2 = tok->next(); tok2; tok2 = tok2->next()) {
		while (tok2->link() && Token::Match(tok2, "(|[|<"))
			tok2 = tok2->link()->next();

		if (tok2->str() == ")")
			return type || tok2->strAt(-1) == "*" || Token::Match(tok2, ") &|~") ||
			(Token::Match(tok2, ") %any%") &&
				!tok2->next()->isOp() &&
				!Token::Match(tok2->next(), "[[]);,?:.]"));
		if (!Token::Match(tok2, "%name%|*|&|::"))
			return false;

		if (tok2->isStandardType() && tok2->next()->str() != "(")
			type = true;
	}

	return false;
}

SExprLocation CreateSExprByExprTok(const Token* tok)
{
	if (!tok)
	{
		return SExprLocation::EmptyExprLoc;
	}

	if (tok->isName())
	{
		return SExprLocation(tok, tok->next(), tok, tok->varId());
	}
	else if (tok->str() == "(")
	{
		//handle casts 
		if (iscast(tok))
		{
			return CreateSExprByExprTok(tok->astOperand1());
		}
		if (tok->astOperand1())
		{
			if (!Token::Match(tok->astOperand1(), "%name%|::|."))
			{
				return SExprLocation::EmptyExprLoc;
			}

		}
		const Token* end = tok->link();
		if (end)
		{
			end = tok->link()->next();

			const Token* begin = tok;
			while (begin->astOperand1())
			{
				begin = begin->astOperand1();
			}
			return SExprLocation(begin, end, tok, 0);
		}
	}
	else if (tok->str() == "." || tok->str() == "::")
	{
		if (const Token* tok2 = tok->astOperand2())
		{
			if (tok2->isName())
			{
				const Token* begin = tok;
				while (begin->astOperand1())
				{
					begin = begin->astOperand1();
				}

				// this . var to var ,make sure var's expr is same when var id =0 (tokenize bug)
				if (Token::Match(begin, "this . %name%") && begin->tokAt(2)->varId() == 0)
				{
					begin = begin->tokAt(2);
				}

				return SExprLocation(begin, tok2->next(), tok, tok2->varId());
			}
		}
	}
	else if (tok->str() == "[")
	{
		const Token* end = tok->link();
		if (end)
		{
			end = tok->link()->next();
			//possible lambda expression
			//[]() mutable throw() ->int{}
			if (end && end->str() == "(")
			{
				for (const Token *tokLoop = end->link(); tokLoop; tokLoop = tokLoop->next())
				{
					if (Token::Match(tokLoop, "throw ("))
					{
						tokLoop = tokLoop->tokAt(2)->link();
						if (!tokLoop)
						{
							break;
						}
						continue;
					}
					if (Token::Match(tokLoop, ";|}"))
					{
						break;
					}
					if (tokLoop->str() == "{")
					{
						return SExprLocation::EmptyExprLoc;
					}
				}
			}
			const Token* begin = tok;
			while (begin->astOperand1())
			{
				begin = begin->astOperand1();
			}
			return SExprLocation(begin, end, tok, 0);
		}
	}
	else if (tok->str() == "*")
	{

		if (tok->astOperand1() && !tok->astOperand2())
		{
			// handle *((int*)asyc_data); and not handle *func()
			if (tok->astOperand1()->str() == "(" && tok->tokAt(2) == tok->astOperand1() && tok->tokAt(1)->str() == "(")
			{
				// handle *((int*)asyc_data); ;
			}
			else
			{
				SExprLocation exprLoc = CreateSExprByExprTok(tok->astOperand1());
				if (!exprLoc.Empty())
				{
					exprLoc.Begin = tok;
					exprLoc.TokTop = tok;
					exprLoc.Expr.ExprStr = tok->str() + exprLoc.Expr.ExprStr;
					exprLoc.Expr.VarId = 0;
				}
				return exprLoc;
			}
		}
	}
	else if (tok->str() == "&" || tok->str() == "+" || tok->str() == "-")//handle (&p[i])->xx;  AND c=((tlist_node*)((long)(&p.stLink)+(&p.stLink).iPrev))->a;  AND (la-2)->xx;
	{
		return SExprLocation::EmptyExprLoc;
	}
	return SExprLocation::EmptyExprLoc;
}

unsigned GetStandardTypeSize(const Token* typeStartTok, const Token* typeEndTok)
{
	const Settings* settings = Settings::Instance();

	//only compute standard types
	if (typeStartTok == typeEndTok)
	{
		if (typeStartTok->isStandardType())
		{
			if (typeStartTok->str() == "bool")
			{
				return settings->sizeof_bool;
			}
			else if (typeStartTok->str() == "char")
			{
				return 1;
			}
			else if (typeStartTok->str() == "short")
			{
				return settings->sizeof_short;
			}
			else if (typeStartTok->str() == "int")
			{
				return settings->sizeof_int;
			}
			else if (typeStartTok->str() == "long")
			{
				if (typeStartTok->isLong())
				{
					return settings->sizeof_long_long;
				}
				else
				{
					return settings->sizeof_long;
				}
			}
			else if (typeStartTok->str() == "float")
			{
				return settings->sizeof_float;
			}
			else if (typeStartTok->str() == "double")
			{
				if (typeStartTok->isLong())
				{
					return settings->sizeof_long_double;
				}
				else
				{
					return settings->sizeof_double;
				}
			}
			else if (typeStartTok->str() == "size_t")
			{
				return settings->sizeof_size_t;
			}
		}
	}

	return 0;
}

unsigned GetStandardTypeSize(const gt::CField* gtField)
{
	const Settings* settings = Settings::Instance();

	if (gtField->GetType().size() != 1)
	{
		return 0;
	}
	const std::string& sType = gtField->GetType().front();

		if (sType == "bool")
		{
			return settings->sizeof_bool;
		}
		else if (sType == "char")
		{
			return 1;
		}
		else if (sType == "short")
		{
			return settings->sizeof_short;
		}
		else if (sType == "int")
		{
			return settings->sizeof_int;
		}
		else if (sType == "long")
		{
			return settings->sizeof_long;
		}
		else if (sType == "float")
		{
			return settings->sizeof_float;
		}
		else if (sType == "double")
		{
			return settings->sizeof_double;
		}
		else if (sType == "size_t")
		{
			return settings->sizeof_size_t;
		}
		
	

	return 0;
}

void getParamsbyAst(const Token* tok, std::vector<const Token*> &params)
{
	if (!tok || tok->str() != "(")
	{
		return;
	}

	std::list<const Token*> paramList;
	tok = tok->astOperand2();

	while (tok && tok->str() == ",")
	{
		if (tok->astOperand2())
		{
			params.insert(params.begin(), tok->astOperand2());
		}
		tok = tok->astOperand1();
	}

	if (tok)
	{
		params.insert(params.begin(), tok);
	}
}

int tryGetExprSize(const Token* tok)
{
	int s = -1;
	if (const Variable* pVar = tok->variable())
	{
		s = (int)GetStandardTypeSize(pVar->typeStartToken(), pVar->typeEndToken());
	}
	// array
	else if (tok->str() == "[" && tok->astOperand1())
	{
		s = tryGetExprSize(tok->astOperand1());
	}
	else if (tok->str() == "." && tok->astOperand2())
	{
		const Token* tok2 = tok->astOperand2();
		s = tryGetExprSize(tok2);
	}
	else if (tok->str() == "(" && iscast(tok) && tok->astOperand1())
	{
		s = tryGetExprSize(tok->astOperand1());
	}
	else
	{
		const gt::CField* fi = CGlobalTokenizer::Instance()->GetFieldInfoByToken(tok);
		if (fi)
		{
			s = GetStandardTypeSize(fi);
		}
	}

	return s;
}
