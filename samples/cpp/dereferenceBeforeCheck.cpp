void Demo(C* obj)
{
	// obj先解引用，但在后面对obj进行了判空，此处解引用并不在判空保护范围以内
	obj->dosth2();

	if(obj != nullptr) // obj判空
	{
		obj->dosth();
	}
}