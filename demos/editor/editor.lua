-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local Object = require "object"

local newTransformWidget = require "tools/transform"
local newCurvesWidget = require "tools/curves"
local newPenWidget = require "tools/pen"
local newGradientTool = require "tools/gradient"
local newRectangleTool = require "tools/rectangle"
local newEllipseTool = require "tools/ellipse"

local newTransform = require "transform"

local ColorDabs = require "ui/colordabs"
local ColorWheel = require "ui/colorwheel"
local Toolbar = require "ui/toolbar"

local boxy = require "boxy"
local utils = require "ui.utils"


local Selection = {}
Selection.__index = Selection

function Selection:objects()
	return ipairs(self._ordered)
end

function Selection:first()
	return self._n > 0 and self._ordered[1]
end

function Selection:size()
	return self._n
end

function Selection:empty()
	return self._n == 0
end

function Selection:clear()
	for o in pairs(self._objects) do
		o.selected = false
	end
	self._objects = {}
	self._ordered = {}
	self._n = 0
	self:_changed()
end

function Selection:add(object)
	if self._objects[object] == nil then
		self._objects[object] = true
		object.selected = true
		self._n = self._n + 1
		table.insert(self._ordered, object)
		self:_changed()
	end
end

function Selection:remove(object)
	if self._objects[object] ~= nil then
		self._objects[object] = nil
		object.selected = false
		self._n = self._n - 1
		for i, o in ipairs(self._ordered) do
			if o == object then
				table.remove(self._ordered, i)
				break
			end
		end
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
		self._aabb = {g:computeAABB("high")}
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
		_ordered = {},
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

function Editor:boolean(type)
	local objects = {}
	local editObject, editPath

	for i, object in self.selection:objects() do
		if i == 1 then
			table.insert(objects, object)
			editObject = object
			editPath = object.graphics.paths[1]
		else
			local transform = editObject.transform:get("full"):
				inverse():apply(object.transform:get("full"))

			for j = 1, object.graphics.paths.count do
				local path = object.graphics.paths[j]
				for k = 1, path.subpaths.count do
					local subpath = path.subpaths[k]:clone()
					subpath:transform(transform)
					if type == "subtract" then
						subpath:invert()
					end
					editPath:addSubpath(subpath)
				end
			end
		end
	end

	if editObject == nil then
		return
	end

	editObject:refresh()

	for i, o in ipairs(self.objects) do
		if not self.selection:contains(o) then
			table.insert(objects, o)
		end
	end

	self.objects = objects
	self.selection:clear()
	self.selection:add(editObject)
	self:selectionChanged()
end

function Editor:moveUpDown(d)
	for _, s in self.selection:objects() do
		for i, o in ipairs(self.objects) do
			if o == s then
				table.remove(self.objects, i)
				table.insert(self.objects,
					math.min(math.max(1, i + d), #self.objects + 1), o)
				break
			end
		end
	end
end

local function isArray(t)
	-- see https://stackoverflow.com/questions/7526223/how-do-i-know-if-a-table-is-an-array
	local i = 0
	for _ in pairs(t) do
		i = i + 1
		if t[i] == nil then return false end
	end
	return true
end

function toLuaCode(t)
	if type(t) == "table" then
		local entries = {}
		if isArray(t) then
			for _, v in pairs(t) do
				table.insert(entries, toLuaCode(v))
			end
		else
			for k, v in pairs(t) do
				table.insert(entries, k .. " = " .. toLuaCode(v))
			end
		end
		return "{" .. table.concat(entries, ", ") .. "}"
	elseif type(t) == "string" then
		return "\"" .. t .. "\""
	else
		return tostring(t)
	end
end

function Editor:save()
	local objects = {}
	for i, object in ipairs(self.objects) do
		table.insert(objects, object:serialize())
	end
	local data = toLuaCode({version = 1, objects = objects})
	love.system.setClipboardText(data)
	love.filesystem.setIdentity("miniedit")
	love.filesystem.write("save.lua", "return " .. data)
end

function Editor:export()
	if #self.objects < 1 then
		return
	end

	local g = tove.newGraphics()
	local proto = self.objects[1].scaledGraphics
	g:setDisplay(proto)
	g:setUsage(proto)

	for i, object in ipairs(self.objects) do
		for i, path in tove.ipairs(object.scaledGraphics.paths) do
			path = path:clone()
			path:transform(object.transform:get("draw"))
			g:addPath(path)
		end
	end

	local s = g:serialize()
	love.system.setClipboardText(toLuaCode(s))
end

function Editor:startup()
	local editor = self

	self.transform = love.math.newTransform()
	self.scale = 1

	self.font = love.graphics.newFont(11)

	local colorPicked = function(what, type, ...)
		if self.widget and self.widget.colorPicked then
			self.widget:colorPicked(what, type, ...)
		elseif type == "solid" or type == "none" then
			for _, object in self.selection:objects() do
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
	end

	self.opacitySlider = boxy.Slider(0, 1)
	self.colorWheel = ColorWheel(self.opacitySlider)
	self.colorDabs = ColorDabs(colorPicked, self.colorWheel)

	self.lineWidthSlider = boxy.Slider(0, 20)
	self.lineWidthSlider.valueChanged:connect(function(value)
		for _, object in self.selection:objects() do
			for i = 1, object.graphics.paths.count do
				local path = object.graphics.paths[i]
				path:setLineWidth(value)
			end
			object:refresh()
		end
	end)
	self.miterLimitSlider = boxy.Slider(0, 20)
	self.miterLimitSlider.valueChanged:connect(function(value)
		for _, object in self.selection:objects() do
			for i = 1, object.graphics.paths.count do
				local path = object.graphics.paths[i]
				path:setMiterLimit(value)
			end
			object:refresh()
		end
	end)

	self.toolbar = Toolbar()
	self.toolbar:setGeometry(
		0, 0, 32, love.graphics.getHeight())
	boxy.add(self.toolbar)
	self.toolbar.tool:connect(function()
		self:updateWidget()
	end)
	self.toolbar.operation:connect(function(name)
		if name == "add" or name == "subtract" then
			self:boolean(name)
		elseif name == "up_arrow" then
			self:moveUpDown(1)
		elseif name == "down_arrow" then
			self:moveUpDown(-1)
		elseif name == "eye" then
			enterDemoMode()
		elseif name == "download" then
			self:export()
		end
	end)

	self.displaycontrol = boxy.VBox():setMargins(11, 0, 11, 11)

	self.rendererButtons = {}
	local rendererButtons = utils.makeButtons(
		{"texture", "mesh", "gpux"},
		"monochrome",
		function(name, button)
			button:setMode("radio")
			self.rendererButtons[name] = button
	        button:onChecked(function()
				self:setDisplayMode(name)
	        end)
    	end)
	self.rendererBox = boxy.GroupBox()
	self.rendererBox:add(
		boxy.VBox {
			boxy.HBox(rendererButtons),
			self.displaycontrol
		}:removeMargins())

	self.lineJoinButtons = {}
	local lineJoinButtons = utils.makeButtons(
		{"round", "bevel", "miter"},
		"monochrome",
		function(name, button)
			button:setMode("radio")
			self.lineJoinButtons[name] = button
	        button:onChecked(function()
				for _, object in self.selection:objects() do
					for i = 1, object.graphics.paths.count do
						local path = object.graphics.paths[i]
						path:setLineJoin(name)
					end
					object:refresh()
				end
	        end)
    	end)
	self.lineJoinBox = boxy.GroupBox()
	self.lineJoinBox:add(
		boxy.HBox(lineJoinButtons))

	self.rpanel = boxy.Panel()
	self.rpanel:setLayout(boxy.VBox {
		boxy.Label("Color"),
		self.colorDabs,
		self.colorWheel,
		self.colorDabs.gradientBox,
		boxy.Label("Opacity"),
		self.opacitySlider,	
		boxy.Label("Line Width"),
		self.lineWidthSlider,
		boxy.Label("Line Join"),
		self.lineJoinBox,
		boxy.Label("Miter Limit"),
		self.miterLimitSlider,
		boxy.Label("Renderer Settings"),
		self.rendererBox,
		boxy.Spacer(1)
	})

	rendererButtons[1]:setChecked(true)

	self.rpanel:setGeometry(
		love.graphics.getWidth() - 180, 0,
		180, love.graphics.getHeight())
	boxy.add(self.rpanel)

	love.filesystem.setIdentity("miniedit")
	chunk = love.filesystem.load("save.lua")
	if chunk then
		self:restore(chunk)
	end
end

function Editor:setDisplayMode(newmode)
	local mode = self:getDisplay()
	if mode ~= newmode then
		if newmode == "gpux" then
			quality = {"vertex", 1.0}
		else
			quality = {200}
		end
		self:setDisplay(newmode, unpack(quality))
		mode = newmode
	end
	self:updateDisplayUI()
end

function Editor:updateLineUI()
	local mode, quality1, quality2 = self:getDisplay()

	for _, button in pairs(self.lineJoinButtons) do
		button:setEnabled(true)
	end

	self.miterLimitSlider:setEnabled(true)

	if mode == "gpux" then
		if quality1 == "vertex" then
			self.lineJoinButtons["round"]:setEnabled(false)
		else
			for _, button in pairs(self.lineJoinButtons) do
				button:setEnabled(false)
			end
			self.miterLimitSlider:setEnabled(false)
		end
	end
end

function Editor:updateDisplayUI()
	local mode = self:getDisplay()
	local usage = self:getUsage()

	self.displaycontrol:clear()
	if mode == "mesh" then
		local animatedCheckBox = boxy.CheckBox("For Animation")
		animatedCheckBox.valueChanged:connect(function(value)
			self:setUsage("points", value and "dynamic" or "static")
		end)
		animatedCheckBox:setChecked(usage.points == "dynamic")

		local qualitySlider = boxy.Slider(0, 1000)
		qualitySlider.valueChanged:connect(function(value)
			self:setDisplay("mesh", value)
		end)
		qualitySlider:setValue(self:getQuality())

		self.displaycontrol:add(
			boxy.Label("Tesselation Quality"),
			qualitySlider,
			animatedCheckBox)
	elseif mode == "gpux" then
		local lineType, lineQuality = self:getQuality()

		local vslCheckBox = boxy.CheckBox("Mesh Lines")
		vslCheckBox.valueChanged:connect(function(value)
			local _, lineQuality = self:getQuality()
			self:setDisplay(
				"gpux",
				value and "vertex" or "fragment",
				lineQuality)
			self:updateLineUI()
		end)
		vslCheckBox:setChecked(lineType == "vertex")

		local lineQualitySlider = boxy.Slider(0, 1)
		lineQualitySlider.valueChanged:connect(function(value)
			local lineType, _ = self:getQuality()
			self:setDisplay(
				"gpux",
				lineType,
				value)
			local _, qq = self:getDisplay()
		end)
		lineQualitySlider:setValue(lineQuality or 0)

		self.displaycontrol:add(
			vslCheckBox,
			boxy.Label("Line Quality"),
			lineQualitySlider)
	end

	self:updateLineUI()
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
		self.rpanel.visible = true
	else
		self.rpanel.visible = false
	end

	boxy.draw()
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
	self.widgetDirty = false
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
	elseif tool == "gradient" then
		local object = self.selection:first()
		self.widget = newGradientTool(
			self, self.colorDabs, self.handles, object)
	elseif tool == "rectangle" then
		self.widget = newRectangleTool(self)
	elseif tool == "circle" then
		self.widget = newEllipseTool(self)
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
	local mode = object:getDisplay()
	self.rendererButtons[mode]:setChecked(true)
	self.lineJoinButtons[path:getLineJoin()]:setChecked(true)
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
					local tool = self.toolbar:current()

					if clickCount == 1 and tool ~= "gradient" then
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

function Editor:mousepressed(gx, gy, button, clickCount)
	if boxy.mousepressed(gx, gy, button, clickCount) then
		self.clicked = nil
		return
	end

	local x, y = self.transform:inverseTransformPoint(gx, gy)

	if button == 1 then
		self.clicked = nil

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
	if boxy.mousereleased(x, y, button, clickCount) then
		--return
	end

	if button == 1 then
		local tool = self.toolbar:current()
		if self.clicked ~= nil and (tool == "cursor" or tool == "gradient")
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
		self:updateWidget()
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
	boxy.update()

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

function Editor:removeObject(object)
	for i, o in ipairs(self.objects) do
		if o == object then
			table.remove(self.objects, i)
			break
		end
	end
end

function Editor:setDisplay(...)
	for _, o in self.selection:objects() do
		o:setDisplay(...)
	end
end

function Editor:getDisplay()
	if not self.selection:empty() then
        return self.selection:first():getDisplay()
    end
end

function Editor:getQuality()
	if not self.selection:empty() then
        return self.selection:first():getQuality()
    end
end

function Editor:setUsage(what, value)
	for _, o in self.selection:objects() do
		o:setUsage(what, value)
	end
end

function Editor:getUsage()
	if not self.selection:empty() then
        return self.selection:first():getUsage()
    end
end

function Editor:newObjectWithPath(mode)
	local object = Object.new(0, 0, tove.newGraphics())
	table.insert(self.objects, object)

    local subpath = tove.newSubpath()
    local path = tove.newPath()
    path:addSubpath(subpath)
    object.graphics:addPath(path)

    local dabs = self.colorDabs
    local path = object.graphics.paths[1]
    local line = dabs:getPaint("line")
    local fill = dabs:getPaint("fill")

	local lineWidth = self.lineWidthSlider:getValue()
	if mode == "line" then
		lineWidth = math.max(lineWidth, 1)
	end
	if lineWidth < 0.1 then
		line = nil
	end

	if mode == "line" then
		fill = nil
	end
    if line == nil and fill == nil then
		if mode == "line" then
			line = tove.newColor(0.5, 0.5, 0.5)
		else
        	fill = tove.newColor(0.5, 0.5, 0.5)
		end
    end

    path:setLineColor(line)
    path:setLineWidth(lineWidth)
    path:setFillColor(fill)

	self.selection:clear()
	self.selection:add(object)
	self:selectionChanged()

	return object
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

function Editor:clear()
	self.transform = love.math.newTransform()
	self:transformChanged()
	self.objects = {}
	self.selection:clear()
	self:selectionChanged()
end

function Editor:restore(chunk)
	self:clear()
	local data = chunk()
	for _, object in ipairs(data.objects) do
		local object = Object.deserialize(object)
		--object:setDisplay("mesh", 100)
		table.insert(self.objects, object)
	end
end

function Editor:loadfile(file)
	self:clear()

	file:open("r")
	local data = file:read()
	file:close()

	if string.sub(data, 1, 1) == "{" then -- lua code?
		local load = loadstring("return " .. data)
		data = load()
		if data.objects then
			self:restore(data)
			return
		end
	end

	local graphics = tove.newGraphics(data, 512)
	graphics:clean()

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
