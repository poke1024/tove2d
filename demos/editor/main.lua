-- TÖVE Demo.
-- (C) 2018 Bernhard Liebl, MIT license.

require "lib/tove"

local createColorWheel = require "colorwheel"

-- msaa = 4
love.window.setMode(800, 600, {highdpi = true})

local screenwidth = love.graphics.getWidth()
local screenheight = love.graphics.getHeight()

graphics = tove.newGraphics()
graphics:drawCircle(screenwidth / 2, screenheight / 2, screenwidth / 4)
graphics:setFillColor(0.8, 0.1, 0.1)
graphics:fill()
graphics:setLineColor(0, 0, 0)
graphics:stroke()

graphics:setDisplay("mesh")
graphics:setUsage("points", "dynamic")

local handleRadius = 5
local handleColor = {0.4, 0.4, 0.8}
local handleClickRadiusSqr = math.pow(handleRadius * 1.5, 2)

local function createHandle(selected)
	local handle = tove.newGraphics()
	handle:drawCircle(0, 0, handleRadius)
	if selected then
		handle:setFillColor(unpack(handleColor))
		handle:fill()
		handle:setLineColor(1, 1, 1)
		handle:setLineWidth(1)
		handle:stroke()
	else
		handle:setFillColor(1, 1, 1)
		handle:fill()
		handle:setLineColor(unpack(handleColor))
		handle:setLineWidth(1)
		handle:stroke()
	end
	handle:setDisplay("bitmap")
	handle:setResolution(2)
	return handle
end

local handles = {
	normal = createHandle(false),
	selected = createHandle(true)
}

local overlayLine

function updateOverlayLine()
	overlayLine = graphics:clone()
	overlayLine:setUsage("points", "dynamic")
	for i = 1, overlayLine.npaths do
		local p = overlayLine.paths[i]
		p:setFillColor()
		p:setLineColor(unpack(handleColor))
		p:setLineWidth(1)
	end
end
updateOverlayLine()


local Transform = {}
Transform.__index = Transform

function Transform:update()
	local tfm = self.transform
	tfm:reset()
	tfm:translate(self.tx, self.ty)
end

function Transform:apply()
end

local function newTransform()
	return setmetatable({
		tx = 0,
		ty = 0,
		r = 0,
		sx = 1,
		sy = 1,
		transform = love.math.newTransform()
	}, Transform)
end



local selected = nil
local dragging = nil
local dragfunc = nil
local editmode = "none"  -- "transform"
local edited = nil
--local gtransform = love.math.newTransform()
local gtransform = newTransform()
local widget = nil

local function lengthSqr(dx, dy)
	return dx * dx + dy * dy
end

local function allpoints(f)
	for i = 1, graphics.npaths do
		local path = graphics.paths[i]
		for j = 1, path.ntrajs do
			local traj = path.trajs[j]

			for k = 1, traj.npts, 3 do
				local pts = traj.pts
				local pt = pts[k]
				f(pts, i, j, k, pt.x, pt.y)
			end
		end
	end
end

local function isControlPointVisible(traj, i, j, k)
	if selected ~= nil and selected.path == i and selected.traj == j then
		local s = selected.pt
		local distance = math.abs(s - k)
		distance = math.min(distance, traj.npts - distance)

		return distance <= 2
	end
end

local function allcontrolpoints(f)
	for i = 1, graphics.npaths do
		local path = graphics.paths[i]
		for j = 1, path.ntrajs do
			local traj = path.trajs[j]

			for k = 1, traj.npts, 3 do
				local pts = traj.pts

				local isActive = false
				if dragging ~= nil and dragging.path == i and dragging.traj == j and dragging.pt0 == k then
					isActive = true
				end
				
				if isActive or isControlPointVisible(traj, i, j, k - 1) then
					local pt1 = pts[k - 1]
					f(traj, pts, i, j, k, k - 1, pt1.x, pt1.y)
				end

				if isActive or isControlPointVisible(traj, i, j, k + 1) then
					local pt2 = pts[k + 1]
					f(traj, pts, i, j, k, k + 1, pt2.x, pt2.y)
				end
			end
		end
	end
end

local function polar(x, y, ox, oy)
	local dx = x - ox
	local dy = y - oy
	return math.atan2(dy, dx), math.sqrt(dx * dx + dy * dy)
end

local function mirrorControlPoint(qx, qy, px, py, p0x, p0y, rx, ry)
	local oldphi, oldmag = polar(qx, qy, p0x, p0y)
	local newphi, newmag = polar(px, py, p0x, p0y)

	local cp1phi, cp1mag = polar(rx, ry, p0x, p0y)

	cp1phi = cp1phi + (newphi - oldphi)

	return p0x + math.cos(cp1phi) * cp1mag, p0y + math.sin(cp1phi) * cp1mag
end


local PointsWidget = {}
PointsWidget.__index = PointsWidget

function PointsWidget:draw()
	local tfm = gtransform.transform

	love.graphics.push("transform")
	love.graphics.applyTransform(tfm)
	overlayLine:draw()
	love.graphics.pop()

	allcontrolpoints(function(traj, pts, i, j, k0, k, lx, ly)
		local x, y = tfm:transformPoint(lx, ly)
		local kx, ky = tfm:transformPoint(pts[k0].x, pts[k0].y)
		love.graphics.line(kx, ky, x, y)
		handles.selected:draw(x, y, 0, 0.75, 0.75)
	end)

	allpoints(function(pts, i, j, k, lx, ly)
		local handle = handles.normal
		if selected ~= nil and selected.path == i and selected.traj == j and selected.pt == k then
			handle = handles.selected
		end
		local x, y = tfm:transformPoint(lx, ly)
		handle:draw(x, y)
	end)
end

function PointsWidget:mousedown(gx, gy, lx, ly)
	allpoints(function(pts, i, j, k, px, py)
		if lengthSqr(lx - px, ly - py) < handleClickRadiusSqr then
			selected = {path = i, traj = j, pt = k}
			dragging = selected
			hit = true
		end
	end)

	allcontrolpoints(function(traj, pts, i, j, k0, k, px, py)
		if lengthSqr(lx - px, ly - py) < handleClickRadiusSqr then
			dragging = {path = i, traj = j, pt = k, pt0 = k0}
			hit = true
		end
	end)

	-- click on curve?

	for i = 1, graphics.npaths do
		local path = graphics.paths[i]
		for j = 1, path.ntrajs do
			local traj = path.trajs[i]
			local t = traj:closest(lx, ly, 2 + path:getLineWidth())
			if t >= 0 then

				dragfunc = function(gx, gy)
					local mx, my = gtransform.transform:inverseTransformPoint(gx, gy)

					local ci = 1 + math.floor(t)
					local c1 = traj.curves[ci]

					local cp1x, cp1y = c1.cp1x, c1.cp1y
					local cp2x, cp2y = c1.cp2x, c1.cp2y

					traj:mould(t, mx, my)

					local prev = traj.curves[ci - 1]
					prev.cp2x, prev.cp2y = mirrorControlPoint(
						cp1x, cp1y, c1.cp1x, c1.cp1y, c1.x0, c1.y0, prev.cp2x, prev.cp2y)

					local next = traj.curves[ci + 1]
					next.cp1x, next.cp1y = mirrorControlPoint(
						cp2x, cp2y, c1.cp2x, c1.cp2y, c1.x, c1.y, next.cp1x, next.cp1y)

					updateOverlayLine()
				end

				if false then
					local k = traj:insertCurveAt(t)
					selected = {path = i, traj = j, pt = k}
					updateOverlayLine()
				end
				
				return
			end
		end
	end
end


local TransformWidget = {}
TransformWidget.__index = TransformWidget

function rotationHandlePosition(x00, y00, x10, y10)
	local phi = math.atan2(y10 - y00, x10 - x00) - math.pi / 2
	local mx, my = (x00 + x10) / 2, (y00 + y10) / 2
	local toplen = 30
	return mx + toplen * math.cos(phi), my + toplen * math.sin(phi), mx, my
end

function TransformWidget:draw()
	local lx0, ly0, lx1, ly1 = graphics:computeAABB()

	local tfm = gtransform.transform
	local x00, y00 = tfm:transformPoint(lx0, ly0)
	local x10, y10 = tfm:transformPoint(lx1, ly0)
	local x11, y11 = tfm:transformPoint(lx1, ly1)
	local x01, y01 = tfm:transformPoint(lx0, ly1)

	love.graphics.setColor(1, 1, 1)
	love.graphics.line(x00, y00, x10, y10)
	love.graphics.line(x10, y10, x11, y11)
	love.graphics.line(x11, y11, x01, y01)
	love.graphics.line(x01, y01, x00, y00)

	local handle = handles.normal
	handle:draw(x00, y00)
	handle:draw(x10, y10)
	handle:draw(x11, y11)
	handle:draw(x01, y01)

	handle:draw((x00 + x10) / 2, (y00 + y10) / 2)
	handle:draw((x10 + x11) / 2, (y10 + y11) / 2)
	handle:draw((x11 + x01) / 2, (y11 + y01) / 2)
	handle:draw((x01 + x00) / 2, (y01 + y00) / 2)

	local tx, ty, mx, my = rotationHandlePosition(x00, y00, x10, y10)
	love.graphics.line(mx, my, tx, ty)
	handle:draw(tx, ty)	
end

function TransformWidget:mousedown(gx, gy, lx, ly)
	local lx0, ly0, lx1, ly1 = graphics:computeAABB()

	local tfm = gtransform.transform
	local x00, y00 = tfm:transformPoint(lx0, ly0)
	local x10, y10 = tfm:transformPoint(lx1, ly0)
	local x11, y11 = tfm:transformPoint(lx1, ly1)
	local x01, y01 = tfm:transformPoint(lx0, ly1)

	local cx = (x00 + x11) / 2
	local cy = (y00 + y11) / 2
	local phi0 = math.atan2(gy - cy, gx - cx)

	local tx, ty = rotationHandlePosition(x00, y00, x10, y10)
	if lengthSqr(tx - gx, ty - gy) < handleClickRadiusSqr then
		dragfunc = function(gx, gy)
			local phi = math.atan2(gy - cy, gx - cx)
			tfm:rotate(phi - phi0)
			phi0 = phi

		end
	end
end


local colorwheel
function love.load()
	colorwheel = createColorWheel(function(r, g, b)
		for i = 1, graphics.npaths do
			graphics.paths[i]:setFillColor(r, g, b)
		end
	end)
end

function love.draw()
	love.graphics.setBackgroundColor(0.7, 0.7, 0.7)
	love.graphics.setColor(1, 1, 1)

	love.graphics.push("transform")
	love.graphics.applyTransform(gtransform.transform)
	graphics:draw()
	love.graphics.pop()

	if widget ~= nil then
		widget:draw()
	end

	if false then
		local x = love.mouse.getX()
		local y = love.mouse.getY()

		if graphics.paths[1]:inside(x, y) then
			love.graphics.setColor(0, 1, 0)
		else
			love.graphics.setColor(1, 0, 0)
		end
		love.graphics.circle("fill", x, y, 5)
	end

	if false then
		local x = love.mouse.getX()
		local y = love.mouse.getY()
		local tr = graphics.paths[1].trajs[1]
		local t = tr:closest(x, y)
		if t >= 0 then
			local px, py = tr:position(t)
			love.graphics.circle("fill", px, py, 5)
		end
	end

	colorwheel.draw()
end

function love.update()
	if dragfunc ~= nil then
		local mx = love.mouse.getX()
		local my = love.mouse.getY()
		dragfunc(mx, my)
	end
end

local function makeDragObjectFunc(x0, y0)
	local x, y = x0, y0
	return function(mx, my)
		gtransform.tx = gtransform.tx + (mx - x)
		gtransform.ty = gtransform.ty + (my - y)
		gtransform:update()
		x, y = mx, my
	end
end

function love.mousepressed(x, y, button, isTouch, clickCount)
	if button == 1 then
		local lx, ly = gtransform.transform:inverseTransformPoint(x, y)

		local hit = false
		dragging = nil

		if widget ~= nil then
			widget:mousedown(x, y, lx, ly)
		end

		if dragging ~= nil then
			dragfunc = function(gx, gy)
				local mx, my = gtransform.transform:inverseTransformPoint(gx, gy)

				local traj = graphics.paths[dragging.path].trajs[dragging.traj]
				local p = traj.pts[dragging.pt]

				local qx, qy = p.x, p.y

				p.x = mx
				p.y = my

				if (dragging.pt - 1) % 3 == 0 then
					-- if this is not a control point, also drag adjacent control points.
					local dx = mx - qx
					local dy = my - qy

					local cp0 = traj.pts[dragging.pt - 1]
					cp0.x = cp0.x + dx
					cp0.y = cp0.y + dy

					local cp1 = traj.pts[dragging.pt + 1]
					cp1.x = cp1.x + dx
					cp1.y = cp1.y + dy
				else
					-- if control point, mirror movement on other control point
					local p0 = traj.pts[dragging.pt0]
					local p = traj.pts[dragging.pt]
					local cp1 = traj.pts[dragging.pt0 - (dragging.pt - dragging.pt0)]

					cp1.x, cp1.y = mirrorControlPoint(qx, qy, mx, my, p0.x, p0.y, cp1.x, cp1.y)
				end

				updateOverlayLine()
			end

			return
		end

		if dragfunc == nil then
			dragfunc = colorwheel.click(love.mouse.getX(), love.mouse.getY())
		end

		if dragfunc == nil then
			widget = nil

			for i = 1, graphics.npaths do
				if graphics.paths[i]:inside(lx, ly) then
					if clickCount == 2 then
						widget = PointsWidget
					elseif clickCount == 1 then
						widget = TransformWidget
					end
					dragfunc = makeDragObjectFunc(x, y)
					return
				end
			end
		end

		--selected = nil
		--dragging = nil
	end
end

function love.mousereleased(x, y, button)
	if button == 1 then
		dragging = nil
		dragfunc = nil
	end
end


function love.keypressed(key, scancode, isrepeat)
	if key == "backspace" then
		if selected ~= nil then
			if (selected.pt - 1) % 3 == 0 then
				local traj = graphics.paths[selected.path].trajs[selected.traj]
				traj:removeCurve((selected.pt - 1) / 3)
				updateOverlayLine()
			end
		end
	end
end

