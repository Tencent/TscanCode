void Demo( int iMax )
{
	// <<和+号的计算顺序可能存在问题
	int iTemp = 10 << iMax + 3; //FN
}