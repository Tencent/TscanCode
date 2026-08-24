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

unsigned short TernaryDemo(unsigned short input)
{
	unsigned char flag;
	if (input > 0)
	{
		flag = 1;
	}
	// 三目运算符的条件会读取变量flag
	return flag ? 10 : 0;
}