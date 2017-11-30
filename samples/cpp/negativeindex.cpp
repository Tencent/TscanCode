void Demo(int j)
{
	int a[10] = {0};

	int i=9;
	if (j > 5)
	{
		i = -1;
	}

	// i可能等于-1，并且被用作数组下标
	a[i] = a[i] + 1;
}