function Demo(param1, param2)
	local p = param1
	foo(p)
	--p已经是local变量，这里不需要再次声明为local
	local p = param2
	foo(p)
end