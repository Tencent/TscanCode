class Demo{
public:
	Demo(){
		m_p = new char[10]
	}
	//析构函数中没有正确释放资源
	~Demo(){
	}
private:
	char *m_p;
};