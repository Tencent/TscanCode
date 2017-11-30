class C
{
	public void Demo()
	{
		List<int> list = new List<int>() { 1, 2, 3 };
		foreach (var i in list)
		{
			//遍历过程中删除了容器内容
			list.Remove(i);  
		}
	}
}