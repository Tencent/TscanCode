int* func_ret_null()
{
	return xxx();
}

void func0(int* p)
{
	p = func_ret_null();
	assert(p);

	*p = 42;
}

void func1(int* p)
{
	p = func_ret_null();
	if(p == NULL)
		return;

	*p = 42;
}

void func2(int* p)
{
	p = func_ret_null();
	if(p == NULL)
		return;

	*p = 42;
}

void func3(int* p)
{
	p = func_ret_null();
	if(p == NULL)
		return;

	*p = 42;
}

void func4(int* p)
{
	p = func_ret_null();
	if(p == NULL)
		return;

	*p = 42;
}

void func_error(int* p)
{
	// 返回值没有判空就使用，但是其他调用func_ret_null函数的地方都判了空，推断此处可能需要判空
	p = func_ret_null();
	*p = 42;
}