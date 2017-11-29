void Demo(std::string &str, char ch)
{
	for (std::string::const_iterator iter = str.begin(), end = str.end();
		iter != end; ++iter)
	{
		//访问可能越界
		if (*(iter + 1) == ch)
		{
		}
	}
}