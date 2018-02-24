void Demo()
{
	for (int ii = 0; ii < 10; ++ii)
	{
		// 嵌套for循环，使得外层嵌套没有意义
		for (int ii = 0; ii < 20; ++ii)
		{
			DoSomething();
		}
	}
}