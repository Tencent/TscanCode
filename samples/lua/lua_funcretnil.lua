function Demo(param)
	if param then
		return param.a
	end
	return nil
end

ret = Demo()
--Demo函数可能返回nil, 但是这里没有检查。但是lua的返回值特别复杂，这条规则准确率不高
print(ret + 1) 