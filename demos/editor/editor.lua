-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local newTransformWidget = require "widgets/transform"
local newCurvesWidget = require "widgets/curves"
local ColorPanel = require "ColorPanel"

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

function Editor:load()
	local editor = self

	self.colorpanel = ColorPanel.new(
		love.graphics.getWidth() - 75,
		8,
		function(what, r, g, b)
			local widget = editor.widget
			if widget ~= nil then
	            local object = widget.object
				for i = 1, object.graphics.npaths do
					local path = object.graphics.paths[i]
					if what == "fill" then
						path:setFillColor(r, g, b)
					else
						path:setLineColor(r, g, b)
					end
				end
				object:refresh()
			end
		end)
end

function Editor:draw()
	for _, object in ipairs(self.objects) do
		object:draw()
	end

	if self.widget ~= nil then
		self.widget:draw()
	end

	self.colorpanel:draw()
end

function Editor:startdrag(x, y, func)
    if func == nil then
        return false
    else
        self.drag = {
            startx = x,
            starty = y,
            active = false,
            func = func
        }
        return true
    end
end

function Editor:mousedown(x, y, button, clickCount)
	if button == 1 then
		if self:startdrag(x, y, self.colorpanel:click(x, y)) then
			return
		end

		if self.widget ~= nil then
			if not self:startdrag(x, y, self.widget:mousedown(x, y, button)) then
				self.widget = nil
				self.colorpanel.visible = false
			end
		end
		if self.widget == nil then
			for _, object in ipairs(self.objects) do
				local graphics = object.graphics
				local lx, ly = object.transform:inverseTransformPoint(x, y)

				for i = 1, graphics.npaths do
					if graphics.paths[i]:inside(lx, ly) then
						if clickCount == 2 then
							self.widget = newCurvesWidget(self.handles, object)
						elseif clickCount == 1 then
							self.widget = newTransformWidget(self.handles, object)
						end
						--local r, g, b =
						--self.colorwheel:setRGBColor(r, g, b)

						local path = object.graphics.paths[1]
						self.colorpanel.visible = true
						self.colorpanel:setLineColor(unpack(path:getLineColor().rgba))
						self.colorpanel:setFillColor(unpack(path:getFillColor().rgba))

						self:startdrag(x, y, makeDragObjectFunc(object, x, y))
						return
					end
				end
			end
		end
	end
end

function Editor:mousereleased(x, y, button)
	if button == 1 then
		if self.widget ~= nil then
            local drag = self.drag
            if drag ~= nil and not drag.active then
                self.widget:click(x, y, button)
            end
			self.widget:mousereleased(x, y, button)
		end
		self.drag = nil
	end
end

function Editor:keypressed(key, scancode, isrepeat)
    if self.widget ~= nil then
        self.widget:keypressed(key, scancode, isrepeat)
    end
end

function Editor:update()
    local drag = self.drag
	if drag ~= nil then
        local x = love.mouse.getX()
		local y = love.mouse.getY()

        if not drag.active then
            local dx = x - drag.startx
            local dy = y - drag.starty
            if math.sqrt(dx * dx + dy * dy) < 2 then
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

local editor = setmetatable({
	handles = require "handles",
	objects = {},
	widget = nil,
	drag = nil
}, Editor)

return editor
