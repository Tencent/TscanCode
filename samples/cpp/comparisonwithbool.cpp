bool Rand()
{
	return rand() > 10;
}

void Demo()
{
	// Rand()返回的是bool变量
	if (Rand() > 3)
		DoSomething();
}