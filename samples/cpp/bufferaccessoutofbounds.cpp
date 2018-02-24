void Demo()
{
	char sz[2];
	// "123"长度超过了sz的长度，造成缓冲区越界
	strcpy(sz, "123");
}