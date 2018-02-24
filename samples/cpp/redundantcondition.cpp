void Demo(int a)
{
	// 存在冗余的表达式
	if(a < 3 && a < 25)
	{
		return;
	}
	
	if(a >= 10 && a >= 20)
	{
		return;
	}
}