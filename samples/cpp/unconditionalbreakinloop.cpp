int Demo( int iMax )
{
	int iCount=0;
	while (iCount < iMax)
	{
		//没有任何条件的break一定会执行，会导致循环异常中断
		break;
		iCount++;
	}
	return iCount;
}