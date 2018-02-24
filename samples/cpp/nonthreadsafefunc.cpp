void Demo()
{
	//asctime属于非线程安全函数
	//参考 https://www.ibm.com/developerworks/cn/linux/l-cn-posixsec/
	std::time_t tm = asctime(nullptr);
}