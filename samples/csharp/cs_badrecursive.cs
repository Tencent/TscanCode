class C
{
	public MyEnum MyEnum
	{
		get
		{
			MyEnum mye = MyEnum.Sunday; 
			MyEnum = mye;   
			//这一行代码可能导致递归            
			return MyEnum;
		}
		set
		{
			value.ToString();
		}
	}
}