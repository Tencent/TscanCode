class C
{
	public void Demo()
	{
		Double d = 11.222;
		//检查数字是否为NaA，请使用IsNaN方法
		if (d == Double.NaN)
		{
			Console.WriteLine(d);
		}
	}
}