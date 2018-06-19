-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local Control = require "ui/control"

local Slider = {}
Slider.__index = Slider
setmetatable(Slider, {__index = Control})

local knobRadius = 8

local slider = tove.newGraphics()
slider:drawCircle(0, 0, knobRadius)
slider:setLineColor(0, 0, 0)
slider:setLineWidth(2)
slider:stroke()
slider:setFillColor(1, 1, 1)
slider:fill()

function Slider:setBounds(x, y, w, h)
    local px = self.xpad + knobRadius
    local py = self.ypad
	self.x, self.y, self.w, self.h = x + px, y + py, w - 2 * px, h - 2 * py
	self:layout()
end

function Slider:getOptimalHeight()
	return knobRadius
end

function Slider:xpos()
    return self.x + self.w * ((self.value - self.min) / (self.max - self.min))
end

function Slider:draw()
    local x, y, w = self.x, self.y, self.w
    love.graphics.setColor(0.3, 0.3, 0.3)
    love.graphics.line(x, y, x + w, y)
    love.graphics.setColor(1, 1, 1)
    slider:draw(self:xpos(), y)
end

function Slider:slide(x, y, dx)
    local f = function(x, y)
        x = x - dx
        local v = self.min + math.min(math.max(
            (x - self.x) / self.w, 0), 1) * (self.max - self.min)
        if v ~= self.value then
            self.value = v
            self.callback(v)
        end
    end
    f(x, y)
    return f
end

function Slider:click(x, y)
    local dx, dy = x - self:xpos(), y - self.y
    if dx * dx + dy * dy < knobRadius * knobRadius then
        return self:slide(x, y, dx)
    elseif x > self.x and x < self.x + self.w and
        math.abs(dy) <= knobRadius then
        return self:slide(x, y, 0)
    end
    return nil
end

function Slider:setValue(value)
    self.value = value
end

function Slider:init()
    self.xpad = 0
    self.ypad = 0
    return Control.init(self)
end

Slider.new = function(callback, min, max)
    return Slider.init(setmetatable({value = 0, min = min or 0, max = max or 1,
        callback = callback}, Slider))
end

return Slider
