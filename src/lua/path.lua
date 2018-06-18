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

local Subpaths = {}
Subpaths.__index = function (self, i)
	if i == "count" then
		return lib.PathGetNumSubpaths(self.path)
	else
		return ffi.gc(lib.PathGetSubpath(self.path, i), lib.ReleaseSubpath)
	end
end

local Path = {}
Path.__index = function (path, key)
	if key == "subpaths" then
		return setmetatable({path = path}, Subpaths)
	else
		return Path[key]
	end
end

Path.clone = lib.ClonePath
Path.setLineDashOffset = lib.PathSetLineDashOffset
Path.getLineWidth = lib.PathGetLineWidth
Path.setLineWidth = lib.PathSetLineWidth
Path.setMiterLimit = lib.PathSetMiterLimit
Path.getMiterLimit = lib.PathGetMiterLimit
Path.getNumCurves = lib.PathGetNumCurves
Path.getOpacity = lib.PathGetOpacity
Path.setOpacity = lib.PathSetOpacity
Path.clearChanges = lib.PathClearChanges
Path.fetchChanges = lib.PathFetchChanges
Path.inside = lib.PathIsInside

function Path:beginSubpath()
	return ffi.gc(lib.PathBeginSubpath(self), lib.ReleaseSubpath)
end

function Path:addSubpath(t)
	lib.PathAddSubpath(self, t)
end

function Path:moveTo(x, y)
	lib.SubpathMoveTo(self:beginSubpath(), x, y)
end

function Path:lineTo(x, y)
	lib.SubpathLineTo(self:beginSubpath(), x, y)
end

function Path:curveTo(...)
	lib.SubpathCurveTo(self:beginSubpath(), ...)
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

function Path:nearest(x, y, max, min)
	local n = lib.PathGetNumSubpaths(self)
	local subpaths = self.subpaths
	for i = 1, n do
		local traj = subpaths[i]
		local t, d = traj:nearest(x, y, max, min)
		if t >= 0 then
			return d, t, traj
		end
	end
	return false
end

ffi.metatype("TovePathRef", Path)

tove.newPath = function(d)
	return ffi.gc(lib.NewPath(d), lib.ReleasePath)
end

return Path
