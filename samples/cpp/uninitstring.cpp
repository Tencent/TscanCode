void Demo(char *p, char *q)
{
	//如果p的长度超过q, p不能保证已'\0'结尾
	strncpy(p, q, strlen(q));
}