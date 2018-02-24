void Demo()
{
	short *pShort = new short;
	// new的对象应该使用delete来释放
	free(pShort);
	pShort = NULL;
}