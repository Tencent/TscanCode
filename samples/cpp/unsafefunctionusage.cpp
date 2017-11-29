void Demo(char *p, char *q)
{
	//strcpy,gets,strcat等函数存在缓冲区溢出风险，属于高危函数
	strcpy (p, q, strlen(p));
}