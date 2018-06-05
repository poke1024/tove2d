-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local handleRadius = 5
local handleColor = {0.4, 0.4, 0.8}

local function createHandle(selected)
	local handle = tove.newGraphics()
	handle:drawCircle(0, 0, handleRadius)
	if selected then
		handle:setFillColor(unpack(handleColor))
		handle:fill()
		handle:setLineColor(1, 1, 1)
		handle:setLineWidth(1)
		handle:stroke()
	else
		handle:setFillColor(1, 1, 1)
		handle:fill()
		handle:setLineColor(unpack(handleColor))
		handle:setLineWidth(1)
		handle:stroke()
	end
	handle:setDisplay("bitmap")
	handle:setResolution(2)
	return handle
end

return {
	normal = createHandle(false),
	selected = createHandle(true),
	color = handleColor,
	clickRadiusSqr = math.pow(handleRadius * 1.5, 2)
}
