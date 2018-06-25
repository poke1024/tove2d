-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local Object = require "object"

local newTransformWidget = require "widgets/transform"
local newCurvesWidget = require "widgets/curves"
local newPenWidget = require "widgets/pen"

local newTransform = require "transform"

local VBox = require "ui/vbox"
local HBox = require "ui/hbox"
local Panel = require "ui/panel"
local Label = require "ui/label"
local Slider = require "ui/slider"
local Checkbox = require "ui/checkbox"
local ColorDabs = require "ui/colordabs"
local ColorWheel = require "ui/colorwheel"
local RadioGroup = require "ui/radiogroup"
local Toolbar = require "ui/toolbar"
local ImageButton = require "ui/imagebutton"


local Selection = {}
Selection.__index = Selection

function Selection:objects()
	return pairs(self._objects)
end

function Selection:first()
	return next(self._objects)
end

function Selection:empty()
	return self._n == 0
end

function Selection:clear()
	for o in pairs(self._objects) do
		o.selected = false
	end
	self._objects = {}
	self._n = 0
	self:_changed()
end

function Selection:add(object)
	if self._objects[object] == nil then
		self._objects[object] = true
		object.selected = true
		self._n = self._n + 1
		self:_changed()
	end
end

function Selection:remove(object)
	if self._objects[object] ~= nil then
		self._objects[object] = nil
		object.selected = false
		self._n = self._n - 1
		self:_changed()
	end
end

function Selection:contains(object)
	return self._objects[object] ~= nil
end

function Selection:_changed()
	self._aabb = nil
	self._transforms = nil
end

function Selection:refreshAABB()
	self._aabb = nil
	self._transforms = nil
end

function Selection:computeAABB()
	if self._aabb ~= nil then
		return unpack(self._aabb)
	end
	local n = self._n
	if n == 1 then
		local g = self:first().graphics
		self._aabb = {g:computeAABB("exact")}
	elseif n > 1 then
		local x0, y0, x1, y1 = nil, nil, nil, nil
		local min, max = math.min, math.max
		for o in pairs(self._objects) do
			local tx0, ty0, tx1, ty1 = o:computeAABB()
			if x0 == nil then
				x0, y0, x1, y1 = tx0, ty0, tx1, ty1
			else
				x0 = min(x0, tx0)
				y0 = min(y0, ty0)
				x1 = max(x1, tx1)
				y1 = max(y1, ty1)
			end
		end
		self._aabb = {x0, y0, x1, y1}
	end
	return unpack(self._aabb)
end

function Selection:getTransform()
	local n = self._n
	if n == 1 then
		return self:first().transform
	elseif n > 1 then
		if self._transforms == nil then
			local x0, y0, x1, y1 = self:computeAABB()
			local ox, oy = (x0 + x1) / 2, (y0 + y1) / 2

			local t = newTransform(ox, oy)
			t.ox = ox
			t.oy = oy
			t:update()

			local objects = {}
			for o in pairs(self._objects) do
				objects[o] = o.transform:values()
			end

			self._transforms = {
				selection = t,
				objects = objects
			}
		end
		return self._transforms.selection
	end
end

function Selection:transform(f, ...)
	local n = self._n

	local tfm = self:getTransform()

	local stretchChanged = f(tfm, ...)
	if n == 1 then
		self:first():transformChanged(stretchChanged)
	else
		tfm:update()

		for o, ot in pairs(self._transforms.objects) do
			o.transform:set(tfm)
			o.transform:compose(ot)
			o:transformChanged(stretchChanged)
		end
	end
end

function newSelection()
	return setmetatable({
		_n = 0,
		_objects = {},
		_aabb = nil,
		_transforms = nil
	}, Selection)
end


local Editor = {}
Editor.__index = Editor

local function makeDragSelectionFunc(selection, x0, y0)
	local x, y = x0, y0

	local move = function(transform, dx, dy)
		transform.tx = transform.tx + dx
		transform.ty = transform.ty + dy
	end

	return function(mx, my)
		selection:transform(move, mx - x, my - y)
		x, y = mx, my
		return false
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
		for object in self.selection:objects() do
			for i = 1, object.graphics.paths.count do
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
		for object in self.selection:objects() do
			for i = 1, object.graphics.paths.count do
				local path = object.graphics.paths[i]
				path:setLineWidth(value)
			end
			object:refresh()
		end
	end, 0, 20)
	self.miterLimitSlider = Slider.new(function(value)
		for object in self.selection:objects() do
			for i = 1, object.graphics.paths.count do
				local path = object.graphics.paths[i]
				path:setMiterLimit(value)
			end
			object:refresh()
		end
	end, 0, 20)

	self.toolbar = Toolbar.new(function(button)
		self:updateWidget()
	end)
	self.toolbar:setBounds(
		0,
		0,
		32,
		love.graphics.getHeight())

	self.rpanel = Panel.new()
	self.rpanel:add(Label.new(self.font, "Color"))
	self.rpanel:add(self.colorDabs)
	self.rpanel:add(self.colorWheel)
	self.rpanel:add(Label.new(self.font, "Line Width"))
	self.rpanel:add(self.lineWidthSlider)
	self.rpanel:add(Label.new(self.font, "Miter Limit"))
	self.rpanel:add(self.miterLimitSlider)

	self.rpanel:add(Label.new(self.font, "Renderer"))

	do
		local hbox = HBox.new()
		local buttons = ImageButton.group(
			hbox, "texture", "mesh", "shader")
	    self.radios = RadioGroup.new(unpack(buttons))
		self.rpanel:add(hbox)
	end

	self.displaycontrol = VBox.new()
	self.displaycontrol.xpad = 0
	self.displaycontrol.ypad = 0
	function updateDisplayUI(mode, quality, usage)
		self.displaycontrol:empty()
		if mode == "mesh" then
			self.displaycontrol:add(Label.new(self.font, "Settings"))
			local vbox = VBox.new()
			vbox.frame = true

			local dynamic = Checkbox.new(
				self.font, "animated properties", function(value)
					self:setUsage("points", value and "dynamic" or "static")
				end)
			vbox:add(dynamic)
			dynamic:setChecked(usage.points == "dynamic")

			vbox:add(Label.new(
				self.font, "tesselation quality"))
			local slider = Slider.new(function(value)
				self:setDisplay("mesh", value)
			end, 0, 1)
			vbox:add(slider)
			slider:setValue(quality)

			self.displaycontrol:add(vbox)
		elseif mode == "shader" then
			self.displaycontrol:add(Label.new(self.font, "Settings"))
			local vbox = VBox.new()
			vbox.frame = true

			local checkbox = Checkbox.new(
				self.font, "vertex shader lines", function(value)
					local mode, quality = self:getDisplay()
					self:setDisplay("shader",
						{line = {
							type = value and "vertex" or "fragment",
							quality = quality.line.quality
						}})
				end)
			checkbox:setChecked(quality.line.type == "vertex")
			vbox:add(checkbox)
			vbox:add(Label.new(
				self.font, "line quality"))
			local slider = Slider.new(function(value)
				local mode, quality = self:getDisplay()
				self:setDisplay("shader",
					{line = {
						type = quality.line.type,
						quality = value
					}})
				local _, qq = self:getDisplay()
			end, 0, 1)
			slider:setValue(quality.line.quality)
			vbox:add(slider)

			self.displaycontrol:add(vbox)
		end
	end
	self.rpanel:add(self.displaycontrol)
	self.radios:setCallback(function(button)
		local newmode = button.name
		local mode, quality = self:getDisplay()
		if mode ~= newmode then
			if newmode == "shader" then
				quality = {line = {type = "vertex", quality = 1.0}}
			else
				quality = 0.5
			end
			self:setDisplay(newmode, quality)
			mode = newmode
		end
		updateDisplayUI(mode, quality, self:getUsage())
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
	if self.drag and self.drag.active and self.drag.drawfunc then
		self.drag.drawfunc()
	end

	if self.widget ~= nil then
		self.widget:draw(self.transform)
		self.rpanel:draw()
	end
	self.toolbar:draw()
end

function Editor:startdrag(transform, x, y, button, func, drawfunc)
    if func == nil then
        return false
    else
        self.drag = {
			transform = transform,
            startx = x,
            starty = y,
			button = button,
            active = false,
            func = func,
			drawfunc = drawfunc
        }
        return true
    end
end

function Editor:deselect()
	self.selection:clear()
	self.widget = nil
end

function Editor:updateWidget()
	local tool = self.toolbar:current()
	if tool == "cursor" then
		if self.selection:empty() then
			self.widget = nil
		else
			self.selection:refreshAABB()

			self.widget = newTransformWidget(
				self.handles, self.selection)
		end
	elseif tool == "location" then
		if self.selection:empty() then
			self.widget = nil
		else
			self.widget = newCurvesWidget(
				self.handles, self.selection:first())
		end
	elseif tool == "pencil" then
		self.widget = newPenWidget(self)
	else
		self.widget = nil
	end
end

function Editor:selectionChanged()
	if self.selection:empty() then
		self.widget = nil
		return
	end

	local object = self.selection:first()
	local path = object.graphics.paths[1]
	self.colorDabs:setLineColor(path:getLineColor())
	self.colorDabs:setFillColor(path:getFillColor())
	self.lineWidthSlider:setValue(path:getLineWidth())
	self.miterLimitSlider:setValue(path:getMiterLimit())
	local mode, quality = object:getDisplay()
	self.radios:select(mode)
	self.widgetDirty = true
end

local function isShiftDown()
	return love.keyboard.isDown("lshift") or
		love.keyboard.isDown("rshift")
end

function Editor:selectRectangle(x0, y0, x1, y1)
	self:deselect()

	x0, x1 = math.min(x0, x1), math.max(x0, x1)
	y0, y1 = math.min(y0, y1), math.max(y0, y1)

	for _, o in ipairs(self.objects) do
		local tx0, ty0, tx1, ty1 = o:computeAABB()
		if tx0 >= x0 and tx1 <= x1 and ty0 >= y0 and ty1 <= y1 then
			self.selection:add(o)
		end
	end

	self:selectionChanged()
end

function Editor:mouseselect(x, y, button, clickCount)
	local shift = isShiftDown()

	local objects = self.objects
	local nobjects = #objects
	for o = nobjects, 1, -1 do
		local object = objects[o]
		local graphics = object.graphics
		local scaledGraphics = object.scaledGraphics
		local lx, ly = object.transform:inverseTransformPoint(x, y)
		local ux, uy = object.transform:inverseUnscaledTransformPoint(x, y)
		local gs = self.scale

		for i = 1, graphics.paths.count do
			local hit = graphics.paths[i]:inside(lx, ly)
			if not hit then
				local scaledpath = scaledGraphics.paths[i]
				local offset = scaledpath:getLineWidth() / 2
				hit = scaledpath:nearest(ux, uy, offset + 2 * gs) ~= false
			end

			if hit then
				self.clicked = object

				do
					local s = true
					if not shift and not self.selection:contains(object) then
						self:deselect()
					end
					if shift and self.selection:contains(object) then
						self.selection:remove(object)
					else
						self.selection:add(object)
					end
				end

				if not self.selection:empty() then
					if clickCount == 2 then
						self.toolbar:select("location")
					end

					if clickCount == 1 then
						self:startdrag(self.transform, x, y, button,
							makeDragSelectionFunc(self.selection, x, y))
					else
						self.drag = nil
					end
				end

				self:selectionChanged()

				return true
			end
		end
	end

	return false
end

function Editor:mousedown(gx, gy, button, clickCount)
	local x, y = self.transform:inverseTransformPoint(gx, gy)

	if button == 1 then
		self.clicked = nil

		if self:startdrag(nil, gx, gy, button,
			self.toolbar:click(gx, gy)) then
			return
		end

		if not self.selection:empty() then
			if self:startdrag(nil, gx, gy, button,
				self.rpanel:click(gx, gy)) then
				return
			end
		end

		if self.widget ~= nil then
			if self:startdrag(self.transform, x, y, button,
				self.widget:mousedown(x, y, self.scale, button)) then
				return
			end
		end

		if not self:mouseselect(x, y, button, clickCount) then
			self:deselect()
			if clickCount == 1 then
				local x0, y0, x1, y1 = x, y, x, y
				local linecolor = self.handles.color
				local fillcolor = {unpack(linecolor)}
				table.insert(fillcolor, 0.2)
				self:startdrag(self.transform, x, y, button,
					function(mx, my)
						x1 = mx
						y1 = my
						self:selectRectangle(x0, y0, x1, y1)
					end,
					function()
						local gx0, gy0 = self.transform:transformPoint(x0, y0)
						local gx1, gy1 = self.transform:transformPoint(x1, y1)

						love.graphics.setColor(unpack(fillcolor))
						love.graphics.polygon("fill", gx0, gy0, gx1, gy0,
							gx1, gy1, gx0, gy1)

						love.graphics.setColor(unpack(linecolor))
						love.graphics.line(gx0, gy0, gx1, gy0,
							gx1, gy1, gx0, gy1, gx0, gy0)
					end)
			end
		end
	end

	if button == 2 and clickCount == 1 then
		local transform0 = self.transform:clone()
		self:startdrag(self.transform, x, y, button,
			function(mx, my)
				local t = transform0:clone()
				t:translate(mx - x, my - y)
				self.transform = t
			end)
	end
end

function Editor:mousereleased(x, y, button, clickCount)
	if button == 1 then
		local tool = self.toolbar:current()
		if self.clicked ~= nil and tool == "cursor"
			and (self.drag == nil or not self.drag.active)
			and not isShiftDown() then
			self:deselect()
			self.selection:add(self.clicked)
			self:selectionChanged()
		end

		if self.widget ~= nil then
            local drag = self.drag
			local lx, ly = self.transform:inverseTransformPoint(x, y)

            if drag ~= nil and drag.button == button and not drag.active then
                self.widget:click(lx, ly, self.scale, button)
            end
			self.widget:mousereleased(lx, ly, button)
		elseif self.widgetDirty then
			self:updateWidget()
		end
		self.widgetDirty = false
		self.drag = nil
	end
end

function Editor:keypressed(key, scancode, isrepeat)
	local command = love.keyboard.isDown("lgui") or
		love.keyboard.isDown("rgui")

	if key == "escape" then
		self:deselect()
	elseif key == "p" then
		self.toolbar:select("pencil")
	elseif command and key == "a" then
		self.selection:clear()
		for _, o in ipairs(self.objects) do
			self.selection:add(o)
		end
		self:selectionChanged()
    elseif self.widget ~= nil then
        if not self.widget:keypressed(key, scancode, isrepeat) then
			if key == "backspace" then
				local objects = {}
				for i, o in ipairs(self.objects) do
					if not self.selection:contains(o) then
						table.insert(objects, o)
					end
				end
				self.objects = objects
				self.selection:clear()
				self:selectionChanged()
			end
		end
    end
end

function Editor:update(dt)
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

function Editor:setDisplay(...)
	for o in self.selection:objects() do
		o:setDisplay(...)
	end
end

function Editor:getDisplay()
	if not self.selection:empty() then
        return self.selection:first():getDisplay()
    end
end

function Editor:setUsage(what, value)
	for o in self.selection:objects() do
		o:setUsage(what, value)
	end
end

function Editor:getUsage()
	if not self.selection:empty() then
        return self.selection:first():getUsage()
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
	self.selection:clear()
	self:selectionChanged()

	local screenwidth = love.graphics.getWidth()
	local screenheight = love.graphics.getHeight()

	for i = 1, graphics.paths.count do
		local g = tove.newGraphics()
		g:addPath(graphics.paths[i])
		self:addObject(Object.new(screenwidth / 2, screenheight / 2, g))
	end
end

local editor = setmetatable({
	handles = require "handles",
	objects = {},
	selection = newSelection(),
	widget = nil,
	widgetDirty = false,
	drag = nil,
	clicked = nil
}, Editor)

return editor
