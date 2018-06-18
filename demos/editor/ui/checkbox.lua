-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local Control = require "ui/control"

local Checkbox = {}
Checkbox.__index = Checkbox
setmetatable(Checkbox, {__index = Control})

function Checkbox:draw()
    love.graphics.rectangle("line", self.x, self.y, 12, 12)
    if self.checked then
        love.graphics.rectangle("fill", self.x + 2, self.y + 2, 8, 8)
    end

	love.graphics.setFont(self.font)
    love.graphics.setColor(0.8, 0.8, 0.8)
	love.graphics.print(self.text, self.x + 20, self.y)
end

function Checkbox:click(x, y)
    if x >= self.x and y >= self.y and
        x <= self.x + 20 + self.textwidth and y <= self.y + 12 then
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
        font = font, text = text, textwidth = font:getWidth(text),
        checked = false, callback = callback}, Checkbox))
end

return Checkbox
