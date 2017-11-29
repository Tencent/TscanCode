class C
{
	public Demo(int a, int c)
	{
		//这个分号很可能是多余的，导致后续代码无条件运行
		if ((a == 1) || (c == 2)); 
		{
			  Console.WriteLine("1");
		}
	}
}