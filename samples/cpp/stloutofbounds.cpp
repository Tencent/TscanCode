void Demo()
{
	std::vector<int> foo;
	for (unsigned int ii = 0; ii <= foo.size(); ++ii)
	{
		// 应该是ii < foo.size()
		foo[ii] = 0;
	}
}