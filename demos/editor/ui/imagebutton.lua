-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local Control = require "ui/control"

local ImageButton = {}
ImageButton.__index = ImageButton
setmetatable(ImageButton, {__index = Control})

function ImageButton:draw()
	local x, y = self.x, self.y
	if self.selected then
		self.sframe:draw(x, y)
	else
		self.frame:draw(x, y)
	end
	self.image:draw(x, y)
end

function ImageButton:inside(mx, my)
	local x, y = self.x, self.y
	return mx >= x - 16 and my >= y - 16 and mx <= x + 16 and my <= y + 16
end

function ImageButton:getOptimalHeight()
    return 32
end

ImageButton.new = function(name)
	local svg = love.filesystem.read("images/" .. name .. ".svg")
	local image = tove.newGraphics(svg, 24)
	image:setResolution(2)

	local r = 4

	local frame = tove.newGraphics()
	frame:drawRect(-16, -16, 32, 32, r, r)
	frame:setLineColor(0, 0, 0)
	frame:setLineWidth(1)
	frame:stroke()
	frame:setResolution(2)

	local sframe = tove.newGraphics()
	sframe:drawRect(-16, -16, 32, 32, r, r)
	sframe:setLineColor(0, 0, 0)
	sframe:setLineWidth(1)
	sframe:stroke()
	sframe:setFillColor(0.8, 0.8, 0.8)
	sframe:fill()
	sframe:setResolution(2)

	return Control.init(setmetatable({
		selected = false,
		frame = frame,
		sframe = sframe,
		image = image}, ImageButton))
end

return ImageButton
