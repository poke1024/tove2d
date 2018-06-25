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

local Curve = {}
Curve.__index = function (self, key)
	if key == "count" then
		return lib.SubpathGetNumCurves(self.traj)
	else
		return lib.SubpathGetCurveValue(
			self.traj, self.curve, _attributes[key])
	end
end
Curve.__newindex = function (self, key, value)
	return lib.SubpathSetCurveValue(
		self.traj, self.curve, _attributes[key], value)
end

local Curves = {}
Curves.__index = function (self, curve)
	return setmetatable({traj = self.traj, curve = curve - 1}, Curve)
end

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
		return setmetatable({traj = self.traj, i = i - 1}, Point)
	end
end


local Subpath = {}
Subpath.__index = function(self, key)
	if key == "curves" then
		return setmetatable({traj = self}, Curves)
	elseif key == "points" then
		return setmetatable({traj = self}, Points)
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

function Subpath:position(t)
	local v = lib.SubpathGetPosition(self, t)
	return v.x, v.y
end

function Subpath:normal(t)
	local v = lib.SubpathGetNormal(self, t)
	return v.x, v.y
end

function Subpath:nearest(x, y, dmax, dmin)
	local n = lib.SubpathNearest(self, x, y, dmin or 1e-4, dmax or 1e50)
	return n.t, math.sqrt(n.distanceSquared)
end

Subpath.insertCurveAt = lib.SubpathInsertCurveAt
Subpath.removeCurve = lib.SubpathRemoveCurve
Subpath.mould = lib.SubpathMould
Subpath.move = lib.SubpathMove
Subpath.setPoints = lib.SubpathSetPoints
Subpath.commit = lib.SubpathCommit
Subpath.moveTo = lib.SubpathMoveTo
Subpath.lineTo = lib.SubpathLineTo
Subpath.curveTo = lib.SubpathCurveTo

function Subpath:arc(x, y, r, a1, a2, ccw)
	lib.SubpathArc(self, x, y, r, a1, a2, ccw or false)
end

function Subpath:drawRect(x, y, w, h, rx, ry)
	return newCommand(self, lib.SubpathDrawRect(
		self, x, y, w, h or w, rx or 0, ry or 0))
end

function Subpath:drawCircle(x, y, r)
	return newCommand(self, lib.SubpathDrawEllipse(
		self, x, y, r, r))
end

function Subpath:drawEllipse(x, y, rx, ry)
	return newCommand(self, lib.SubpathDrawEllipse(
		self, x, y, rx, ry or rx))
end

function Subpath:isLineAt(k, dir)
	return lib.SubpathIsLineAt(self, k, dir or 0)
end

ffi.metatype("ToveSubpathRef", Subpath)

tove.newSubpath = function()
	return ffi.gc(lib.NewSubpath(), lib.ReleaseSubpath)
end

return Subpath
