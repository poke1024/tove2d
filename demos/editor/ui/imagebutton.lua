-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local Control = require "ui/control"

local ImageButton = {}
ImageButton.__index = ImageButton
setmetatable(ImageButton, {__index = Control})

function ImageButton:draw()
	local x, y = self.x, self.y

	x = x + 12
	y = y + 12

	if self.selected then
		love.graphics.setColor(0.75, 0.75, 0.75)
		love.graphics.rectangle("fill", x - 12, y - 12, 24, 24)
	end

	love.graphics.setColor(0.5, 0.5, 0.5)
    love.graphics.rectangle("line", x - 12, y - 12, 24, 24)

	if self.selected then
		love.graphics.setColor(0.25, 0.25, 0.25)
	else
		love.graphics.setColor(1, 1, 1)
	end

	self.image:draw(x, y)
end

function ImageButton:inside(mx, my)
	local x, y = self.x, self.y
	return mx >= x and my >= y and mx <= x + 24 and my <= y + 24
end

function ImageButton:click(x, y)
	if self:inside(x, y) then
		self.selected = true
		if self.callback ~= nil then
			self.callback(self)
		end
		return function() end
	end
end

function ImageButton:getOptimalWidth()
    return 24
end

function ImageButton:getOptimalHeight()
    return 24
end

local function loadImage(name)
    local svg = love.filesystem.read("images/" .. name .. ".svg")
    local image = tove.newGraphics(svg, 16)
    image:setResolution(2)
    image:setMonochrome(1, 1, 1)
    return image
end

function ImageButton:setCallback(callback)
    self.callback = callback
end

ImageButton.group = function(container, ...)
	local buttons = {}
	for _, name in ipairs({...}) do
		local b = ImageButton.new(name)
		b.ypad = 4
		b.xpad = 4
		container:add(b)
		table.insert(buttons, b)
	end
	return buttons
end

ImageButton.new = function(path, callback)
	local name
	for n in string.gmatch(path, "[^/]+$") do
		name = n
	end
	return Control.init(setmetatable({
		name = name,
		selected = false,
		image = loadImage(path),
		callback = callback}, ImageButton))
end

return ImageButton
