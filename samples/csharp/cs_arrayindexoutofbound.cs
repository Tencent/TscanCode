class C
{
	void Demo()
	{
		int[] a = new int[10];

		for (int i = 0; i < 11; i++)
		{
			//可能访问到a[10], 导致越界错误
			a[i] = i; 
		}
	}
}