-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local Control = require "ui/control"
local stripedCircle = require "ui/striped"

local ColorDabs = {}
ColorDabs.__index = ColorDabs
setmetatable(ColorDabs, {__index = Control})

local other = {
    fill = "line",
    line = "fill"
}

function ColorDabs:update(r, g, b)
    if r == nil then
        self.paint[self.active] = nil
        self.callback(self.active, nil)
    else
        local path = self.dabs[self.active].path[1]
        path:setFillColor(r, g, b)
        self.paint[self.active] = path:getFillColor()
        self.callback(self.active, r, g, b)
    end
end

function ColorDabs:updateColorWheel()
    local paint = self.paint[self.active]
    if paint ~= nil then
        self.colorwheel:setRGBColor(unpack(paint.rgba))
    else
        self.colorwheel:setEmpty()
    end
end

function ColorDabs:setLineColor(paint)
    self.paint["line"] = paint
    if paint ~= nil then
        self.dabs.line.path[1]:setFillColor(unpack(paint.rgba))
    end
    self:updateColorWheel()
end

function ColorDabs:setFillColor(paint)
    self.paint["fill"] = paint
    if paint ~= nil then
        self.dabs.fill.path[1]:setFillColor(unpack(paint.rgba))
    end
    self:updateColorWheel()
end

function ColorDabs:setActive(active)
    self.active = active
    self:updateColorWheel()
end

function ColorDabs:draw()
    love.graphics.setColor(1, 1, 1)

    local x, y = self:center()

    local dabs = self.dabs
    local dabRadius = self.dabRadius

    if self.paint.line ~= nil then
        dabs.line:draw(x, y)
    else
        dabs.noline:draw(x, y)
    end
    if self.paint.fill ~= line then
        dabs.fill:draw(x, y)
    else
        dabs.nofill:draw(x, y)
    end

    dabs.highlight[self.active]:draw(x, y)
end

function ColorDabs:click(mx, my)
    local x, y = self:center()
    local dabRadius = self.dabRadius
    local x = mx - x
    local y = my - y
    local dabs = self.dabs
    if dabs.line:hit(x, y) ~= nil then
        self:setActive("line")
        return function() end
    elseif dabs.fill:hit(x, y) ~= nil then
        self:setActive("fill")
        return function() end
    end
    return nil
end

function ColorDabs:center()
	return self.x + self.w / 2, self.y + self.h / 2
end

function ColorDabs:getOptimalHeight()
	return 30
end

ColorDabs.new = function(callback, colorwheel)
    local dabs = {}
    local dabRadius = 15
    local pos = dabRadius * 1.5

    dabs.line = tove.newGraphics()
    dabs.line:drawCircle(-pos, 0, dabRadius)
    dabs.line:setLineColor(0, 0, 0)
    dabs.line:setLineWidth(1)
    dabs.line:setFillColor(1, 1, 1)
    dabs.line:closeSubpath()
    dabs.line:drawCircle(-pos, 0, dabRadius / 2)
    dabs.line:invertSubpath()
    dabs.line:fill()
    dabs.line:stroke()

    dabs.fill = tove.newGraphics()
    dabs.fill:drawCircle(pos, 0, dabRadius)
    dabs.fill:setLineColor(0, 0, 0)
    dabs.fill:setLineWidth(1)
    dabs.fill:setFillColor(1, 1, 1)
    dabs.fill:stroke()
    dabs.fill:fill()

    dabs.noline = stripedCircle(-pos, 0, dabRadius, dabRadius / 2, math.pi / 4)
    dabs.nofill = stripedCircle(pos, 0, dabRadius, 0, math.pi / 4)

    local function highlight(x)
        local high = tove.newGraphics()
        high:drawCircle(x, 0, dabRadius + 4)
        high:setLineWidth(2)
        high:setLineColor(1, 1, 1)
        high:stroke()
        return high
    end

    dabs.highlight = {
        line = highlight(-dabRadius * 1.5),
        fill = highlight(dabRadius * 1.5)
    }

    --dabs.line:setDisplay("flatmesh", tove.quasifixed(4))
    --dabs.line:setUsage("colors", "dynamic")
    --dabs.line:setResolution(5)
    --dabs.fill:setDisplay("flatmesh", tove.quasifixed(4))
    --dabs.fill:setUsage("colors", "dynamic")
    --dabs.fill:setResolution(5)

    local p = Control.init(setmetatable({
        active = "fill", colorwheel = colorwheel, paint = {},
        dabs = dabs, dabRadius = dabRadius, callback = callback}, ColorDabs))
    colorwheel:setCallback(function(...)
        p:update(...)
    end)
    p:setActive("fill")
    return p
end

return ColorDabs
