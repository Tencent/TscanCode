void Demo()
{
	struct STemp
	{
		int a;
		int b;
	};

	STemp sTemp;
	// sTemp.a没有初始化
	int c = sTemp.a;
	c++;
}