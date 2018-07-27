local basepath = (...) .. '.'
local cassowary = require (basepath .. "cassowary.cassowary")

local timers = {timers = nil}

function timers.add(t, f)
	if t <= love.timer.getTime() then
		f()
	else
		if timers.timers == nil then
			timers.timers = {}
		end
		timers.timers[f] = t
		return {
			cancel = function()
				if timers.timers then
					timers.timers[f] = nil
				end
			end
		}
	end
end

function timers.update()
	local all = timers.timers
	if all then
		timers.timers = nil
		local now = love.timer.getTime()
		for f, t in pairs(all) do
			if t < now then
				f()
			else
				if timers.timers == nil then
					timers.timers = {}
				end
				timers.timers[f] = t
			end
		end
	end
end



local zero = cassowary.Variable{value = 0}

local defaultStyle = {
	r = 4, -- roundness
	rs = 2, -- roundness segments
	fg = {1, 1, 1},
	bg = {0, 0, 0},
	font = love.graphics.getFont(),
	rectangle = function(self, mode, x, y, w, h)
		local r, rs = self.r, self.rs
		love.graphics.rectangle(mode, x, y, w, h, r, r, rs)
	end
}


local class = require(basepath .. "class")


local Signal = class("Signal")

function Signal:__init()
	self.connected = {}
	self.ignore = false
end

function Signal:emit(...)
	if not self.ignore then
		for _, f in ipairs(self.connected) do
			f(...)
		end
	end
end

function Signal:connect(f)
	table.insert(self.connected, f)
end


local Spacer = class("Spacer")

function Spacer:__init(s, mode)
	self.stretch = s
	self.mode = mode or "relative"
	self.along = "x"
	self.visible = true
end

function Spacer:addConstraints(solver, x, y, w, h)
	if self.mode == "absolute" then
		solver:addConstraint(cassowary.Equation(
			self.along == "x" and w or h,
			self.stretch))
	else
		solver:addConstraint(cassowary.Equation(
			self.along == "x" and w or h,
			cassowary.times(self.layout.stretch, self.stretch)))
	end
end

function Spacer:reparent(widget)
end

function Spacer:mousepressed()
end

function Spacer:draw()
end

function Spacer:updateLayout(solver)
end

function Spacer:getDesiredSize()
	return 0, 0
end

function Spacer:foreach(f)
end



local Layout = class("Layout")

function Layout:__init()
	self.widget = nil
	self.parentLayout = nil
	self.children = {}
	self.dirty = true
	self.margins = {11, 11, 11, 11}
	self.visible = true
end

function Layout:setDirty()
	-- print("setDirty", self.__name)
	if self.parentLayout then
		if not self.parentLayout:setDirty() then
			self.dirty = true
		end
	end
	if self.widget then
		if not self.widget:setDirty() then
			self.dirty = true
		end
	end
	return true
end

function Layout:setMargins(mx, my, mw, mh)
	self.margins = {mx, my, mw, mh}
	return self
end

function Layout:removeMargins()
	self.margins = {0, 0, 0, 0}
	return self
end

function Layout:foreach(f)
	for _, c in ipairs(self.children) do
		f(c.item)
	end
end

function Layout:reparent(widget)
	self.widget = widget
	self:setDirty()
	for _, c in ipairs(self.children) do
		c.item:reparent(widget)
	end
end

function Layout:layout()
	local widget = self.widget
	local solver = cassowary.SimplexSolver()
	local x = cassowary.Variable()
	local y = cassowary.Variable()
	local w = cassowary.Variable()
	local h = cassowary.Variable()
	solver:addConstraint(cassowary.Equation(x, widget.x))
	solver:addConstraint(cassowary.Equation(y, widget.y))
	solver:addConstraint(cassowary.Inequality(w, ">=", widget.w))
	solver:addConstraint(cassowary.Inequality(h, ">=", widget.h))
	self:addConstraints(solver, x, y, w, h)
	solver:solve()
	self:updateLayout(solver)
end

function Layout:draw()
	if self.dirty then
		self:layout()
	end

	for _, c in ipairs(self.children) do
		c.item:draw()
	end
end

function Layout:mousepressed(...)
	if self.dirty then
		self:layout()
	end

	for _, c in ipairs(self.children) do
		local r = c.item:mousepressed(...)
		if r then
			return r
		end
	end
end

function Layout:updateLayout(solver)
	for _, c in ipairs(self.children) do
		local item = c.item
		if item.visible then
			item:updateLayout(solver, c)
		end
	end
	self.dirty = false
end

function Layout:clear()
	self.children = {}
	self:setDirty()
end


local HBox = class("HBox", Layout)

function HBox:__init(items)
	Layout.__init(self)
	self.stretch = cassowary.Variable()
	self.spacing = 8
	for _, item in ipairs(items or {}) do
		self:add(item)
	end
end

function HBox:setMargins(mx, my, mw, mh)
	self.margins = {mx, my, mw, mh}
	return self
end

function HBox:add(...)
	for _, item in ipairs {...} do
		item.parentLayout = self
		if class.is_a(item, Spacer) then
			item.layout = self
			item.along = "x"
		end
		local w = cassowary.Variable()
		table.insert(self.children,
			{item = item, x = nil, y = nil, w = w, h = nil})
	end
	self:setDirty()
end

function HBox:addStretch(s)
	self:addItem(Spacer(s))
end

function HBox:getDesiredSize()
	return 0, 0
end

function HBox:addConstraints(solver, lx, ly, lw, lh)
	local mx, my, mw, mh = unpack(self.margins)

	local h = cassowary.Variable()
	solver:addConstraint(cassowary.Inequality(h, "<=", cassowary.minus(lh, my + mh)))

	local x0 = cassowary.Variable()
	solver:addConstraint(cassowary.Equation(x0, cassowary.plus(lx, mx)))

	local y0 = cassowary.Variable()
	solver:addConstraint(cassowary.Equation(y0, cassowary.plus(ly, my)))

	local x1 = x0

	local n = #self.children
	for i, c in ipairs(self.children) do
		if c.item.visible then
			c.x = x0
			c.y = y0
			c.h = h

			local w = c.w
			c.item:addConstraints(solver, x0, y0, w, h)

			if i < n then
				w = cassowary.plus(w, self.spacing)
			end

			x1 = cassowary.Variable()
			solver:addConstraint(cassowary.Equation(x1, cassowary.plus(x0, w)))

			x0 = x1
		end
	end

	if x1 ~= nil then
		solver:addConstraint(cassowary.Inequality(
			x1, "<=", cassowary.minus(cassowary.plus(lx, lw), mw)))
	end
end




local VBox = class("VBox", Layout)

function VBox:__init(items)
	Layout.__init(self)
	self.stretch = cassowary.Variable()
	self.spacing = 8
	for _, item in ipairs(items or {}) do
		self:add(item)
	end
end

function VBox:add(...)
	for _, item in ipairs {...} do
		item.parentLayout = self
		if class.is_a(item, Spacer) then
			item.layout = self
			item.along = "y"
		end
		local h = cassowary.Variable()
		table.insert(self.children,
			{item = item, x = nil, y = nil, w = nil, h = h})
	end
	self:setDirty()
end

function VBox:addStretch(s)
	self:addItem(Spacer(s))
end

function VBox:getDesiredSize()
	return 0, 0
end

function VBox:addConstraints(solver, lx, ly, lw, lh)
	local mx, my, mw, mh = unpack(self.margins)

	local w = cassowary.Variable()
	solver:addConstraint(cassowary.Inequality(w, "<=", cassowary.minus(lw, mx + mw)))

	local x0 = cassowary.Variable()
	solver:addConstraint(cassowary.Equation(x0, cassowary.plus(lx, mx)))

	local y0 = cassowary.Variable()
	solver:addConstraint(cassowary.Equation(y0, cassowary.plus(ly, my)))

	local y1 = y0

	local n = #self.children
	for i, c in ipairs(self.children) do
		if c.item.visible then
			c.x = x0
			c.y = y0
			c.w = w

			local h = c.h
			c.item:addConstraints(solver, x0, y0, w, h)

			if i < n then
				h = cassowary.plus(h, self.spacing)
			end

			y1 = cassowary.Variable()
			solver:addConstraint(cassowary.Equation(y1, cassowary.plus(y0, h)))

			y0 = y1
		end
	end

	if y1 ~= nil then
		solver:addConstraint(cassowary.Inequality(
			y1, "<=", cassowary.minus(cassowary.plus(ly, lh), mh)))
	end
end




local Widget = class("Widget")

function Widget:__init(parent)
	self.x, self.y, self.w, self.h = 0, 0, 0, 0
	self.visible = true
	self.enabled = true
	self.style = defaultStyle
	self.children = {}
	if parent then
		table.insert(parent.children, self)
	end
end

function Widget:setVisible(visible)
	if visible ~= self.visible then
		self.visible = visible
		self:setDirty(true)
	end
end

function Widget:setEnabled(enabled)
	self.enabled = enabled
end

function Widget:updateLayout(solver, data)
	if not self.visible then
		return
	end
	self.x = data.x.value
	self.y = data.y.value
	self.w = data.w.value
	self.h = data.h.value
	for _, c in ipairs(self.children) do
		c:updateLayout(solver)
	end
end

function Widget:setDirty()
	if self.parentLayout then
		return self.parentLayout:setDirty()
	else
		return false
	end
end

function Widget:inside(mx, my)
	local x, y, w, h = self.x, self.y, self.w, self.h
	return mx >= x and my >= y and mx < x + w and my < y + h
end

function Widget:foreach(f)
	for _, c in ipairs(self.children) do
		f(c)
	end
end

function Widget:move(x, y)
	self.x, self.y = x, y
end

function Widget:resize(w, h)
	self.w, self.h = w, h
end

function Widget:setGeometry(x, y, w, h)
	self.x, self.y, self.w, self.h = x, y, w, h
end

function Widget:getDesiredSize()
	return 0, 0
end

function Widget:addConstraints(solver, x, y, w, h)
	if not self.visible then
		return
	end
	local dw, dh = self:getDesiredSize()
	if dw > 0 then
		solver:addConstraint(cassowary.Inequality(w, ">=", dw))
	end
	if dh > 0 then
		solver:addConstraint(cassowary.Inequality(h, ">=", dh))
	end

	for _, c in ipairs(self.children) do
		c:addConstraints(solver, x, y, w, h)
	end
end

function Widget:reparent(widget)
	self.parent = widget
end

function Widget:add(widget)
	table.insert(self.children, widget)
	widget:reparent(self)
end

function Widget:setLayout(layout)
	self.children = {layout}
	layout:reparent(self)
end

function Widget:draw()
	if not self.visible then
		return
	end
	for _, c in ipairs(self.children) do
		c:draw()
	end
end

function Widget:mousepressed(x, y, ...)
	if not self.visible then
		return
	end
	for _, c in ipairs(self.children) do
		local r = c:mousepressed(x, y, ...)
		if r then
			return r
		end
	end
	if self:inside(x, y) then
		return function() end
	end
end


local Panel = class("Panel", Widget)

function Panel:__init(r, g, b, parent)
	Widget.__init(self, parent)
	self.rgb = {r or 0.2, g or 0.2, b or 0.2}
end

function Panel:draw()
	if not self.visible then
		return
	end
	love.graphics.setColor(unpack(self.rgb))
	love.graphics.rectangle("fill", self.x, self.y, self.w, self.h)
	Widget.draw(self)
end


local GroupBox = class("GroupBox", Widget)

function GroupBox:__init(title, parent)
	Widget.__init(self, parent)
	self.title = title
	self.frame = true
end

function GroupBox:setFrame(frame)
	self.frame = frame
end

function GroupBox:draw()
	if not self.visible then
		return
	end
	if self.frame then
		love.graphics.setColor(unpack(self.style.fg))
		self.style:rectangle("line", self.x, self.y, self.w, self.h)
	end
	Widget.draw(self)
end


local Button = class("Button", Widget)

local function untoggle(root, except)
	root:foreach(function(b)
		if not class.is_a(b, Button) then
			untoggle(b, except)
		elseif b ~= except then
			b.checked = false
		end
	end)
end

function Button:__init(mode, content, scale)
	Widget.__init(self)

	self.active = false
	self.timer = nil
	self.checked = false
	self.mode = mode  -- "push", "toggle", "radio"
	self.valueChanged = Signal()
	self.style = defaultStyle

	if type(content) == "string" then
		self.text = content
	else
		assert(content:typeOf("Image"))
		self.image = content
		self.scale = scale or 1
	end

	self:configure(8)
end

function Button:onClicked(f)
	self.valueChanged:connect(f)
end

function Button:onChecked(f)
	self.valueChanged:connect(function(checked, button)
		if checked then
			f(button)
		end
	end)
end

function Button:configure(padding, size)
	if self.text then
		local font = self.style.font
		local w = font:getWidth(self.text)
		self.extents = {2 * padding + w, 22, w, font:getHeight()}
	else
		local scale = self.scale
		local w = self.image:getWidth() * scale
		local h = self.image:getHeight() * scale
		local s
		if size ~= nil then
			s = size
		else
			s = math.max(w, h)
		end
		self.extents = {s + 2 * padding, s + 2 * padding, w, h}
	end
	self.padding = padding
end

function Button:getDesiredSize()
	local ex, ey = unpack(self.extents)
	return ex, ey
end

function Button:draw()
	if not self.visible then
		return
	end
	local x, y, w, h = self.x, self.y, self.w, self.h
	local checked = self.checked
	local ex, ey, ex0, ey0 = unpack(self.extents)

	if self.active and self.mode ~= "radio" then
		checked = not checked
	end

	love.graphics.setColor(1, 1, 1)
	if self.enabled then
		if checked then
			self.style:rectangle("fill", x, y, w, h)
			love.graphics.setColor(0, 0, 0)
		else
			self.style:rectangle("line", x, y, w, h)
		end
	else
		love.graphics.setColor(0.5, 0.5, 0.5)
		self.style:rectangle("line", x, y, w, h)
	end
	if self.text then
		love.graphics.print(self.text,
			x + w / 2 - ex0 / 2, y + h / 2 - ey0 / 2)
	else
		local s = self.scale
		love.graphics.draw(self.image,
			x + w / 2 - ex0 / 2, y + h / 2 - ey0 / 2, 0, s, s)
	end
end

function Button:setMode(mode)
	self.mode = mode
	return self
end

function Button:isChecked()
	return self.checked
end

function Button:setChecked(checked)
	if self.checked == checked then
		return self
	end
	self.checked = checked
	if self.parent and self.mode == "radio" then
		if class.is_a(self.parent, GroupBox) then
			untoggle(self.parent, self)
		end
	end
	self.valueChanged:emit(checked)
	return self
end

function Button:mousepressed(mx, my)
	if not self.visible or not self.enabled then
		return
	end
	if self:inside(mx, my) then
		if self.timer then
			self.timer:cancel()
			self.timer = nil
		end

		local t = love.timer.getTime() + 0.1
		self.active = true
		return {
			update = function(mx, my)
				self.active = self:inside(mx, my)
			end,
			release = function()
				if self.active then
					if self.mode == "toggle" then
						self:setChecked(not self.checked)
					elseif self.mode == "radio" then
						self:setChecked(true)
					else
						self.valueChanged:emit()
					end
					if self.mode == "push" then
						self.timer = timers.add(t, function()
							self.active = false
						end)
					else
						self.active = false
					end
				end
			end
		}
	end
end


local PushButton = class("PushButton", Button)

function PushButton:__init(...)
	Button.__init(self, "push", ...)
end


local CheckBox = class("CheckBox", Button)

function CheckBox:__init(content)
	Button.__init(self, "toggle", content)
	self.boxsize = 16
	self.extents[1] = self.extents[1] + self.boxsize
	self.extents[2] = self.boxsize
end

function CheckBox:inside(mx, my)
	return Button.inside(self, mx, my) and mx < self.x + self.extents[1]
end

function CheckBox:draw()
	if not self.visible then
		return
	end
	local x, y, w, h = self.x, self.y, self.w, self.h
	local checked = self.checked
	local ex, ey, ex0, ey0 = unpack(self.extents)
	local boxsize = self.boxsize

	if self.active then
		checked = not checked
	end

	love.graphics.setColor(1, 1, 1)
	self.style:rectangle("line", x, y, boxsize, boxsize)
	if checked then
		self.style:rectangle("fill", x + 2, y + 2, boxsize - 4, boxsize - 4)
	else
	end
	if self.text then
		love.graphics.print(self.text, x + boxsize + 8, y + h / 2 - ey0 / 2)
	else
		local s = self.scale
		love.graphics.draw(self.image, x + boxsize + 8, y + h / 2 - ey0 / 2, 0, s, s)
	end
end



local function sqr(x)
	return x * x
end

local function lengthSqr(dx, dy)
	return dx * dx + dy * dy
end

local Slider = class("Slider", Widget)

function Slider:__init(min, max, initial, step, parent)
	Widget.__init(self, parent)

	self.min = min or 0
	self.max = max or 1
	self.value = initial or self.min
	self.step = step or (self.max - self.min) / 1e6

	self.radius = 7
	self.grabbed = false

	self.valueChanged = Signal()
end

function Slider:getDesiredSize()
	return 0, 2 * self.radius
end

function Slider:knob()
	local r = self.radius
	local x, y, w, h = self.x, self.y, self.w, self.h
	local d = (self.value - self.min) / (self.max - self.min)
	return x + r + (w - 2 * r) * d, y + h / 2
end

function Slider:dragTo(mx, my)
	local r = self.radius
	local x, y, w, h = self.x, self.y, self.w, self.h
	local d = (mx - (x + r)) / (w - 2 * r)
	d = math.min(math.max(d, 0), 1) * (self.max - self.min)
	d = math.floor(d / self.step + 0.5) * self.step
	self:setValue(self.min + d)
end

function Slider:draw()
	if not self.visible then
		return
	end
	local r = self.radius
	local x, y, w, h = self.x, self.y, self.w, self.h
	y = y + h / 2
	if self.enabled then
		local style = self.style
		love.graphics.setColor(unpack(style.fg))
		love.graphics.line(x + r, y, x + w - r, y)
		local kx, ky = self:knob()
		local ks = self.radius
		self.style:rectangle("fill", kx - ks, ky - ks, ks * 2, ks * 2)
	else
		love.graphics.setColor(0.5, 0.5, 0.5)
		love.graphics.line(x + r, y, x + w - r, y)
	end
end

function Slider:mousepressed(mx, my)
	if not self.visible or not self.enabled then
		return
	end
	local r = self.radius
	local x, y, w, h = self.x, self.y, self.w, self.h
	y = y + h / 2
	if my >= y - r and my <= y + r and mx >= x and mx <= x + w then
		self.grabbed = true
		local dx = 0
		local kx, ky = self:knob()
		if my >= ky - r and my <= ky + r and mx >= kx - r and mx <= kx + r then
			dx = kx - mx
		end
		self:dragTo(mx + dx, my)
		return {
			update = function(mx, my)
				self:dragTo(mx + dx, my)
			end,
			release = function()
				self.grabbed = false
			end
		}
	end
end

function Slider:setValue(value)
	if self.value ~= value then
		self.value = value
		self.valueChanged:emit(value, self)
	end
end

function Slider:getValue()
	return self.value
end



local Label = class("Label", Widget)

function Label:__init(text, parent)
	Widget.__init(self, parent)
	local font = love.graphics.getFont()
	self.font = font
	self.textHeight = font:getHeight()
	self:setText(text)
	self.alignment = "left"
end

function Label:setAlignment(alignment)
	self.alignment = alignment
end

function Label:setText(text)
	self.text = text
	self.textWidth = self.font:getWidth(self.text)
end

function Label:getDesiredSize()
	return self.textWidth, self.textHeight
end

function Label:draw()
	if not self.visible then
		return
	end
	local x
	if self.alignment == "right" then
		x = self.x + self.w - self.textWidth
	else
		x = self.x
	end
	local style = self.style
	love.graphics.setColor(unpack(style.fg))
	love.graphics.print(self.text, x, self.y)
end

local boxy
local parameterUI = nil

local function addParameterUI(box)
	if not parameterUI then
		local panel = Panel(0.2, 0.2, 0.2)
		parameterUI = VBox()
		local sw, sh = love.graphics.getWidth(), love.graphics.getHeight()
		panel:setGeometry(0, 0, 200, sh)
		panel:setLayout(parameterUI)
		boxy.add(panel)
	end
	parameterUI:add(box)
end

local function addNumericParameter(name, min, max, initial, domain)
	min = min or 0
	max = max or 1
	initial = initial or min
	_G[name] = initial
	local step, format
	if domain == "integer" then
		step = 1
		format = function(x) return string.format("%d", x) end
	else
		step = (max - min) / 1e6
		format = function(x) return string.format("%.1f", x) end
	end
	local slider = Slider(min, max, initial, step)
	local valueLabel = Label(format(slider.value))
	valueLabel:setAlignment("right")
	local box = VBox {
		HBox {
			Label(name),
			Spacer(1),
			valueLabel
		}:setMargins(0, 0, 0, 0),
		slider
	}:setMargins(0, 0, 0, 0)
	slider.valueChanged:connect(function(v)
		valueLabel:setText(format(v))
		_G[name] = v
	end)
	addParameterUI(box)
end

local function addBooleanParameter(name, initial)
	initial = initial or false
	_G[name] = initial
	local checkBox = CheckBox(name):setChecked(initial)
	checkBox.valueChanged:connect(function(checked)
		_G[name] = checked
	end)
	addParameterUI(checkBox)
end

boxy = {
	Signal = Signal,

	HBox = HBox,
	VBox = VBox,
	Spacer = Spacer,

	Widget = Widget,
	Label = Label,
	Slider = Slider,
	PushButton = PushButton,
	CheckBox = CheckBox,
	GroupBox = GroupBox,
	Panel = Panel,

	root = Widget(),
	dragging = nil,

	add = function(w)
		boxy.root:add(w)
	end,
	draw = function()
		boxy.root:draw()
		love.graphics.setColor(1, 1, 1)
	end,
	update = function()
		timers.update()
		local dragging = boxy.dragging
		if dragging then
			local update
			if type(dragging) == "table" then
				update = dragging.update
			else
				update = dragging
			end
			if update then
				update(love.mouse.getX(), love.mouse.getY())
			end
		end
	end,
	mousepressed = function(...)
		boxy.dragging = boxy.root:mousepressed(...)
		return boxy.dragging ~= nil
	end,
	mousereleased = function()
		local dragging = boxy.dragging
		if dragging then
			if type(dragging) == "table" and dragging.release then
				dragging.release()
			end
			boxy.dragging = nil
			return true
		else
			return false
		end
	end,

	setup = function()
		love.draw = boxy.draw
		love.update = boxy.update
		love.mousepressed = boxy.mousepressed
		love.mousereleased = boxy.mousereleased
	end,
	parameter = {
		number = function(name, min, max, initial)
			addNumericParameter(name, min, max, initial, "number")
		end,
		integer = function(name, min, max, initial)
			addNumericParameter(name, min, max, initial, "integer")
		end,
		boolean = function(name, initial)
			addBooleanParameter(name, initial)
		end
	}
}

return boxy
