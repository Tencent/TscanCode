class C
{
	public Demo()
	{
		int nCondtion=getActive();
		//这里的条件重复了
		if ((nCondtion == 0) && (nCondtion == 0))
		{
			Console.Write("Error");
		}
	}
}