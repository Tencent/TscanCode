function Demo(param1, param2)
	local p = param1
	local p2 = param2
	--p赋值后还没有使用，又再次赋值，其中一行代码是多余的
	local p = param2 
	foo(p)
end