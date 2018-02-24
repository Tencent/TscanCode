//
//  globaltokenizer.h
//
//
//

#ifndef globaltokenizer_h
#define globaltokenizer_h

#include <map>
#include <list>
#include "config.h"
#include "globalsymboldatabase.h"


#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

class SymbolDatabase;
class Scope;
class Variable;

//from bebezhang 20140623
struct  FUNCINFO
{
	std::string filename;
	std::string functionname;
	int startline;
	int endline;
};

struct SPack1Scope 
{
	unsigned StartLine;
	unsigned EndLine;

	SPack1Scope(unsigned start, unsigned end) : StartLine(start), EndLine(end) { }
	SPack1Scope() : StartLine(0), EndLine(0) { }

	bool operator <(const SPack1Scope& scope) const
	{
		return StartLine < scope.StartLine;
	}
};

typedef std::map<std::string, std::set<SPack1Scope> >::iterator			Pack1MapI;
typedef std::map<std::string, std::set<SPack1Scope> >::const_iterator	Pack1MapCI;

class CGlobalTokenizeData
{
public:
    CGlobalTokenizeData();
    ~CGlobalTokenizeData();

	void Dump(const char* szPath) const;
    
    void Merge(const CGlobalTokenizeData& threadData);
	void RelateTypeInfo();
    void InitScopes(const SymbolDatabase* symbolDB);
	const gt::CGlobalScope* GetData() const { return m_data; }
	
	void RecordFuncInfo(const Tokenizer* tokenizer, const std::string &strFileName);
	void RecordInfoForLua(const Tokenizer* tokenizer, const std::string &strFileName);

	void DumpFuncinfo();
	void MergeLuaInfoToOneData(std::set<std::string>& m_oneLuaData, std::set<std::string> &luaExportClass);

	void AddPack1Scope(const std::string& filename, unsigned start, unsigned end);
	std::map<std::string, std::set<SPack1Scope> >& GetPack1Scope();
	void RecordRiskTypes(const Tokenizer* tokenizer);

	bool IsRiskType(const Variable& var, const Scope* scope);
	const std::set<std::string>& GetRiskTypes() { return m_riskTypes; }

	void AddLuaInfo(const std::string &info) { m_InfoForLua.insert(info); }
	std::set<std::string>& GetLuaInfo() { return m_InfoForLua; }

	void AddExportClass(const std::string &str) { m_luaExportClass.insert(str); }
	std::set<std::string>& GetExportClass() { return m_luaExportClass; }
	bool RecoredExportClass() const { return m_bRecoredExportClass; }
	void RecoredExportClass(bool b) { m_bRecoredExportClass = b; }
private:
    void SyncScopes(const Scope* scope, gt::CScope* gtScope, std::set<const Scope*>& added);
	void RecordScope(gt::CScope* scope);

	gt::CScope* FindType(const std::vector<std::string>& tl, gt::CScope* cs);

public:
	typedef std::multimap<std::string, gt::CScope*> SM;
	typedef std::multimap<std::string, gt::CScope*>::iterator SMI;
	typedef std::multimap<std::string, gt::CScope*>::const_iterator SMCI;
private:

    gt::CGlobalScope* m_data;
	std::multimap<std::string, gt::CScope*> m_scopeMap;
    std::map<const Scope*, gt::CScope*> m_scopeCache;

	std::list<FUNCINFO> m_funcInfoList;
	std::set<std::string> m_InfoForLua;
	/////[LUA.EXPORT.CLASS]
	std::set<std::string> m_luaExportClass;
	bool m_bRecoredExportClass;
	std::map<std::string, std::set<SPack1Scope> > m_pack1Scopes;
	std::set<std::string> m_riskTypes;

	static std::set<std::string> s_stdTypes;
};


class TSCANCODELIB CGlobalTokenizer
{
public:
    static CGlobalTokenizer* Instance();
    static CGlobalTokenizer* s_instance;

	static void Uninitialize();

    CGlobalTokenizeData* GetGlobalData(void* pKey);
    
    void Merge(bool dump = false);
 
	gt::CFuncData::RetNullFlag CheckFuncReturnNull(const Token* tokFunc);

	bool CheckFuncExit(const Token*tokFunc) const;

	bool CheckIfMatchSignature(const Token *tokFunc, 
		const std::multimap<std::string, std::pair<std::string, std::vector<std::string> > >&
		, const std::set<std::string> &equalTypes) const;

	bool CheckDerefVars(const Token* tokFunc, std::set<gt::SVarEntry>& derefVars);

	bool CheckReturnVars(const Token* tokFunc, int& memberVarId);

	bool CheckAssignedVars(const Token* tokFunc, std::vector<Variable*>& assignedVars, int& rval);

	const std::set<gt::CFuncData::ParamRetRelation> * CheckRetParamRelation(const gt::CFunction *gtFunc);
	const gt::CFunction* FindFunctionData(const Token* tokFunc);
	bool CheckIfReturn(const Token *tok) const;
	bool IsVarInitInFunc(const gt::CFunction *gf, const Variable *var, const std::string& mem) const;
	const static std::string FindFullFunctionName(const Token *tok);


	const gt::CScope* GetGlobalScope(const Variable* pVar);
	const gt::CField* GetFieldInfoByToken(const Token* tokVar);

	void SetAnalyze(bool bAnalyze) { m_bAnalyze = bAnalyze; }
	bool IsAnalyze() const { return m_bAnalyze; }

	const std::set<std::string>& GetRiskTypes();
	std::map<std::string, std::set<SPack1Scope> >& GetPack1Scope();

private:
	const gt::CFunction* FindFunctionWrapper(const Token* tokFunc) const;
	const gt::CFunction* FindFunction(const Token* tokFunc, bool requireScope = false) const;
	const gt::CFunction* FindFunction(const gt::CScope* gtScope, const Token* tokFunc, bool requireScope = false, bool requireConst = false) const;
	const gt::CScope* FindScope(const Scope* scope, const std::list<std::string>& missedScope) const;
	void FindFunctionInBase(const gt::CScope* gtScope, const Token* tok, size_t args, bool requireScope, std::vector<const gt::CFunction *> & matches, int level = 0) const;

	const gt::CScope* FindGtScopeByStringType(const Scope* currScope, const std::vector < std::string >& stringType) const;
private:
    CGlobalTokenizer();
    ~CGlobalTokenizer();

    std::map<void*, CGlobalTokenizeData*> m_threadData;
    CGlobalTokenizeData m_oneData;
	std::set<std::string> m_oneLuaData;
	bool m_bAnalyze;
};

class TSCANCODELIB CGlobalErrorList
{
public:
	static CGlobalErrorList* Instance();
	static CGlobalErrorList* s_instance;

	std::list< std::string >& GetThreadErrorList(void* pKey);
	
	std::set<std::string>& GetOneData();

	void Merge(Settings& setting);

private:

	CGlobalErrorList();
	~CGlobalErrorList();

	std::map<void*, std::list<std::string> > m_threadData;
	std::set<std::string> m_oneData;
};

struct FuncRetInfo
{
	enum Operation
	{
		Unknown,
		Assign,
		Dereference,
		Use,
		CheckNull,
	};

	Operation Op;
	unsigned LineNo;
	std::string VarName;

	FuncRetInfo() : Op(Unknown), LineNo(0)
	{
	}

	FuncRetInfo(Operation op, unsigned line, std::string name)
		: Op(op), LineNo(line), VarName(name)
	{
	}

	static FuncRetInfo UnknownInfo;
};

struct FuncRetStatus
{
	struct ErrorMsg
	{
		std::string UsedFile;
		unsigned UsedLine;
		std::string UsedVarName;

		ErrorMsg() : UsedLine(0) { }
		ErrorMsg(const std::string& fileName, unsigned line, const std::string& varName)
			: UsedFile(fileName), UsedLine(line), UsedVarName(varName)
		{
		}
	};

	unsigned CheckNullCount;
	unsigned UsedCount;

	FuncRetStatus() : CheckNullCount(0), UsedCount(0)
	{
	}

	std::list<ErrorMsg> UsedList;
};

struct ArrayIndexInfo
{
	std::string BoundStr;
	std::string BoundType;
	unsigned BoundLine;
	std::string ArrayStr;
	std::string ArrayType;
	unsigned ArrayLine;
};


struct ArrayIndexRetInfo
{
	
};

struct StatisticThreadData
{
	std::map<std::string, std::map<const gt::CFunction*, std::list<FuncRetInfo> > > FuncRetNullInfo;

	std::map<std::string, std::list<ArrayIndexInfo> > OutOfBoundsInfo;

	void Clear()
	{
		FuncRetNullInfo.clear();
	}

	void Dump(const std::string& file_suffix) const;
	void DumpFuncRetNull(const std::string& file_suffix) const;
	void DumpOutOfBounds(const std::string& file_suffix) const;

};

struct StatisticMergedData
{
	struct Pos
	{
		std::string FilePath;
		unsigned BoundaryLine;
		unsigned ArrayLine;
	};

	std::map<const gt::CFunction*, FuncRetStatus> FuncRetNullInfo;

	std::map<std::string, std::map<std::string, std::vector<Pos> > > OutOfBoundsInfo;
	
	void Dump();

	void DumpFuncRetNull();

	void DumpOutOfBounds();

};



class TSCANCODELIB CGlobalStatisticData
{
public:
	static CGlobalStatisticData* Instance();
	static CGlobalStatisticData* s_instance;

	std::map<std::string, std::map<const gt::CFunction*, std::list<FuncRetInfo> > >& GetFuncRetNullThreadData(void* pKey);
	std::map<const gt::CFunction*, FuncRetStatus>& GetFuncRetNullMergedData();

	std::map<std::string, std::list<ArrayIndexInfo> >& GetOutOfBoundsThreadData(void* pKey);
	
	void Merge(bool bDump);

	void MergeInternal(StatisticThreadData& tempData, bool bDump);

	void HandleData(StatisticThreadData& tempData, bool bDump);

	void HandleFuncRetNull(StatisticThreadData &tempData);
	void HandleOutOfBound(StatisticThreadData &tempData);

	void ReportErrors(Settings& setting, std::set<std::string>& errorList);

	void ReportFuncRetNullErrors(Settings& setting, std::set<std::string>& errorList);
	void ReportOutOfBoundsErrors(Settings& setting, std::set<std::string>& errorList);

private:
	void DumpMergedData();
	void DumpThreadData(const StatisticThreadData& data, const std::string& file_suffix);

private:

	CGlobalStatisticData();
	~CGlobalStatisticData();

	std::map<void*, StatisticThreadData > m_threadData;
	StatisticMergedData m_mergedData;

	TSC_LOCK m_lock;
};



#ifdef USE_GLOG
class TSCANCODELIB CLog
{
public:
	static void Initialize();

private:
	static bool s_initFlag;
};
#endif

#endif /* globaltokenizer_h */
