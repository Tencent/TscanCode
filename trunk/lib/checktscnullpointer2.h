//
//  checkTSCNullPointer2.h
//
//
//

#ifndef checkTSCNullPointer2_h
#define checkTSCNullPointer2_h

#include "config.h"
#include "check.h"
#include <stack>
#include "utilities.h"
#include <fstream>
#include <sstream>
#include <memory>

enum ECondType
{
	CT_NONE = 0,

	CT_IF = 1 << 1,
	CT_LOGIC = 1 << 2,
	CT_INIT = 1 << 3,

	CT_NEGATIVE = 1 << 4,
	CT_FORWARD = 1 << 5,

	CT_CHECK_NULL = 1 << 6,
	CT_FUNC_RET = 1 << 7,
	CT_ITERATOR = 1 << 8,
	CT_DYNAMIC = 1 << 9,

	CT_INIT_CERTAIN = 1 << 10,

	CT_INIT_VAR = 1<<11,//p=NULL; p=var;认为p不为空

	CT_IFDEREF = 1<<12,	// 标记if条件中解引用，然后转成pre条件
};

enum MergeResult
{
	MR_CONFLICT,
	MR_OK,
	MR_MERGE,
};

class CValueInfo
{
public:
	static const CValueInfo PossibleNull;
	static const CValueInfo Unknown;

	void Negation()
	{
		m_equal = !m_equal;
	}
	void SetPossible(bool possible)
	{
		m_possible = possible;
	}
	bool GetPossible() const { return m_possible; }
	int GetValue() const { return m_value; }
	const std::string& GetStrValue() const { return m_sValue; }

	bool IsStrValue() const { return m_bStrValue; }
	bool GetEqual() const { return m_equal; }
	bool GetUnknown() const { return m_unknown; }

	bool IsEqualZero() const { return !m_unknown && !m_possible && !m_bStrValue && m_equal == true && m_value == 0; }
	bool IsNotEqualZero() const { return !m_unknown && !m_possible && !m_bStrValue && m_equal == false && m_value == 0; }

	bool IsOpposite(const CValueInfo& vi) const;

	bool IsCentainAndEqual() const { return m_equal && !m_possible && !m_unknown; }

	bool CanBeNull() const
	{
		if (m_unknown)
		{
			return false;
		}
		else if (m_bStrValue)
		{
			return false;
		}
		else if (m_possible)
		{
			return true;
		} 
		else
		{
			return m_equal && (m_value == 0);
		}
	}

	bool CanNotBeNull() const//一定不为NULL 返回true
	{
		if (m_unknown)
		{
			return false;
		}
		else if (m_bStrValue)
		{
			return false;
		}
		else if (m_possible)
		{
			return false;
		}
		else
		{
			return !m_equal && (m_value == 0);
		}
	}

	CValueInfo() : m_value(0), m_possible(true), m_equal(false), m_unknown(true), m_bStrValue(false)
	{

	}

	CValueInfo(int value, bool equal, bool possible = false, bool unknown = false)
		: m_value(value), m_possible(possible), m_equal(equal), m_unknown(unknown), m_bStrValue(false)
	{

	}

	CValueInfo(const std::string& sValue, bool equal, bool possible = false, bool unknown = false)
		: m_value(0), m_sValue(sValue), m_possible(possible), m_equal(equal), m_unknown(unknown), m_bStrValue(true)
	{

	}

	bool operator == (const CValueInfo& other) const
	{
		return m_equal == other.m_equal &&
			m_unknown == other.m_unknown &&
			m_possible == other.m_possible &&
			m_bStrValue == other.m_bStrValue &&
			( m_bStrValue ? (m_sValue == other.m_sValue) : (m_value == other.m_value) );
	}

	bool operator != (const CValueInfo& other) const
	{
		return !(*this == other);
	}
private:
	int m_value;
	std::string m_sValue;
	bool m_possible;
	bool m_equal;
	bool m_unknown;
	bool m_bStrValue;
};   

class CExprValue
{
public:
	static CExprValue EmptyEV;

	CExprValue(const SExprLocation& exprLoc, const CValueInfo& value) : m_type(CT_NONE)
	{
		ExprLoc = exprLoc;
		Info = value;
	}

	CExprValue()
	{
	}

	bool Empty() const{ return ExprLoc.Empty(); }

	const SExpr& GetExpr() const
	{
		return ExprLoc.Expr;
	}
	
	void SetExpr( const SExpr & exp)
	{
		ExprLoc.Expr = exp;
	}

	const SExprLocation& GetExprLoc() const
	{
		return ExprLoc;
	}

	const CValueInfo& GetValueInfo() const
	{
		return Info;
	}

	void NegationValue()
	{
		Info.Negation();
	}

	bool GetPossible() const
	{
		return Info.GetPossible();
	}

	bool CanBeNull() const
	{
		return Info.CanBeNull();
	}

	bool CanNotBeNull() const
	{
		return Info.CanNotBeNull();
	}

	void SetPossible(bool possible)
	{
		Info.SetPossible(possible);
	}

	bool IsCertain() const
	{
		return (!Info.GetUnknown() && !Info.GetPossible());
	}

	bool IsCertainAndEqual() const
	{
		return Info.IsCentainAndEqual();
	}

	bool operator == (const CExprValue& other) const
	{
		return (ExprLoc.Expr == other.ExprLoc.Expr) && (Info == other.Info);
	}
	bool operator != (const CExprValue& other) const
	{
		return !(*this == other);
	}

	void SetCondNone() { m_type = CT_NONE; }

	void SetCondLogic() 
	{ 
		m_type = m_type & (~CT_INIT & ~CT_IF);
		m_type = (m_type | CT_LOGIC); 
	}
	bool IsCondLogic() const { return (m_type & CT_LOGIC) != 0; }

	void SetCondNegative() { m_type = m_type | CT_NEGATIVE; }
	bool IsCondNegative() const { return (m_type & CT_NEGATIVE) != 0; }

	bool IsInitCertain() const { return (m_type & CT_INIT_CERTAIN) != 0; }

	bool IsInitVar() const { return (m_type & CT_INIT_VAR) != 0; }

	void SetInitVar(bool bInit = true)
	{
		if (bInit)
			m_type = (m_type | CT_INIT_VAR);
		else
			m_type = m_type & (~CT_INIT_VAR);
	}

	void SetCondInit(bool flag = false) 
	{ 
		m_type = m_type & (~CT_LOGIC & ~CT_IF);
		m_type = (m_type | CT_INIT); 
		m_type = m_type & (~CT_INIT_CERTAIN );
		if (flag)
		{			
			m_type = (m_type | CT_INIT_CERTAIN);
		}
	}
	bool IsCondInit() const { return (m_type & CT_INIT) != 0; }

	void SetCondIf() 
	{ 
		m_type = m_type & (~CT_LOGIC & ~CT_INIT);
		m_type = (m_type | CT_IF); 
	}
	bool IsCondIf() const { return (m_type & CT_IF) != 0; }

	void SetCondNoneType() 
	{
		m_type = m_type & (~CT_LOGIC & ~CT_INIT & ~CT_IF);
	}

	void SetCondForward() { m_type = (m_type | CT_FORWARD); }
	bool IsCondForward() const { return (m_type & CT_FORWARD) != 0; }

	void SetCondCheckNull()
	{
		m_type = m_type & (~CT_FUNC_RET);
		m_type = (m_type | CT_CHECK_NULL);
	}
	bool IsCondCheckNull() const { return (m_type & CT_CHECK_NULL) != 0; }

	void SetCondFuncRet()
	{
		m_type = m_type & (~CT_CHECK_NULL);
		m_type = (m_type | CT_FUNC_RET);
	}
	bool IsCondFuncRet() const { return (m_type & CT_FUNC_RET) != 0; }

	void SyncCondErrorType(const CExprValue ev)
	{
		m_type = m_type & (~CT_FUNC_RET & ~CT_CHECK_NULL);
		m_type = m_type | (ev.m_type & CT_FUNC_RET) | (ev.m_type & CT_CHECK_NULL);
	}

	void SetCondIterator()
	{
		m_type = m_type | CT_ITERATOR;
	}

	bool IsCondIterator() const { return (m_type & CT_ITERATOR) != 0; }

	void SetCondDynamic()
	{
		m_type = m_type | CT_DYNAMIC;
	}

	void SetCondIfDeref()
	{
		m_type = m_type | CT_IFDEREF;
	}

	bool IsCondDynamic() const { return (m_type & CT_DYNAMIC) != 0; }

	bool IsCondIfDeref() const { return (m_type & CT_IFDEREF) != 0; }

    static bool IsExprValueMatched(const CExprValue& v1, const CExprValue& v2);
    static bool IsExprValueConflictedForEqualCond(const CExprValue& v1, const CExprValue& v2);
	static bool IsExprValueOpposite(const CExprValue& v1, const CExprValue& v2);

	static void SwapLoc(CExprValue &v1, CExprValue&v2) { SExprLocation tmp = v1.ExprLoc; v1.ExprLoc = v2.ExprLoc;  v2.ExprLoc = tmp; }
	void SetExpLoc(const SExprLocation &loc) { ExprLoc = loc; }


	bool CanReportBeforeCheckError() const { return IsCondFuncRet() || IsCondDynamic() || IsCondIfDeref(); }
private:
	SExprLocation ExprLoc;
	CValueInfo Info;
	int m_type;
};

struct SDerefInfo
{
	enum EDerefType
	{
		Unknown,
		Star,
		Dot,
		ArrayIndex,
		Func,
	};
	
	//EDerefType DerefType;
	SExprLocation ExprLoc;

	SDerefInfo()
	{

	}

	SDerefInfo(const SExprLocation& exprLoc) : ExprLoc(exprLoc)
	{

	}
};

typedef std::vector<CExprValue>::const_iterator VEV_CI;
typedef std::map<SExpr, CExprValue>::iterator MEV_I;
typedef std::map<SExpr, CExprValue>::const_iterator MEV_CI;
typedef std::map<SExpr, CExprValue> MEV;

class CCompoundExpr
{
public:
	enum ConjType
	{
		None,
		Unknown,
		And,
		Or,
		Equal,
	};
public:
	CCompoundExpr(CCompoundExpr* parent = nullptr) 
		: m_conj(None), m_left(nullptr), m_right(nullptr), m_parent(parent)
	{

	}

	~CCompoundExpr()
	{
		SAFE_DELETE(m_left);
		SAFE_DELETE(m_right);
	}

	void SetConj(ConjType t) { m_conj = t; }
	void SetExprValue(const CExprValue& ev) { m_ev = ev; }

	const ConjType GetConj() const { return m_conj; }
	const CExprValue& GetExprValue() const { return m_ev; }
	CExprValue& GetExprValue() { return m_ev; }

	static CCompoundExpr* CreateConjNode(ConjType type)
	{
		CCompoundExpr* conj = new CCompoundExpr;
		conj->SetConj(type);
		return conj;
	}

	CCompoundExpr* GetTop()
	{
		CCompoundExpr* top = this;
		while (top->GetParent())
		{
			top = top->GetParent();
		}
		return top;
	}

	CCompoundExpr* CreateLeft()
	{
		if (!m_left)
			m_left = new CCompoundExpr(this);
		return m_left;

	}
	CCompoundExpr* GetLeft() const
	{
		return m_left;
	}
	CCompoundExpr* CreateRight()
	{
		if (!m_right)
			m_right = new CCompoundExpr(this);
		return m_right;
	}
	CCompoundExpr* GetRight() const
	{
		return m_right;
	}
	CCompoundExpr* GetParent() const { return m_parent; }
	void SetParent(CCompoundExpr* parent) { m_parent = parent; }

	bool IsEmpty() const
	{
		return (m_conj == None) && !m_right && !m_left && m_ev.Empty();
	}

	bool IsConjNode() const { return m_conj > Unknown; }
	bool IsUnknown() const { return m_conj == Unknown; }
	void SetUnknown() 
	{
		if (m_left)
		{
			SAFE_DELETE(m_left);
		}
		if (m_right)
		{
			SAFE_DELETE(m_right);
		}
		m_conj = Unknown;
		m_ev = CExprValue::EmptyEV;
	}

	void SetEmpty()
	{
		if (m_left)
		{
			SAFE_DELETE(m_left);
		}
		if (m_right)
		{
			SAFE_DELETE(m_right);
		}
		m_conj = None;
		m_ev = CExprValue::EmptyEV;
	}


	bool IsUnknownOrEmpty() const { return IsUnknown() || IsEmpty(); }

	void ConvertToList(std::vector<CCompoundExpr*>& listCE, bool includeExpand = true);

	void GetCondjStatus(bool& bAnd, bool& bOr) const
	{
		bAnd = false;
		bOr = false;
		CCompoundExpr* ce = m_parent;
		while (ce)
		{
			if (ce->GetConj() == And)
			{
				bAnd = true;
			}
			else if (ce->GetConj() == Or)
			{
				bOr = true;
			}
			ce = ce->GetParent();
		}
	}

	void Negation()
	{
		if (IsUnknown())
			return;

		if (m_left)
			m_left->Negation();
		
		if (m_right)
			m_right->Negation();


		if (!m_ev.Empty())
			m_ev.NegationValue();
		
		if (m_conj == And)
			m_conj = Or;
		else if (m_conj == Or)
			m_conj = And;
	}

	void InsertParent(CCompoundExpr* parent, bool bLeft = true)
	{
		parent->m_parent = m_parent;
		if (bLeft)
			parent->m_left = this;
		else
			parent->m_right = this;
		m_parent = parent;
	}

	void SetLeft(CCompoundExpr* left)
	{
		m_left = left;
		if (left)
			left->m_parent = this;
	}

	void SetRight(CCompoundExpr* right)
	{
		m_right = right;
		if (right)
			right->m_parent = this;
	}

	CCompoundExpr* DeepCopy()
	{
		CCompoundExpr* root = new CCompoundExpr;
		root->SetConj(m_conj);
		root->SetExprValue(m_ev);

		if (m_left)
		{
			root->m_left = this->m_left->DeepCopy();
			root->m_left->m_parent = root;
		}
		if (m_right)
		{
			root->m_right = this->m_right->DeepCopy();
			root->m_right->m_parent = root;
		}

		return root;
	}

	static void DeleteSelf(CCompoundExpr* ce, CCompoundExpr*& top);

	void Reset() 
	{
		m_conj = None;
		m_right = m_left = nullptr;
		m_ev = CExprValue::EmptyEV;
	}

	bool HasSExpr(const SExpr& expr, CCompoundExpr** ce = nullptr);
	void ReplaceSExpr(const SExpr& expr, const SExpr& newExpr);

	bool IsEqual(const CCompoundExpr* ce);

	bool IsExpanded() const;
private:
	ConjType m_conj;
	CCompoundExpr* m_left;
	CCompoundExpr* m_right;
	CCompoundExpr* m_parent;
	CExprValue m_ev;
};

struct SEqualCond
{
	CExprValue EV;
	std::shared_ptr<CCompoundExpr> EqualCE;

	SEqualCond(const CExprValue& ev, std::shared_ptr<CCompoundExpr>& ce) : EV(ev), EqualCE(ce)
	{
		EV.SetCondNoneType();
	}

	SEqualCond()
	{
		//EqualCE.reset();
	}
};

typedef std::multimap<SExpr, SEqualCond> MMEC;
typedef std::multimap<SExpr, SEqualCond>::iterator MMEC_I;
typedef std::multimap<SExpr, SEqualCond>::const_iterator MMEC_CI;
typedef std::pair<SExpr, SEqualCond> PEC;

typedef std::map<SExpr, SDerefInfo> MDEREF;
typedef std::map<SExpr, SDerefInfo>::iterator MDEREF_I;

struct SLocalVar
{
	MEV PreCond;
	MEV PreCond_if;
	MEV InitStatus;
	MEV InitStatus_temp;
	MEV PossibleCond;
	MMEC EqualCond;
	MMEC EqualCond_if;

	MDEREF DerefedMap;

	bool Empty() const
	{
		return PreCond.empty() && 
			PreCond_if.empty() && 
			InitStatus.empty() && 
			InitStatus_temp.empty() && 
			EqualCond.empty() && 
			EqualCond_if.empty() && 
			DerefedMap.empty() &&
			(m_returnTok == nullptr) && 
			(m_ifCond == nullptr);
	}

	void UpdateStatusWhenStepIntoScope()
	{
		EqualCond_if.clear();
		PreCond_if.clear();
		InitStatus_temp.clear();
		SetIfReturn(false);
		SetReturn(false);
	}

	void Clear()
	{
		PreCond.clear();
		InitStatus.clear();
		InitStatus_temp.clear();
		PossibleCond.clear();
		EqualCond.clear();
		PreCond_if.clear();
		EqualCond_if.clear();
		DerefedMap.clear();
		m_ifCond.reset();
		m_ifCondExpanded.reset();
		m_returnTok = nullptr;
		m_ifReturn = false;
		m_return = false;
	}

	void ClearSExpr(const SExpr& expr)
	{
		EraseInitStatus(expr);
		ErasePreCond(expr);
		EraseEqualCond(expr);
		EraseEqualCondBySExpr(expr);

	}

	void ErasePreCond(const SExpr& expr)
	{
		MEV_I I = PreCond.find(expr);
		if (I != PreCond.end())
		{
			PreCond.erase(I);
		}
	}

	void EraseDerefedStatus(const SExpr& expr)
	{
		MDEREF_I I = DerefedMap.find(expr);
		if (I != DerefedMap.end())
		{
			DerefedMap.erase(I);
		}
	}

	void EraseInitStatus(const SExpr& expr)
	{
		InitStatus.erase(expr);
		InitStatus_temp.erase(expr);
	}

	void ErasePossibleCond(const SExpr& expr)
	{
		MEV_I I = PossibleCond.find(expr);
		if (I != PossibleCond.end())
		{
			PossibleCond.erase(I);
		}
	}

	void EraseEqualCond(const SExpr& expr)
	{
		MMEC_I I = EqualCond.find(expr);
		while (I != EqualCond.end() && I->first == expr)
		{
			//I = EqualCond.erase(I);
			EqualCond.erase(I++);
		}
	}

	bool HasPreCond(const SExpr& expr)
	{
		MEV_I I = PreCond.find(expr);
		return (I != PreCond.end());
	}

	bool HasPossibleCond(const SExpr& expr)
	{
		MEV_I I = PossibleCond.find(expr);
		return (I != PossibleCond.end());
	}

	bool HasDerefedExpr(const SExpr& expr)
	{
		MDEREF_I I = DerefedMap.find(expr);
		return I != DerefedMap.end();
	}

	void AddPreCond(const CExprValue& ev, bool force = false);

	bool AddPossibleCond(const CExprValue& ev, bool force = false)
	{
		if (!force)
		{
			if (HasPossibleCond(ev.GetExpr()))
			{
				return false;
			}
		}

		PossibleCond[ev.GetExpr()] = ev;
		return true;
	}

	bool HasInitStatus(const SExpr& expr)
	{
		MEV_I I = InitStatus.find(expr);
		return (I != InitStatus.end());
	}

	bool MergeInitStatus(const SExpr& expr, const CExprValue& value);

	bool HasTempInit(const SExpr& expr)
	{
		MEV_I I = InitStatus_temp.find(expr);
		return (I != InitStatus_temp.end());
	}

	bool AddInitStatus(const CExprValue& ev, bool force = false, bool forceIfCertain = true)
	{
		const SExpr& expr = ev.GetExpr();
		if (!force)
		{
			if (HasInitStatus(expr))
				return false;
		}

		if (!forceIfCertain)
		{
			if (HasInitStatus(expr))
			{
				if (InitStatus[expr].IsCertainAndEqual())
				{
					return false;
				}
			}
		}

		InitStatus_temp[expr] = ev;
		InitStatus[expr] = ev;
		return true;
	}

	void AddEqualCond(const CExprValue& ev, std::shared_ptr<CCompoundExpr>& ce, bool bHandleConflict = true);

	void AddEqualCond_if(const CExprValue& ev, std::shared_ptr<CCompoundExpr>& ce, bool bHandleConflict = true);

	bool HasSameEqualCond(const SEqualCond& ec);

	void EraseEqualCondBySExpr(const SExpr& expr);

	void EraseSubFiled(const SExpr& expr);

	void SetReturnTok(const Token* tok) 
	{ 
		m_returnTok = tok; 
	}

	void SetReturn(bool bReturn)
	{
		m_return = bReturn;
	}

	const Token* GetReturnTok() const { return m_returnTok; }
	bool GetReturn() const { return m_return; }

	void SetIfReturn(bool bReturn) { m_ifReturn = bReturn; }
	bool GetIfReturn() const{ return m_ifReturn; }

	CCompoundExpr* GetIfCondPtr()
	{
		return m_ifCond.get();
	}

	std::shared_ptr<CCompoundExpr>& GetIfCond()
	{
		return m_ifCond;
	}


	CCompoundExpr* GetIfCondExpandedPtr()
	{
		return m_ifCondExpanded.get();
	}

	std::shared_ptr<CCompoundExpr>& GetIfCondExpanded()
	{
		return m_ifCondExpanded;
	}

	std::vector<SExprLocation>& GetIfCondDerefed()
	{
		return m_ifCondDeRef;
	}


	void SetIfCond(std::shared_ptr<CCompoundExpr>& ce)
	{
		m_ifCond = ce;
	}

	void SetIfCondExpanded(std::shared_ptr<CCompoundExpr>& ce)
	{
		m_ifCondExpanded = ce;
	}

	
	void SwapMapKeyValue(const SExprLocation& expr1, const SExprLocation& expr2, MEV &data);

	void SwapMMapKeyValue(const SExprLocation& expr1, const SExprLocation& expr2, MMEC &data);

	void SwapCondition(const SExprLocation& expr1, const SExprLocation& expr2);

	void ClearIfCond()
	{
		// not delete it , just set NULL
		//m_ifCond = nullptr;
		m_ifCond.reset();
		//m_ifCondExpanded.reset();
		m_ifCondDeRef.clear();
	}


	void AddDerefExpr(const SExprLocation& exprLoc)
	{
		DerefedMap[exprLoc.Expr] = SDerefInfo(exprLoc);
	}


	void ResetExpr(const SExpr& expr);

	SLocalVar() : m_returnTok(nullptr), m_ifReturn(false)
	{
		//m_ifCond.reset();
	}
private:
	std::shared_ptr<CCompoundExpr> m_ifCond;
	std::shared_ptr<CCompoundExpr> m_ifCondExpanded;
	// stores exprs derefed in if cond
	std::vector<SExprLocation> m_ifCondDeRef;
	const Token* m_returnTok;
	bool m_ifReturn;
	bool m_return;

};

/// @addtogroup Checks
/// @{
class TSCANCODELIB CheckTSCNullPointer2 : public Check {
public:
    /** @brief This constructor is used when registering the CheckClass */
    CheckTSCNullPointer2() : Check(myName()) {
    }
    
    /** @brief This constructor is used when running checks. */
    CheckTSCNullPointer2(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
    : Check(myName(), tokenizer, settings, errorLogger) {
    }
    
    /** @brief Run checks against the normal token list */
    void runChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) {
        CheckTSCNullPointer2 CheckTSCNullPointer2(tokenizer, settings, errorLogger);
        
        // Checks

    }
    
    /** @brief Run checks against the simplified token list */
    void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) {
			CheckTSCNullPointer2 CheckTSCNullPointer2(tokenizer, settings, errorLogger);
			// Checks
			CheckTSCNullPointer2.TSCNullPointerCheck2();
			CheckTSCNullPointer2.checkMissingDerefOperator();
    }

public:
	static bool TryGetDerefedExprLoc(const Token* tok, SExprLocation& exprLoc, const SymbolDatabase* symbolDB);


	static void GetForLoopIndex(const Token* tokFor, std::vector<SExprLocation>& indexList);
private:
	static void GetForLoopIndexByLoopTok(const Token* tokLoop, std::vector<SExprLocation>& indexList);

private:

	// the main check method
	void TSCNullPointerCheck2();

	// check for missingDerefOperator
	void checkMissingDerefOperator();
	void reportMissingDerefOperatorError(const SExprLocation& elAssign, const SExprLocation& elCheckNull);

	const Token* HandleOneToken(const Token * tok);

	const Token* HandleOneToken_deref(const Token * tok);

	void HandleReturnToken(const Token * tok);

	//// "if(p == null)", get (p == 0); 
	//// "if(a[i] != null", get (a[i]); 
	void UpdateIfCondFromCondition(const Token* tok, const Token* tokKey);

	CCompoundExpr* ExpandIfCondWithEqualCond(CCompoundExpr* ifCond, SLocalVar& localVar);
	CCompoundExpr* ExpandIfCondWithEqualCond_Recursively(CCompoundExpr* ifCond, SLocalVar& localVar, std::set<SEqualCond*>& expandedECList, bool bStrict);

	bool FindMostMatchedEqualCond(SLocalVar& localVar, bool bStrict, const CExprValue& ev, SEqualCond*& ec);

	void ExpandPreCondWithEqualCond(const CExprValue& pre, SLocalVar& equalCond);


	void IfToPreInCond();

	void CastIfCondDerefedToPreCond(CCompoundExpr* ifCond);

	void CastIfCondToPreCond(CCompoundExpr* ifCond, ECondType condType = CT_NONE);

	void IfToPreAfterCond(const Token* tok, bool bReturn, bool bHasElse, SLocalVar& localvar, SLocalVar& topLocalvar);
	void IfCondElse();
	void IfCondEnd(const Token* tok, bool bReturn, bool bHasElse, SLocalVar& localvar, SLocalVar& topLocalvar);
	void UpdateLocalVarWhenScopeBegin();
	void UpdateLocalVarWhenScopeEnd(const Token* tokEnd);

	void AscendDerefedMap(SLocalVar &topLocalVar);

	void HandleAlwaysTrueCond(bool bReturn, SLocalVar &topLocalVar);

	void UpdateAssignedPreCond(SLocalVar &topLocalVar);

	void ExtractEqualCond(std::shared_ptr<CCompoundExpr> &ifCond, SLocalVar &topLocalVar, MMEC& tempEqualCond, bool hasElse);
	void ExtractEqualCondReturn(const Token* tok, std::shared_ptr<CCompoundExpr> &ifCond, SLocalVar &topLocalVar, MMEC& tempEqualCond);
	void ExtractDerefedEqualCond(std::shared_ptr<CCompoundExpr> &ifCond, SLocalVar &topLocalVar, MMEC& tempEqualCond, bool hasElse);
    void ExtractPreCond(MEV& tempPreCond, SLocalVar &topLocalVar);

	void AscendEqualCond(SLocalVar &topLocalVar, std::shared_ptr<CCompoundExpr>& ifCond, MMEC& tempEC, bool bReturn);
	bool CheckIf_ElseIf(const CExprValue& ev, CCompoundExpr* ifCond);

	void MergeEqualCond(MMEC& temp, bool bReturn, SLocalVar& topLocalVar);
	void MergePreCond(MEV& temp, bool bReturn, SLocalVar& topLocalVar);

	void MergeTwoIfCond(CCompoundExpr* ce1, CCompoundExpr* ce2, CCompoundExpr::ConjType conj, CCompoundExpr*& result);
	MergeResult CheckTwoCExprValueCompatible(const CExprValue& ev1, const CExprValue& ev2, CCompoundExpr::ConjType conj, bool& deleteEV1, bool& deleteEV2);

	//ussed when "=" exist
	bool HandleRetParamRelation(const SExprLocation& exprLoc, const SExprLocation& exprLoc2, const gt::CFunction *gtFunc);
	//used when not "=" exist
	bool HandleRetParamRelation(const Token *tokFunc, const Token *, const gt::CFunction *gtFunc);

	void Clear();
	const Token* HandleAssignOperator(const Token* tokAssign);

	//value set by "=" or something like 'A *a(new A)'
	//for that A *(new A) is not simplified to A *a = new A;
	//    and that A *a(p->a) is not simplified to A *a = p->a;
	//return true if handled 
	const Token* HandleAssign(const Token *tok);
	void HandleAssign(const Token * const tok, const SExprLocation &, const CExprValue& curPre);

	bool CheckIsLoopIndexCall(const Token* tokFuncTop);

	void HandleFuncParamAssign(const Token* tok, const Token *tokFunc);

	void ResetReportedError(const SExpr& expr);

	const Token* HandleFuncParam(const Token* tok);

	bool ExtractCondFromToken(const Token* tokTop, CCompoundExpr* compExpr, bool bHanldeComp = false);

	bool SimplifyIfCond(CCompoundExpr* cond, CCompoundExpr*& top);
	//handle if (p != 0 || p != 0 && q)
	bool SimplifyIfCond2(CCompoundExpr* cond, CCompoundExpr*& top);
	//handle if(m_pTimeOut == NULL && CreateCommonObject(&m_pTimeOut))
	bool SimplifyIfCond3(CCompoundExpr* cond, CCompoundExpr*& top);
	CCompoundExpr* SimplifyIfCond3_FindGetP(CCompoundExpr* ce, CCompoundExpr* findNode);
	//handle NP bug:if(CreateCommonObject(&p) || p == NULL){p->xx;}
	bool SimplifyIfCond4(CCompoundExpr* cond, CCompoundExpr*& top);
	bool SimplifyCondWithUpperCond(CCompoundExpr*& cond);

	void ExtractDerefedExpr(const Token* tok, const Token* tokKey,std::vector<SExprLocation>& derefed);

	bool HandleEqualExpr(const Token* tok, const Token*& tokExpr, CValueInfo& info, bool& bIterator);
	bool AddEVToCompoundExpr(const Token* tok, const CValueInfo& info, CCompoundExpr* compExpr, bool bIterator = false);

	bool CanBeNull(const SExprLocation& sExprLoc, CExprValue& ev);

	bool CanNotBeNull(const SExprLocation& sExprLoc, CExprValue& ev);
	bool CheckSpecialCheckNull(const SExpr& expr, const Token* tok, bool& bSafe, CExprValue& ouyputEV);
	bool HasSpecialCheckNull(const SExprLocation& sExprLoc, bool& bSafe, CExprValue& ouyputEV);
	//handle  if (*p == 1 && p)
	bool HasSpecialCheckNull2(const SExprLocation& sExprLoc, bool& bSafe, CExprValue& ouyputEV);
	bool IsDerefTok(const Token* tok);
	void HandleDerefTok(const Token* tok);
	void HandleDeref(const SExprLocation& exprLoc);

	bool IsDerefedTokPossibleBeNull(const SExprLocation& exprLoc);

	const Token* HandleCheckNull(const Token* tok);

	bool IsParameter(const Token* tok);
	bool IsClassMember(const Token* tok);

	bool IsIterator(const Token* tok);
	bool IsIteratorBeginOrEnd(const Token* tok, bool bBegin);

	void HandleJumpCode(const Token* tok);
	void HandleArrayDef(const Token* tok);

	void HanldeDotOperator(const Token* tok);

	bool CheckIfCondAlwaysTrueOrFalse(SLocalVar& localVar, CCompoundExpr* ifCond, bool& bAlwaysTrue);

	bool SpecialHandleForCond(const Token* tokFor);

	void CheckDerefBeforeCondError(CCompoundExpr* ifCond, const Token *tok, const Token* tokKey);

    void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const {
        CheckTSCNullPointer2 c(0, settings, errorLogger);
    }

	bool checkVarId(const SExprLocation& sExprLoc, const CExprValue& ev);
	void HandleNullPointerError(const SExprLocation& sExprLoc, const CExprValue& ev);
	void debugError(const Token* tok, const char* errMsg);
	void nullPointerError(const SExprLocation& sExprLoc, const CExprValue& ev);
	void derefBeforeCheckError(const SExprLocation& exprLoc, const CExprValue& ev);
	void dereferenceIfNullError(const SExprLocation& exprLoc, const CExprValue& ev);

	void checkNullDefectError(const SExprLocation& exprLoc);
	
	// possible errors
	void HandleNullPointerPossibleError(const SExprLocation& sExprLoc);
	void parameterPossibleError(const SExprLocation& exprLoc);
	void classMemberPossibleError(const SExprLocation& exprLoc);
	void directFuncPossibleError(const SExprLocation& exprLoc);
	void arrayPossibleError(const SExprLocation& exprLoc);
	void possibleNullPointerError(const SExprLocation& exprLoc);

	bool nullPointerErrorFilter(const SExprLocation& sExprLoc, const CExprValue& ev);
    
    static std::string myName() {
        return "TSC Null Pointer";
    }
    
    std::string classInfo() const {
        return "Null pointers\n"
        "- null pointer dereferencing\n";
    }

	
	//this function exist for convenience
	//return false if pToken is null
	inline bool CheckIfLibFunctionReturnNull(const Token* const pToken) const;

	//get function paramter indexs that should never be null
	//return true if at least one parameter should not be null
	//caller should ensure that @param derefIndex is empty
	inline bool GetLibNotNullParamIndexByName(std::set<int> &derefIndex, const Token *tok) const;

	bool CheckIfDelete(const Token *tok, const CExprValue & ev) const;

	bool IsEmbeddedClass(const Token* tok);

	const Token* HandleSmartPointer(const Token *Token);

	void HandleSelfOperator(const Token *tok);

	const Token* HandleTryCatch(const Token* tok);

	const Token* HandleCase(const Token* tokCase);

	const void CheckFuncCallToUpdateVarStatus(const Token* tokFunc, const Token* tok);

	void InitNameVarMap();

	bool FilterExplicitNullDereferenceError(const SExprLocation& sExprLoc, const CExprValue& ev);

	std::string& FindValueReturnFrom(const Token *tok, std::string &retFrom) const ;
	const std::string FindValueReturnFrom(const Token *tokBegin, const Token* tokEnd) const ;
	//be sure funcName is empty before this function
	void FindFuncDrefIn(const Token *tok, std::string &funcName) const;
	
	inline void GetDerefLocStr(const Token *tok, std::string &strLoc) const;

	bool CheckIfDirectFuncNotRetNull(const SExprLocation&) const;

	bool MakeSubFieldNotNull(const Token *tok);

	//array considered not pointer
	inline bool CanBePointer(const SExprLocation &) const;

	inline bool CheckIfExprLocIsArray(const SExprLocation& exprLoc) const;

private:
	struct SErrorBak
	{
		SExprLocation ExprLoc;
		CExprValue EV;
		
		SErrorBak(const SExprLocation& exprLoc, const CExprValue& ev) : ExprLoc(exprLoc), EV(ev)
		{

		}

		SErrorBak() {}
	};

	void ReportFuncRetNullErrors();

private:
	std::stack<SLocalVar> m_stackLocalVars;
	// record vars had been reported error, if reassigned, then remove it from this set;
	// avoid that same var is reported error many times.
	std::set<SExpr> m_reportedErrorSet;
	std::set<SExpr> m_possibleReportedErrorSet;
	std::set<SExpr> m_ifNullreportedErrorSet;
	std::map<SExpr, SErrorBak> m_funcRetNullErrors;

	SLocalVar m_localVar;

	//cache name variable map
	//map name and scope name to var
	std::map<std::string, const Variable *> m_nameVarMap;

#pragma region DUMP LOG
private:
	void OpenDumpFile();
	void TryDump(const Token* tok);
	void DumpLocalVar(unsigned lineno);
	void CloseDumpFile();

	std::ofstream m_dumpFile;
	bool m_dump;
	unsigned m_lineno;
#pragma endregion

};
/// @}
//---------------------------------------------------------------------------

#endif /* checkTSCNullPointer2_h */
