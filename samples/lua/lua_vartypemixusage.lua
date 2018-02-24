function Demo()
	local count = DataService.PropPackageData:GetPropCount()
	if count >= ConsumePropNum then 
		--count在上一行当做数值，这里当成table，且项目没有重载__lt，应该是写错了
		return count.ToString() .. "/" .. ConsumePropNum 
	end
	return ""
end