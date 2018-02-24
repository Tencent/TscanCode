class C
{
	public Demo()
	{
		int j=100;
		//for循环很可能错误，这里应该是j++？
		for (int i = 0; i < 1000; j--) 
		{
			Console.Write("");
		}
	}
}