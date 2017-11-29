void Demo()
{
	list<int> l1;
	list<int> l2;
	list<int>::iterator it = l1.begin();
	// l1和l2的迭代器搞混了
	while (it != l2.end())
	{
		++it;
	}
}