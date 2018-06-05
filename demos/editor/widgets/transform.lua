-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local matrix = require "../matrix"

local TransformWidget = {}
TransformWidget.__index = TransformWidget

local function lengthSqr(dx, dy)
	return dx * dx + dy * dy
end

local function rotationHandlePosition(x00, y00, x10, y10, sx, sy)
	local phi = math.atan2(y10 - y00, x10 - x00) - math.pi / 2
	if sx * sy < 0 then
		phi = phi + math.pi
	end
	local mx, my = (x00 + x10) / 2, (y00 + y10) / 2
	local toplen = 30
	return mx + toplen * math.cos(phi), my + toplen * math.sin(phi), mx, my
end

function TransformWidget:draw()
	local lx0, ly0, lx1, ly1 = self.object.graphics:computeAABB()

	local transform = self.object.transform

	local tfm = transform.mousetransform
	local x00, y00 = tfm:transformPoint(lx0, ly0)
	local x10, y10 = tfm:transformPoint(lx1, ly0)
	local x11, y11 = tfm:transformPoint(lx1, ly1)
	local x01, y01 = tfm:transformPoint(lx0, ly1)

	love.graphics.setColor(1, 1, 1)
	love.graphics.line(x00, y00, x10, y10)
	love.graphics.line(x10, y10, x11, y11)
	love.graphics.line(x11, y11, x01, y01)
	love.graphics.line(x01, y01, x00, y00)

	local tx, ty, mx, my = rotationHandlePosition(x00, y00, x10, y10, transform.sx, transform.sy)
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

local function transformFromMapping(px1, py1, qx1, qy1, px2, py2, qx2, qy2, px3, py3, qx3, qy3)
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

local function printmatrix(name, ...)
	local s = name .. ": "
	for _, x in ipairs({...}) do
		s = s .. "   " .. string.format("%.1f", x)
	end
	print(s)
end

local function transformUpdater(tfm)
	local r = tfm.r
	local ox = tfm.ox
	local oy = tfm.oy

	return function(...)
		local a, b, c, d, e, f = transformFromMapping(...)

		--local sx_abs = math.sqrt(a * a + d * d)
		local r2 = math.atan2(d, a)

		local sinr = math.sin(r2)
		local cosr = math.cos(r2)

		-- a = sx Cos[r]
		-- e = sy Cos[r]

		local sx = a / cosr
		local sy = e / cosr

		-- c = tx - ox sx Cos[r] + oy sy Sin[r]
		-- f = ty - oy sy Cos[r] - ox sx Sin[r]

		local tx = c + ox * sx * cosr - oy * sy * sinr
		local ty = f + oy * sy * cosr + ox * sx * sinr

		if math.abs(math.abs(r - r2) - math.pi) < 0.01 then
			-- we don't want a 180 degree rotation change. convert
			-- it to an equivalent change in scale.
			sx = -sx
			sy = -sy
		end

		tfm.tx = tx
		tfm.ty = ty
		tfm.sx = sx
		tfm.sy = sy

		--print(sx, sy)

		tfm:update()

		-- a b c
		-- d e f

		--print(gtransform.tx, gtransform.ty, gtransform.r, gtransform.sx, gtransform.sy, gtransform.ox, gtransform.oy)
		--printmatrix("m1", a, b, 0, c, d, e, 0, f, 0, 0, 0, 1)
		--printmatrix("m2", gtransform.mousetransform:getMatrix())

		--local t = {...}
		--print("verify:", t[3], t[4],
		--	a * t[1] + b * t[2] + c,
		--	d * t[1] + e * t[2] + f)
	end
end

local function distanceLinePoint(x0, y0, x1, y1, x2, y2)
	-- https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
	return -((y2 - y1) * x0 - (x2 - x1) * y0 + x2 * y1 - y2 * x1) /
		math.sqrt(math.pow(y2 - y1, 2) + math.pow(x2 - x1, 2))
end

local function moduloarray(a)
	local n = #a
	return setmetatable(a, {
		__index = function(self, i)
			return self[((i - 1) % n + n) % n + 1]
		end
	})
end

function TransformWidget:rotwidget(lcorners, gcorners, gx, gy)
	local clickRadiusSqr = self.handles.clickRadiusSqr
	local transform = self.object.transform

	local x00, y00 = unpack(gcorners[1])
	local x10, y10 = unpack(gcorners[2])

	local tx, ty = rotationHandlePosition(x00, y00, x10, y10, transform.sx, transform.sy)
	if lengthSqr(tx - gx, ty - gy) < clickRadiusSqr then
		local x11, y11 = unpack(gcorners[3])

		local cx = (x00 + x11) / 2
		local cy = (y00 + y11) / 2

		local phi0 = transform.r - math.atan2(gy - cy, gx - cx)
		return function(gx, gy)
			local phi = math.atan2(gy - cy, gx - cx) + phi0
			transform.r = phi
			transform:update()
		end
	end

	return nil
end

function TransformWidget:midwidgets(lcorners, gcorners, gx, gy)
	local clickRadiusSqr = self.handles.clickRadiusSqr

	for i = 1, 4 do
		local x0, y0 = unpack(gcorners[i])
		local x1, y1 = unpack(gcorners[i + 1])

		if lengthSqr((x0 + x1) / 2 - gx, (y0 + y1) / 2 - gy) < clickRadiusSqr then
			local transform = self.object.transform

			local lx0, ly0 = unpack(lcorners[i])
			local lx1, ly1 = unpack(lcorners[i + 1])

			local opposite_x0, opposite_y0 = unpack(gcorners[i + 3])
			local opposite_x1, opposite_y1 = unpack(gcorners[i + 2])

			local opposite_lx0, opposite_ly0 = unpack(lcorners[i + 3])
			local opposite_lx1, opposite_ly1 = unpack(lcorners[i + 2])

			local r = transform.r
			local upx = math.cos(r + i * math.pi / 2)
			local upy = math.sin(r + i * math.pi / 2)

			local update = transformUpdater(transform)

			return function(gx, gy)
				local l = distanceLinePoint(gx, gy,
					opposite_x0, opposite_y0, opposite_x0 + upy, opposite_y0 - upx)

				update(
					opposite_lx0, opposite_ly0, opposite_x0, opposite_y0,
					opposite_lx1, opposite_ly1, opposite_x1, opposite_y1,
					(lx0 + lx1) / 2, (ly0 + ly1) / 2,
						(opposite_x0 + opposite_x1) / 2 + upx * l, (opposite_y0 + opposite_y1) / 2 + upy * l)

				self.object:refresh()
			end
		end
	end

	return nil
end

function TransformWidget:cornerwidgets(lcorners, gcorners, gx, gy)
	local clickRadiusSqr = self.handles.clickRadiusSqr

	for i = 1, 4 do
		local x0, y0 = unpack(gcorners[i])

		if lengthSqr(x0 - gx, y0- gy) < clickRadiusSqr then
			local transform = self.object.transform
			local lx0, ly0 = unpack(lcorners[i])

			local opposite_x0, opposite_y0 = unpack(gcorners[i + 2])
			local opposite_lx0, opposite_ly0 = unpack(lcorners[i + 2])

			local edge_x0, edge_y0 = opposite_x0, opposite_y0
			--local edge_x1, edge_y1 = unpack(gcorners[i + 1])

			local edge_lx1, edge_ly1 = unpack(lcorners[i + 1])

			local r = transform.r
			local upx = math.cos(r + (i - 1) * math.pi / 2)
			local upy = math.sin(r + (i - 1) * math.pi / 2)

			local update = transformUpdater(transform)

			return function(gx, gy)
				local l = distanceLinePoint(gx, gy, edge_x0, edge_y0, edge_x0 - upy, edge_y0 + upx)

				update(
					edge_lx1, edge_ly1, gx + upx * l, gy + upy * l,
					opposite_lx0, opposite_ly0, opposite_x0, opposite_y0,
					lx0, ly0, gx, gy)

				self.object:refresh()
			end
		end
	end

	return nil
end

function TransformWidget:mousedown(gx, gy, button)
	local transform = self.object.transform
	local lx, ly = transform:inverseTransformPoint(gx, gy)

	local lx0, ly0, lx1, ly1 = self.object.graphics:computeAABB()

	local tfm = transform.mousetransform
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
	for _, w in ipairs {self.rotwidget, self.midwidgets, self.cornerwidgets} do
		dragfunc = w(self, lcorners, gcorners, gx, gy)
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
end

return function(handles, object)
	return setmetatable({handles = handles, object = object}, TransformWidget)
end
