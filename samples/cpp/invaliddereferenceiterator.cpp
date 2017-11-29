void Demo(std::vector<int>& ivec)
{
	std::vector<int>::iterator iter,iter2;
	for(iter = ivec.begin();iter!=ivec.end();iter++)
	{
		
		
	}
	// iter此时可能指向了end()，解引用出错
	cout<<*iter;
}