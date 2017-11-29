function Demo(a, b)
	if a == nil and b == nil then
		return
	end
	--前面对a的判定是无效的，这里a仍然可能是nil。前面的and应该修改成or
	print(a + 1) 
end