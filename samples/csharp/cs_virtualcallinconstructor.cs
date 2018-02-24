public class C
{
	public C()
	{
		//构造函数中使用了虚函数
		int c = getInt();  
		Console.Write(c);
	}
	public virtual int getInt()
	{
		return 1;
	}
}