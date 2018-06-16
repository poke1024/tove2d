-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local Control = require "ui/control"

local Checkbox = {}
Checkbox.__index = Checkbox
setmetatable(Checkbox, {__index = Control})

function Checkbox:draw()
    love.graphics.rectangle("line", self.x, self.y, 16, 16)
    if self.checked then
        love.graphics.rectangle("fill", self.x + 2, self.y + 2, 12, 12)
    end

	love.graphics.setFont(self.font)
    love.graphics.setColor(0.8, 0.8, 0.8)
	love.graphics.print(self.text, self.x + 24, self.y)
end

function Checkbox:click(x, y)
    if x >= self.x and y >= self.y and
        x <= self.x + 16 and y <= self.y + 16 then
        self.checked = not self.checked
        self.callback(self.checked)
    end
end

function Checkbox:getOptimalHeight()
	return self.font:getHeight()
end

function Checkbox:setChecked(checked)
    self.checked = checked
end

function Checkbox.new(font, text, callback)
    return Control.init(setmetatable({
        font = font, text = text,
        checked = false, callback = callback}, Checkbox))
end

return Checkbox
