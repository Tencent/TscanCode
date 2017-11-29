char * Demo()
{
	char sz[]="hello TspyCode!";
	// 返回了局部变量的地址（栈上的地址出了该函数就可能失效）
	return sz;
}