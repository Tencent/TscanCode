function Demo(param1, param2) --param2 is not used
	for k,v in pairs(param1) --变量v没有使用，可以不用声明
		if k == param2 then 
			return param1[k]
		end
	end
	return nil
end