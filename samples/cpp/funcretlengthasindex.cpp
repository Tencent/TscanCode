void Demo(char* buf)
{
	int length = read(PATH, buf, LENGTH);
	// read函数返回值length最大可以等于LENGTH，直接用作下标可能越界
	buf[length] = 'Q';
}