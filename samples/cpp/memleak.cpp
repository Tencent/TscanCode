void Demo()
{
	// new了对象后没有释放
	char *p = new char;
	p = nullptr;
}