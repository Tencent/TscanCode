class A
{
	public int add_function(int add1,int add2)
	{
		return add1+add2;
	}
}

class B:A
{
	//派生类这里应该使用new
	public int add_function(int add1, int add2) 
	{
		return add1+add2;
	}
}