STNullPointer* DemoFunc()
{
	if (rand() > 1024)
	{
		STNullPointer* npSt = new STNullPointer;
		return npSt;
	}
	return nullptr;
}

int Demo()
{
	STNullPointer* npSt = DemoFunc();
	// ReturnNULLFunction可能返回空，此处没有判空直接使用
	int nResult = npSt->m_node;  // error
	ClearMemory(npSt);
	return nResult;
}