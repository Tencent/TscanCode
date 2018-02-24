void Demo()
{
	// 格式化字符串和参数个数不匹配
	printf("hello %s\n");
	printf("hello %s\n", "world", 123);
	printf("hello %.*s\n", "world");
	printf("hello %*-*s\n", "world");
}