-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local Transform = {}
Transform.__index = Transform

function Transform:update()
	local sx, sy = self.sx, self.sy

	local x0, y0, x1, y1 = self.graphics:computeAABB()

	local drawtransform = self.drawtransform
	drawtransform:reset()

	drawtransform:translate(self.tx, self.ty)

	local cx = (x0 + x1) / 2
	local cy = (y0 + y1) / 2
	--drawtransform:translate(cx, cy)
	drawtransform:rotate(self.r)
	drawtransform:translate(-self.ox * sx, -self.oy * sy)

	local mousetransform = self.mousetransform
	mousetransform:reset()

	mousetransform:translate(self.tx, self.ty)

	--mousetransform:translate(cx * sx, cy * sy)
	mousetransform:rotate(self.r)

	mousetransform:scale(sx, sy)

	mousetransform:translate(-self.ox, -self.oy)
end

function Transform:inverseTransformPoint(x, y)
	return self.mousetransform:inverseTransformPoint(x, y)
end

function Transform:apply()
end

local function newTransform(graphics, tx, ty)
	local t = setmetatable({
		graphics = graphics,
		tx = tx,
		ty = ty,
		r = 0,
		sx = 1,
		sy = 1,
		ox = 0,
		oy = 0,
		mousetransform = love.math.newTransform(),
		drawtransform = love.math.newTransform()
	}, Transform)
	t:update()
	return t
end

return newTransform
