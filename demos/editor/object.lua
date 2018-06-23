-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local newTransform = require "transform"
local handles = require "handles"

Object = {}
Object.__index = Object

function Object:refresh()
	self.scaledgraphics:set(tove.transformed(
		self.graphics, self.transform:get("sr")))

	local overlayline = self.overlayline
	local handleColor = handles.color
	overlayline:set(self.scaledgraphics)
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
	self.scaledgraphics:draw()
	if self.selected then
		self.overlayline:draw()
	end
	love.graphics.pop()
end

function Object:changePoints(f)
	local graphics = self.graphics
	local transform = self.transform

	local x0, y0, x1, y1
	x0, y0, x1, y1 = graphics:computeAABB()
	--local ox0 = (x0 + x1) / 2
	--local oy0 = (y0 + y1) / 2
	local ox0 = transform.ox
	local oy0 = transform.oy

	f()

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

	transform.tx = tx0 - (ox0 - ox1) * sx * cosr + (oy0 - oy1) * sy * sinr
	transform.ty = ty0 - (oy0 - oy1) * sy * cosr - (ox0 - ox1) * sx * sinr

	transform:update()

	self:refresh()
end

function Object:setDisplay(...)
    self.scaledgraphics:setDisplay(...)
end

function Object:getDisplay()
	local mode, quality = self.scaledgraphics:getDisplay()
	return mode, quality
end

function Object:setUsage(what, value)
	return self.scaledgraphics:setUsage(what, value)
end

function Object:getUsage()
	return self.scaledgraphics:getUsage()
end

Object.new = function(tx, ty, graphics)
	graphics:setDisplay("mesh", 0.5)

	local scaledgraphics = tove.newGraphics()
	scaledgraphics:setDisplay("mesh", 0.5)

	local overlayline = tove.newGraphics()
	overlayline:setDisplay("mesh")
	overlayline:setUsage("points", "dynamic")

	local object = setmetatable({
		graphics = graphics,
		scaledgraphics = scaledgraphics,
		transform = newTransform(tx, ty),
		overlayline = overlayline,
		selected = false
	}, Object)

	object:refresh()

	return object
end

return Object
