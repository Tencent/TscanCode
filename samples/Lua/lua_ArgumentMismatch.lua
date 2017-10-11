function CheckFull(idx)
	return _G['bag'][idx] >= 10
end

function Demo()
	--函数有一个参数，且参数不可省略
	CheckFull()
end
