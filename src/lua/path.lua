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

local Trajectories = {}
Trajectories.__index = function (self, i)
	return ffi.gc(lib.PathGetTrajectory(self.path, i), lib.ReleaseTrajectory)
end

local Path = {}
Path.__index = function (path, key)
	if key == "trajs" then
		return setmetatable({path = path}, Trajectories)
	elseif key == "ntrajs" then
		return lib.PathGetNumTrajectories(path)
	else
		return Path[key]
	end
end

Path.clone = lib.ClonePath
Path.setLineDashOffset = lib.PathSetLineDashOffset
Path.getLineWidth = lib.PathGetLineWidth
Path.setLineWidth = lib.PathSetLineWidth
Path.setMiterLimit = lib.PathSetMiterLimit
Path.getNumCurves = lib.PathGetNumCurves
Path.getOpacity = lib.PathGetOpacity
Path.setOpacity = lib.PathSetOpacity
Path.clearChanges = lib.PathClearChanges
Path.fetchChanges = lib.PathFetchChanges
Path.inside = lib.PathIsInside

function Path:beginTrajectory()
	return ffi.gc(lib.PathBeginTrajectory(self), lib.ReleaseTrajectory)
end

function Path:addTrajectory(t)
	lib.PathAddTrajectory(self, t)
end

function Path:moveTo(x, y)
	lib.TrajectoryMoveTo(self:beginTrajectory(), x, y)
end

function Path:lineTo(x, y)
	lib.TrajectoryLineTo(self:beginTrajectory(), x, y)
end

function Path:curveTo(...)
	lib.TrajectorCurveTo(self:beginTrajectory(), ...)
end

function Path:getFillColor()
	return Paint._fromRef(lib.PathGetFillColor(self))
end

function Path:getLineColor()
	return Paint._fromRef(lib.PathGetLineColor(self))
end

function Path:setFillColor(r, g, b, a)
	lib.PathSetFillColor(self, Paint._wrap(r, g, b, a)._ref)
end

function Path:setLineColor(r, g, b, a)
	lib.PathSetLineColor(self, Paint._wrap(r, g, b, a)._ref)
end

local _fillrule = {
	nonzero = lib.FILLRULE_NON_ZERO,
	evenodd = lib.FILLRULE_EVEN_ODD
}

function Path:setFillRule(rule)
	lib.PathSetFillRule(self, _fillrule[rule])
end

function Path:animate(a, b, t)
	lib.PathAnimate(self, a, b, t)
end

ffi.metatype("TovePathRef", Path)

tove.newPath = function(d)
	return ffi.gc(lib.NewPath(d), lib.ReleasePath)
end

return Path
