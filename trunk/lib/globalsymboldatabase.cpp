//
//  globalsymboldatabase.cpp
//
//
//

#include "globalsymboldatabase.h"
#include <fstream>
#include "globaltokenizer.h"
#include "settings.h"
#include <stack>

namespace gt
{
    typedef std::map<std::string, CScope*>::const_iterator ScopeCI;
    typedef std::map<std::string, CScope*>::iterator ScopeI;
    typedef std::pair<std::string, CFunction*> FuncPair;
    typedef std::multimap<std::string, CFunction*>::iterator FuncI;
    typedef std::multimap<std::string, CFunction*>::const_iterator FuncCI;
	typedef std::map<std::string, CField*>::iterator FieldI;
	typedef std::map<std::string, CField*>::const_iterator FieldCI;
    
    CScope::~CScope()
    {
        for (ScopeI I = m_scopeMap.begin(), E = m_scopeMap.end(); I != E; ++I) {
            SAFE_DELETE(I->second);
        }
        
        for (FuncI I = m_funcMap.begin(), E = m_funcMap.end(); I != E; ++I) {
            SAFE_DELETE(I->second);
        }
    }
    
	bool CScope::AddChildScope(CScope* scope)
	{
        if (!scope)
        {
            return false;
        }
        
        if (HasChildScope(scope->GetName())) {
            return false;
        }

		m_scopeMap[scope->GetName()] = scope;
		scope->SetParent(this);
		return true;
	}
    
    bool CScope::AddChildFunc(CFunction* func)
    {
        if (!func) {
            return false;
        }
        
        m_funcMap.insert(FuncPair(func->GetName(), func));
        func->SetParent(this);
        return true;
    }
    
	bool CScope::AddChildFiled(CField* field)
	{
		if (!field)
		{
			return false;
		}

		m_fieldMap[field->GetName()] = field;
		return true;
	}

	CScope* CScope::TryGetChildScope(const std::string& name)
    {
        if (!HasChildScope(name))
            return nullptr;
        
		ScopeI I = m_scopeMap.find(name);
		return I->second;
    }

	const CScope* CScope::FindChildScope(const std::string& name) const
	{
		if (!HasChildScope(name))
			return nullptr;

		ScopeCI I = m_scopeMap.find(name);
		return I->second;
	}
    
    CFunction* CScope::TryGetFunc(const CFunction* func)
    {
        FuncCI I = m_funcMap.find(func->GetName());
        if (I == m_funcMap.end()) {
            return nullptr;
        }
        
        for (; I != m_funcMap.end() && I->first == func->GetName(); ++I) {
            if (*(I->second) == *func) {
                return I->second;
            }
        }
        return nullptr;
    }
    
	CField* CScope::TryGetField(const std::string& name)
	{
		FieldI I = m_fieldMap.find(name);
		if (I != m_fieldMap.end())
		{
			return I->second;
		}
		return nullptr;
	}

	const CField* CScope::TryGetField(const std::string& name) const
	{
		FieldCI I = m_fieldMap.find(name);
		if (I != m_fieldMap.end())
		{
			return I->second;
		}
		return nullptr;
	}

	void CScope::Merge(const CScope* other)
    {
		for (std::set<std::string>::const_iterator I = other->m_usingList.begin(), E = other->m_usingList.end(); I != E; ++I)
		{
			m_usingList.insert(*I);
		}

		if (GetKind() == CScope::Type && other->GetKind() == CScope::Type)
		{
			CType* pType1 = dynamic_cast<CType*>(this);
			const CType* pType2 = dynamic_cast<const CType*>(other);
			if (pType1 && pType2)
			{
				pType1->MergeDerived(pType2);
			}
		}

		if (m_needInit < other->m_needInit)
		{
			SetNeedInit(other->m_needInit);
		}

        for (ScopeCI I = other->m_scopeMap.begin(), E = other->m_scopeMap.end(); I != E; ++I) {
            
            CScope* childScope = TryGetChildScope(I->first);
            if (!childScope)
            {
                childScope = I->second->GetCopy();
                AddChildScope(childScope);//ignore TSC
            }
            childScope->Merge(I->second);
        }
        
        for (FuncCI I = other->m_funcMap.begin(), E = other->m_funcMap.end(); I != E; ++I) 
		{
            CFunction* func = TryGetFunc(I->second);
            if (!func)
            {
                func = I->second->GetCopy();
                AddChildFunc(func);//ignore TSC
            }
			else
			{
				func->MergeFunc(I->second);//ignore TSC
			}
        }

		for (FieldCI I = other->m_fieldMap.begin(), E = other->m_fieldMap.end(); I != E; ++I)
		{
			const gt::CField* field = I->second;
			if (field)
			{
				CField* f = TryGetField(field->GetName());
				if (!f)
				{
					AddChildFiled(field->DeepCopy());
				}
				else
				{
					f->Merge(field);
				}
			}
		}

    }

	void CScope::InitScopeMap(std::multimap<std::string, CScope*>& globalMap)
	{
		for (ScopeCI I = m_scopeMap.begin(), E = m_scopeMap.end(); I != E; ++I)
		{
			globalMap.insert(std::pair < std::string, CScope* >(I->first, I->second));
			I->second->InitScopeMap(globalMap);
		}
	}

	void CScope::RecordFunc()
	{
		for (ScopeCI I = m_scopeMap.begin(), E = m_scopeMap.end(); I != E; ++I)
		{
			I->second->RecordFunc();
		}

		std::string cur = emptyString;
		FuncI startI = m_funcMap.end();
		std::vector<gt::CFunction*> sortVec;
		for (FuncI I = m_funcMap.begin(), E = m_funcMap.end(); I != E; ++I)
		{
			const std::string& funcName = I->first;
			if (cur != funcName)
			{
				if (sortVec.size() >= 2)
				{
					std::sort(sortVec.begin(), sortVec.end(), CFunction::Compare);

					for (std::size_t i = 0; i < sortVec.size(); ++i)
					{
						startI->second = sortVec[i];
						++startI;
					}
				}
				cur = funcName;
				startI = I;
				sortVec.clear();
				sortVec.push_back(I->second);
			}
			else
			{
				sortVec.push_back(I->second);
			}
		}

		if (sortVec.size() >= 2)
		{
			std::sort(sortVec.begin(), sortVec.end(), CFunction::Compare);

			for (std::size_t i = 0; i < sortVec.size(); ++i)
			{
				startI->second = sortVec[i];
				++startI;
			}
		}
	}

#ifdef TSC2_DUMP_TYPE_TREE

	char GetScopeChar(const CScope* scope)
	{
		if (scope->GetKind() == CScope::Global)
		{
			return 'G';
		}
		else if (scope->GetKind() == CScope::Type)
		{
			return 'T';
		}
		//else if (scope->GetKind() == CScope::Namespace)
		//{
		//	return 'N';
		//}
		else
		{
			return 'S';
		}
	}

	void RecursiveDump(std::ofstream& fs, const CScope* scope, unsigned tab)
	{
		for (unsigned i = 0; i < tab; ++i)
		{
			fs << "\t";
		}
		
		fs << "[" << GetScopeChar(scope) << "] " << scope->GetName() << "@" <<scope;
		if (scope->GetKind() == CScope::Type)
		{
			const CType* pType1 = dynamic_cast<const CType*>(scope);
			if (pType1)
			{
				const std::set < std::string >& derived = pType1->GetDerived();
				if (!derived.empty())
				{
					fs << " : ";
					for (std::set < std::string >::const_iterator I = derived.begin(), E = derived.end(); I != E; ++I)
					{
						fs << *I << ", ";
					}
				}

				fs << " " << pType1->GetNeedInitStr();
			}
		}
		fs << std::endl;

		for (ScopeCI I = scope->GetScopeMap().begin(), E = scope->GetScopeMap().end(); I != E; ++I)
		{
			//for (unsigned i = 0; i < tab + 1; ++i)
			//{
			//	fs << "\t";
			//}
			//fs << "[" << GetScopeChar(I->second) << "] " << I->second->GetName() << std::endl;
			RecursiveDump(fs, I->second, tab + 1);
		}

		for (FuncCI I = scope->GetFunctionMap().begin(), E = scope->GetFunctionMap().end(); I != E; ++I)
		{
			for (unsigned i = 0; i < tab + 1; ++i)
			{
				fs << "\t";
			}
			fs << "[F] " << I->second->GetFuncStr() << " ";
			const CFuncData& funcData = I->second->GetFuncData();
			fs << "[ " << I->second->GetFlag() << ", " << I->second->GetType() << ", " << I->second->GetArgCount() << ", " << I->second->GetMinArgCount() << " ] ";
			fs << ((funcData.GetFuncRetNull() == CFuncData::RetNullFlag::fNull) ? "[NULL] " : "");
			fs << ((funcData.GetFuncRetNull() == CFuncData::RetNullFlag::fNotNull) ? "[NOT_NULL] " : "");
			fs << ((funcData.GetExitFlag() == CFuncData::ExitFlag::fExit) ? "[EXIT] " : "");
			fs << (I->second->HasScope() ? "[SCOPE] " : "");

			if (!funcData.GetOutScopeVarState().empty())
			{
				fs << "[";
				for (std::set<SOutScopeVarState>::const_iterator iter = funcData.GetOutScopeVarState().begin(), end = funcData.GetOutScopeVarState().end();
				iter != end; ++iter)
				{
					fs << *iter << ",";
				}
				fs << "]";
			}

			for (size_t ii = 0; ii < CFuncData::ParamRetRelationNum; ++ii)
			{
				if (funcData.GetRetParamRelation()[ii].empty())
				{
					continue;
				}
				fs << "[";
				for (std::set<CFuncData::ParamRetRelation>::const_iterator iter = funcData.GetRetParamRelation()[ii].begin(), 
					end = funcData.GetRetParamRelation()[ii].end();
				iter != end; ++iter)
				{
					fs << *iter << ",";
				}
				fs << "]";
			}

			if (funcData.AssignThis())
			{
				fs << "[assign this]";
			}

			const std::set<SVarEntry>& varSet = funcData.GetDerefedVars();
			for (std::set<SVarEntry>::iterator I2 = varSet.begin(), E2 = varSet.end(); I2 != E2; ++I2)
			{
				if (I2->iParamIndex >= 0)
				{
					fs << "[" << I2->iParamIndex << ", " << I2->sName << "] ";
				}
			}
			fs << std::endl;
		}

		for (FieldCI CI = scope->GetFieldMap().begin(), CE = scope->GetFieldMap().end(); CI != CE; ++CI)
		{
			for (unsigned i = 0; i < tab + 1; ++i)
			{
				fs << "\t";
			}
			fs << "[V] " << CI->second->GetFieldStr();
			
			if (CI->second->isArray())
			{
				for (std::vector<gt::CField::Dimension>::const_iterator I2 = CI->second->GetDimensions().begin(), E2 = CI->second->GetDimensions().end(); I2 != E2; ++I2)
				{

					fs << "[" << I2->Output() << "]";
				}
			}
			if (CI->second->GetGType())
			{
				fs << "@" << CI->second->GetGType();
			}
			fs << " ";

			fs << "{" << CI->second->GetFlags() << "}";
			fs << std::endl;
		}
	}

#endif // TSC2_DUMP_TYPE_TREE


	void CScope::Dump(std::ofstream& fs) const
	{
		unsigned tab = 0;
		RecursiveDump(fs, this, tab);
	}

	CType::CType(const Scope* scope) : CScope(CScope::Type, scope->className)
	{
		if (scope->definedType)
		{
			for (std::vector<Type::BaseInfo>::const_iterator I = scope->definedType->derivedFrom.begin(),
				E = scope->definedType->derivedFrom.end();
				I != E; ++I)
			{
				m_derived.insert(I->name);
			}
		}
	}

	const std::set< std::string >& CType::GetDerived() const
	{
		return m_derived;
	}


	void CType::MergeDerived(const CType* pType)
	{
		const std::set<std::string>& derived = pType->GetDerived();
		if (!derived.empty())
		{
			m_derived.insert(derived.begin(), derived.end());
		}
	}

	void CType::MergeDerived(const Scope* scope)
	{
		if (scope->definedType)
		{
			for (std::vector<Type::BaseInfo>::const_iterator I = scope->definedType->derivedFrom.begin(),
				E = scope->definedType->derivedFrom.end();
				I != E; ++I)
			{
				m_derived.insert(I->name);
			}
		}
	}

	CFunction::CFunction(const ::Function& func)
		: m_type(0), m_initArgCount(0), m_flag(0), m_hasScope(false), m_parent(nullptr)
    {
        InitByScope(func);
    }

	CFunction::CFunction(const ::Scope* funcScope)
		: m_type(0), m_initArgCount(0), m_flag(0), m_hasScope(false), m_parent(nullptr)
	{
		InitByScope(funcScope);
	}
    
	void CFunction::InitByScope(const ::Scope* funcScope)
	{
		if (!funcScope || !funcScope->classDef)
		{
			return;
		}

		m_name = funcScope->className;
		m_type = Function::eFunction;
		const Token* tokFunc = funcScope->classDef;

		if (!tokFunc->previous())
		{
			return;
		}

		if (tokFunc->previous()->str() == "~")
		{
			m_type = Function::eDestructor;
		}
		else if (tokFunc->tokAt(-2) && Token::Match(tokFunc->tokAt(-2), "%type ::"))
		{
			const Token* tokClass = tokFunc->tokAt(-2);
			if (tokClass->str() == tokFunc->str())
			{
				m_type = Function::eConstructor;
			}
		}

		const Token* tokClass = tokFunc;
		while (tokClass && tokClass->tokAt(-2) && Token::Match(tokClass->tokAt(-2), "%type% ::"))
		{
			tokClass = tokClass->tokAt(-2);
		}

		const Token* tokRet = tokClass->previous();
		if (tokRet)
		{
			const Settings *pSettings = Settings::Instance();
			while (tokRet && Token::Match(tokRet, "%name%|::|<|>|*|&"))
			{
				if (!pSettings->CheckIfKeywordInFuncRet(tokRet->str()))
				{
					m_return.insert(m_return.begin(), tokRet->str());
				}
				tokRet = tokRet->previous();
			}
		}
		

		if (tokFunc->next() && tokFunc->next()->str() == "(")
		{
			const Token* tokArg = tokFunc->tokAt(2);
			const Token* tokEnd = tokFunc->linkAt(1);
			if (tokEnd)
			{
				
				while (tokArg && tokArg != tokEnd->next())
				{
					const Token* varTypeStart = tokArg;
					const Token* varName = nullptr;
					const Token* varTypeEnd = nullptr;

					unsigned count = 0;
					while (tokArg && !Token::Match(tokArg, ",|)|="))
					{
						if (tokArg->str() == "<" && tokArg->link())
						{
							tokArg = tokArg->link()->next();
							continue;
						}

						tokArg = tokArg->next();
						++count;
					}

					if (tokArg && count >= 2)
					{
						varName = tokArg->previous();
						varTypeEnd = tokArg->tokAt(-2);

						m_args.push_back(CVariable(varTypeStart, varTypeEnd, varName));

						if (tokArg->str() == "=")
						{
							tokArg = Token::findmatch(tokArg, ",|)", tokEnd->next());
							++m_initArgCount;
						}
					}

					tokArg = tokArg->next();
				}
			}
		}

		m_hasScope = funcScope->classStart->str() == "{";
	}

    void CFunction::InitByScope(const ::Function& func)
    {
        m_name = func.name();
        m_type = func.type;
        m_flag = func.getBaseFlag();
		const Settings *pSettings = Settings::Instance();
        for (const Token* tok = func.retDef; tok && tok != func.tokenDef; tok = tok->next()) {
			if (pSettings->CheckIfKeywordInFuncRet(tok->str()))
			{
				continue;
			}
            m_return.push_back(tok->str());
        }
        
        for (std::list<Variable>::const_iterator I = func.argumentList.begin(), E = func.argumentList.end(); I != E; ++I)
        {
            m_args.push_back(CVariable(&(*I)));
            if (I->hasDefault())
            {
                ++m_initArgCount;
            }
        }

		m_hasScope = func.functionScope != nullptr;
        
    }
    
	bool CFunction::operator<(const CFunction& other) const
	{
		if (m_type < other.m_type)
			return true;
		else if (m_type > other.m_type)
			return false;


		if (m_args.size() < other.m_args.size())
			return true;
		else if (m_args.size() > other.m_args.size())
			return false;

		return GetFuncStr() < other.GetFuncStr();
	}
    
    bool CFunction::operator==(const CFunction& other) const
    {
        if (m_type != other.m_type) {
            return false;
        }
        
        if (m_name != other.m_name) {
            return false;
        }
        
        if (m_args.size() != other.m_args.size()) {
            return false;
        }

		if (GetReturnStr() != other.GetReturnStr())
		{
			return false;
		}
        
        for (unsigned i = 0; i < m_args.size(); ++i) {
            if (m_args[i] != other.m_args[i]) {
                return false;
            }
        }
        
        return true;
    }
    
    bool CFunction::operator!=(const CFunction& other) const
    {
        return !(*this == other);
    }
    
    CFunction* CFunction::GetCopy()
    {
        return new CFunction(*this);
    }

	bool CFunction::InitFuncData(const ::Function& func)
	{
		return m_funcData.Init(func, this);
	}

	bool CFunction::InitFuncData(const ::Scope* func)
	{
		return m_funcData.Init(func, this);
	}

	const CVariable* CFunction::GetVariableByIndex(unsigned index) const
	{
		if (index >= m_args.size())
		{
			return nullptr;
		}

		return &m_args[index];
	}

	std::string CFunction::GetReturnStr() const
	{
		std::string sRet;
		for (std::vector<std::string>::const_iterator I = m_return.begin(), E = m_return.end(); I != E; ++I)
		{
			sRet += *I;
		}
		return sRet;
	}

	std::string CFunction::GetFuncStr() const
	{
		std::string sFunc = GetReturnStr() + " " + GetName() + "(" + GetArgStr() + ")";
		return sFunc;
	}

	void CFunction::SyncWithOtherFunc(const CFunction* other)
	{
		m_hasScope = other->m_hasScope;
		m_funcData = other->m_funcData;
	}

	std::string CFunction::GetArgStr() const
	{
		std::string sArg;
		std::vector<CVariable>::const_iterator I = m_args.begin();
		std::vector<CVariable>::const_iterator E = m_args.end();
		if (I != E)
		{
			sArg += I->GetTypeStr();
			++I;
			for (; I != E; ++I)
			{
				sArg += ", ";;
				sArg += I->GetTypeStr();
			}
		}
		return sArg;
	}

	int CFunction::GetParamIndex(const std::string& sName) const
	{
		int index = -1;
		for (std::vector<CVariable>::const_iterator I = m_args.begin(), E = m_args.end(); I != E; ++I)
		{
			++index;
			if (I->GetParamName() == sName)
			{
				break;
			}
		}
		return index;
	}


	bool CFunction::MergeFunc(gt::CFunction* newFunc)
	{
		m_initArgCount = TSC_MAX(m_initArgCount, newFunc->m_initArgCount);
		m_flag = m_flag | newFunc->m_flag;

		if (m_return.empty() && !newFunc->m_return.empty())
		{
			m_return = newFunc->m_return;
		}
		
		if (!newFunc->HasScope())
		{
			return false;
		}
		if (!HasScope())
		{
			SyncWithOtherFunc(newFunc);
			return true;
		}

		gt::CFuncData& newData = newFunc->GetFuncData();
		gt::CFuncData& oldData = GetFuncData();

		if ((int)newData.GetFuncRetNull() > (int)oldData.GetFuncRetNull())
		{
			oldData.SetRetNullFlag(newData.GetFuncRetNull());
		}

		if ((int)newData.GetExitFlag() > (int)oldData.GetExitFlag())
		{
			oldData.SetExitFlag(newData.GetExitFlag());
		}

		//merge out scope var
		{
			std::set<SOutScopeVarState>& sOld = oldData.GetOutScopeVarState();
			std::set<SOutScopeVarState>& sNew = newData.GetOutScopeVarState();
			sOld.insert(sNew.begin(), sNew.end());
		}
		//merge ret param relation
		std::set<gt::CFuncData::ParamRetRelation> *pOld = oldData.GetRetParamRelation();
		std::set<gt::CFuncData::ParamRetRelation> *pNew = newData.GetRetParamRelation();
		for (size_t ii = 0; ii < gt::CFuncData::ParamRetRelationNum; ++ii)
		{
			pOld[ii].insert(pNew[ii].begin(), pNew[ii].end());
		}


		std::set<SVarEntry>& oldDeref = m_funcData.GetDerefedVars();
		const std::set<SVarEntry>& newDeref = newData.GetDerefedVars();
		
		for (std::set<SVarEntry>::const_iterator I = oldDeref.begin(), E = oldDeref.end(); I != E; )
		{
			const SVarEntry& entry = *I;
			if (!newDeref.count(entry))
			{
				oldDeref.erase(I++);
				continue;
			}
			++I;
		}

		return true;
	}

	bool CFunction::Compare(const CFunction* func1, const CFunction* func2)
	{
		return (*func1) < (*func2);
	}

	CVariable::CVariable(const ::Variable* var)
		: m_flag(0), m_flagex(0)
    {
        if (!var) {
            return;
        }
        InitByVariable(var);
    }
    
	CVariable::CVariable(const Token* typeStart, const Token* typeEnd, const Token* tokName) : m_flag(0)
	{
		if (!typeStart || !typeEnd || ! tokName)
		{
			return;
		}

		SetFlagEx(fUnsigned, typeStart->isUnsigned());
		SetFlagEx(fLong, typeStart->isLong());

		for (const Token * tok = typeStart, *tokEnd = typeEnd->next(); tok != tokEnd; tok = tok->next()) {
			if (Token::Match(tok, "struct"))
			{
				continue;
			}
			m_type.push_back(tok->str());
		}

		m_paramName = tokName->str();
	}

	void CVariable::InitByVariable(const ::Variable* var)
    {
        m_flag = var->getBaseFlag();
		if (var->typeStartToken())
		{
			SetFlagEx(fUnsigned, var->typeStartToken()->isUnsigned());
			SetFlagEx(fLong, var->typeStartToken()->isLong());
		}
		// The tokenization may have a defect which makes var's typeEndToken invalid
		// Add an additional check here to keep tsc from crash
		if (!Token::Match(var->typeEndToken(), "(|,"))
		{
			for (const Token * tok = var->typeStartToken(), *tokEnd = var->typeEndToken()->next(); tok && tok != tokEnd; tok = tok->next()) 
			{
				if (Token::Match(tok, "struct"))
				{
					continue;
				}
				m_type.push_back(tok->str());
			}
		}

		m_paramName = var->name();
    }
    
    bool CVariable::operator== (const CVariable& other) const
    {
		return GetTypeStr() == other.GetTypeStr();
    }
    bool CVariable::operator!= (const CVariable& other) const
    {
        return !(*this == other);
    }

	void CVariable::SetFlagEx(FlagEx flag, bool state)
	{
		m_flagex = state ? m_flagex | flag : m_flagex & ~flag;
	}

	bool CVariable::GetFlagEx(FlagEx flag) const
	{
		return ((m_flagex & flag) != 0);
	}

	const std::string& CVariable::GetTypeStartString() const
	{
		if (m_type.empty())
		{
			return emptyString;
		}
		
		return m_type[0];
	}

	void CVariable::SetFlag(unsigned flag, bool state)
	{
		m_flag = state ? m_flag | flag : m_flag & ~flag;
	}

	bool CVariable::GetFlag(unsigned flag) const
	{
		return ((m_flag & flag) != 0);
	}

	std::string CVariable::GetTypeStr() const
	{
		std::string sType;
		for (std::vector<std::string>::const_iterator I = m_type.begin(), E = m_type.end(); I != E; ++I)
		{
			sType += *I;
		}
		return sType;
	}

	//helper class for function body handle
	class CFunctionScopeHandle
	{
		enum RetType
		{
			RetUnknown,
			NoReturn,
			RetRef,
			RetPtr,
			RetInt
		};
		enum  ArgType
		{
			Normal,
			Ptr,
			Ref,
			PtrPtr,
			PtrRef,
			ArgNotFound
		};

		enum PointerValue
		{
			NotInit,
			Null,
			NotNull,
			PossibleNull,
			PossibleNotNull
		};

		enum ConditionType {
			CondUnknown,
			CondAdd,
			CondOr, 
			CondNot
		};
	public:
		CFunctionScopeHandle(const ::Function& f, CFunction* g)
			: func(f)
			, gtFunc(g)
			, m_retParamRelation(g->m_funcData.GetRetParamRelation())
		{

		}
		bool HandleAssignRetValue(CFuncData::ParamRetRelation &prr, const Token *tok)
		{
			if (Token::Match(tok, "%any% ;"))
			{
				if (Token::Match(tok, "false|FALSE"))
				{
					prr.nValue = 0;
					prr.bStr = false;
				}
				else if (Token::Match(tok, "true|TRUE"))
				{
					prr.nValue = 1;
					prr.bStr = false;
				}
				else if (tok->isNumber())
				{
					prr.nValue = atoi(tok->str().c_str());
					prr.bStr = false;
				}
				//todo: return p;
				else
				{
					return false;
				}
				return true;
			}
			return false;
		}

		void ParamCheck()
		{
			//param check
			int nIndex = 0;
			bHasPtrRef = false;
			for (std::list<Variable>::const_iterator iter = func.argumentList.begin(), end = func.argumentList.end();
			iter != end; ++iter, ++nIndex)
			{
				int argType = Normal;
				const Variable *pVar = &(*iter);
				if (pVar->isPointer())
				{
					if (pVar->isReference())
					{
						argType = PtrRef;
						bHasPtrRef = true;
					}
					else
					{
						int starCount = 0;
						for (const Token *tokDef = pVar->typeStartToken(); tokDef && tokDef != pVar->typeEndToken(); tokDef = tokDef->next())
						{
							if (tokDef->str() == "*")
							{
								++starCount;
								if (starCount >= 2)
								{
									break;
								}
							}
						}
						if (pVar->typeEndToken() && pVar->typeEndToken()->str() == "*")
						{
							++starCount;
						}
						if (starCount >= 2)
						{
							argType = PtrPtr;
							bHasPtrRef = true;
						}
						else
						{
							argType = Ptr;
							bHasPtrRef = true;
						}
					}
				}
				else if (pVar->isReference())
				{
					argType = Ref;
					bHasPtrRef = true;
				}
				if (argType != Normal)
				{
					paramIndex[iter->name()] = std::make_pair(nIndex, argType);
				}
			}

			//init arg
			vArgState.resize(func.argCount(), NotInit);
		}

		void RetCheck()
		{
			nRetType = RetUnknown;
			//pass return value type
			if (gtFunc->m_return.empty())
			{
				nRetType = NoReturn;
			}
			else
			{
				if (*(gtFunc->m_return.rbegin()) == "void")
				{
					nRetType = NoReturn;
					return;
				}
				int nStartCount = 0;
				int nAndCount = 0;
				for (int nIndex = 0, nSize = gtFunc->m_return.size(); nIndex < nSize; ++nIndex)
				{
					if (gtFunc->m_return[nIndex] == "*")
					{
						++nStartCount;
					}
					else if (gtFunc->m_return[nIndex] == "&")
					{
						++nAndCount;
					}
				}
				if (nAndCount == 0)
				{
					if (nStartCount == 0)
					{
						nRetType = RetInt;
					}
					else if (nStartCount == 1)
					{
						nRetType = RetPtr;
					}
				}
			}
		}

		inline bool AssignNull(const Token *tok)
		{
			if (tok->next()->str() == "0")
			{
				return true;
			}
			if (tok->next()->str() == "new")
			{
				return false;
			}

			//default to not null
			return false;
		}
		void HandleAssign(const Token *tok)
		{
			if (!tok->previous() || !tok->astOperand1())
			{
				return;
			}
			if (!tok->next())
			{
				return;
			}
			//todo: a[1] = xxx;
			const Variable *pVar = tok->previous()->variable();
			if (!pVar || !pVar->nameToken())
			{
				return;
			}

			//check p->x = 0;
			if (tok->astOperand1()->str() == ".")
			{
				const Token *tokTmp = tok->astOperand1();
				while (tokTmp && tokTmp->astOperand1())
				{
					if (tokTmp->str() != ".")
					{
						break;
					}
					tokTmp = tokTmp->astOperand1();
				}
				if (tokTmp->str() == "::" && tokTmp->astParent())
				{
					tokTmp = tokTmp->astParent()->astOperand2();
				}
				if (tokTmp && tokTmp->variable())
				{
					const Variable *pVarParent = tokTmp->variable();
					if (pVarParent->isArgument() || pVarParent->isLocal())
					{
						return;
					}
				}
			}
			//handle only g=xxx; escape *g = xxx;
			if ((!pVar->isLocal() && !pVar->isArgument()))
			{
				const Token *tokTmp = tok->astOperand1();
				while (tokTmp && tokTmp->astOperand1())
				{
					if (tokTmp->str() == "*")
					{
						return;
					}
					tokTmp = tokTmp->astOperand1();
				}
				bool bNull;
				if (AssignNull(tok))
				{
					bNull = true;
				}
				else
				{
					bNull = false;
				}
				const std::string &varName = pVar->name();
				std::string strScope = "";
				const Scope *pScope = pVar->scope();
				//todo a.xxx = new xxx;
				while (pScope)
				{
					strScope = pScope->className + ":" + strScope;
					pScope = pScope->nestedIn;
				}
				strScope = strScope + varName;
				if (vOutVarState.find(strScope) == vOutVarState.end() || !bNull)
				{
					vOutVarState[strScope] = SOutScopeVarState(strScope, bNull);
				}
			}
			//handle argument
			else if (pVar->isArgument())
			{
				std::map<std::string, std::pair<int, int> >::const_iterator iter = paramIndex.find(pVar->name());
				if (iter == paramIndex.end())
				{
					return;
				}
				//foo(T **p) => *p = xxx;
				if (iter->second.second != PtrPtr && iter->second.second != PtrRef)
				{
					return;
				}
				if (iter->second.second == PtrPtr)
				{
					if (!tok->astOperand1() || tok->astOperand1()->str() != "*" 
						|| (tok->astOperand1()->astOperand1() && tok->astOperand1()->astOperand1()->str() == "*"))
					{
						return;
					}
				}
				else
				{
					if (!tok->astOperand1() || tok->astOperand1()->str() == "*")
					{
						return;
					}
				}
				if ((size_t)iter->second.first < vArgState.size())
				{
					if (AssignNull(tok))
					{
						vArgState[iter->second.first] = Null;
					}
					else
					{
						vArgState[iter->second.first] = NotNull;
					}
					bHasAssign = true;
				}
			}
		}

		void SaveOutScopeVarState()
		{
			std::set<SOutScopeVarState> &vOut = gtFunc->m_funcData.GetOutScopeVarState();
			for (std::map<std::string, SOutScopeVarState>::const_iterator iter = vOutVarState.begin(), end = vOutVarState.end(); iter != end; ++iter)
			{
				vOut.insert(iter->second);
			}
		}

		const inline std::pair<int, int> FindParamIndex(const Token *tok)
		{
			if (!tok)
			{
				return std::make_pair(-1, -1);
			}
			if (!tok->variable())
			{
				return std::make_pair(-1, -1);;
			}
			if (!tok->variable()->isArgument())
			{
				return std::make_pair(-1, -1);;
			}
			std::map<std::string, std::pair<int, int> >::const_iterator iter = paramIndex.find(tok->str());
			if (iter == paramIndex.end())
			{
				return std::make_pair(-1, -1);
			}
			
			return iter->second;
		}

		inline bool IsPtrType(int nType)
		{
			return nType == PtrPtr || nType == Ptr || nType == PtrRef;
		}

		void SaveRetParamRelation(CFuncData::ParamRetRelation &prr, int nIndex, int nValue, bool bNull)
		{
			prr.nIndex = nIndex;
			prr.bEqual = true;
			prr.bStr = false;
			prr.bNull = bNull;
			prr.nValue = nValue;
			s.insert(prr);
		}

		void HandleReturn(const Token *tok)
		{
			if (bUnderIf)
			{
				bIfThenRet = true;
			}
			if (!tok->next())
			{
				return;
			}
			if (tok->next()->str() == ";")
			{
				return;
			}

			//function return int, possible that return value means parameter not null
			if (nRetType == RetInt)
			{
				if (!bReturnUnknown)
				{
					CFuncData::ParamRetRelation prr;
					if (!HandleAssignRetValue(prr, tok->next()))
					{
						bStrictIf = false;
						if (HandleCondition(tok->next()))
						{
							//Compute prefix expr value
							int countOr = 0;
							int countAnd = 0;
							int bHasArgument = false;
							for (size_t ii = 0, nSize = vPrefixExpr.size(); ii < nSize; ++ii)
							{
								if (vPrefixExpr[ii].first == -2)
								{
									if (vPrefixExpr[ii].second == CondAdd)
									{
										++countAnd;
									}
									else if (vPrefixExpr[ii].second == CondOr)
									{
										++countOr;
									}
								}
								if (vPrefixExpr[ii].first >= 0)
								{
									bHasArgument = true;
								}
							}
							if (!bHasArgument)
							{
								bHasError = true;
								return;
							}
							assert(vPrefixExpr.size() > 0);
							bStrictIf = false;
							// strictly if (p)
							if (countOr == 0 && countAnd == 0)
							{
								if (vPrefixExpr[0].first >= 0)
								{
									if (vPrefixExpr[0].second == Null)
									{
										SaveRetParamRelation(prr, vPrefixExpr[0].first, 0, false);
										SaveRetParamRelation(prr, vPrefixExpr[0].first, 1, true);
										paramCheckIndex.insert(prr.nIndex);
									}
									else if (vPrefixExpr[0].second == NotNull)
									{
										SaveRetParamRelation(prr, vPrefixExpr[0].first, 1, false);
										SaveRetParamRelation(prr, vPrefixExpr[0].first, 0, true);
										paramCheckIndex.insert(prr.nIndex);
									}
								}
							}
							else if (countOr > 0 && countAnd == 0)
							{
								std::set<std::pair<int, int> > sParamSet;
								for (size_t ii = 0, nSize = vPrefixExpr.size(); ii < nSize; ++ii)
								{
									if (vPrefixExpr[ii].first >= 0)
									{
										std::set<std::pair<int, int> >::const_iterator iterTmp = sParamSet.find(vPrefixExpr[ii]);
										if (iterTmp != sParamSet.end() && iterTmp->second != vPrefixExpr[ii].second)
										{
											std::fill(vArgState.begin(), vArgState.end(), NotInit);
											return;
										}
										if (vPrefixExpr[ii].second == Null)
										{
											SaveRetParamRelation(prr, vPrefixExpr[ii].first, 0, false);
											paramCheckIndex.insert(vPrefixExpr[ii].first);
										}
										else if (vPrefixExpr[ii].second == NotNull)
										{
											SaveRetParamRelation(prr, vPrefixExpr[ii].first, 0, true);
											paramCheckIndex.insert(vPrefixExpr[ii].first);
										}
									}
								}
							}
							else if (countOr == 0 && countAnd > 0)
							{
								std::set<std::pair<int, int> > sParamSet;
								for (size_t ii = 0, nSize = vPrefixExpr.size(); ii < nSize; ++ii)
								{
									if (vPrefixExpr[ii].first >= 0)
									{
										std::set<std::pair<int, int> >::const_iterator iterTmp = sParamSet.find(vPrefixExpr[ii]);
										if (iterTmp != sParamSet.end() && iterTmp->second != vPrefixExpr[ii].second)
										{
											std::fill(vArgState.begin(), vArgState.end(), NotInit);
											return;
										}
										if (vPrefixExpr[ii].second == NotNull)
										{
											SaveRetParamRelation(prr, vPrefixExpr[ii].first, 1, false);
											paramCheckIndex.insert(prr.nIndex);
										}
										else if (vPrefixExpr[ii].second == Null)
										{
											SaveRetParamRelation(prr, vPrefixExpr[ii].first, 1, true);
											paramCheckIndex.insert(prr.nIndex);
										}
									}
								}
							}
						}
						else
						{
							bHasError = true;
						}
						retTypes.insert(std::make_pair(0, prr.str));
						retTypes.insert(std::make_pair(1, prr.str));
					}
					else
					{
						if ((bUnderIf && bStrictIf) || (!bUnderIf && bStrictElse))
						{
							for (int ii = 0, nSize = vArgState.size(); ii < nSize; ++ii)
							{
								if (vArgState[ii] == NotNull)
								{
									prr.bEqual = true;
									prr.nIndex = ii;
									prr.bNull = false;
									CFuncData::ParamRetRelation tmp = prr;
									tmp.bNull = !prr.bNull;
									if (s.find(tmp) == s.end())
									{
										s.insert(prr);
										paramCheckIndex.insert(ii);
									}
								}
								else if (vArgState[ii] == Null)
								{
									prr.bEqual = true;
									prr.nIndex = ii;
									prr.bNull = true;
									CFuncData::ParamRetRelation tmp = prr;
									tmp.bNull = !prr.bNull;
									if (s.find(tmp) == s.end())
									{
										s.insert(prr);
										paramCheckIndex.insert(ii);
									}
								}
								//todo: handle conflict
							}
						}
						//no matter return produce an  equal condition
						//increase return value type count
						retTypes.insert(std::make_pair(prr.nValue, prr.str));
					}
				}
			}
		}

		template <class T>
		static void MoveForward(std::vector<T> &v, std::size_t nStart, std::size_t nEnd)
		{
			for (std::size_t ii = nStart; ii < nEnd; ++ii)
			{
				v[ii - 1] = v[ii];
			}
		}

		//p.x   together cannot be function parameter
		bool HandleConditionLeaf(const Token *tok, std::pair<int, int> &leafValue)
		{
			leafValue.first = -1;
			if (!tok)
			{
				return false;
			}
			bool bEqualNull = false;
			const Token *ifName = nullptr;
			//p
			if (!tok->astOperand1() && !tok->astOperand2())
			{
				ifName = tok;
				bEqualNull = false;
			}
			//!p
			else if (Token::Match(tok, "!") && !tok->astOperand2())
			{
				assert(0);//already handled in up level function
				ifName = tok->astOperand1();
				bEqualNull = true;
			}
			//p ==|!= 0
			else if (tok->astOperand1() && tok->astOperand2())
			{
				if (tok->str() == "==")
				{
					bEqualNull = true;
				}
				else if (tok->str() == "!=")
				{
					bEqualNull = false;
				}
				else
				{
					return false;
				}
				if (tok->astOperand1()->str() == "0" && tok->astOperand2()->isName())
				{
					ifName = tok->astOperand2();
				}
				else if (tok->astOperand2()->str() == "0" && tok->astOperand1()->isName())
				{
					ifName = tok->astOperand1();
				}
				else
				{
					return false;
				}
			}
			if (!ifName)
			{
				return false;
			}
			const Variable * pVar = ifName->variable();
			if (!pVar)
			{
				return false;
			}
			std::map<std::string, std::pair<int, int> >::const_iterator iter = paramIndex.find(pVar->name());
			if (iter == paramIndex.end())
			{
				return false;
			}
			if (!IsPtrType(iter->second.second))
			{
				return false;
			}

			leafValue.first = iter->second.first;
			leafValue.second = bEqualNull ? Null : NotNull;
			return true;
		}

		int NegateCheckstate(int cs)
		{
			if (cs == Null || cs == PossibleNull)
			{
				return NotNull;
			}

			if (cs == NotNull || cs == PossibleNotNull)
			{
				return Null;
			}
			
			return NotInit;
		}

		int NegateCondition(int cd)
		{
			if (cd == CondAdd)
			{
				return CondOr;
			}

			if (cd == CondOr)
			{
				return CondAdd;
			}

			return cd;
		}

#ifdef _DEBUG_IF_Condition
		void DumpPrefixExpr(const std::vector<std::pair<int, int> > &expr, int invalid)
		{
			for (std::size_t ii = 0, nSize = expr.size() - invalid; ii < nSize; ++ii)
			{
				if (expr[ii].first == -2)
				{
					if (expr[ii].second == CondAdd)
					{
						std::cout << "&";
					}
					else if (expr[ii].second == CondOr)
					{
						std::cout << "|";
					}
					else if (expr[ii].second == CondNot)
					{
						std::cout << "!";
					}
					else
					{
						std::cout << "[op]";
					}
				}
				else if (expr[ii].first >= -1)
				{
					if (expr[ii].second == Null)
					{
						std::cout << "(" << expr[ii].first << "==0)";
					}
					else if (expr[ii].second == NotNull)
					{
						std::cout << "(" << expr[ii].first << "!=0)";
					}
					else if (expr[ii].second == NotInit)
					{
						std::cout << "(" << expr[ii].first << " notset)";
					}
				}
			}
			std::cout << std::endl;
		}
#else
#define DumpPrefixExpr(a, b) ((void)0);
#endif // _DEBUG

		//return not count
		void RemoveCondNot(std::vector<std::pair<int, int> > &expr, int start, int countNot)
		{
			//!p
			if (expr[start].first >= -1)
			{
				expr[start].second = NegateCheckstate(expr[start].second);
			}
			else
			{
				std::stack<int> sTmp;
				sTmp.push(start);
				while (!sTmp.empty())
				{
					int index = sTmp.top();
					sTmp.pop();
					if (expr[index].first != -2)
					{
						return;
					}
					if (expr[index + 1].first > -2 && expr[index + 2].first > -2)
					{
						expr[index + 1].second = NegateCheckstate(expr[index + 1].second);
						expr[index + 2].second = NegateCheckstate(expr[index + 2].second);
						expr[index].second = NegateCondition(expr[index].second);
					}
					else if (expr[index + 1].first == -2 && expr[index + 2].first > -2)
					{
						sTmp.push(index + 1);
						expr[index + 2].second = NegateCheckstate(expr[index + 2].second);
					}
					else if (expr[index + 1].first > -2 && expr[index + 2].first == -2)
					{
						sTmp.push(index + 2);
						expr[index + 1].second = NegateCheckstate(expr[index + 1].second);
					}
					else
					{
						sTmp.push(index + 1);
						sTmp.push(index + 2);
					}
				}
			}
			MoveForward(expr, start, expr.size() - countNot);
			return ;
		}

		//@param tok: parent of ast
		//!(p || q) supported
		//p || !(p || q), (p||q) considered as leaf
		bool HandleConditionTree(const Token* tok)
		{
			if (!tok)
			{
				return false;
			}
			std::stack<const Token *> sCond;
			sCond.push(tok);
			//prefix expression
			std::pair<int, int> leafValue;
			while (!sCond.empty())
			{
				const Token *tmp = sCond.top();
				sCond.pop();
				if (tmp->str() == "&&")
				{
					if (!tmp->astOperand2())
					{
						return false;
					}
					sCond.push(tmp->astOperand2());

					if (!tmp->astOperand1())
					{
						return false;
					}
					sCond.push(tmp->astOperand1());
					vPrefixExpr.push_back(std::make_pair<int, int>(-2, CondAdd));
				}
				else if (tmp->str() == "||")
				{
					if (!tmp->astOperand2())
					{
						return false;
					}
					sCond.push(tmp->astOperand2());
					if (!tmp->astOperand1())
					{
						return false;
					}
					sCond.push(tmp->astOperand1());
					vPrefixExpr.push_back(std::make_pair<int, int>(-2, CondOr));
				}
				else if (tmp->str() == "!")
				{
					if (!tmp->astOperand1())
					{
						return false;
					}
					sCond.push(tmp->astOperand1());
					vPrefixExpr.push_back(std::make_pair<int, int>(-2, CondNot));
				}
				else if (Token::Match(tmp, "==|!=|%var%"))
				{
					if (HandleConditionLeaf(tmp, leafValue))
					{
						vPrefixExpr.push_back(leafValue);
					}
					else
					{
						vPrefixExpr.push_back(UnKnownLeaf);
					}
				}
				//a.b, a::b, *p, p[0],foo(p)
				//todo: check foo(a, b)
				else if (Token::Match(tmp, ".|::|*|[|("))
				{
					vPrefixExpr.push_back(UnKnownLeaf);
				}
				else
				{
					return false;
				}
			}
			DumpPrefixExpr(vPrefixExpr, 0);
			
			//simplify prefix expr value
			//!||pq => &&!p!q
			//!&&pq => ||!p!q
			int countNot = 0;
			for (int nSize = (int)vPrefixExpr.size(), ii = nSize - 2; ii >= 0; --ii)
			{
				if (vPrefixExpr[ii].first == -2)
				{
					if (vPrefixExpr[ii].second == CondNot)
					{
						RemoveCondNot(vPrefixExpr, ii + 1, countNot);
						++countNot;
					}
				}
			}
			DumpPrefixExpr(vPrefixExpr, countNot);
			vPrefixExpr.resize(vPrefixExpr.size() - countNot);
			return true;
		}
		//[tokStart, tokEnd)
		bool HandleCondition(const Token *tokStart)
		{
			vPrefixExpr.clear();
			if (!tokStart)
			{
				return false;
			}

			const Token *astTop = tokStart->astTop();
			if (!astTop)
			{
				return false;
			}
			if (astTop->str() == "return")
			{
				return HandleConditionTree(astTop->astOperand1());
			}
			else if (astTop->str() == "(")
			{
				if (Token::Match(astTop->astOperand1(), "if|return"))
				{
					return HandleConditionTree(astTop->astOperand2());
				}
				//handle return (p == NULL);
				else if (!astTop->astOperand1() && Token::simpleMatch(astTop->previous(), "return"))
				{
					return HandleConditionTree(astTop->previous()->astOperand1());
				}
			}
			return false;
		}

		
		void HandleScopeBegin(const Token *tok)
		{
			bScopeHandled = false;
			if (!tok->previous())
			{
				return;
			}
			//handle else
			if (tok->previous()->str() == "else" && bHasElse)
			{
				NegateArgState();
				bHasElse = false;
			}
			else
			{
				if (tok->previous()->str() != ")")
				{
					bHasError = true;
					return;
				}
				//cannot be null, for loop from second token
				const Token *ifEnd = tok->previous();
				if (!ifEnd->link())
				{
					bHasError = true;
					return;
				}
				const Token* ifStart = ifEnd->link();
				if (ifStart->previous()->str() == "if")
				{
					if (HandleCondition(ifStart->next()))
					{
						//Compute prefix expr value
						int countOr = 0;
						int countAnd = 0;
						int bHasArgument = false;
						for (size_t ii = 0, nSize = vPrefixExpr.size(); ii < nSize; ++ii)
						{
							if (vPrefixExpr[ii].first == -2)
							{
								if (vPrefixExpr[ii].second == CondAdd)
								{
									++countAnd;
								}
								else if (vPrefixExpr[ii].second == CondOr)
								{
									++countOr;
								}
							}
							if (vPrefixExpr[ii].first >= 0)
							{
								bHasArgument = true;
							}
						}
						if (!bHasArgument)
						{
							bHasError = true;
							return;
						}
						if (vPrefixExpr.size() == 0)
						{
							return;
						}
						bStrictIf = false;
						bStrictElse = false;
						// strictly if (p)
						if (countOr == 0 && countAnd == 0)
						{
							if (vPrefixExpr[0].first >= 0)
							{
								bStrictIf = true;
								bStrictElse = true;
								vArgState[vPrefixExpr[0].first] = vPrefixExpr[0].second;
							}
						}
						else if (countOr > 0 && countAnd == 0)
						{
							std::set<std::pair<int, int> > sParamSet;
							for (size_t ii = 0, nSize = vPrefixExpr.size(); ii < nSize; ++ii)
							{
								if (vPrefixExpr[ii].first >= 0)
								{
									std::set<std::pair<int, int> >::const_iterator iterTmp = sParamSet.find(vPrefixExpr[ii]);
									if (iterTmp != sParamSet.end() && iterTmp->second != vPrefixExpr[ii].second)
									{
										std::fill(vArgState.begin(), vArgState.end(), NotInit);
										return;
									}
									sParamSet.insert(vPrefixExpr[ii]);
									if (vPrefixExpr[ii].second == NotNull)
									{
										vArgState[vPrefixExpr[ii].first] = PossibleNotNull;
									}
									else if (vPrefixExpr[ii].second == Null)
									{
										vArgState[vPrefixExpr[ii].first] = PossibleNull;
									}
								}
							}
							//if (!p || *p == 0) else return true;
							bStrictElse = true;
						}
						else if (countOr == 0 && countAnd > 0)
						{
							std::set<std::pair<int, int> > sParamSet;
							for (size_t ii = 0, nSize = vPrefixExpr.size(); ii < nSize; ++ii)
							{
								if (vPrefixExpr[ii].first >= 0)
								{
									std::set<std::pair<int, int> >::const_iterator iterTmp = sParamSet.find(vPrefixExpr[ii]);
									if (iterTmp != sParamSet.end() && iterTmp->second != vPrefixExpr[ii].second)
									{
										std::fill(vArgState.begin(), vArgState.end(), NotInit);
										return;
									}
									sParamSet.insert(vPrefixExpr[ii]);
									vArgState[vPrefixExpr[ii].first] = vPrefixExpr[ii].second;
								}
							}
							bStrictIf = true;
						}
						//has both && and ||
						else
						{
							//1. p || XXX
							if (vPrefixExpr[0].first == -2 )
							{
								if (vPrefixExpr[1].first >= 0)
								{
									if (vPrefixExpr[1].second == NotNull)
									{
										vArgState[vPrefixExpr[1].first] = PossibleNotNull;
									}
									else if (vPrefixExpr[1].second == Null)
									{
										vArgState[vPrefixExpr[1].first] = PossibleNull;
									}
									bStrictElse = true;
								}
								else if (vPrefixExpr[1].first == -2)
								{
									std::stack<int> sVisit;
									size_t nLast = vPrefixExpr.size() - 1;
									sVisit.push(1);
									size_t tmp = 1;
									while (!sVisit.empty() && tmp + 1 <= nLast)
									{
										if (vPrefixExpr[tmp + 1].first == -2)
										{
											sVisit.push(tmp);
										}
										else if (vPrefixExpr[tmp + 1].first >= -1)
										{
											if (vPrefixExpr[sVisit.top()].first >= -1)
											{
												sVisit.pop();
												sVisit.pop();
												if (!sVisit.empty() && vPrefixExpr[sVisit.top()].first == -2)
												{
													sVisit.push(tmp + 1);
												}
											}
											else
											{
												sVisit.push(tmp + 1);
											}
										}
										tmp += 1;
									}
									//2. XXX || p
									if (tmp == nLast - 1 && sVisit.empty())
									{
										if (vPrefixExpr[nLast].second == NotNull)
										{
											vArgState[vPrefixExpr[nLast].first] = PossibleNotNull;
										}
										else if (vPrefixExpr[nLast].second == Null)
										{
											vArgState[vPrefixExpr[nLast].first] = PossibleNull;
										}
										bStrictElse = true;
									}
									//XXX || XXX
									else
									{
									}
								}
							}
						}
					}
					else
					{
						bHasError = true;
					}
					
					bUnderIf = true;
					//go on check if there is an else
					const Token *bodyEnd = tok->link();
					if (!bodyEnd)
					{
						bHasError = true;
						return;
					}
					if (Token::Match(bodyEnd, "} else { "))
					{
						bHasElse = true;
					}
					else
					{
						bHasElse = false;
					}
				}
				else
				{
					//not if (xxx)
					return;
				}
			}
			bScopeHandled = true;
		}

		void NegateArgState()
		{
			for (size_t nIndex = 0, nSize = vArgState.size(); nIndex < nSize; ++nIndex)
			{
				vArgState[nIndex] = NegateCheckstate(vArgState[nIndex]);
			}
		}

		void HandleFunctionParam(const Token *tok)
		{
			const Token *paramEnd = tok->link();
			if (!paramEnd)
			{
				return;
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
						return;
					}
				}
				if (tokFunc->astUnderSizeof())
				{
					return;
				}
			}
			if (tokFunc->tokType() != Token::eVariable)
			{
				if (tokFunc->isName() && !Token::Match(tokFunc, "if|while|for|switch"))
				{
					tok = tok->astOperand2();
					std::vector<const Token*> params;
					while (tok && tok->str() == ",")
					{
						if (tok->astOperand2())
						{
							params.push_back(tok->astOperand2());
						}
						tok = tok->astOperand1();
					}

					if (tok)
					{
						params.push_back(tok);
					}
					//fread like function may init the first param
					bool bFuncRead = std::string::npos != tokFunc->str().find("read") || std::string::npos != tokFunc->str().find("Read");
					for (std::vector<const Token*>::iterator iter = params.begin(), end = params.end(); iter != end; ++iter)
					{
						const Token *t = nullptr;
						//passible casts
						//consider static_cast style casts as function
						if ((*iter)->str() == "(")
						{
							*iter = (*iter)->astOperand1();
						}
						//possible assign value
						if (*iter && (*iter)->str() == "&")
						{
							t = (*iter)->astOperand1();
							while (t->astOperand2())
							{
								t = t->astOperand2();
							}
						}
						if (!t && bFuncRead && iter == params.begin())
						{
							bFuncRead = false;
							t = *iter;
							if (t->str() == ".")
							{
								t = t->astOperand2();
							}
						}
						if (t && t->variable())
						{
							const Variable *pVar = t->variable();
							if (!pVar->isLocal() && !pVar->isArgument())
							{
								const std::string &varName = pVar->name();
								std::string strScope = "";
								const Scope *pScope = pVar->scope();
								while (pScope)
								{
									strScope = pScope->className + ":" + strScope;
									pScope = pScope->nestedIn;
								}
								strScope = strScope + varName;
								if (vOutVarState.find(strScope) == vOutVarState.end())
								{
									vOutVarState[strScope] = SOutScopeVarState(strScope, false);
								}
							}
						}
					}
				}
			}
		}

		void HandleThis(const Token *tok)
		{
			//case 1, memset(this, 0, sizeof(*this))
			if (Token::Match(tok->tokAt(-2), ZERO_MEM_PATTERN))
			{
				gtFunc->m_funcData.AssignThis(true);
			}
		}

		void HandleFunctionScope()
		{
			pSettings = Settings::Instance();
			RetCheck();
			ParamCheck();
			bHasAssign = false;
			bHasElse = false;
			bReturnUnknown = false;
			bHasError = false;
			bIfThenRet = false;
			bUnderIf = false;
			bScopeHandled = false;
			bStrictIf = false;
			bStrictElse = false;
			gtFunc->m_funcData.SetExitFlag(CFuncData::EF_fUnknown);
			std::stack<int> sScopeHandle;
			std::set<std::string>::const_iterator endLibExit = pSettings->library.functionexit.end();
			for (const Token *tok = func.functionScope->classStart->next(); tok && tok != func.functionScope->classEnd; tok = tok->next())
			{
				if (tok->str() == "=")
				{
					HandleAssign(tok);
				}
				else if (tok->str() == "{")
				{
					if (!bHasError)
					{
						HandleScopeBegin(tok);
						if (bScopeHandled)
						{
							st.push(vArgState);
						}
						sScopeHandle.push(bScopeHandled);
					}
				}
				else if (tok->str() == "}")
				{
					if (!bHasError)
					{
						if (st.empty() || sScopeHandle.empty())
						{
							bHasError = true;//there is an error
							continue;
						}
						if (!sScopeHandle.top())
						{
							sScopeHandle.pop();
							continue;
						}
						sScopeHandle.pop();
						vArgState = st.top();
						st.pop();
						//if (xxx){ return ;} then !xxx
						if (bUnderIf && bIfThenRet && !bHasElse)
						{
							NegateArgState();
							bIfThenRet = false;
							bUnderIf = false;
						}
					}
				}
				else if (tok->str() == "return")
				{
					if (!bHasError)
					{
						HandleReturn(tok);
					}
				}
				else if (tok->str() == "this")
				{
					HandleThis(tok);
				}
				else if (tok->isName())
				{
					if (Token::Match(tok, "throw") || pSettings->library.functionexit.find(CGlobalTokenizer::FindFullFunctionName(tok)) != endLibExit)
					{
						gtFunc->m_funcData.SetExitFlag(CFuncData::ExitFlag::fExit);
						if (bUnderIf)
						{
							bIfThenRet = true;
						}
					}
					if (bUnderIf && !bIfThenRet && Token::Match(tok, pSettings->returnKeyWord))
					{
						bIfThenRet = true;
					}
				}
				else if (tok->str() == "(")
				{
					HandleFunctionParam(tok);
				}
			}
			SaveOutScopeVarState();
			if (!bHasError && !bReturnUnknown)
			{
				//set always true
				if (retTypes.size() == 1 && bHasAssign)
				{
					for (std::map<std::string, std::pair<int, int> >::const_iterator iter = paramIndex.begin(), end = paramIndex.end(); iter != end; ++iter)
					{
						m_retParamRelation[0].insert(CFuncData::ParamRetRelation((unsigned short)iter->second.first, 0, false, true, true));
					}
				}
				else if (retTypes.size() > 1)
				{
					if (bStrictIf)
					{
						//add more equal condition
						//0->null, 1->not, 2->not, then add !0->not
						for (std::set<int>::const_iterator iterIndex = paramCheckIndex.begin(), endIndex = paramCheckIndex.end(); iterIndex != endIndex; ++iterIndex)
						{
							int zeroState = NotInit;
							bool overState = true;
							int lastOverState = NotInit;
							bool underState = true;
							int lastUnderState = NotInit;
							int nParamIndex = -1;
							for (std::set<CFuncData::ParamRetRelation>::const_iterator iter = s.begin(), end = s.end(); iter != end; ++iter)
							{
								if (iter->nIndex != *iterIndex)
								{
									continue;
								}
								nParamIndex = iter->nIndex;
								if (iter->nValue == 0)
								{
									//todo: if conflict
									zeroState = iter->bNull ? Null : NotNull;
								}
								else if (iter->nValue < 0)
								{
									if (!underState)
									{
										continue;
									}
									if (lastUnderState == NotInit)
									{
										lastUnderState = iter->bNull ? Null : NotNull;
									}
									else
									{
										if ((lastUnderState == Null && !iter->bNull)
											|| (lastUnderState == NotNull && iter->bNull))
										{
											underState = false;
										}
									}
								}
								else
								{
									if (!overState)
									{
										continue;
									}
									if (lastOverState == NotInit)
									{
										lastOverState = iter->bNull ? Null : NotNull;
									}
									else
									{
										if ((lastOverState == Null && !iter->bNull)
											|| (lastOverState == NotNull && iter->bNull))
										{
											overState = false;
										}
									}
								}
							}
							if (nParamIndex != -1 && zeroState != NotInit && overState && underState && (lastOverState != NotInit || lastUnderState != NotInit))
							{
								CFuncData::ParamRetRelation prr((unsigned short)nParamIndex, 0, false, false);
								if (zeroState == Null)
								{
									if ((lastOverState == NotNull && lastUnderState != Null)
										|| (lastOverState != Null && lastUnderState == NotNull))
									{
										prr.bNull = false;
										s.insert(prr);
									}
								}
								else if (zeroState == NotNull)
								{
									if ((lastOverState == Null && lastUnderState != NotNull)
										|| (lastOverState != Null && lastUnderState == Null))
									{
										prr.bNull = true;
										s.insert(prr);
									}
								}
							}
						}
					}
					m_retParamRelation[0].insert(s.begin(), s.end());
				}
			}

			if (nRetType != NoReturn && m_retParamRelation[0].empty())
			{
				//no relation found
				for (std::map<std::string, std::pair<int, int> >::const_iterator iterParam = paramIndex.begin(), end = paramIndex.end(); iterParam != end; ++iterParam)
				{
					if (iterParam->second.second == PtrPtr || iterParam->second.second == PtrRef)
					{
						m_retParamRelation[0].insert(CFuncData::ParamRetRelation((unsigned short)iterParam->second.first, 0, false, true, true));
					}
				}
			}

			//handle T * foo(flag &)
			if (nRetType != RetPtr || bReturnUnknown || !m_retParamRelation[0].empty())
			{
				return;
			}
			std::vector<CFuncData::ParamRetRelation> vArgValue(func.argCount());
			std::vector<bool> vArgInit(func.argCount(), false);
			s.clear();
			bHasError = false;
			for (const Token *tok = func.functionScope->classStart; tok && tok != func.functionScope->classEnd; tok = tok->next())
			{
				if (bHasError)
				{
					return;
				}
				if (tok->str() == "=")
				{
					if (!tok->previous())
					{
						continue;
					}
					if (!tok->next())
					{
						continue;
					}
					const Variable *pVar = tok->previous()->variable();
					if (!pVar)
					{
						continue;
					}
					if (!pVar->isArgument())
					{
						bHasError = true;
						continue;
					}
					std::map<std::string, std::pair<int, int> >::const_iterator iter = paramIndex.find(pVar->name());
					if (iter == paramIndex.end())
					{
						continue;
					}
					if (iter->second.second == Normal)
					{
						continue;
					}
					CFuncData::ParamRetRelation prr;
					if (!HandleAssignRetValue(prr, tok->next()))
					{
						s.clear();
						break;
					}
					if ((size_t)iter->second.first < vArgInit.size())
					{
						vArgInit[iter->second.first] = true;
						vArgValue[iter->second.first] = prr;
					}
				}
				else if (tok->str() == "return")
				{
					if (tok->next() && tok->next()->str() != ";")
					{
						int bRetNull = NotInit;
						if (tok->next()->str() == "0")
						{
							bRetNull = Null;
						}
						else
						{
							bRetNull = false;
						}
						for (int ii = 0, nSize = vArgInit.size(); ii < nSize; ++ii)
						{
							if (vArgInit[ii])
							{
								vArgValue[ii].bEqual = (bRetNull == Null);
								vArgValue[ii].nIndex = (unsigned short)ii;
								vArgValue[ii].bEqual = true;
								s.insert(vArgValue[ii]);
							}
							else
							{
								bHasError = true;
							}
						}
					}
				}
			}
			m_retParamRelation[1].insert(s.begin(), s.end());
		}
	private:
		const ::Function& func;
		CFunction *gtFunc;
		std::set<CFuncData::ParamRetRelation> *m_retParamRelation;
		std::map<std::string, SOutScopeVarState> vOutVarState;
		std::set<std::string> sConflictVarState;
		std::map<std::string, std::pair<int, int> > paramIndex;
		std::vector<int>  vArgState;
		std::set<std::pair<int, std::string> > retTypes;
		std::set<CFuncData::ParamRetRelation> s;
		std::stack<std::vector<int> > st;
		std::set<int> paramCheckIndex;
		std::vector<std::pair<int, int> > vPrefixExpr;
		static const std::pair<int, int> UnKnownLeaf;
		Settings *pSettings;
		int nRetType;
		bool bHasPtrRef;
		bool bScopeHandled;
		bool bHasAssign;
		bool bHasElse;
		bool bReturnUnknown;
		bool bHasError;
		bool bIfThenRet;
		bool bUnderIf;
		bool bUnknownIf;
		bool bStrictIf;
		bool bStrictElse;
	};

	const std::pair<int, int> CFunctionScopeHandle::UnKnownLeaf(-1, CFunctionScopeHandle::NotInit);

	bool CFuncData::Init(const ::Function& func, CFunction* gtFunc)
	{
		InitFuncRetFlag(gtFunc, func.functionScope);
		InitFuncDerefedVars(gtFunc, func.functionScope);
		//InitExitFlag(gtFunc, func);
		InitFunctionData(gtFunc, func);
		return true;
	}

	bool CFuncData::Init(const ::Scope* func, CFunction* gtFunc)
	{
		InitFuncRetFlag(gtFunc, func);
		InitFuncDerefedVars(gtFunc, func);
		//InitExitFlag(gtFunc, func);
		//InitFunctionData(gtFunc, func);
		return true;
	}

	CFuncData::CFuncData()
		: m_flagRetNull(RF_fUnknown)
		, m_flagExit(EF_fUnknown)
		, m_bAssignThis(0)
	{

	}

	CFuncData::~CFuncData()
	{

	}

	void CFuncData::InitFuncRetFlag(CFunction* gtFunc, const ::Scope* func)
	{
		RetNullFlag retNullFlag = RF_fUnknown;

		for (std::vector<std::string>::const_iterator I = gtFunc->m_return.begin(), E = gtFunc->m_return.end(); I != E; ++I) 
		{
			if (*I == "*") 
			{
				retNullFlag = RetNullFlag::fPossibleNull;
			}
		}

		if (const Scope* scope = func)
		{
			for (const Token* tok = scope->classStart; tok && tok != scope->classEnd; tok = tok->next())
			{
				if (tok->str() == "return")
				{
					if (const Token* tokReturn = tok->astOperand1())
					{

						// return (NULL); 
						if (tokReturn->getValue(0) || tokReturn->str() == "NULL")
						{
							retNullFlag = RetNullFlag::fNull;
							break;
						}
						else if (Token::Match(tokReturn, "&|new"))
						{
							if (retNullFlag != RetNullFlag::fNull)
							{
								retNullFlag = RetNullFlag::fNotNull;
							}
						}
					}
				}
			}
		}

		SetRetNullFlag(retNullFlag);
	}



	void CFuncData::InitFuncDerefedVars(const CFunction* gtFunc, const ::Scope* func)
	{
		std::set<unsigned> safeExprList;
		if (const Scope* scope = func)
		{
			for (const Token* tok = scope->classStart; tok && tok != scope->classEnd; tok = tok->next())
			{
				if (tok->str() == "*" || tok->str() == "[" || (tok->str() == "." && tok->originalName() == "->"))
				{
					HandleDerefTok(tok, safeExprList, gtFunc);
				}
				else if (tok->varId())
				{
					const Variable* pVar = tok->variable();
					if (pVar)
					{
						if (!pVar->isLocal())
						{
							if (const Token* tokP = tok->astParent())
							{
								// consider as checking null
								if (Token::Match(tokP, "?|(|,|==|!=|!|&&|%oror%"))
								{
									safeExprList.insert(pVar->declarationId());
								}
								else if (tokP->str() == "=")
								{
									if (tokP->astOperand1() == tok)
									{
										safeExprList.insert(pVar->declarationId());
									}
								}
							}
						}
						else
						{
							safeExprList.insert(pVar->declarationId());
						}
					}
					else
					{
						safeExprList.insert(tok->varId());
					}
					
				}
			}
		}
	}

	

	void CFuncData::InitFunctionData(CFunction* gtFunc, const ::Function &func)
	{
		if (!func.functionScope || !func.functionScope->classStart || !gtFunc)
		{
			return;
		}
		
		CFunctionScopeHandle sh(func, gtFunc);
		sh.HandleFunctionScope();
	}



	void CFuncData::HandleDerefTok(const Token* tok, std::set<unsigned>& safeExprList, const CFunction* gtFunc)
	{
		if (tok->astUnderSizeof())
		{
			return;
		}

		if (tok->str() == "[" || 
			(tok->str() == "." && tok->originalName() == "->") ||
			(tok->str() == "*" && tok->astOperand2() == nullptr))
		{
			const Token* ltok = tok->astOperand1();
			if (ltok)
			{
				const Variable* pVar = ltok->variable();
				if (pVar)
				{
					if (safeExprList.count(pVar->declarationId()))
					{
						return;
					}
					else
					{
						if (!pVar->isLocal())
						{
							int index = -1;
							if (pVar->isArgument())
							{
								index = gtFunc->GetParamIndex(pVar->name());
							}
							SVarEntry entry(pVar->name(), pVar->getAccess(), index);
							m_derefedVars.insert(entry);
						}
						safeExprList.insert(pVar->declarationId());
					}
				}
			}
		}
	}

	const std::set<SVarEntry>& CFuncData::GetDerefedVars() const
	{
		return m_derefedVars;
	}

	std::set<SVarEntry>& CFuncData::GetDerefedVars()
	{
		return m_derefedVars;
	}

	void CField::Merge(const CField* other)
	{
		m_flags = m_flags | other->m_flags;

		if (m_type.size() < other->m_type.size())
		{
			m_type = other->m_type;
		}
		else if (m_type.size() == other->m_type.size())
		{
			for (std::size_t i = 0; i < m_type.size(); ++i)
			{
				if (m_type[i] == other->m_type[i])
				{
					continue;
				}
				else if (m_type[i] > other->m_type[i])
				{
					break;
				}
				else if (m_type[i] < other->m_type[i])
				{
					m_type = other->m_type;
					break;
				}
			}
		}

		if (m_dimensions.size() < other->m_dimensions.size())
		{
			m_dimensions = other->m_dimensions;
		}
		else if (m_dimensions.size() == other->m_dimensions.size())
		{
			for (std::size_t i = 0; i < m_dimensions.size(); ++i)
			{
				if (m_dimensions[i] == other->m_dimensions[i])
				{
					continue;
				}
				else if (m_dimensions[i] > other->m_dimensions[i])
				{
					break;
				}
				else if (m_dimensions[i] < other->m_dimensions[i])
				{
					m_dimensions = other->m_dimensions;
					break;
				}
			}
		}
		if (m_needInit != Type::True || other->m_needInit != Type::True)
		{
			m_needInit = Type::False;
		}
		
	}

};
