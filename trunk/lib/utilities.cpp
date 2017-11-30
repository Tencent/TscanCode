#include "utilities.h"
#include "path.h"

#ifdef WIN32
#include <Windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

std::string Utility::CreateLogDirectory()
{
	std::string sPath = GetProgramDirectory();
	sPath += "log";
	sPath = Path::toNativeSeparators(sPath);

	if (!FileExists(sPath))
	{
#ifdef WIN32
		::CreateDirectoryA(sPath.c_str(), nullptr);
		//SHCreateDirectoryExA(NULL, sPath.c_str(), NULL);
#else
		mkdir(sPath.c_str(), 0777);
#endif
	}
	return sPath;
}

std::string Utility::GetProgramDirectory()
{
	std::string sPath;
#ifdef WIN32
	char szFilePath[MAX_PATH] = { 0 };
	::GetModuleFileNameA(NULL, szFilePath, MAX_PATH);
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

	for (int i = cnt; i >= 0; --i)
	{
		if (szFilePath[i] == '/')
		{
			szFilePath[i + 1] = '\0';
			break;
		}
	}
	sPath = szFilePath;
#endif

	return sPath;
}

bool Utility::FileExists(const std::string &path)
{
#ifdef WIN32
	bool bExist = false;
	WIN32_FIND_DATAA wfd;
	HANDLE hFind = ::FindFirstFileA(path.c_str(), &wfd);  
	if (INVALID_HANDLE_VALUE != hFind)
	{
		bExist = true;
		::FindClose(hFind);
	}
	return bExist;
#else
	struct stat statinfo;
	int result = stat(path.c_str(), &statinfo);

	if (result < 0) { // Todo: should check errno == ENOENT?
		// File not found
		return false;
	}

	// Check if file is regular file
	if ((statinfo.st_mode & S_IFMT) == S_IFREG)
		return true;

	return false;
#endif
}

bool Utility::IsNameLikeMacro(const std::string &strName)
{
	bool bHasUpper = false;
	for (std::size_t ii = 0, nLen = strName.size(); ii < nLen; ++ii)
	{
		if (::islower(strName[ii]) || isdigit(strName[ii]))
		{
			return false;
		}
		if (!bHasUpper && ::isupper(strName[ii]))
		{
			bHasUpper = true;
		}
	}
	return bHasUpper;
}