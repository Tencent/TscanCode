void Demo()
{
	char* p = new char[100];
	// 0和100写反了
	memset(p, 100, 0);
	delete [] p;
}