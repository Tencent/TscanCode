class C
{
	public Demo()
	{
		string str_error = "This is a test string";
		//IndexOf返回-1表示未找到， 返回值检查应该是>=0
		if (str_error.IndexOf("a", 0) > 0)
		{
			Console.Write("Error");
		}
	}
}