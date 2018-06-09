-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local Control = require "ui/control"

local Label = {}
Label.__index = Label
setmetatable(Label, {__index = Control})

function Label:draw()
	love.graphics.setFont(self.font)
    love.graphics.setColor(0.8, 0.8, 0.8)
	love.graphics.print(self.text, self.x, self.y)
end

function Label:getOptimalHeight()
	return self.font:getHeight()
end

function Label.new(font, text)
    return Control.init(setmetatable({font = font, text = text}, Label))
end

return Label
