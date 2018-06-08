-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local newTransform = require "transform"

Object = {}
Object.__index = Object

function Object:refresh()
	self.scaledgraphics:set(tove.transformed(
		self.graphics, 0, 0, 0, self.transform.sx, self.transform.sy))
end

function Object:draw()
	love.graphics.push("transform")
	love.graphics.applyTransform(self.transform.drawtransform)
	self.scaledgraphics:draw()
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

	transform.tx = tx0 - (ox0 - ox1) * sx* cosr + (oy0 - oy1) * sy * sinr
	transform.ty = ty0 - (oy0 - oy1) * sy * cosr - (ox0 - ox1) * sx * sinr

	transform:update()

	self:refresh()
end

function Object:setDisplay(mode)
    self.scaledgraphics:setDisplay(mode)
end

local function newObject(tx, ty, graphics)
	graphics:setDisplay("mesh")

	local scaledgraphics = tove.newGraphics()
	scaledgraphics:setDisplay("mesh")

	-- we use adaptive tesselation for meshes, since only they support
	-- proper fill rules; for this we need a resolution of at least 2,
	-- otherwise the strokes will look very jaggy.
	scaledgraphics:setResolution(2)

	local object = setmetatable({
		graphics = graphics,
		scaledgraphics = scaledgraphics,
		transform = newTransform(graphics, tx, ty)
	}, Object)

	object:refresh()

	return object
end

return newObject
