-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local newTransform = require "transform"
local handles = require "handles"

Object = {}
Object.__index = Object

function Object:transformChanged(scaleChanged)
	self.transform:update()
	if scaleChanged then
		self:refresh()
	end
	self._aabb = nil
end

function Object:refresh()
	self.scaledGraphics:set(tove.transformed(
		self.graphics, self.transform:get("stretch")))

	local overlayline = self.overlayline
	local handleColor = handles.color
	overlayline:set(self.scaledGraphics)
	for i = 1, overlayline.paths.count do
		local p = overlayline.paths[i]
		p:setFillColor()
		p:setLineColor(unpack(handleColor))
		p:setLineWidth(1)
	end
end

function Object:draw()
	love.graphics.push("transform")
	love.graphics.applyTransform(self.transform:get("draw"))
	self.scaledGraphics:draw()
	if self.selected then
		self.overlayline:draw()
	end
	love.graphics.pop()
end

function Object:computeAABB()
	if self._aabb == nil then
		local g = self.aaabGraphics
		local t = self.transform
		g:set(tove.transformed(
			self.graphics, t:get("full")), true)
		self._aabb = {g:computeAABB("high")}
	end
	return unpack(self._aabb)
end

function Object:changePoints(f, ...)
	local graphics = self.graphics
	local transform = self.transform

	local x0, y0, x1, y1
	x0, y0, x1, y1 = graphics:computeAABB()
	--local ox0 = (x0 + x1) / 2
	--local oy0 = (y0 + y1) / 2
	local ox0 = transform.ox
	local oy0 = transform.oy

	f(...)

	x0, y0, x1, y1 = graphics:computeAABB()
	local ox1 = (x0 + x1) / 2
	local oy1 = (y0 + y1) / 2

	transform.ox = ox1
	transform.oy = oy1

	local tx0 = transform.tx
	local ty0 = transform.ty
	local r = transform.r
	local cosr = math.cos(r)
	local sinr = math.sin(r)

	local sx, sy = transform.sx, transform.sy
	local a, b, _, _, c, d, _, _ = transform:get("rest"):getMatrix()

	transform.tx = tx0 +
		(a*(-ox0 + ox1) + b*(-oy0 + oy1))*sx*cosr +
		(c*(ox0 - ox1) + d*(oy0 - oy1))*sy*sinr
	transform.ty = ty0 +
		(c*(-ox0 + ox1) + d*(-oy0 + oy1))*sy*cosr +
		(a*(-ox0 + ox1) + b*(-oy0 + oy1))*sx*sinr

	transform:update()
	self._aabb = nil

	self:refresh()
end

function Object:setDisplay(...)
    self.scaledGraphics:setDisplay(...)
end

function Object:getDisplay()
	return self.scaledGraphics:getDisplay()
end

function Object:getQuality()
	return self.scaledGraphics:getQuality()
end

function Object:setUsage(what, value)
	return self.scaledGraphics:setUsage(what, value)
end

function Object:getUsage()
	return self.scaledGraphics:getUsage()
end

function Object:serialize()
	return {
		graphics = self.graphics:serialize(),
		transform = self.transform:serialize()
	}
end

local function newObject(graphics, transform)
	local scaledGraphics = tove.newGraphics()
	scaledGraphics:setUsage(graphics)

	local overlayline = tove.newGraphics()
	overlayline:setDisplay("mesh")
	overlayline:setUsage("points", "dynamic")

	local object = setmetatable({
		graphics = graphics,
		scaledGraphics = scaledGraphics,
		aaabGraphics = tove.newGraphics(),
		_aabb = nil,
		transform = transform,
		overlayline = overlayline,
		selected = false
	}, Object)

	object:refresh()
	return object
end

Object.deserialize = function(t)
	return newObject(
		tove.newGraphics(t.graphics, "none"),
		newTransform(t.transform))
end

Object.new = function(tx, ty, graphics)
	return newObject(
		graphics,
		newTransform(tx, ty))
end

return Object
