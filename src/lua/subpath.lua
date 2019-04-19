-- *****************************************************************
-- TÖVE - Animated vector graphics for LÖVE.
-- https://github.com/poke1024/tove2d
--
-- Copyright (c) 2018, Bernhard Liebl
--
-- Distributed under the MIT license. See LICENSE file for details.
--
-- All rights reserved.
-- *****************************************************************

--- A vector graphics subpath (sometimes called trajectory), usually lives inside a @{Path}.
-- @classmod Subpath

--- @{tove.List} of @{Curve}s in this @{Subpath}
-- @table[readonly] curves

--- is this subpath closed?
-- @table[readonly] isClosed

--

local _pt = {
	x = 0,
	y = 1
}

local Point = {}
Point.__index = function (self, key)
	return lib.SubpathGetPtValue(self.traj, self.i, _pt[key] or -1)
end
Point.__newindex = function (self, key, value)
	lib.SubpathSetPtValue(self.traj, self.i, _pt[key] or -1, value)
end

local Points = {}
Points.__index = function (self, i)
	if i == "count" then
		return lib.SubpathGetNumPoints(self.traj)
	else
		return setmetatable({traj = self.traj, i = self.i0 + i - 1}, Point)
	end
end

local Curve = {}
Curve.__index = function (self, key)
	if key == "p" then
		return setmetatable({traj = self.traj, i0 = self.curve * 4}, Points)
	end
	local i = _attributes[key]
	if i == nil then return nil end
	return lib.SubpathGetCurveValue(
		self.traj, self.curve, i)
end
Curve.__newindex = function (self, key, value)
	local i = _attributes[key]
	if i == nil then return nil end
	return lib.SubpathSetCurveValue(
		self.traj, self.curve, i, value)
end

function Curve:split(t)
	lib.SubpathInsertCurveAt(self.traj, self.curve + t)
end

function Curve:remove()
	lib.SubpathRemoveCurve(self.traj, self.curve)
end

function Curve:isLine(dir)
	return lib.SubpathIsLineAt(self.traj, self.curve, dir or 0)
end

function Curve:makeFlat(dir)
	return lib.SubpathMakeFlat(self.traj, self.curve, dir or 0)
end

function Curve:makeSmooth(a, dir)
	return lib.SubpathMakeSmooth(self.traj, self.curve, dir or 0, a or 0.5)
end


local Curves = {}
Curves.__index = function (self, curve)
	if curve == "count" then
		return lib.SubpathGetNumCurves(self.traj)
	else
		return setmetatable({traj = self.traj, curve = curve - 1}, Curve)
	end
end


local Subpath = {}
Subpath.__index = function(self, key)
	if key == "curves" then
		return setmetatable({traj = self}, Curves)
	elseif key == "points" then
		return setmetatable({traj = self, i0 = 0}, Points)
	elseif key == "isClosed" then
		return lib.SubpathIsClosed(self)
	else
		return Subpath[key]
	end
end
Subpath.__newindex = function(self, key, value)
	if key == "isClosed" then
		lib.SubpathSetIsClosed(self, value)
	end
end

--- Evaluate position at t.
-- @tparam number t t at which to evaluate position. 0 < t < 1
-- will lie in the first curve, 1 < t < 2 will lie in the
-- second curve, and so on.
-- @treturn number x position
-- @treturn number y position

function Subpath:position(t)
	local v = lib.SubpathGetPosition(self, t)
	return v.x, v.y
end

--- Evaluate normal at t.
-- @tparam number t t at which to evaluate position. 0 < t < 1
-- will lie in the first curve, 1 < t < 2 will lie in the
-- second curve, and so on.
-- @treturn number x component of normal
-- @treturn number y component of normal

function Subpath:normal(t)
	local v = lib.SubpathGetNormal(self, t)
	return v.x, v.y
end

--- Find nearest point.
-- @tparam number x x component of point to search against
-- @tparam number y y component of point to search against
-- @tparam number[opt=1e50] dmax ignore curve points above this distance
-- @tparam number[opt=1e-4] dmin return as soon as a distance smaller than this is found
-- @treturn number t which produces closest distance
-- @treturn number distance distance at given t

function Subpath:nearest(x, y, dmax, dmin)
	local n = lib.SubpathNearest(self, x, y, dmin or 1e-4, dmax or 1e50)
	return n.t, math.sqrt(n.distanceSquared)
end

Subpath.insertCurveAt = lib.SubpathInsertCurveAt
Subpath.removeCurve = lib.SubpathRemoveCurve

--- Mould curve.
-- Moves a specific point on the curve to a new location and adapts the surrounding
-- curve.
-- @tparam number t t of point on the curve which should be moved, 0 <= t < = number of curves
-- @tparam number x x coordinate of where to move
-- @tparam number y y coordinate of where to move
-- @function mould

Subpath.mould = lib.SubpathMould

Subpath.commit = lib.SubpathCommit

--- Move to position (x, y).
-- @tparam number x new x coordinate
-- @tparam number y new y coordinate
-- @treturn Command move command
-- @function moveTo

Subpath.moveTo = lib.SubpathMoveTo

--- Draw a line to (x, y).
-- @usage
-- subpath:moveTo(10, 20)
-- subpath:lineTo(5, 30)
-- @tparam number x x coordinate of line's end position
-- @tparam number y y coordinate of line's end position
-- @treturn Command line command
-- @function lineTo

Subpath.lineTo = lib.SubpathLineTo

--- Draw a cubic bezier curve to (x, y).
-- See <a href="https://en.wikipedia.org/wiki/B%C3%A9zier_curve">Bézier curves on Wikipedia</a>.
-- @usage
-- subpath:moveTo(10, 20)
-- subpath:curveTo(7, 5, 10, 3, 50, 30)
-- @tparam number cp1x x coordinate of curve's first control point (P1)
-- @tparam number cp1y y coordinate of curve's first control point (P1)
-- @tparam number cp2x x coordinate of curve's second control point (P2)
-- @tparam number cp2y y coordinate of curve's second control point (P2)
-- @tparam number x x coordinate of curve's end position (P3)
-- @tparam number y y coordinate of curve's end position (P3)
-- @treturn Command curve command
-- @function curveTo

Subpath.curveTo = lib.SubpathCurveTo

local handles = {
	free = lib.TOVE_HANDLE_FREE,
	aligned = lib.TOVE_HANDLE_ALIGNED
}

function Subpath:move(k, x, y, h)
	lib.SubpathMove(self, k, x, y, h and handles[h] or lib.TOVE_HANDLE_FREE)
end

--- Draw an arc.
-- @tparam number x x coordinate of centre of arc
-- @tparam number y y coordinate of centre of arc
-- @tparam number r radius of arc
-- @tparam number startAngle angle at which to start drawing
-- @tparam number endAngle angle at which to stop drawing
-- @tparam bool ccw true if drawing between given angles is counterclockwise
-- @function arc

function Subpath:arc(x, y, r, a1, a2, ccw)
	lib.SubpathArc(self, x, y, r, a1, a2, ccw or false)
end

--- Draw a rectangle.
-- @tparam number x x coordinate of top left corner
-- @tparam number y y coordinate of top left corner
-- @tparam number w width
-- @tparam number h height
-- @tparam number rx horizontal roundness of corners
-- @tparam number ry vertical roundness of corners
-- @treturn Command rectangle command
-- @function drawRect

function Subpath:drawRect(x, y, w, h, rx, ry)
	return newCommand(self, lib.SubpathDrawRect(
		self, x, y, w, h or w, rx or 0, ry or 0))
end

--- Draw a circle.
-- @tparam number x x coordinate of centre
-- @tparam number y y coordinate of centre
-- @tparam number r radius
-- @treturn Command circle command

function Subpath:drawCircle(x, y, r)
	return newCommand(self, lib.SubpathDrawEllipse(
		self, x, y, r, r))
end

--- Draw an ellipse.
-- @tparam number x x coordinate of centre
-- @tparam number y y coordinate of centre
-- @tparam number rx horizontal radius
-- @tparam number ry vertical radius
-- @treturn Command ellipse command

function Subpath:drawEllipse(x, y, rx, ry)
	return newCommand(self, lib.SubpathDrawEllipse(
		self, x, y, rx, ry or rx))
end

function Subpath:isLineAt(k, dir)
	return lib.SubpathIsLineAt(self, k, dir or 0)
end

function Subpath:makeFlat(k, dir)
	return lib.SubpathMakeFlat(self, k, dir or 0)
end

function Subpath:makeSmooth(k, a, dir)
	return lib.SubpathMakeSmooth(self, k, dir or 0, a or 0.5)
end

function Subpath:set(arg)
	if getmetatable(arg) == tove.Transform then
		lib.SubpathSet(
			self,
			arg.s,
			unpack(arg.args))
	else
		lib.SubpathSet(
			self,
			arg,
			1, 0, 0, 0, 1, 0)
	end
end

--- Transform this @{Subpath}.
-- @usage
-- subpath:transform(0, 0, 0, sx, sy)  -- scale by (sx, sy)
-- @tparam number|Translation x move by x in x-axis, or a LÖVE <a href="https://love2d.org/wiki/Transform">Transform</a>
-- @tparam[opt] number y move by y in y-axis
-- @tparam[optchain] number angle applied rotation in radians
-- @tparam[optchain] number sy scale factor in x
-- @tparam[optchain] number sy scale factor in y
-- @tparam[optchain] number ox x coordinate of origin
-- @tparam[optchain] number oy y coordinate of origin
-- @tparam[optchain] number kx skew in x-axis
-- @tparam[optchain] number ky skew in y-axis
-- @see Graphics:transform
-- @see Path:transform

function Subpath:transform(...)
	self:set(tove.transformed(self, ...))
end

--- Invert.
-- Reverses orientation by reversing order of points. You can use this to
-- create holes in a @{Path}.

function Subpath:invert()
	lib.SubpathInvert(self)
end

--- Set points.
-- @tparam {number,number,...} points x and y coordinates of points to set

function Subpath:setPoints(...)
	local p = {...}
	local n = #p
	local f = ffi.new("float[?]", n)
	for i = 1, n do
		f[i - 1] = p[i]
	end
	lib.SubpathSetPoints(self, f, n / 2)
end

--- Warp.
-- @tparam function f warp function
-- @see Graphics:warp

function Subpath:warp(f)
	local c = lib.SubpathSaveCurvature(self)
	local n = lib.SubpathGetNumPoints(self)
	local p = lib.SubpathGetPointsPtr(self)
	local j = 0
	for i = 0, n - 1, 3 do
		x, y, curvature = f(p[2 * i + 0], p[2 * i + 1], c[j].curvature)
		p[2 * i + 0] = x
		p[2 * i + 1] = y
		if curvature ~= nil then c[j].curvature = curvature end
		j = j + 1
	end
	lib.SubpathFixLoop(self)
	lib.SubpathRestoreCurvature(self)
end

ffi.metatype("ToveSubpathRef", Subpath)

--- Create new subpath.
-- @tparam[opt] {number,number,...} points x and y components of points to use
-- @treturn Subpath new subpath

tove.newSubpath = function(...)
	local self = ffi.gc(lib.NewSubpath(), lib.ReleaseSubpath)
	if #{...} > 0 then
		self:setPoints(...)
	end
	return self
end

--- Clone.
-- Creates a deep clone.
-- @treturn Subpath cloned subpath

function Subpath:clone()
	return lib.CloneSubpath(self)
end

function Subpath:serialize()
	local n = lib.SubpathGetNumPoints(self)
	local p = lib.SubpathGetPointsPtr(self)
	local q = {}
	for i = 1, n * 2 do
		q[i] = p[i - 1]
	end
	return {
		points = q,
		closed = self.isClosed
	}
end

return Subpath
