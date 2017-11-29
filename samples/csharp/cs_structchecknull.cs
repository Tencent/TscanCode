class C
{
	void Demo(int i)
	{
		//基础类型是不能为null的，这个判定是无意义的
		if (i != null)  
		{
			Func();
		}
	}
}