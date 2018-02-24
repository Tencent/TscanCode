void Demo()
{
	A* p = func();
	if (rand() != -1)
	{
		// 此处delete了指针，下面还在使用该指针
		delete p;
	}
	int a = p->a;
}