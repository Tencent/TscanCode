int Demo(STNullPointer* npSt)
{
	// if条件表达式存在逻辑漏洞，&&应该换成||
	if(npSt == nullptr && npSt->m_node)
	{
		return nResult;
	}
	return 0;
}