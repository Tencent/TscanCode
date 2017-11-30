#ifndef utilities_h__
#define utilities_h__

#include <string>

class Utility
{
public:
	static std::string CreateLogDirectory();
	static bool IsNameLikeMacro(const std::string &strName);
private:
	static std::string GetProgramDirectory();

	static bool FileExists(const std::string &path);
};


#endif // utilities_h__
