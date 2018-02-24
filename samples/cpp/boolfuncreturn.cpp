bool RetFunc()
{
	if (rand() > 10)
	{
		return 10;
	}
	return 1;
}
void Demo()
{
	//函数有返回值，却没有处理
	RetFunc();
}