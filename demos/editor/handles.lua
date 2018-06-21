-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local handleRadius = 5
local handleColor = {0.4, 0.4, 0.8}

local function createHandle(smooth, selected)
	local handle = tove.newGraphics()
	if smooth then
		handle:drawCircle(0, 0, handleRadius)
	else
		handle:drawRect(-handleRadius, -handleRadius,
			2 * handleRadius, 2 * handleRadius)
	end
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
	handle:setDisplay("texture")
	return handle
end

local function createHandles(selected)
	return {
		edge = createHandle(false, selected),
		smooth = createHandle(true, selected)}
end

local knots = {
	normal = createHandles(false),
	selected = createHandles(true)
}

return {
	normal = knots.normal.smooth,
	selected = knots.selected.smooth,
	knots = knots,
	color = handleColor,
	clickRadiusSqr = math.pow(handleRadius * 1.5, 2)
}
