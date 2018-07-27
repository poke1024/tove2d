-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local boxy = require "boxy"
local class = require "boxy.class"
local stripedCircle = require "ui/striped"

local ColorDabs = class("ColorDabs", boxy.Widget)

local other = {
    fill = "line",
    line = "fill"
}

local gradients = {
    linear = true,
    radial = true
}

function ColorDabs:getPaint(type)
    return self.paint[type]
end

function ColorDabs:getActivePaint()
    return self.paint[self.active]
end

function ColorDabs:setGradientTypeUI(type)
    self.gradientTypeButtons[type]:setChecked(true)
end

function ColorDabs:getGradientTypeUI()
    for n, b in pairs(self.gradientTypeButtons) do
        if b:isChecked() then
            return n
        end
    end
    return "linear"
end

function ColorDabs:updateGradientBox()
    local paint = self.paint[self.active]
    local type = paint and paint:getType() or "none"
    local grad = gradients[type]
    self.gradientBox:setVisible(grad)
    if grad then
        self:setGradientTypeUI(type)
    end
end

function ColorDabs:colorPicked(r, g, b, a)
    if r == nil then
        self.paint[self.active] = nil
        self.callback(self.active, "none", nil)
    else
        local path = self.dabs[self.active].paths[1]
        path:setFillColor(r, g, b)
        self.paint[self.active] = tove.newColor(r, g, b, a)
        self.callback(self.active, "solid", r, g, b, a)
    end
    self:updateGradientBox()
end

function ColorDabs:updateColorWheel()
    local paint = self.paint[self.active]
    if paint == nil then
        self.colorwheel:setEmpty()
    elseif paint:getType() == "solid" then
        local r, g, b, a = unpack(paint.rgba)
        self.colorwheel:setRGBColor(r, g, b, a)
    end
end

function ColorDabs:setActiveColor(paint)
    self.paint[self.active] = paint
    self:updateGradientBox()
    self:updateColorWheel()
end

function ColorDabs:setLineColor(paint)
    self.paint["line"] = paint
    if paint ~= nil then
        self.dabs.line.paths[1]:setFillColor(unpack(paint.rgba))
    end
    self:updateGradientBox()
    self:updateColorWheel()
end

function ColorDabs:setFillColor(paint)
    self.paint["fill"] = paint
    if paint ~= nil then
        self.dabs.fill.paths[1]:setFillColor(unpack(paint.rgba))
    end
    self:updateGradientBox()
    self:updateColorWheel()
end

function ColorDabs:setActive(active)
    self.active = active
    self:updateGradientBox()
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

function ColorDabs:mousepressed(mx, my)
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

function ColorDabs:getDesiredSize()
	return 0, 30
end

function ColorDabs:makeGradientTypeButton(name, title)
    local button = boxy.PushButton(title):setMode("radio")
    button:onChecked(function()
        self.callback(self.active, name)
    end)
    return button
end

function ColorDabs:__init(callback, colorwheel, opacity)
    boxy.Widget.__init(self)

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

    self.active = "fill"
    self.colorwheel = colorwheel
    self.paint = {}
    self.dabs = dabs
    self.dabRadius = dabRadius
    self.callback = callback

    local gradientBox = boxy.GroupBox()
    self.gradientTypeButtons = {
        linear = self:makeGradientTypeButton("linear", "Linear"),
		radial = self:makeGradientTypeButton("radial", "Radial")
    }
	gradientBox:add(boxy.HBox {
        self.gradientTypeButtons.linear,
        self.gradientTypeButtons.radial
    })
	gradientBox:setFrame(false)
    gradientBox.visible = false
    self.gradientBox = gradientBox

    colorwheel:setCallback(function(...)
        self:colorPicked(...)
    end)
    self:setActive("fill")
end

return ColorDabs
