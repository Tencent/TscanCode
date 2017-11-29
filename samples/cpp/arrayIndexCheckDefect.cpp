char buf[10];

void Demo(int i)
{
	// i可能等于10，下标保护条件存在漏洞
	if(i < 0 || i > 10)
		return;

	// i等于10时，数组越界
	buf[i] = 'Q';
}