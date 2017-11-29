class C
{
	public A m_A;
	public A getObject(int index)
	{
		if (index < 0)
			return null;
		else
			return m_A;
	}
	
	public void Demo()
	{
		int a = 0,b=0;
		A myA = getObject(-1);
		//getObject可能返回null，这里对应该先对返回值作判定再使用
		a = myA.a;
	}
}