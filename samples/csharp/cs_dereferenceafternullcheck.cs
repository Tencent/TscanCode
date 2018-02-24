class C
{
	public void Demo(A myA)
	{
		int a = 0;
		if (myA != null)
		{
			a=3;
		}
		//对应myA的判空保护并没有覆盖到当前行，myA可能存在空引用
		a = myA.a;
	}
}