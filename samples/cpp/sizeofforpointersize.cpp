void Demo(CPointer* pArg) 
{
	if (pArg != nullptr)
	{
		char *szBuff = pArg->szBuff;
		if (szBuff != nullptr)
		{
			// 这里sizeof指针是错误的
			memset(szBuff, 0, sizeof(szBuff));
		}
	}
}