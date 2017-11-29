class C
{
	static C()
	{
	   try
	   {
		   Object emptyObj = new Object();
		   if (emptyObj != null)       
		   {
			   string exceptionStr = emptyObj.ToString();
		   }
		}
	   catch (System.Exception ex)
	   {
		   Console.Write(ex.ToString());
	   }
	   finally
	   {
		   //构造函数抛出异常。会导致new赋值出现异常
		   throw new System.Exception("NULL.ToString");
	   }
	}
	~C()
	{
		Console.Write("destructor");
	}
}