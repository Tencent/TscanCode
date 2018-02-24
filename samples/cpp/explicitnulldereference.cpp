int Demo(int i)
{
	STNullPointer* npSt = nullptr;
	if(i == 42)
		return;
	// 存在一条代码路径，使得空指针解引用
	npSt->m_node = 42;
}