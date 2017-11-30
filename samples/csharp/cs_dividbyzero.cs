class C
{
	void Demo()
	{
		int a = 1 - 1;
		//这里的除数是0，会导致除0异常
		int b = 1 / a; 
	}
}