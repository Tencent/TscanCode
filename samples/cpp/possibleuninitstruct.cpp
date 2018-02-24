struct A
{
	int m1;
	int m2;
};
void Demo(struct A &a)
{
	struct A b;
	b.m1 = 0;
	//虽然部分变量未初始化，但不一定产生后果
	memcpy(&a, &b, sizeof(b));
}