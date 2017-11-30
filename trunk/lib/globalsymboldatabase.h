//
//  globalsymboldatabase.hpp
//
//
//

#ifndef globalsymboldatabase_h
#define globalsymboldatabase_h

#include <list>
#include <string>
#include <map>
#include <vector>
#include "config.h"
#include "symboldatabase.h"

#define ZERO_MEM_PATTERN "memset|memcpy|bzero|ZeroMemory ("

namespace gt 
{

	class IData
	{
	protected:
		IData() {}
		~IData() {}
	public:
		
	};

	struct SVarEntry
	{
		std::string sName;
		AccessControl eType;
		int iParamIndex;

		SVarEntry(const std::string name, AccessControl access, int paramIndex = -1) 
			: sName(name), eType(access), iParamIndex(paramIndex)
		{

		}
		~SVarEntry(){}

		bool operator == (const SVarEntry& other) const 
		{
			return (sName == other.sName) && (eType == other.eType) && (iParamIndex == other.iParamIndex);
		}

		bool operator != (const SVarEntry& other) const
		{
			return !(*this == other);
		}

		bool operator < (const SVarEntry& other) const
		{
			if (sName == other.sName)
			{
				if (eType == other.eType)
				{
					return iParamIndex < other.iParamIndex;
				}
				else
					return eType < other.eType;
			}
			else
			{
				return sName < other.sName;
			}
		}
	};

	struct SOutScopeVarState
	{
		std::string strName;
		bool bNull;
		SOutScopeVarState(const std::string &name, bool n)
			: strName(name)
			, bNull(n)
		{
		}

		bool operator < (const SOutScopeVarState & s) const
		{
			if (strName < s.strName)
			{
				return true;
			}
			else if (strName == s.strName)
			{
				if (bNull < s.bNull)//ignore TSC
				{
					return true;
				}
			}
			return false;
		}
		
		friend std::ostream& operator << (std::ostream &os, const SOutScopeVarState &s)
		{
			os << s.strName << ":" << (s.bNull ? "null" : "notnull");
			return os;
		}
		
		SOutScopeVarState()
			: bNull(false)
		{}
	};

	class CFuncData : public IData
	{
	public:
		enum
		{
			ParamRetRelationNum = 2
		};
		enum RetNullFlag
		{
			RF_fUnknown,
			fNotNull,
			fPossibleNull,
			fNull,
		};
		enum ExitFlag
		{
			EF_fUnknown,
			fNotExit,
			fPossibleExit,
			fExit
		};
		struct ParamRetRelation
		{
			int nValue; 
			int nIndex : 16; 
			bool bNull : 1;
			bool bEqual : 1;
			bool bStr : 1;
			bool bNerverNull : 1; //if this this set to 1, param where never be null
			std::string str;
			ParamRetRelation()
				: nValue(0)
				, nIndex(0)
				, bNull(false)
				, bEqual(false)
				, bStr(false)
				, bNerverNull(false)
			{}
			ParamRetRelation(unsigned short i, int v, bool isnull, bool equal, bool neverNull=false)
				: nValue(v)
				, nIndex(i)
				, bNull(isnull)
				, bEqual(equal)
				, bStr(false)
				, bNerverNull(neverNull)
			{}
			ParamRetRelation(unsigned short i, const std::string &v, bool isnull, bool equal, bool neverNull =false)
				: nValue(0)
				, nIndex(i)
				, bNull(isnull)
				, bEqual(equal)
				, bStr(true)
				, bNerverNull(neverNull)
                , str(v)
			{}
			bool operator < (const ParamRetRelation &p) const
			{
				if (nValue < p.nValue)
				{
					return true;
				}
				else if (nValue == p.nValue)
				{
					int old = nIndex;
					old = (old << 8) + bNull;
					old = (old << 1) + bEqual;
					old = (old << 1) + bStr;
					old = (old << 1) + bNerverNull;
					int newVal = p.nIndex;
					newVal = (newVal << 8) + p.bNull;
					newVal = (newVal << 1) + p.bEqual;
					newVal = (newVal << 1) + p.bStr;
					newVal = (newVal << 1) + p.bNerverNull;

					if (old < newVal)
					{
						return true;
					}
					if (old == newVal)
					{
						if (str < p.str)
						{
							return true;
						}
					}
				}
				return false;
			}
			friend std::ostream & operator << (std::ostream &os, const ParamRetRelation &p)
			{
				os << "ret" << (p.bEqual ? "==":"!=") << p.nValue << "=>" << "param" << p.nIndex;
				if (p.bNerverNull)
				{
					os << "nevernull";
				}
				else
				{
					os << (p.bNull ? "null" : "notnull");
				}
				return os;
			}
		}; 
		

		explicit CFuncData();
		~CFuncData();
		bool Init(const ::Function& func, CFunction* gtFunc);
		bool Init(const ::Scope* func, CFunction* gtFunc);

		void InitFuncRetFlag(CFunction* gtFunc, const ::Scope* func);
		void InitFuncDerefedVars(const CFunction* gtFunc, const ::Scope* func);
		
		void InitFunctionData(CFunction* gtFunc, const ::Function &func);

		void HandleDerefTok(const Token* tok, std::set<unsigned>& safeExprList, const CFunction* gtFunc);


		void SetRetNullFlag(RetNullFlag flag){ m_flagRetNull = flag; }
		RetNullFlag GetFuncRetNull() const { return m_flagRetNull; }
		const std::set<SVarEntry>& GetDerefedVars() const;
		std::set<SVarEntry>& GetDerefedVars();

		ExitFlag GetExitFlag() const { return m_flagExit; }
		void SetExitFlag(ExitFlag f) { m_flagExit = f; }
		bool FunctionExit() const {return m_flagExit == ExitFlag::fPossibleExit || m_flagExit == ExitFlag::fExit;}
		const std::set<ParamRetRelation>* GetRetParamRelation() const { return m_retParamRelation; }
		std::set<ParamRetRelation>* GetRetParamRelation()  { return m_retParamRelation; }
		void SetRetParamRelation(const std::set<ParamRetRelation> *r, size_t nIndex) { assert(nIndex < ParamRetRelationNum); m_retParamRelation[nIndex] = r[nIndex]; }
		const std::set<SOutScopeVarState>& GetOutScopeVarState() const { return m_vOutScopeVar; }
		std::set<SOutScopeVarState>& GetOutScopeVarState() { return m_vOutScopeVar; }
		void SetOutScopeVarState(const std::set<SOutScopeVarState>& vVar) { m_vOutScopeVar = vVar; }
		void AssignThis(bool bAssign) { m_bAssignThis = bAssign; }
		bool AssignThis() const { return m_bAssignThis != 0; }
	private:
		//int foo(T** t) || (T*& t)
		//return value is xxx means param at @index is not null
		//T *p foo(int &flag)
		//param at @index is xxx mean return value is not null
		std::set<ParamRetRelation> m_retParamRelation[ParamRetRelationNum];
		//outter scope variable made null or not null by this function
		std::set<SOutScopeVarState> m_vOutScopeVar;
		std::set<SVarEntry> m_derefedVars;
		RetNullFlag   m_flagRetNull : 8;
		ExitFlag      m_flagExit : 7;
		unsigned int  m_bAssignThis : 1;
	};

    class CFunction;
	class CField;
	
	class CScope
	{
	public:
		enum Kind 
		{
			None,
			Global,
			Type,
		};
	public:
		explicit CScope(Kind kind, const std::string& name)
			: m_name(name), m_parent(nullptr), m_kind(kind), m_needInit(Type::Unknown)
		{

		}
        
        virtual ~CScope();
        
        virtual CScope* GetCopy() const = 0;

		virtual void RecordFunc();

        template<typename T>
        T* CopyAs() const
        {
            return new T(m_name);
        }
        
		template<typename T>
		T* GetAs() const
		{
			if (!T::IsKind(this))
				return nullptr;
			
			return dynamic_cast<T*>(this);
		}

		bool AddChildScope(CScope* scope);
        bool AddChildFunc(CFunction* func);
		bool AddChildFiled(CField* field);
        void Merge(const CScope* other);

		void AddUsingInfo(const std::string& info)
		{
			m_usingList.insert(info);
		}

		const std::set<std::string>& GetUsingInfo() const
		{
			return m_usingList;
		}
        
        bool HasChildScope(const std::string& name) const
        {
            return m_scopeMap.count(name) > 0;   
        }
        
        CScope* TryGetChildScope(const std::string& name);
		
        CFunction* TryGetFunc(const CFunction* func);
		CField* TryGetField(const std::string& name);
		const CField* TryGetField(const std::string& name) const;

		const CScope* FindChildScope(const std::string& name) const;

		const std::multimap<std::string, CFunction*>& GetFunctionMap() const
		{
			return m_funcMap;
		}

		std::map<std::string, CField*>& GetFieldMap()
		{
			return m_fieldMap;
		}

		const std::map<std::string, CField*>& GetFieldMap() const
		{
			return m_fieldMap;
		}

		const std::map<std::string, CScope*>& GetScopeMap() const
		{
			return m_scopeMap;
		}

		const std::string& GetName() const { return m_name; }
		Kind GetKind() const { return m_kind; }
		void SetKind(Kind kind) { m_kind = kind; }
		void SetParent(CScope* parent) { m_parent = parent; }
		const CScope* GetParent() const { return m_parent; }
		CScope* GetParent() { return m_parent; }
		bool NeedInit() const { return m_needInit == Type::True; }
		void SetNeedInit(Type::NeedInitialization ni)
		{
			m_needInit = ni;
		}

		Type::NeedInitialization GetNeedInit() const
		{
			return m_needInit;
		}

		const std::string GetNeedInitStr() const
		{
			std::string s;
			switch (m_needInit)
			{
			case Type::Unknown:
				s = "[NI:Unknown]";
				break;
			case Type::True:
				s = "[NI:True]";
				break;
			case Type::False:
				s = "[NI:False]";
				break;
			default:
				break;
			}
			return s;
		}

		void Dump(std::ofstream& fs) const;

	protected:
		virtual void InitScopeMap(std::multimap<std::string, CScope*>& globalMap);

		static bool IsKind(const CScope* scope)
		{
			(void)scope;
			return true;
		}
	protected:
		std::string m_name;
		std::map<std::string, CScope*> m_scopeMap;
        std::multimap<std::string, CFunction*> m_funcMap;
		std::map<std::string, CField*> m_fieldMap;
		CScope* m_parent;
		Kind m_kind;
		std::set<std::string> m_usingList;
		Type::NeedInitialization m_needInit;
	};

	class CGlobalScope : public CScope
	{
	public:
        explicit CGlobalScope(std::string name = emptyString)
			: CScope(CScope::Global, name)
		{

		}
        
        virtual CScope* GetCopy() const
        {
            return CopyAs<CGlobalScope>();
        }

		//const std::multimap<std::string, CScope*>& GetGlobalScopeMap() const { return m_globalScopeMap; }
    
	public:
		static bool IsKind(const CScope* scope)
		{
			return scope->GetKind() == CScope::Global;
		}
	private:
		//std::multimap<std::string, CScope*> m_globalScopeMap;
	};

	class CType : public CScope
	{
	public:
		explicit CType(const std::string& name)
			: CScope(CScope::Type, name)
		{

		}
		explicit CType(const Scope* scope);
        
        virtual CScope* GetCopy() const
        {
			CType* newType = CopyAs<CType>();
			newType->m_derived = m_derived;
			return newType;
        }

		const std::set< std::string >& GetDerived() const;

		void MergeDerived(const CType* pType);
		void MergeDerived(const Scope* scope);
        
	public:
		static bool IsKind(const CScope* scope)
		{
			return scope->GetKind() == CScope::Type;
		}
	protected:
		std::set< std::string > m_derived;
	};
    
    class CVariable
    {
    public:
		enum FlagEx
		{
			fUnsigned	= 1 << 0,
			fLong		= 1 << 1,
		};
        explicit CVariable(const ::Variable* var);
		explicit CVariable(const Token* typeStart, const Token* typeEnd, const Token* tokName);
        
        bool operator == (const CVariable& other) const;
        bool operator != (const CVariable& other) const;

		bool IsUnsigned() const { return GetFlagEx(fUnsigned); }
		bool IsLong() const { return GetFlagEx(fLong); }
		const std::string& GetTypeStartString() const;
		bool GetFlag(unsigned flag) const;
		std::string GetTypeStr() const;
		const std::string& GetParamName() const { return m_paramName; }
    private:
        void InitByVariable(const ::Variable* var);
		void SetFlagEx(FlagEx flag, bool state);
		bool GetFlagEx(FlagEx flag) const;
		void SetFlag(unsigned flag, bool state);		
    private:
		unsigned m_flag : 20;
		unsigned m_flagex : 12;
        std::vector<std::string> m_type;
		std::string m_paramName;
    };
    
    class CFunction
    {
		friend class CFunctionScopeHandle;
        friend class CFuncData;
    public:
        explicit CFunction(const ::Function& func);
		explicit CFunction(const ::Scope* funcScope);
        
        void SetParent(CScope* parent){ m_parent = parent; }
		const CScope* GetParent() const { return m_parent; }
        const std::string& GetName() const { return m_name; }
        
        CFunction* GetCopy();
		bool InitFuncData(const ::Function& func);
		bool InitFuncData(const ::Scope* scope);
		const CFuncData& GetFuncData() const { return m_funcData; }
		CFuncData& GetFuncData() { return m_funcData; }

		unsigned GetArgCount() const { return m_args.size(); }
		unsigned GetMinArgCount() const { return GetArgCount() - m_initArgCount; }
		std::string GetArgStr() const;

		const CVariable* GetVariableByIndex(unsigned index) const;
        
        const std::vector<std::string>& GetReturn() const { return m_return; }

		bool GetFlag(unsigned int flag) const {
			return bool((m_flag & flag) != 0);
		}

		unsigned GetFlag() const
		{
			return m_flag;
		}

		unsigned  GetType() const
		{
			return m_type;
		}

		bool HasScope() const { return m_hasScope; }
		void SetHasScope(bool bScope) { m_hasScope = bScope; }

		std::string GetReturnStr() const;

		std::string GetFuncStr() const;

		void SyncWithOtherFunc(const CFunction* other);

		int GetParamIndex(const std::string& sName) const;

		const std::vector<CVariable> &GetArgs() const { return m_args; }
		const CScope* GetScope() const { return m_parent; }


		bool MergeFunc(gt::CFunction* newFunc);

    public:
		static bool Compare(const CFunction* func1, const CFunction* func2);
        bool operator < (const CFunction& other) const;
        bool operator == (const CFunction& other) const;
        bool operator != (const CFunction& other) const;
    private:
        void InitByScope(const ::Function& scopeFunc);
		void InitByScope(const ::Scope* scopeFunc);

		void SetFlag(unsigned int flag, bool state) {
			m_flag = state ? m_flag | flag : m_flag & ~flag;
		}

		
    private:
        unsigned m_type : 3;
		unsigned m_initArgCount : 5;
		unsigned m_flag : 16;
		bool m_hasScope;
        CScope* m_parent;
        std::string m_name;
        std::vector<CVariable> m_args;
        std::vector<std::string> m_return;
	private:
		CFuncData m_funcData;
    };

	class CField
	{
	public:
		struct Dimension 
		{
			#define DIMENSION_MAGIC -282
			Dimension() : iNum(DIMENSION_MAGIC){ }

			Dimension(unsigned num) : iNum(num) { }

			Dimension(std::string num) : sNum(num), iNum(DIMENSION_MAGIC) { }

			std::string sNum;
			int iNum; // (assumed) dimension length when size is a number, 0 if not known

			bool IsKnown() const
			{
				return iNum != DIMENSION_MAGIC || !sNum.empty();
			}

			bool IsNum() const
			{
				return iNum != DIMENSION_MAGIC;
			}

			bool IsStr() const
			{
				return iNum == DIMENSION_MAGIC && !sNum.empty();
			}

			void Init(const ::Dimension& d)
			{
				if (d.known)
				{
					iNum = (int)d.num;
				}
				else
				{
					if (d.start && d.start == d.end)
					{
						sNum = d.start->str();
					}
				}
			}

			std::string Output() const
			{
				std::stringstream ss;
				if (IsKnown())
				{
					if (IsNum())
					{
						ss << iNum;
					}
					else
						ss << sNum;
				}
				else
				{
					ss << "UNKNOWN";
				}
				return ss.str();
			}

			bool operator ==(const Dimension& other)
			{
				return (sNum == other.sNum) && (iNum == other.iNum);
			}

			bool operator !=(const Dimension& other)
			{
				return !((sNum == other.sNum) && (iNum == other.iNum));
			}

			bool operator <(const Dimension& other)
			{
				if (iNum < other.iNum)
				{
					return true;
				}
				else if (iNum > other.iNum)
				{
					return false;
				}
				else
				{
					return sNum < other.sNum;
				}
			}

			bool operator > (const Dimension& other)
			{
				if (iNum > other.iNum)
				{
					return true;
				}
				else if (iNum < other.iNum)
				{
					return false;
				}
				else
				{
					return sNum > other.sNum;
				}
			}
		};
	public:
		CField(const std::string& name, unsigned flags, AccessControl ac, Type::NeedInitialization init)
			: m_name(name), m_access(ac), m_flags(flags), m_gtType(nullptr), m_needInit(init)
		{
			
		}
		const std::string& GetName() const
		{
			return m_name;
		}
		
		std::vector<std::string>& GetType()
		{
			return m_type;
		}

		const std::vector<std::string>& GetType() const
		{
			return m_type;
		}

		CField* DeepCopy() const
		{
			CField* newF = new CField(*this);
			return newF;
		}

		std::string GetFieldStr() const
		{
			std::string str;
			
			for (std::vector < std::string>::const_iterator I = m_type.begin(), E = m_type.end(); I != E; ++I)
			{
				str += *I;
			}
			str += " ";
			str += m_name;
			return str;
		}

		unsigned GetFlags() const
		{
			return m_flags;
		}
		std::vector<Dimension>& GetDimensions()
		{
			return m_dimensions;
		}

		const std::vector<Dimension>& GetDimensions() const
		{
			return m_dimensions;
		}

		void Merge(const CField* other);


		bool isArray() const 
		{
			return getFlag(Variable::fIsArray) && !getFlag(Variable::fIsPointer);
		}

		bool isPointer() const { return getFlag(Variable::fIsPointer); }
		bool hasDefault() const { return getFlag(Variable::fHasDefault); }
		bool needInit() const { return m_needInit == Type::True; }
		bool isClass() const { return getFlag(Variable::fIsClass);}
		bool isStatic() const { return getFlag(Variable::fIsStatic); }
		void SetGType(CScope* s)
		{
			m_gtType = s;
		}

		CScope* GetGType()
		{
			return m_gtType;
		}

		const CScope* GetGType() const
		{
			return m_gtType;
		}

	private:
		bool getFlag(unsigned int flag_) const 
		{
			return bool((m_flags & flag_) != 0);
		}
	private:
		std::vector<std::string> m_type;
		std::string m_name;
		AccessControl m_access;
		unsigned int m_flags;
		std::vector<Dimension> m_dimensions;
		CScope* m_gtType;
		Type::NeedInitialization m_needInit;
	};
};



#endif /* globalsymboldatabase_h */