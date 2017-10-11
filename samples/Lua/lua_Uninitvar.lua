--前序代码省略
function Demo:setData(idx, data)
	self.chatBg.setFlipX(idx % 2 == 0)
	if data then
		local person = nil
		if data.type == 1 then
			person = "爱人"
		elseif data.type == 2 then
			person = "好友"
		elseif data.type == 3 then
			person = "父母"
		end
		--person 初始值为nil, 且缺少else分支
		self.ToLabel:setString("对他的"..person.."说")
	end
end