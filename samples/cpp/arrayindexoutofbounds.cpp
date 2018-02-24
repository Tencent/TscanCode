void Demo(int b)
{
	int max = 5;
	int a[10];
	if (b > 0)
	{
		max = 10;
	}

	// max在b>0的时候等于10，造成数组越界
	a[max] = max;
}