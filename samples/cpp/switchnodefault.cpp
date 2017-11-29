int Demo()
{
	int i = rand() % 4;
	// switch语句没有default
	switch (i)
	{
	case 1:
		return 1;
	case 2:
		return 2;
	}

	return 0;
}