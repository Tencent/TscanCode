function Demo(count)
	local t 
	if count > 1 then
		t = {1, 2}
	elseif count == 1 then
		t = {1}
	end
	--t没有在默认分支中赋值，这里可能为nil
	t[0] = 0 
	return t
end