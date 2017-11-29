#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <list>


typedef unsigned long long UINT64;
typedef bool (*fp_fileFilter)(const std::string &filename);
class CFileBase;
class CFolder;
class CCodeFile;

class CFileDependTable
{

public:
	CFileDependTable();
	~CFileDependTable();
	// 依靠输入路径，创建文件依赖表
	bool Create(
		const std::vector<std::string>& paths, 
		const std::vector<std::string>& excludesPaths, 
		const std::vector<std::string>& includePaths);

	CCodeFile* GetFirstFile();
	CFileBase* FindFile(const std::string& filePath);
	void DumpFileDependResults();

	static std::string GetProgramDirectory();
	static bool CreateLogDirectory(std::string* pLogPath = NULL);
private:
	bool CreateFileDependTree(const std::vector<std::string> &paths, const std::vector<std::string> &includePaths, const std::vector<std::string>& excludesPaths);

	bool BuildFileDependTree(std::string &sPath, fp_fileFilter fp);

	void UpdateIncludes(const std::vector<std::string>& includePaths);
	bool BuildTable(CFolder* pRoot, const std::string& sPath, fp_fileFilter fp);
	void ReleaseTable();

	static unsigned char ReadChar(std::istream &istr, unsigned int bom);

	void GetIncludes(const std::string &fileName, std::vector<std::string> &strIncludes);
	void ParseIncludes(CCodeFile* pCode);
	CCodeFile* FindMatchedFile(CCodeFile* pFile, std::string sInclude);
	CCodeFile* FindShortestPath(const std::vector<CCodeFile*>& vecFiles, CCodeFile* pFile);
	std::size_t GetFileSize(const std::string& sPath);


	std::string getAbsolutePath(const std::string& path);
	
private:
	// 文件结构树的根节点
	CFolder* m_pRoot;
	CCodeFile* m_begin;
	CCodeFile* m_flag;
	// 字符串到文件节点的映射，可能存在一对多的情况（文件重名）
	std::multimap<std::string, CCodeFile*> m_fileDict;
	std::vector<CFolder*> m_includePaths;

};

enum EFileType { FT_NONE, FT_FOLDER, FT_CODE };

// 文件节点基类
class CFileBase
{
public:
	void SetParent(CFolder* pParent);
	EFileType GetFileType();
	const std::string& GetName();
	CFolder* GetParent();
	std::string GetFullPath();

	virtual void SetIgnore(bool ignore) { m_bIgnore = ignore; }
	bool GetIgnore() const { return m_bIgnore; }
	virtual ~CFileBase();
protected:
	CFileBase();

	// 每一个文件对应的唯一编号
	unsigned int m_uID;
	// 文件夹或者文件名称
	std::string m_name;
	// 父节点指针
	CFolder* m_parent;
	EFileType m_fileType;

	bool m_bIgnore;
private:
	static unsigned int s_id;
};

// 文件夹类
class CFolder : public CFileBase
{
public:
	CFolder(const char* szName);
	virtual ~CFolder();

	void AddFile(CFileBase* pFile);
	void RemoveFile(const std::string& sName);
	CCodeFile*  AddCodeFile(const std::string& sName, std::size_t uSize, bool& bNew);
	CFolder* AddFolder(const std::string& sName, bool& bNew);
	CFileBase* FindFile(const std::string& fileName);
	void Release();
	const std::vector<CFileBase*>& GetSubs();
	
	virtual void SetIgnore(bool ignore);
private:
	// 子节点指针集合
	std::vector<CFileBase*> m_subs;
};

// 代码文件类
class CCodeFile : public CFileBase
{
public:
	CCodeFile(const char* szName, std::size_t size);
	virtual ~CCodeFile();

	void SetNext(CCodeFile* next);
	CCodeFile* GetNext();
	;
	std::size_t GetSize() const;

	void AddDependFile(CCodeFile* pFile);
	std::vector<CCodeFile*>& GetDepends();

	
	std::list<CCodeFile*>& GetAllDepends();

	void AddExpandCount();
	unsigned int GetExpandCount() const { return m_nExpandCount; }
	bool IsExpaned() const { return m_nExpandCount != 0; }
private:
	void FillAllDepends();
	static void ExpandIncludes(CCodeFile* pCodeFile, std::list<CCodeFile*>& allDepends, std::set<CCodeFile*>& included);
private:
	// 文件大小
	std::size_t m_size;
	// 代码文件依赖的文件列表
	std::vector<CCodeFile*> m_depends;
	// 代码文件被依赖的文件列表
	std::vector<CCodeFile*> m_beDepends;

	std::list<CCodeFile*> m_allDepends;

	CCodeFile* m_next;

	unsigned int m_nExpandCount;
};