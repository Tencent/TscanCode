void Demo( int index)
{
	char buf[10] = {0};
	// 先使用index作为数组下标再判定index的取值范围，不符合逻辑
	if( buf[index] > 0 && index < INDEX)
		return;
}