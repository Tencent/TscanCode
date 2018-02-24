class C
{
	public void Demo(int value)
	{
		//存在一次冗余赋值，很可能是变量写错了
		int c = c = 10; 
	}
}