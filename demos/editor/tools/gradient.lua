-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local GradientTool = {}
GradientTool.__index = GradientTool

local function lengthSqr(dx, dy)
	return dx * dx + dy * dy
end

local function projectOntoRay(px, py, sx, sy, rx, ry)
	local x, y = px - sx, py - sy
	return x * rx + y * ry
end

function GradientTool:colorPicked(what, type, ...)
    if self.selectedColor >= 1 and type == "solid" then
        self.colors[self.selectedColor].color = {...}
    end
	self:commit() -- might need to change gradient type
end

function GradientTool:commit()
    local object = self.object
    if object == nil or self.x0 == nil then
        return
    end

	local paint
	if #self.colors == 1 then
		paint = tove.newColor(unpack(self.colors[1].color))
	else
	    local transform = object.transform:get("full")
	    local x0, y0 = transform:inverseTransformPoint(self.x0, self.y0)
	    local x1, y1 = transform:inverseTransformPoint(self.x1, self.y1)

		local type = self.dabs:getGradientTypeUI()
		if type == "linear" then
	    	paint = tove.newLinearGradient(x0, y0, x1, y1)
		elseif type == "radial" then
			local dx, dy = x1 - x0, y1 - y0
			local r = math.sqrt(dx * dx + dy * dy)
			paint = tove.newRadialGradient(x0, y0, r, r, r)
		else
			return
		end

		for _, c in ipairs(self.colors) do
		    paint:addColorStop(c.offset, unpack(c.color))
		end
	end

	self.dabs:setActiveColor(paint)
    for i = 1, object.graphics.paths.count do
        local path = object.graphics.paths[i]
        if self.dabs.active == "fill" then
            path:setFillColor(paint)
        end
    end
    self.object:refresh()
end

function GradientTool:addColor(t)
	local colors = self.colors
	local j = 0
	for i = 2, #colors do
		if colors[i].offset > t then
			j = i
			break
		end
	end
	local r0, g0, b0, a0 = unpack(colors[j - 1].color)
	local r1, g1, b1, a1 = unpack(colors[j].color)
	local t0 = colors[j - 1].offset
	local dt = colors[j].offset - t0
	local b = (t - t0) / dt
	local a = 1 - b
	table.insert(colors, j, {
		offset = t,
		color = {
			r0 * a + r1 * b,
			g0 * a + g1 * b,
			b0 * a + b1 * b,
			a0 * a + a1 * b
		}})
	self:commit()

	self.selectedColor = j
end

function GradientTool:draw(gtransform)
	local x0, y0, x1, y1 = self.x0, self.y0, self.x1, self.y1

    if x0 == nil or #self.colors < 2 then
        return
    end

	x0, y0 = gtransform:transformPoint(x0, y0)
	x1, y1 = gtransform:transformPoint(x1, y1)

    love.graphics.setColor(0.2, 0.2, 0.2)
    love.graphics.line(x0, y0, x1, y1)

	for i, c in ipairs(self.colors) do
		local t = c.offset
		local s = 1 - t

		local x = x0 * s + x1 * t
		local y = y0 * s + y1 * t

		love.graphics.setColor(1, 1, 1)
	    if self.selectedColor == i then
	        love.graphics.setColor(0, 0, 0)
	        self.handles.selected:draw(x, y, 0, 1.5, 1.5)
	    else
	        love.graphics.setColor(1, 1, 1)
	        self.handles.normal:draw(x, y)
	    end

		love.graphics.setColor(unpack(c.color))
	    self.handles.normal:draw(x, y)
	end
end

function GradientTool:mousedown(gx, gy, gs, button)
    local clickRadiusSqr = self.handles.clickRadiusSqr * gs * gs
	local x0, y0, x1, y1 = self.x0, self.y0, self.x1, self.y1

    if x0 ~= nil and #self.colors >= 2 then
		local dx, dy = x1 - x0, y1 - y0
		local d = math.sqrt(dx * dx + dy * dy)
		dx = dx / d
		dy = dy / d

		local colors = self.colors
		local best = -1

		for i, c in ipairs(colors) do
			local t = c.offset
			local s = 1 - t
			local x = x0 * s + x1 * t
			local y = y0 * s + y1 * t

			if lengthSqr(gx - x, gy - y) < clickRadiusSqr then
				self.selectedColor = i
				if i == 1 then
					best = 1
				elseif i == #self.colors then
					return function(x, y)
		                self.x1 = x
		                self.y1 = y
		                self:commit()
		            end
				else
					return function(x, y)
						local t = projectOntoRay(
							x, y, x0, y0, dx, dy) / d
						t = math.max(t, colors[i - 1].offset)
						t = math.min(t, colors[i + 1].offset)
						colors[i].offset = t
		                self:commit()
		            end
				end
			end
		end

		if best == 1 then
			if self.dabs:getActivePaint():getType() == "radial" then
				local dx, dy = x1 - x0, y1 - y0
				return function(x, y)
					self.x0 = x
					self.y0 = y
					self.x1 = x + dx
					self.y1 = y + dy
					self:commit()
				end
			else
				return function(x, y)
					self.x0 = x
					self.y0 = y
					self:commit()
				end
			end
		end

		local t = projectOntoRay(
			gx, gy, x0, y0, dx, dy)
		if t > 0 and t < d then
			local px, py = x0 + dx * t, y0 + dy * t
			if lengthSqr(gx - px, gy - py) < clickRadiusSqr then
				self:addColor(t / d)
				return function() end
			end
		end

		return nil
    end

    if self.object == nil then
        return nil
    end

	self.dabs:setGradientTypeUI("linear")
    self.x0 = gx
    self.y0 = gy
    self.x1 = gx
    self.y1 = gy
    self.selectedColor = 1
    return function(x, y)
        self.x1 = x
        self.y1 = y
        self:commit()
    end
end

function GradientTool:click(gx, gy, gs, button)
end

function GradientTool:mousereleased()
end

function GradientTool:keypressed(key)
	if key == "backspace" and self.selectedColor >= 1 then
		table.remove(self.colors, self.selectedColor)
		self.selectedColor = 0
		self:commit()
		return true
	end

    return false
end

return function(editor, dabs, handles, object)
    local self = setmetatable({
        editor = editor,
        dabs = dabs,
        handles = handles,
        object = object,
        selectedColor = 0,
        colors = nil,
		x0 = nil,
        y0 = nil}, GradientTool)

    local paint
    local colors = {}

    if object then
        if dabs.active == "fill" then
            paint = object.graphics.paths[1]:getFillColor()
        else
            paint = object.graphics.paths[1]:getLineColor()
        end
    else
        paint = dabs:getActivePaint()
    end

    local n = 0
    if paint then
        local type = paint:getType()
        if type == "linear" then
            local x0, y0, x1, y1 = paint:getGradientParameters()
            local transform = object.transform:get("full")
            self.x0, self.y0 = transform:transformPoint(x0, y0)
            self.x1, self.y1 = transform:transformPoint(x1, y1)
		elseif type == "radial" then
			local x0, y0, fx, fy, r = paint:getGradientParameters()
            local transform = object.transform:get("full")
			self.x0, self.y0 = transform:transformPoint(x0, y0)
			local d = 1 / math.sqrt(2)
			local x1, y1 = x0 + d * r, y0 + d * r
			self.x1, self.y1 = transform:transformPoint(x1, y1)
        end

        n = paint:getNumColors()
        for i = 1, n do
            local c = paint:getColorStop(i)
            table.insert(colors, {
                offset = c.offset,
                color = {c.rgba.r, c.rgba.g, c.rgba.b, c.rgba.a}
            })
        end
    end

    for i = n + 1, 2 do
        table.insert(colors, {
            offset = i - 1,
            color = {1, 1, 1, 1}
        })
    end

    self.colors = colors
	return self
end
