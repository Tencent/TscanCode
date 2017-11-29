void Demo(int b)
{
	int a;
	if (b > 10)
	{
		a = 10;
	}
	// 缺少else分支，变量a可能不会初始化
	a++;
}