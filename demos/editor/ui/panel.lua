-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local VBox = require "ui/vbox"

local Panel = {}
Panel.__index = Panel
setmetatable(Panel, {__index = VBox})

function Panel:draw()
    love.graphics.setColor(0.2, 0.2, 0.2)
	love.graphics.rectangle("fill", self.x, self.y, self.w, self.h)

    love.graphics.setColor(1, 1, 1)
    VBox.draw(self)
end

function Panel.new()
    local self = VBox.init(setmetatable({}, Panel))
    self.padding = 0
    return self
end

return Panel
