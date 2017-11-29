char * Demo(char sz[], char ch)
{
	//这种运算是不合理的，返回的是偏移后的地址
	return sz + ch;
}