class C
{
	long Demo(int a, int b)
	{
		//这里应该先进行类型转换再做乘法
		long l = a * b;
		return l;
	}
}