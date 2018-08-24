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
	elseif key == "id" then
		return ffi.string(lib.PathGetId(path))
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

function Path:set(arg, swl)
	if getmetatable(arg) == tove.Transform then
		lib.PathSet(
			self,
			arg.s,
			swl or false,
			unpack(arg.args))
	else
		lib.PathSet(
			self,
			arg,
			false, 1, 0, 0, 0, 1, 0)
	end
end

function Path:transform(t)
	self:set(tove.transformed(self, t))
end

function Path:getLineJoin()
	return findEnum(tove.lineJoins, lib.PathGetLineJoin(self))
end

function Path:setLineJoin(join)
	lib.PathSetLineJoin(self, tove.lineJoins[join])
end

function Path:getFillRule()
	return findEnum(tove.fillRules, lib.PathGetFillRule(self))
end

function Path:setFillRule(rule)
	lib.PathSetFillRule(self, tove.fillRules[rule])
end

function Path:serialize()
	local subpaths = self.subpaths
	local s = {}
	for i = 1, subpaths.count do
		s[i] = subpaths[i]:serialize()
	end
	local line = nil
	local lineColor = self:getLineColor()
	if lineColor then
		line = {
			paint = lineColor:serialize(),
			join = self:getLineJoin(),
			miter = self:getMiterLimit(),
			width = self:getLineWidth()
		}
	end
	local fill = nil
	local fillColor = self:getFillColor()
	if fillColor then
		fill = {
			paint = fillColor:serialize(),
			rule = self:getFillRule()
		}
	end
	local opacity = self:getOpacity()
	return {
		line = line,
		fill = fill,
		opacity = opacity < 1 and opacity or nil,
		subpaths = s
	}
end

function Path:deserialize(t)
	for _, s in ipairs(t.subpaths) do
		self:addSubpath(tove.newSubpath(s))
	end
	if t.line then
		self:setLineColor(tove.newPaint(t.line.paint))
		self:setLineJoin(t.line.join)
		self:setMiterLimit(t.line.miter)
		self:setLineWidth(t.line.width)
	end
	if t.fill then
		self:setFillColor(tove.newPaint(t.fill.paint))
		self:setFillRule(t.fill.rule)
	end
	self:setOpacity(t.opacity or 1)
end

ffi.metatype("TovePathRef", Path)

tove.newPath = function(data)
	local t = type(data)
	local p = ffi.gc(lib.NewPath(
		t == "string" and data or nil), lib.ReleasePath)
	if t == "table" then
		p:deserialize(data)
	end
	return p
end

return Path
