int Demo(STNullPointer* npSt)
{
	if(npSt == nullptr)
	{
		// npSt为空时解引用
		int nResult = npSt->m_node;  
		return nResult;
	}
	return 0;
}