class C
{
public:
	C()
	: a(0)
	{
		//函数log不初始化也不会导致什么问题
		log(b);
	}
private:
	int a;
	int b;
};