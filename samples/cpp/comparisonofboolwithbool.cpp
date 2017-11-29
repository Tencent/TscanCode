void Demo(){
	bool b = compare2(6);
	bool a = compare1(4);
	// bool变量之间不应该使用比较操作符
    if(b > a){
		printf("foo");
	}
}