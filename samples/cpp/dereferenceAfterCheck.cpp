void Demo(C* obj)
{
	if(obj != NULL) // obj判空
	{
		obj->dosth();
	}

	// obj解引用，此时对于obj的判空保护已经失效
	obj->dosth2();
}