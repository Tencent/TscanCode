class C
{
	void Demo()
	{
		//使用linq表达式可能影响性能
		var dicSort = from objDic in nameDic orderby objDic.Value descending select objDic; 
	}
}