-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local createColorWheel = require "colorwheel"

local ColorPanel = {}
ColorPanel.__index = ColorPanel

local other = {
    fill = "line",
    line = "fill"
}

function ColorPanel:update(r, g, b)
    self.dabs[self.active].paths[1]:setFillColor(r, g, b)
    self.callback(self.active, r, g, b)
end

function ColorPanel:setLineColor(r, g, b)
    self.dabs.line.paths[1]:setFillColor(r, g, b)
end

function ColorPanel:setFillColor(r, g, b)
    self.dabs.fill.paths[1]:setFillColor(r, g, b)
end

function ColorPanel:setActive(active)
    self.active = active
    local inactive = other[active]
    local dabs = self.dabs
    dabs[active].paths[1]:setLineWidth(2.5)
    dabs[inactive].paths[1]:setLineWidth(1.5)

    self.colorwheel:setRGBColor(unpack(
        dabs[active].paths[1]:getFillColor().rgba))
end

function ColorPanel:draw()
    if not self.visible then
        return
    end
    local dabs = self.dabs
    local dabRadius = self.dabRadius
    dabs.line:draw(self.x, self.y + dabRadius)
    dabs.fill:draw(self.x, self.y + dabRadius)

    self.colorwheel:draw()
end

function ColorPanel:click(mx, my)
    if not self.visible then
        return nil
    end
    local dabRadius = self.dabRadius
    local x = mx - self.x
    local y = my - self.y - dabRadius
    local dabs = self.dabs
    if dabs.line:hit(x, y) ~= nil then
        self:setActive("line")
        return function() end
    elseif dabs.fill:hit(x, y) ~= nil then
        self:setActive("fill")
        return function() end
    end
    return self.colorwheel:click(mx, my)
end

ColorPanel.new = function(x, y, callback)
    local dabs = {}
    local dabRadius = 15

    dabs.line = tove.newGraphics()
    dabs.line:drawCircle(-dabRadius * 1.5, 0, dabRadius)
    dabs.line:setLineColor(0, 0, 0)
    dabs.line:setLineWidth(1)
    dabs.line:setFillColor(1, 0, 0)
    dabs.line:closePath()
    dabs.line:drawCircle(-dabRadius * 1.5, 0, dabRadius / 2)
    dabs.line:invertPath()
    dabs.line:fill()
    dabs.line:stroke()

    dabs.fill = tove.newGraphics()
    dabs.fill:drawCircle(dabRadius * 1.5, 0, dabRadius)
    dabs.fill:setLineColor(0, 0, 0)
    dabs.fill:setLineWidth(1)
    dabs.fill:setFillColor(1, 0, 0)
    dabs.fill:stroke()
    dabs.fill:fill()

    --dabs.line:setDisplay("flatmesh", tove.quasifixed(4))
    --dabs.line:setUsage("colors", "dynamic")
    --dabs.line:setResolution(5)
    --dabs.fill:setDisplay("flatmesh", tove.quasifixed(4))
    --dabs.fill:setUsage("colors", "dynamic")
    --dabs.fill:setResolution(5)

    local p = setmetatable({
        visible = false, x = x, y = y, active = "fill",
        dabs = dabs, dabRadius = dabRadius, callback = callback}, ColorPanel)
    p.colorwheel = createColorWheel(
        x - 75,
        y + dabRadius * 2.5,
        function(r, g, b)
            p:update(r, g, b)
        end)
    p:setActive("fill")
    return p
end

return ColorPanel
