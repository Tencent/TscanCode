void Demo()
{
	FILE *pFile = fopen("c:\\test.txt", "w+");
	if (pFile != nullptr)
	{
		// 没有close文件就退出了
		printf("forget to release pFile!");
		return;
	}
}