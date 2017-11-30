#ifndef tokenex_h__
#define tokenex_h__

#include <string>
#include <vector>

class Token;
class Variable;
class Scope;

namespace gt
{
	class CFunction;
	class CField;
};

struct TokenSection
{
	const Token* Begin;
	const Token* End;

	static TokenSection EmptySection;

	TokenSection() : Begin(nullptr), End(nullptr)
	{}
	TokenSection(const Token* begin, const Token* end)
		: Begin(begin), End(end)
	{}

	bool Empty() const
	{
		return (Begin == nullptr) || (End == nullptr);
	}

	void Clear()
	{
		Begin = nullptr;
		End = nullptr;
	}

	const Token* LastToken() const;
	bool IsOneToken() const;

	std::string ToTypeString(const char* sep = nullptr) const;
};


struct SExpr
{
	// not used now, may be used in the future though, just keep it here
	enum ExprType
	{
		None,
		Variable,
		Function,
		Array,
	};

	unsigned VarId;
	std::string ExprStr;

	SExpr() : VarId(0)
	{

	}

	SExpr(unsigned varid, const std::string& str) : VarId(varid), ExprStr(str)
	{

	}

	bool operator<(const SExpr& other) const
	{
		if (VarId != other.VarId)
		{
			return VarId < other.VarId;
		}
		else
		{
			if (VarId != 0)
			{
				return false;
			}
			else
			{
				return ExprStr < other.ExprStr;
			}
		}
	}

	bool operator == (const SExpr& other) const
	{
		return !(*this < other) && !(other < *this);
	}

	bool operator != (const SExpr& other) const
	{
		return !(*this == other);
	}
};

struct SExprLocation : public TokenSection
{
	static SExprLocation EmptyExprLoc;

	SExprLocation() : TokenSection(), TokTop(nullptr)
	{
	}

	bool Empty() const { return TokTop == nullptr; }

	SExprLocation(const Token* begin, const Token* end, const Token* tokTop, unsigned varid) :
		TokenSection(begin, end), TokTop(tokTop), Expr(varid, ToTypeString())
	{

	}

	bool operator == (const SExprLocation& other) const
	{
		return TokTop == other.TokTop;
	}

	bool operator != (const SExprLocation& other) const
	{
		return TokTop != other.TokTop;
	}


	const Variable* GetVariable() const;

public:
	const Token* TokTop;
	SExpr Expr;
};

class TokenEx
{
public:
	TokenEx()
		: m_gtFunc(nullptr)
	{}

	void SetGtFunc(const gt::CFunction* gtFunc) { m_gtFunc = gtFunc; }
	const gt::CFunction* GetGtFunc() const { return m_gtFunc; }
	
private:
	const gt::CFunction* m_gtFunc;
};

bool iscast(const Token *tok);

SExprLocation CreateSExprByExprTok(const Token* tok);


unsigned GetStandardTypeSize(const Token* typeStartTok, const Token* typeEndTok);
unsigned GetStandardTypeSize(const gt::CField* gtField);

void getParamsbyAst(const Token* tok, std::vector<const Token*> &params);

int tryGetExprSize(const Token* tok);

#endif // tokenex_h__
