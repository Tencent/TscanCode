bool comp(int a, int b)
{
	//自定义比较函数，值相等时应该返回false
	if (a < b)
	{
		return false;
	}
	return true;
}

void Demo(std::vector<int> &v)
{
	std::sort(v.begin(), v.end(), comp);
}