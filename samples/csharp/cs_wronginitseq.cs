class C
{
	public static int s_i2 = 1; 
	//初始化顺序有问题，应该先初始化s_i4
	public static int s_i3 = s_i4 * 12 + s_i2; 
	public static int s_i4 = 2;
}