function Demo(a, b)
	print(a + 1) 
	--变量先使用，后判定,前面的使用可能存在问题
	if a then 
		foo(a)
	end
end