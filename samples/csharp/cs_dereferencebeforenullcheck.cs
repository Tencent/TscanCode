class C
{
	public void Demo(A myA)
	{
		int a=0;
		a = myA.a;
		//前面已经使用了myA, 这里才检查，要么检查是多余的，要么前面的使用存在风险
		if (myA == null)
		{
			return;
		}
	}
}