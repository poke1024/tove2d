-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local Object = require "object"

local newTransformWidget = require "widgets/transform"
local newCurvesWidget = require "widgets/curves"
local newPenWidget = require "widgets/pen"

local Panel = require "ui/panel"
local Label = require "ui/label"
local Slider = require "ui/slider"
local ColorDabs = require "ui/colordabs"
local ColorWheel = require "ui/colorwheel"
local ButtonGroup = require "ui/buttongroup"

local Editor = {}
Editor.__index = Editor

local function makeDragObjectFunc(object, x0, y0)
	local x, y = x0, y0
	local transform = object.transform
	return function(mx, my)
		transform.tx = transform.tx + (mx - x)
		transform.ty = transform.ty + (my - y)
		transform:update()
		x, y = mx, my
	end
end

function Editor:transformChanged()
	local x, y = self.transform:inverseTransformPoint(0, 1)
	local nx, ny = self.transform:inverseTransformPoint(1, 1)
	self.scale = math.sqrt((x - nx) * (x - nx) + (y - ny) * (y - ny))
end

function Editor:load()
	local editor = self

	self.transform = love.math.newTransform()
	self.scale = 1

	self.font = love.graphics.newFont(11)

	local updateColor = function(what, ...)
		local widget = editor.widget
		if widget ~= nil then
            local object = widget.object
			for i = 1, object.graphics.npaths do
				local path = object.graphics.paths[i]
				if what == "fill" then
					path:setFillColor(...)
				else
					path:setLineColor(...)
				end
			end
			object:refresh()
		end
	end

	self.colorWheel = ColorWheel.new()
	self.colorDabs = ColorDabs.new(updateColor, self.colorWheel)
	self.lineWidthSlider = Slider.new(function(value)
		local widget = editor.widget
		if widget ~= nil then
			local object = widget.object
			for i = 1, object.graphics.npaths do
				local path = object.graphics.paths[i]
				path:setLineWidth(value)
			end
			object:refresh()
		end
	end, 0, 20)

	self.rpanel = Panel.new()
	self.rpanel:add(Label.new(self.font, "Color"))
	self.rpanel:add(self.colorDabs)
	self.rpanel:add(self.colorWheel)
	self.rpanel:add(Label.new(self.font, "Line Width"))
	self.rpanel:add(self.lineWidthSlider)

	self.rpanel:add(Label.new(self.font, "Renderer"))
	self.radios = ButtonGroup.new("bitmap", "mesh", "curves")
	self.radios:select("mesh")
	self.rpanel:add(self.radios)
	self.radios:setCallback(function(mode)
		self:setDisplay(mode)
	end)

	self.rpanel:setBounds(
		love.graphics.getWidth() - 180,
		0,
		180,
		love.graphics.getHeight())
end

function Editor:draw()
	love.graphics.setColor(1, 1, 1)

	love.graphics.push("transform")
	love.graphics.applyTransform(self.transform)
	for _, object in ipairs(self.objects) do
		object:draw()
	end
	love.graphics.pop()

	if self.widget ~= nil then
		self.widget:draw(self.transform)
		self.rpanel:draw()
	end
end

function Editor:startdrag(transform, x, y, button, func)
    if func == nil then
        return false
    else
        self.drag = {
			transform = transform,
            startx = x,
            starty = y,
			button = button,
            active = false,
            func = func
        }
        return true
    end
end

function Editor:mousedown(gx, gy, button, clickCount)
	local x, y = self.transform:inverseTransformPoint(gx, gy)

	if button == 1 then
		if self.widget ~= nil then
			if self:startdrag(nil, gx, gy, button,
				self.rpanel:click(gx, gy)) then
				return
			end

			if not self:startdrag(self.transform, x, y, button,
				self.widget:mousedown(x, y, self.scale, button)) then
				self.widget = nil
			end
		end
		if self.widget == nil then
			local objects = self.objects
			local nobjects = #objects
			for o = nobjects, 1, -1 do
				local object = objects[o]
				local graphics = object.graphics
				local lx, ly = object.transform:inverseTransformPoint(x, y)

				for i = 1, graphics.npaths do
					if graphics.paths[i]:inside(lx, ly) then
						if clickCount == 2 then
							self.widget = newCurvesWidget(self.handles, object)
						elseif clickCount == 1 then
							self.widget = newTransformWidget(self.handles, object)
						end

						local path = object.graphics.paths[1]
						self.colorDabs:setLineColor(path:getLineColor())
						self.colorDabs:setFillColor(path:getFillColor())
						self.lineWidthSlider:setValue(path:getLineWidth())
						self.radios:select(object:getDisplay())

						if clickCount == 1 then
							self:startdrag(self.transform, x, y, button,
								makeDragObjectFunc(object, x, y))
						else
							self.drag = nil
						end

						return
					end
				end
			end
		end
	end
end

function Editor:mousereleased(x, y, button, clickCount)
	if button == 1 then
		if self.widget ~= nil then
            local drag = self.drag
			local lx, ly = self.transform:inverseTransformPoint(x, y)

            if drag ~= nil and drag.button == button and not drag.active then
                self.widget:click(lx, ly, self.scale, button)
            end
			self.widget:mousereleased(lx, ly, button)
		end
		self.drag = nil
	end
end

function Editor:keypressed(key, scancode, isrepeat)
	if key == "escape" then
		self.widget = nil
	elseif key == "p" then
		self.widget = newPenWidget(self)
    elseif self.widget ~= nil then
        self.widget:keypressed(key, scancode, isrepeat)
    end
end

function Editor:update()
    local drag = self.drag
	if drag ~= nil then
        local x = love.mouse.getX()
		local y = love.mouse.getY()
		if drag.transform ~= nil then
			x, y = drag.transform:inverseTransformPoint(x, y)
		end

        if not drag.active then
            local dx = x - drag.startx
            local dy = y - drag.starty
            if math.sqrt(dx * dx + dy * dy) < 2 * self.scale then
                return
            end
            drag.active = true
        end

		drag.func(x, y)
	end
end

function Editor:addObject(object)
    table.insert(self.objects, object)
end

function Editor:setDisplay(mode)
    if self.widget ~= nil then
        self.widget.object:setDisplay(mode)
    end
end

function Editor:wheelmoved(wx, wy)
	local x = love.mouse.getX()
	local y = love.mouse.getY()
	local lx, ly = self.transform:inverseTransformPoint(x, y)
	self.transform:translate(lx, ly)
	self.transform:scale(1 + wy / 10)
	self.transform:translate(-lx, -ly)
	self:transformChanged()
end

function Editor:loadfile(file)
	file:open("r")
	local svg = file:read()
	file:close()

	local graphics = tove.newGraphics(svg, 512)
	graphics:clean()

	self.transform = love.math.newTransform()
	self:transformChanged()
	self.objects = {}
	self.widget = nil

	local screenwidth = love.graphics.getWidth()
	local screenheight = love.graphics.getHeight()

	for i = 1, graphics.npaths do
		local g = tove.newGraphics()
		g:addPath(graphics.paths[i])
		self:addObject(Object.new(screenwidth / 2, screenheight / 2, g))
	end
end

local editor = setmetatable({
	handles = require "handles",
	objects = {},
	widget = nil,
	drag = nil
}, Editor)

return editor
