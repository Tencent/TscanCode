void Demo(bool b)
{
	char *buf = (char*)malloc(10);
	if (b)
		;
	// realloc之前没有释放旧的内存
	else if (buf = (char*)realloc(buf, 100))
		;
	free(buf);
}