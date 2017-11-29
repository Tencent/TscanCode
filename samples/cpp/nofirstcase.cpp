void Demo(int b,int c)
{
	switch(b)
	{
		c=3;// case之前不应该有代码
	case 2:
		c=2;
		break;
	default:
		break;
	}
}