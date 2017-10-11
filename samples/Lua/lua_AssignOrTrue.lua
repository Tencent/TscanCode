--前序代码省略
function Demo:setAnim(state, loop, callback)
	state = state or 1
	--or true 存在缺陷，如果loop 赋值为false，loop的值还是会变成true
	loop = loop or true 
	callback = callback or 0
	--省略后续代码
end