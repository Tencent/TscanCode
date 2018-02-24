void Demo(int x)
{
	// (x&&0x0f)是bool类型和int类型进行比较
	if ((x && 0x0f) == 6)
		DoSomething();
}