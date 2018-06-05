-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local ImageButton = {}
ImageButton.__index = ImageButton

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

local function newImageButton(name, x, y)
	local svg = love.filesystem.read("images/" .. name .. ".svg")
	local image = tove.newGraphics(svg, 24)
	image:setResolution(2)

	local frame = tove.newGraphics()
	frame:drawRect(-16, -16, 32, 32, 8, 8)
	frame:setLineColor(0, 0, 0)
	frame:setLineWidth(1)
	frame:stroke()
	frame:setResolution(2)

	local sframe = tove.newGraphics()
	sframe:drawRect(-16, -16, 32, 32, 8, 8)
	sframe:setLineColor(0, 0, 0)
	sframe:setLineWidth(1)
	sframe:stroke()
	sframe:setFillColor(0.6, 0.9, 0.6)
	sframe:fill()
	sframe:setResolution(2)

	return setmetatable({
		x = x,
		y = y,
		selected = false,
		frame = frame,
		sframe = sframe,
		image = image}, ImageButton)
end

local ImageRadioButtonGroup = {}
ImageRadioButtonGroup.__index = ImageRadioButtonGroup

function ImageRadioButtonGroup:draw()
	for _, button in ipairs(self.buttons) do
		button:draw()
	end
end

function ImageRadioButtonGroup:click(x, y)
	local sel = 0
	for i, button in ipairs(self.buttons) do
		if button:inside(x, y) then
			sel = i
			break
		end
	end
	if sel > 0 then
		for i, button in ipairs(self.buttons) do
			button.selected = i == sel
		end
		return self.names[sel]
	end
	return nil
end

function ImageRadioButtonGroup:select(name)
	for i, n in ipairs(self.names) do
		self.buttons[i].selected = n == name
	end
end

local function newImageRadioButtonGroup(x, y, ...)
	local buttons = {}
	x = x + 16
	local s = 0
	for _, name in ipairs {...} do
		local button = newImageButton(name, x, y)
		x = x + 40
		table.insert(buttons, button)
	end
	return setmetatable(
		{buttons = buttons, names = {...}}, ImageRadioButtonGroup)
end

return {
    newImageRadioButtonGroup = newImageRadioButtonGroup
}
