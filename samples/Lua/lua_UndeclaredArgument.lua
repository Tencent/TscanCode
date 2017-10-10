
--前序代码省略

function Demo:onClose(eventType, tag)
	self:onOption(enentType, #self.data) --很明显enentType拼写错误, 成为一个新的变量
	self:hide()
end
