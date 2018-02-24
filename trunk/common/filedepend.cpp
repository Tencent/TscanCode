#ifdef _WIN32

#include <windows.h>
#include <Shlwapi.h>
#include <shlobj.h>

#else

///////////////////////////////////////////////////////////////////////////////
////// This code is POSIX-style systems ///////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include <glob.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>

#endif // _WIN32

#include <assert.h>
#include <sstream>
#include <fstream>
#include <ctype.h>
#include <cstring>
#include <list>
#include <algorithm>

#include "filedepend.h"
#include "path.h"
#include "filelister.h"

unsigned int CFileBase::s_id = 0;

CFileDependTable::CFileDependTable()
{
	m_flag = m_begin = NULL;
	m_pRoot = NULL;
}

CFileDependTable::~CFileDependTable()
{
	delete m_pRoot;
	m_flag = m_begin = NULL;
	m_pRoot = NULL;
}

bool CFileDependTable::Create(
	const std::vector<std::string>& paths, 
	const std::vector<std::string>& excludesPaths, 
	const std::vector<std::string>& includePaths)
{
	ReleaseTable();

	if (!paths.size())
		return false;

	bool bRet = CreateFileDependTree(paths, includePaths, excludesPaths);
	if (!bRet)
	{
		return false;
	}
	UpdateIncludes(includePaths);
	
	return true;
}
#ifdef _WIN32
bool CFileDependTable::BuildTable(CFolder* pParent, const std::string& sPath, fp_fileFilter fp)
{
	assert(pParent != NULL);
	// 如果已经有文件了，需要检查是否重复
	bool bCheckExists = (pParent->GetSubs().size() > 0);

	std::string strPath(sPath);
	std::string strNative = Path::toNativeSeparators(sPath);

	std::ostringstream oss;
	oss << strNative.c_str();
	
	if (FileLister::isDirectory(strNative)) 
	{
		char c = strNative[strNative.size() - 1];
		if (c == '\\')
			oss << '*';
		else
			oss << "\\*";
	}


	WIN32_FIND_DATAA ffd;
   	HANDLE hFind = FindFirstFileA(oss.str().c_str(), &ffd);
	if (INVALID_HANDLE_VALUE == hFind)
		return false;

	do 
	{
		if (ffd.cFileName[0] == '.' || ffd.cFileName[0] == '\0')
			continue;

		const char* szFileName = ffd.cFileName;
		if (strchr(szFileName, '?')) 
		{
			szFileName = ffd.cAlternateFileName;
		}
		
		if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) 
		{
			// File
			if ((*fp)(szFileName)) 
			{
				CCodeFile* pFile = NULL;
				bool bNew = true;
				if (bCheckExists)
				{
					pFile = pParent->AddCodeFile(szFileName, ffd.nFileSizeLow, bNew);
				}
				else
				{
					pFile = new CCodeFile(szFileName, ffd.nFileSizeLow);
					pParent->AddFile(pFile);
				}
				
				if (bNew)
				{
					if (!m_begin)
					{
						m_begin = pFile;
					}
					else
					{
						m_flag->SetNext(pFile);
					}
					m_flag = pFile;

					std::pair<std::string, CCodeFile*> newPair(std::string(szFileName), pFile);
					m_fileDict.insert(newPair);
				}

			}
		}
		else
		{
			// Directory
			bool bNew = true;
			CFolder* pFolder = NULL;
			if (bCheckExists)
			{
				pFolder = pParent->AddFolder(szFileName, bNew);
			}
			else
			{
				pFolder = new CFolder(szFileName);
				pParent->AddFile(pFolder);
			}
			
			std::string newPath = strPath + "/" + szFileName;
			bool bRet = BuildTable(pFolder, newPath, fp);
			if (!bRet)
				return false;
		}
	} 
	while (FindNextFileA(hFind, &ffd) != FALSE);

	if (INVALID_HANDLE_VALUE != hFind) 
		FindClose(hFind);

	return true;
}

std::size_t CFileDependTable::GetFileSize(const std::string& sPath)
{
	WIN32_FIND_DATAA ffd;
	HANDLE hFind = FindFirstFileA(Path::toNativeSeparators(sPath).c_str(), &ffd);
	if (INVALID_HANDLE_VALUE == hFind)
		return 0;

	return ffd.nFileSizeLow;
}

std::string CFileDependTable::getAbsolutePath(const std::string& path)
{
	return path;
}
#else

// Get absolute path. Returns empty string if path does not exist or other error.
std::string CFileDependTable::getAbsolutePath(const std::string& path)
{
	std::string absolute_path;

#ifdef PATH_MAX
	char buf[PATH_MAX];
	if (realpath(path.c_str(), buf) != NULL)
		absolute_path = buf;
#else
	char *dynamic_buf;
	if ((dynamic_buf = realpath(path.c_str(), NULL)) != NULL) {
		absolute_path = dynamic_buf;
		free(dynamic_buf);
	}
#endif

	return absolute_path;
}

bool CFileDependTable::BuildTable(CFolder* pParent, const std::string& sPath, fp_fileFilter fp)
{
	assert(pParent != NULL);
	// 如果已经有文件了，需要检查是否重复
	bool bCheckExists = (pParent->GetSubs().size() > 0);

	std::string strPath(sPath);
	std::string strNative = Path::toNativeSeparators(sPath);

	std::ostringstream oss;
	oss << strNative;

	if (strNative.length() > 0)
	{
		if(strNative[strNative.length()-1] == '/')
		{
			oss << "*";
		}
		else
		{
			oss << "/*";
		}
	}

	glob_t glob_results;
	glob(oss.str().c_str(), GLOB_MARK, 0, &glob_results);
	for (unsigned int i = 0; i < glob_results.gl_pathc; i++) 
	{
		std::string sFileName = glob_results.gl_pathv[i];
		if (sFileName == "." || sFileName == ".." || sFileName.length() == 0)
			continue;

		// Determine absolute path. Empty filename if path does not exist
		const std::string absolute_path = getAbsolutePath(sFileName);
		if (absolute_path.empty())
			continue;

		
		if (sFileName[sFileName.length()-1] != '/')
		{
			// File
			sFileName = sFileName.substr(sFileName.rfind('/') + 1);
			if ((*fp)(sFileName)) 
			{
				struct stat sb;
				std::size_t fileSize = 0;
				if (stat(absolute_path.c_str(), &sb) == 0) 
				{
					fileSize = static_cast<std::size_t>(sb.st_size);
				} 

				CCodeFile* pFile = NULL;
				bool bNew = true;
				if (bCheckExists)
				{
					pFile = pParent->AddCodeFile(sFileName, fileSize, bNew);
				}
				else
				{
					pFile = new CCodeFile(sFileName.c_str(), fileSize);
					pParent->AddFile(pFile);
				}

				if (bNew)
				{
					if (!m_begin)
					{
						m_begin = pFile;
					}
					else
					{
						m_flag->SetNext(pFile);
					}
					m_flag = pFile;

					std::pair<std::string, CCodeFile*> newPair(std::string(sFileName), pFile);
					m_fileDict.insert(newPair);
				}
			}
		} 
		else 
		{
			// Directory
			std::size_t begin = sFileName.rfind('/', sFileName.length() - 2);
			sFileName = sFileName.substr(begin + 1, sFileName.length() - begin - 2);
			bool bNew = true;
			CFolder* pFolder = NULL;
			if (bCheckExists)
			{
				pFolder = pParent->AddFolder(sFileName, bNew);
			}
			else
			{
				pFolder = new CFolder(sFileName.c_str());
				pParent->AddFile(pFolder);
			}

			std::string newPath = strPath + "/" + sFileName;
			bool bRet = BuildTable(pFolder, newPath, fp);
			if (!bRet)
				return false;
		}
	}
	globfree(&glob_results);

	return true;
}

std::size_t CFileDependTable::GetFileSize(const std::string& sPath)
{
	struct stat sb;
	std::size_t fileSize = 0;
	if (stat(sPath.c_str(), &sb) == 0) 
	{
		fileSize = static_cast<std::size_t>(sb.st_size);
	} 

	return fileSize;
}

#endif

void CFileDependTable::ReleaseTable()
{
	if (m_pRoot)
	{
		m_flag = m_begin = NULL;
		m_fileDict.clear();
		m_pRoot->Release();
		delete m_pRoot;
		m_pRoot = NULL;
	}
}

void CFileDependTable::ParseIncludes(CCodeFile* pCode)
{
	pCode->GetDepends().clear();

	std::vector<std::string> strIncludes;
	std::string sFullPath =  pCode->GetFullPath();

	GetIncludes(Path::toNativeSeparators(sFullPath), strIncludes);
	std::vector<std::string>::iterator iterBegin = strIncludes.begin();
	std::vector<std::string>::iterator iterEnd = strIncludes.end();
	for (std::vector<std::string>::iterator iter = iterBegin; iter != iterEnd; iter++)
	{
		CCodeFile* pFile = FindMatchedFile(pCode, *iter);
		if (pFile)
		{
			pCode->AddDependFile(pFile);
		}
	}
	return;
}

unsigned char CFileDependTable::ReadChar(std::istream &istr, unsigned int bom)
{
	unsigned char ch = (unsigned char)istr.get();

	// For UTF-16 encoded files the BOM is 0xfeff/0xfffe. If the
	// character is non-ASCII character then replace it with 0xff
	if (bom == 0xfeff || bom == 0xfffe) {
		unsigned char ch2 = (unsigned char)istr.get();
		int ch16 = (bom == 0xfeff) ? (ch << 8 | ch2) : (ch2 << 8 | ch);
		ch = (unsigned char)((ch16 >= 0x80) ? 0xff : ch16);
	}

	// Handling of newlines..
	if (ch == '\r') {
		ch = '\n';
		if (bom == 0 && (char)istr.peek() == '\n')
			(void)istr.get();
		else if (bom == 0xfeff || bom == 0xfffe) {
			int c1 = istr.get();
			int c2 = istr.get();
			int ch16 = (bom == 0xfeff) ? (c1 << 8 | c2) : (c2 << 8 | c1);
			if (ch16 != '\n') {
				istr.unget();
				istr.unget();
			}
		}
	}

	return ch;
}

void CFileDependTable::GetIncludes(const std::string &fileName, std::vector<std::string> &strIncludes)
{
	std::ifstream istr(fileName.c_str());
	if (!istr.good())
		return;

	// The UTF-16 BOM is 0xfffe or 0xfeff.
	unsigned int bom = 0;
	if (istr.peek() >= 0xfe)
	{
		bom = ((unsigned int)istr.get() << 8);
		if (istr.peek() >= 0xfe)
			bom |= (unsigned int)istr.get();
	}

	bool bNewLine = true;
	for (unsigned char ch = ReadChar(istr, bom); istr.good(); ch = ReadChar(istr, bom))
	{
		// Accept only includes that are at the start of a line
		if (ch == '#')
		{
			if (bNewLine == false)
				continue;
			bNewLine = false;

			const char* szInclude = "include";
			bool isInclude = true;
			for (int i = 0; i < 7; i++)
			{
				if (szInclude[i] != ReadChar(istr, bom))
				{
					isInclude = false;
					break;
				}

			}
			if (isInclude)
			{
				// skip spaces
				while (istr.good())
				{
					ch = ReadChar(istr, bom);
					if (!isspace(ch))
					{
						break;
					}
				}

				if (istr.good())
				{
					if (ch == '"')
					{
						std::ostringstream oss;
						while (istr.good())
						{
							ch = ReadChar(istr, bom);
							if (ch != '"')
							{
								oss << ch;
							}
							else
							{
								break;
							}
						}
						if (oss.str().size())
						{
							strIncludes.push_back(oss.str());
						}
					}
				}

			}

		}
		else if (ch == '\n')
		{
			bNewLine = true;
		}
		else if (isspace(ch))
		{
		}
		else
		{
			bNewLine = false;
		}
	}
}

CCodeFile* CFileDependTable::FindMatchedFile(CCodeFile* pFile, std::string sInclude)
{
	std::replace(sInclude.begin(), sInclude.end(), '\\', '/');

	std::vector<std::string> vecInc;
	std::string::size_type begin = 0;
	std::string::size_type end = sInclude.find('/', begin);
	while (end != std::string::npos)
	{
		std::string entry = sInclude.substr(begin, end - begin);
		if (entry == ".." && vecInc.size() && vecInc.back() != "..")
		{
			vecInc.pop_back();
		}
		else
		{
			vecInc.push_back(entry);
		}
		
		begin = end + 1;
		end = sInclude.find('/', begin);
	}
	vecInc.push_back(sInclude.substr(begin, sInclude.size() - begin));

	
	std::string fileName = *vecInc.rbegin();
	if (m_fileDict.count(fileName))
	{
		std::vector<CCodeFile*> vecMatched;

		std::pair<
			std::multimap<std::string, CCodeFile*>::iterator,
			std::multimap<std::string, CCodeFile*>::iterator> p = m_fileDict.equal_range(fileName);
		
		for (std::multimap<std::string, CCodeFile*>::iterator k = p.first; k != p.second; k++)
		{
			bool bMatch = true;
			CFileBase* pNode = k->second;
			int index = vecInc.size() - 2;
			pNode = pNode->GetParent();
			while (index >= 0 && pNode != NULL)
			{
				if (vecInc[index] == "..")
				{
					break;
				}
				if (pNode->GetName() != vecInc[index])
				{
					bMatch = false;
					break;
				}
				index--;
				pNode = pNode->GetParent();
			}
			
			if (bMatch)
			{
				int index2 = index;
				CFileBase* pFlag = pFile->GetParent();
				// 检查是不是当前目录，如是，用之
				while(index2-- >= 0 && pFlag)
				{
					pFlag = pFlag->GetParent();
				}
				if (pFlag == pNode)
				{

					// 为当前目录
					return k->second;
				}
				// 检查是否在include目录
				std::vector<CFolder*>::iterator iter = m_includePaths.begin();
				std::vector<CFolder*>::iterator iterEnd = m_includePaths.end();
				while (iter != iterEnd)
				{
					index2 = index;
					pFlag = pFile->GetParent();
					while(index2-- >= 0 && pFlag)
					{
						pFlag = pFlag->GetParent();
					}
					if (pFlag == pNode)
					{

						// 为当前目录
						return k->second;
					}
					iter++;
				}

				
				// 不在当前目录
				vecMatched.push_back(k->second);
				
			}
		}

		return FindShortestPath(vecMatched, pFile);
	
	}

	return NULL;
}

void CFileDependTable::DumpFileDependResults()
{
	std::ofstream ofs;
	std::string sPath = GetProgramDirectory();
	sPath += "log/fileDependence.log";
	CreateLogDirectory();
	ofs.open(Path::toNativeSeparators(sPath).c_str(), std::ios_base::trunc);

	CCodeFile* pFile = m_begin;
	while (pFile != NULL)
	{
		ofs << Path::toNativeSeparators(pFile->GetFullPath()) << ", " << pFile->GetSize() << ", expanded " << pFile->GetExpandCount() << std::endl;
		std::list<CCodeFile*>::iterator iter = pFile->GetAllDepends().begin();
		std::list<CCodeFile*>::iterator iterEnd = pFile->GetAllDepends().end();
		iterEnd--;
		for (;iter != iterEnd;iter++)
		{
			ofs << "\t\t[" <<  Path::toNativeSeparators((*iter)->GetFullPath()) << ", " << (*iter)->GetSize() << "]" << std::endl;
		}
		ofs << std::endl;
		pFile = pFile->GetNext();
	}
	
	ofs.close();
}

CCodeFile* CFileDependTable::FindShortestPath(const std::vector<CCodeFile*>& vecFiles, CCodeFile* pFile)
{
	if (vecFiles.empty())
	{
		return NULL;
	}

	if (vecFiles.size() == 1)
	{
		return vecFiles[0];
	}

	std::vector<CFileBase*> filePath;
	CFileBase* pFlag = pFile;
	while (pFlag != NULL)
	{
		filePath.push_back(pFlag);
		pFlag = pFlag->GetParent();
	}
	std::multimap<int, CCodeFile*> disDict;
	std::vector<CCodeFile*>::const_iterator iterBegin = vecFiles.begin();
	std::vector<CCodeFile*>::const_iterator iterEnd = vecFiles.end();
	for (std::vector<CCodeFile*>::const_iterator iter = iterBegin; iter != iterEnd; iter++)
	{
		std::vector<CFileBase*> filePath2;
		pFlag = *iter;
		while (pFlag != NULL)
		{
			filePath2.push_back(pFlag);
			pFlag = pFlag->GetParent();
		}

		int index = 0;
		int minSize = filePath.size() <= filePath2.size() ? filePath.size() : filePath2.size();

		while (index < minSize)
		{
			if (filePath[filePath.size() - 1 - index] != filePath2[filePath2.size() - 1 - index])
			{
				break;
			}
			index++;
		}

		int distance = filePath.size() - index + filePath2.size() - index;
		
		std::pair<int, CCodeFile*> newPair(distance, *iter);
		disDict.insert(newPair);
	}
	
	return disDict.begin()->second;
}

bool CFileDependTable::CreateFileDependTree(const std::vector<std::string> &paths, const std::vector<std::string> &includePaths, const std::vector<std::string>& excludesPaths)
{
	m_pRoot = new CFolder("root");
	std::vector<std::string>::const_iterator iterBegin = paths.begin();
	std::vector<std::string>::const_iterator iterEnd = paths.end();
	for (std::vector<std::string>::const_iterator iter = iterBegin; iter != iterEnd; iter++)
	{
		std::string sPath = Path::fromNativeSeparators(getAbsolutePath(*iter));
		if (!BuildFileDependTree(sPath, Path::acceptFile_H))
		{
			return false;
		}
	}

	iterBegin = includePaths.begin();
	iterEnd = includePaths.end();
	for (std::vector<std::string>::const_iterator iter = iterBegin; iter != iterEnd; iter++)
	{
		std::string sPath = Path::fromNativeSeparators(getAbsolutePath(*iter));
		if (!BuildFileDependTree(sPath, Path::isHeader))
		{
			return false;
		}
	}

	iterBegin = excludesPaths.begin();
	iterEnd = excludesPaths.end();
	for (std::vector<std::string>::const_iterator iter = iterBegin; iter != iterEnd; iter++)
	{
		CFileBase* pFile = FindFile(*iter);

		if (pFile)
		{
			pFile->SetIgnore(true);
		}
	}

	return true;
}

void CFileDependTable::UpdateIncludes(const std::vector<std::string>& includePaths)
{
	std::vector<std::string>::const_iterator iterBegin = includePaths.begin();
	std::vector<std::string>::const_iterator iterEnd = includePaths.end();
	for(std::vector<std::string>::const_iterator iter = iterBegin;iter != iterEnd; iter++)
	{
		CFileBase* pFile = FindFile(*iter);
		if (pFile)
		{
			CFolder* pFolder = dynamic_cast<CFolder*>(pFile);
			if (pFolder)
			{
				m_includePaths.push_back(pFolder);
			}
		}
	}
	CCodeFile* pCode = m_begin;
	while (pCode)
	{
		ParseIncludes(pCode);
		pCode = pCode->GetNext();
	}
}

CCodeFile* CFileDependTable::GetFirstFile()
{
	return m_begin;
}

CFileBase* CFileDependTable::FindFile(const std::string& filePath)
{
	CFolder* pFolder = m_pRoot;
	std::string sPath = Path::fromNativeSeparators(filePath);
	// remove last '/' if exists
	if (*sPath.rbegin() == '/')
	{
		sPath = sPath.substr(0, sPath.size() - 1);
	}

	std::string entry;
	std::string::size_type begin = 0;
	std::string::size_type end = sPath.find('/', begin);
	CFileBase* pFile = NULL;
	while (end != std::string::npos)
	{
		entry = sPath.substr(begin, end - begin);
		//if (!entry.empty())
		//{
			pFile = pFolder->FindFile(entry.c_str());
			if (!pFile)
				return NULL;

			pFolder = dynamic_cast<CFolder*>(pFile);
			if (!pFolder)
				return NULL;
		//}
		begin = end + 1;
		end = sPath.find('/', begin);
	}
	entry = sPath.substr(begin, sPath.size() - begin);

	return pFolder->FindFile(entry);
}

bool CFileDependTable::BuildFileDependTree(std::string &sPath, fp_fileFilter fp)
{
	CFolder* pFolder = m_pRoot;
	// remove last '/' if exists
	if (*sPath.rbegin() == '/')
	{
		sPath = sPath.substr(0, sPath.size() - 1);
	}
	std::string sNative = Path::toNativeSeparators(sPath);
	bool bDir = FileLister::isDirectory(sNative);
	if (!bDir && (!FileLister::fileExists(sPath) || !(*fp)(sPath)))
		return true;

	bool bNew = false;
	std::string entry;
	std::string::size_type begin = 0;
	std::string::size_type end = sPath.find('/', begin);
	while (end != std::string::npos)
	{
		entry = sPath.substr(begin, end - begin);
		//if (!entry.empty())
		//{
			pFolder = pFolder->AddFolder(entry.c_str(), bNew);
		//}
		begin = end + 1;
		end = sPath.find('/', begin);
	}
	entry = sPath.substr(begin, sPath.size() - begin);

	if (bDir)
	{
		pFolder = pFolder->AddFolder(entry.c_str(), bNew);

		bool bRet = BuildTable(pFolder, sPath.c_str(), fp);
		if (!bRet)
		{
			ReleaseTable();
			return false;
		}
	}
	else
	{
		std::size_t uSize = GetFileSize(sPath);

		CCodeFile* pFile = pFolder->AddCodeFile(entry.c_str(), uSize, bNew);

		if (bNew)
		{
			if (!m_begin)
			{
				m_begin = pFile;
			}
			else
			{
				m_flag->SetNext(pFile);
			}
			m_flag = pFile;

			std::pair<std::string, CCodeFile*> newPair(entry, pFile);
			m_fileDict.insert(newPair);
		}
	}

	return true;
}

std::string CFileDependTable::GetProgramDirectory()
{
	std::string sPath;
#ifdef WIN32
	char szFilePath[MAX_PATH] = {0};
	::GetModuleFileNameA(NULL, szFilePath, MAX_PATH );
	sPath = Path::simplifyPath(Path::fromNativeSeparators(szFilePath).c_str());
	sPath = sPath.substr(0, sPath.rfind('/') + 1);
#else
	#define MAX_PATH 0x100
	char szFilePath[MAX_PATH];
	int cnt = readlink("/proc/self/exe", szFilePath, MAX_PATH);
	if (cnt < 0 || cnt >= MAX_PATH)
	{
		return sPath;
	}
	
	for (int i = cnt; i >=0; --i)
	{
		if (szFilePath[i] == '/')
		{
			szFilePath[i+1] = '\0';
			break;
		}
	}
	sPath = szFilePath;
#endif

	return sPath;
}

bool CFileDependTable::CreateLogDirectory(std::string* pLogPath)
{
	std::string sPath = GetProgramDirectory();
	sPath +="log";
	sPath = Path::toNativeSeparators(sPath);
	if (pLogPath)
	{
		pLogPath->assign(sPath.c_str());
	}
	if (!FileLister::fileExists(sPath))
	{
#ifdef WIN32
		return (SHCreateDirectoryExA(NULL, sPath.c_str(), NULL) == ERROR_SUCCESS);
#else
		mkdir(sPath.c_str(), 0777);
		return true;
#endif
	}
	else
		return true;
}

CFileBase::CFileBase()
{
	m_fileType = FT_NONE;
	m_uID = s_id++;
	m_parent = NULL;
	m_bIgnore = false;
}

CFileBase::~CFileBase()
{

}

void CFileBase::SetParent(CFolder* pParent)
{
	m_parent = pParent;
}

EFileType CFileBase::GetFileType()
{
	return m_fileType;
}

const std::string& CFileBase::GetName()
{
	return m_name;
}

CFolder* CFileBase::GetParent()
{
	return m_parent;
}

std::string CFileBase::GetFullPath()
{
	std::string sFullPath = GetName();
	CFileBase* pFile = GetParent();
	while (pFile && pFile->GetName() != "root")
	{
		sFullPath = pFile->GetName() + "/" + sFullPath;
		pFile = pFile->GetParent();
	}
	return sFullPath;
}

CFolder::CFolder(const char* szName)
{
	m_fileType = FT_FOLDER;
	m_name = szName;
}

void CFolder::AddFile(CFileBase* pFile)
{
	
	m_subs.push_back(pFile);
	pFile->SetParent(this);
}

CFolder::~CFolder()
{
	for (std::vector<CFileBase*>::iterator I = m_subs.begin(), E = m_subs.end(); I != E; ++I)
	{
		delete *I;
	}
	m_subs.clear();
}

void CFolder::Release()
{
	std::vector<CFileBase*>::iterator iter = m_subs.begin();
	for (; iter != m_subs.end(); iter++)
	{
		if ((*iter)->GetFileType() == FT_FOLDER)
		{
			CFolder* pFolder = dynamic_cast<CFolder*>(*iter);
			if (pFolder)
			{
				pFolder->Release();
			}
			delete pFolder;
		}
		else if ((*iter)->GetFileType() == FT_CODE)
		{
			delete *iter;
		}
	}
	m_subs.clear();
}

CFileBase* CFolder::FindFile(const std::string& fileName)
{
	std::vector<CFileBase*>::iterator iter = m_subs.begin();
	std::vector<CFileBase*>::iterator iterEnd = m_subs.end();
	while(iter != iterEnd)
	{
		if ((*iter)->GetName() == fileName)
		{
			return *iter;
		}
		iter++;
	}
	return NULL;
}

CCodeFile* CFolder::AddCodeFile(const std::string& sName, std::size_t uSize, bool& bNew)
{
	CCodeFile* pCode = NULL;
	CFileBase* pFile = FindFile(sName);
	if (pFile && pFile->GetFileType() == FT_CODE)
	{
		bNew = false;
		return dynamic_cast<CCodeFile*>(pFile);
	}
	else
	{
		bNew = true;
		pCode = new CCodeFile(sName.c_str(), uSize);
		AddFile(pCode);
		return pCode;
	}
}

CFolder* CFolder::AddFolder(const std::string& sName, bool& bNew)
{
	CFolder* pFolder = NULL;
	CFileBase* pFile = FindFile(sName);
	if (pFile && pFile->GetFileType() == FT_FOLDER)
	{
		bNew = false;
		return dynamic_cast<CFolder*>(pFile);
	}
	else
	{
		bNew = true; 
		pFolder = new CFolder(sName.c_str());
		AddFile(pFolder);
		return pFolder;
	}
}

const std::vector<CFileBase*>& CFolder::GetSubs()
{
	return m_subs;
}

void CFolder::SetIgnore(bool ignore)
{
	CFileBase::SetIgnore(ignore);

	for (std::vector<CFileBase*>::iterator iter = m_subs.begin(), E = m_subs.end();iter != E; ++iter)
	{
		CFileBase* pFile = *iter;
		if (pFile)
		{
			pFile->SetIgnore(ignore);
		}
	}
}

void CFolder::RemoveFile(const std::string& sName)
{
	std::vector<CFileBase*>::iterator iter = m_subs.begin();

	while (iter != m_subs.end())
	{
		if ((*iter)->GetName() == sName)
		{
			CFileBase* pFile = *iter;
			m_subs.erase(iter);
			if (pFile->GetFileType() == FT_CODE)
			{
				delete pFile;
			}
			else if (pFile->GetFileType() == FT_FOLDER)
			{
				CFolder* pFolder = dynamic_cast<CFolder*>(pFile);
				if (pFolder)
				{
					pFolder->Release();
					delete pFolder;
				}
			}
			return;
		}
		iter++;
	}
}

CCodeFile::CCodeFile(const char* szName, std::size_t size)
{
	m_fileType = FT_CODE;
	m_name = szName;
	m_size = size;
	m_next = NULL;
	m_nExpandCount = 0;
}

CCodeFile::~CCodeFile()
{

}

void CCodeFile::SetNext(CCodeFile* next)
{
	m_next = next;
}

CCodeFile* CCodeFile::GetNext()
{
	return m_next;
}

void CCodeFile::AddDependFile(CCodeFile* pFile)
{
	if (pFile)
		m_depends.push_back(pFile);
}

std::vector<CCodeFile*>& CCodeFile::GetDepends()
{
	return m_depends;
}

std::size_t CCodeFile::GetSize() const
{
	return m_size;
}

void CCodeFile::FillAllDepends()
{
	//m_allDepends.clear();

	std::set<CCodeFile*> included;
	ExpandIncludes(this, m_allDepends, included);
}

std::list<CCodeFile*>& CCodeFile::GetAllDepends()
{
	if (m_allDepends.empty())
	{
		FillAllDepends();
	}
	return m_allDepends;
}

void CCodeFile::ExpandIncludes(CCodeFile* pCodeFile, std::list<CCodeFile*>& allDepends, std::set<CCodeFile*>& included)
{
	//char szOutput[1024];
	//sprintf(szOutput, "zydebug, %s", pCodeFile->GetFullPath());
	//::OutputDebugStringA(szOutput);

	if (included.count(pCodeFile))
	{
		return;
	}

	included.insert(pCodeFile);
	
	std::vector<CCodeFile*>& depends = pCodeFile->GetDepends();
	
	if (depends.empty())
	{
		pCodeFile->m_allDepends.push_back(pCodeFile);
		allDepends.push_back(pCodeFile);
		return;
	}

	std::vector<CCodeFile*>::iterator iter = depends.begin();
	for (;iter != depends.end(); iter++)
	{
		if (included.count(*iter))
		{
			continue;
		}
		if ((*iter)->m_allDepends.empty())
		{
			//included.insert(*iter);
			ExpandIncludes(*iter, allDepends, included);
		}
		else
		{
			for (std::list<CCodeFile*>::iterator iter2 = (*iter)->m_allDepends.begin(); iter2!= (*iter)->m_allDepends.end(); iter2++)
			{
				if (!included.count(*iter2))
				{
					allDepends.push_back(*iter2);
					included.insert(*iter2);
				}
			} 
		}
	}

	allDepends.push_back(pCodeFile);
}

void CCodeFile::AddExpandCount()
{
	++m_nExpandCount;
}
