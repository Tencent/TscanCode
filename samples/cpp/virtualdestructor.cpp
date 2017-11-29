class Base
{
	// 应该使用虚函数
	public: ~Base()
	{

	}
};
class Derived : public Base
{
public: 
	~Derived()
	{
		(void)11;
	}
};

void Demo()
{
	Base *base = new Derived;
	// 因为析构函数不是虚函数，导致delete时不会调用Derived的析构函数
	delete base;
}