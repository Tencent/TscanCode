class Demo{
public:
	Demo();
private:
	int c;
	int b;
};

Demo::Demo()
{
	//构造函数中c没有初始化就直接使用
	std::cout << c;
}