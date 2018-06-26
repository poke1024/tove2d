-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local matrix = require "../matrix"

local TransformWidget = {}
TransformWidget.__index = TransformWidget

local function lengthSqr(dx, dy)
	return dx * dx + dy * dy
end

local function rotationHandlePosition(x00, y00, x10, y10, x11, y11, gs)
	local phi = math.atan2(y11 - y10, x11 - x10) + math.pi
	local mx, my = (x00 + x10) / 2, (y00 + y10) / 2
	local toplen = 30 * gs
	return mx + toplen * math.cos(phi), my + toplen * math.sin(phi), mx, my
end

function TransformWidget:draw(gtransform)
	local lx0, ly0, lx1, ly1 = self.selection:computeAABB()
	local transform = self.selection:getTransform()

	local tfm = love.math.newTransform()
	tfm:apply(gtransform)
	tfm:apply(transform:get("full"))

	local x00, y00 = tfm:transformPoint(lx0, ly0)
	local x10, y10 = tfm:transformPoint(lx1, ly0)
	local x11, y11 = tfm:transformPoint(lx1, ly1)
	local x01, y01 = tfm:transformPoint(lx0, ly1)

	love.graphics.setColor(1, 1, 1)
	love.graphics.line(x00, y00, x10, y10)
	love.graphics.line(x10, y10, x11, y11)
	love.graphics.line(x11, y11, x01, y01)
	love.graphics.line(x01, y01, x00, y00)

	local tx, ty, mx, my = rotationHandlePosition(
		x00, y00, x10, y10, x11, y11, 1)
	love.graphics.line(mx, my, tx, ty)

	local handle = self.handles.selected
	handle:draw(x00, y00)
	handle:draw(x10, y10)
	handle:draw(x11, y11)
	handle:draw(x01, y01)

	handle:draw((x00 + x10) / 2, (y00 + y10) / 2)
	handle:draw((x10 + x11) / 2, (y10 + y11) / 2)
	handle:draw((x11 + x01) / 2, (y11 + y01) / 2)
	handle:draw((x01 + x00) / 2, (y01 + y00) / 2)

	self.handles.normal:draw(tx, ty)
end

local function getGaussResults(mtx)
	local cols = #mtx[1]
	local tres = {}
	for i = 1, #mtx do
		tres[i] = mtx[i][cols]
	end
	return unpack(tres)
end

local function transformFromMapping(
	px1, py1, qx1, qy1, px2, py2, qx2, qy2, px3, py3, qx3, qy3)

	local m

	m = {
		{px1, py1, 1, qx1},
		{px2, py2, 1, qx2},
		{px3, py3, 1, qx3}
	}
	matrix.dogauss(m)
	local a, b, c = getGaussResults(m)

	m = {
		{px1, py1, 1, qy1},
		{px2, py2, 1, qy2},
		{px3, py3, 1, qy3}
	}
	matrix.dogauss(m)
	local d, e, f = getGaussResults(m)

	--print(a, b, c, d, e, f)

	--local sx = math.sqrt(a * a + d * d)
	--local sy = math.sqrt(b * b + e * e)

	return a, b, c, d, e, f
end

local function printMatrix(name, ...)
	local s = name .. ": "
	for _, x in ipairs({...}) do
		s = s .. "   " .. string.format("%.1f", x)
	end
	print(s)
end

local function newTransformUpdater(selection)
	local phi0, theta0, ox, oy
	do
		local tfm = selection:getTransform()
		phi0, theta0, ox, oy = tfm.r, tfm.k, tfm.ox, tfm.oy
	end

	local change = function(tfm, ...)
		local a, b, c, d, e, f = transformFromMapping(...)
		tfm:setMatrix(a, b, c, d, e, f)
		return true -- scale changed
	end

	return function(...)
		selection:transform(change, ...)
	end
end

local function rayray(sx1, sy1, rx1, ry1, sx2, sy2, rx2, ry2)
	local t = -((-(ry2*sx1) + ry2*sx2 + rx2*sy1 - rx2*sy2)/(rx2*ry1 - rx1*ry2))
	return sx1 + t * rx1, sy1 + t * ry1
end

local function moduloarray(a)
	local n = #a
	return setmetatable(a, {
		__index = function(self, i)
			return self[((i - 1) % n + n) % n + 1]
		end
	})
end

function TransformWidget:rotwidget(lcorners, gcorners, gx, gy, gs)
	local clickRadiusSqr = self.handles.clickRadiusSqr * gs * gs

	local x00, y00 = unpack(gcorners[1])
	local x10, y10 = unpack(gcorners[2])
	local x11, y11 = unpack(gcorners[3])

	local tx, ty = rotationHandlePosition(
		x00, y00, x10, y10, x11, y11, gs)

	if lengthSqr(tx - gx, ty - gy) < clickRadiusSqr then
		local cx = (x00 + x11) / 2
		local cy = (y00 + y11) / 2

		local transform = self.selection:getTransform()
		local phi0 = transform.r - math.atan2(gy - cy, gx - cx)

		local rotate = function(transform, phi)
			transform.r = phi
			return false
		end

		return function(gx, gy)
			local phi = math.atan2(gy - cy, gx - cx) + phi0
			self.selection:transform(rotate, phi)
		end
	end

	return nil
end

local function normalize(x, y)
	local d = math.sqrt(x * x + y * y)
	return x / d, y / d
end

function TransformWidget:midwidgets(lcorners, gcorners, gx, gy, gs)
	local clickRadiusSqr = self.handles.clickRadiusSqr * gs * gs

	for i = 1, 4 do
		local x0, y0 = unpack(gcorners[i])
		local x1, y1 = unpack(gcorners[i + 1])

		if lengthSqr((x0 + x1) / 2 - gx, (y0 + y1) / 2 - gy) < clickRadiusSqr then
			local transform = self.selection:getTransform()

			local lx0, ly0 = unpack(lcorners[i])
			local lx1, ly1 = unpack(lcorners[i + 1])

			local opposite_x0, opposite_y0 = unpack(gcorners[i + 3])
			local opposite_x1, opposite_y1 = unpack(gcorners[i + 2])
			local opposite_x2, opposite_y2 = unpack(gcorners[i + 1])

			local opposite_lx0, opposite_ly0 = unpack(lcorners[i + 3])
			local opposite_lx1, opposite_ly1 = unpack(lcorners[i + 2])

			local r = transform.r
			local upx = opposite_x2 - opposite_x1
			local upy = opposite_y2 - opposite_y1
			upx, upy = normalize(upx, upy)

			local update = newTransformUpdater(self.selection)

			return function(gx, gy)
				local mx = (opposite_x0 + opposite_x1) / 2
				local my = (opposite_y0 + opposite_y1) / 2
				local l = (gx - mx) * upx + (gy - my) * upy

				update(
					opposite_lx0, opposite_ly0,
						opposite_x0, opposite_y0,
					opposite_lx1, opposite_ly1,
						opposite_x1, opposite_y1,
					(lx0 + lx1) / 2, (ly0 + ly1) / 2,
						mx + upx * l,
						my + upy * l)
			end
		end
	end

	return nil
end

function TransformWidget:cornerwidgets(lcorners, gcorners, gx, gy, gs)
	local clickRadiusSqr = self.handles.clickRadiusSqr * gs * gs

	for i = 1, 4 do
		local x0, y0 = unpack(gcorners[i])

		if lengthSqr(x0 - gx, y0- gy) < clickRadiusSqr then
			local transform = self.selection:getTransform()

			local corner_lx, corner_ly = unpack(lcorners[i + 0])
			local corner_x, corner_y = unpack(gcorners[i + 0])

			local opposite_x, opposite_y = unpack(gcorners[i + 2])
			local opposite_lx, opposite_ly = unpack(lcorners[i + 2])

			local between_x, between_y = unpack(gcorners[i + 1])
			local between_lx, between_ly = unpack(lcorners[i + 1])

			local r = transform.r

			local ux = between_x - opposite_x
			local uy = between_y - opposite_y
			ux, uy = normalize(ux, uy)

			local vx = between_x - corner_x
			local vy = between_y - corner_y
			vx, vy = normalize(vx, vy)

			local update = newTransformUpdater(self.selection)

			return function(gx, gy)
				local px, py = rayray(
					opposite_x, opposite_y, ux, uy,
					gx, gy, vx, vy)

				update(
					between_lx, between_ly,
						px, py,
					opposite_lx, opposite_ly,
						opposite_x, opposite_y,
					corner_lx, corner_ly,
						gx, gy)
			end
		end
	end

	return nil
end

function TransformWidget:mousedown(gx, gy, gs, button)
	local transform = self.selection:getTransform()
	local lx, ly = transform:inverseTransformPoint(gx, gy)

	local lx0, ly0, lx1, ly1 = self.selection:computeAABB()

	local tfm = transform:get("full")
	local x00, y00 = tfm:transformPoint(lx0, ly0)
	local x10, y10 = tfm:transformPoint(lx1, ly0)
	local x11, y11 = tfm:transformPoint(lx1, ly1)
	local x01, y01 = tfm:transformPoint(lx0, ly1)

	local lcorners = moduloarray {
		{lx0, ly0},
		{lx1, ly0},
		{lx1, ly1},
		{lx0, ly1}
	}

	local gcorners = moduloarray {
		{x00, y00},
		{x10, y10},
		{x11, y11},
		{x01, y01}
	}

	local dragfunc = nil
	for _, w in ipairs(self.widgets) do
		dragfunc = w(self, lcorners, gcorners, gx, gy, gs)
		if dragfunc ~= nil then
			break
		end
	end

	return dragfunc
end

function TransformWidget:click(x, y)
end

function TransformWidget:mousereleased()
end

function TransformWidget:keypressed(key)
	return false
end

return function(handles, selection)
	return setmetatable({
		handles = handles,
		selection = selection,
		widgets = {
			TransformWidget.rotwidget,
			TransformWidget.midwidgets,
			TransformWidget.cornerwidgets}}, TransformWidget)
end
