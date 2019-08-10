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

--- A vector graphics path, usually lives inside a @{Graphics}.
-- @classmod Path
-- @set sort=true

--- @{tove.List} of @{Subpath}s in this @{Path}
-- @table[readonly] subpaths

--- name of this path
-- @table[readonly] id

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

--- Create a deep copy.
-- This cloned copy will include new copies of all @{Subpath}s.
-- @treturn Path cloned @{Path}.
-- @function clone

Path.clone = lib.ClonePath

--- Set line dash offset.
-- See Mozilla for a good <a href="https://developer.mozilla.org/en-US/docs/Web/SVG/Attribute/stroke-dashoffset">
-- description of line dash offsets</a>.
-- @tparam number offset new line dash offset
-- @function setLineDashOffset

Path.setLineDashOffset = lib.PathSetLineDashOffset

--- Get line width.
-- @treturn number current line width
-- @see setLineWidth
-- @function getLineWidth

Path.getLineWidth = lib.PathGetLineWidth

--- Set line width.
-- @tparam number width new line width
-- @function setLineWidth
-- @see getLineWidth

Path.setLineWidth = lib.PathSetLineWidth

--- Set miter limit.
-- See Mozilla for a good <a href="https://developer.mozilla.org/en-US/docs/Web/SVG/Attribute/stroke-miterlimit">
-- description of miter limits</a>.
-- @tparam number miterLimit new miter limit
-- @function setMiterLimit

Path.setMiterLimit = lib.PathSetMiterLimit

--- Get miter limit.
-- @treturn number current miter limit
-- @see setMiterLimit
-- @function getMiterLimit

Path.getMiterLimit = lib.PathGetMiterLimit

--- Get opacity.
-- @treturn number @{Path}'s opacity
-- @function getOpacity
-- @see setOpacity

Path.getOpacity = lib.PathGetOpacity

--- Set opacity.
-- This will apply to the whole path indepentent of fill and line
-- colors.
-- @tparam number opacity @{Path}'s opacity (between 0 and 1)
-- @function setOpacity
-- @see getOpacity

Path.setOpacity = lib.PathSetOpacity

--- Check if inside.
-- Returns true if a given point is inside the @{Path}.
-- @tparam number x x coordinate of point to test
-- @tparam number y y coordinate of point to test
-- @function inside

Path.inside = lib.PathIsInside

--- Refine curves.
-- Adds additional points without changing the shape.
-- @usage
-- p:refine(2) -- double number of points, same shape
-- @tparam int k scale number of points by this amount
-- @function refine

Path.refine = lib.PathRefine

function Path:beginSubpath()
	return ffi.gc(lib.PathBeginSubpath(self), lib.ReleaseSubpath)
end

--- Add a @{Subpath}.
-- The new @{Subpath} will be appended after other existing @{Subpath}s.
-- @tparam Subpath t @{Subpath} to append

function Path:addSubpath(t)
	lib.PathAddSubpath(self, t)
end

--- Move to position (x, y).
-- Automatically starts a fresh @{Subpath} if necessary. 
-- @tparam number x new x coordinate
-- @tparam number y new y coordinate

function Path:moveTo(x, y)
	lib.SubpathMoveTo(self:beginSubpath(), x, y)
end

--- Draw a line to (x, y).
-- @usage
-- g:moveTo(10, 20)
-- g:lineTo(5, 30)
-- @tparam number x x coordinate of line's end position
-- @tparam number y y coordinate of line's end position

function Path:lineTo(x, y)
	lib.SubpathLineTo(self:beginSubpath(), x, y)
end

--- Draw a cubic bezier curve to (x, y).
-- See <a href="https://en.wikipedia.org/wiki/B%C3%A9zier_curve">Bézier curves on Wikipedia</a>.
-- @usage
-- g:moveTo(10, 20)
-- g:curveTo(7, 5, 10, 3, 50, 30)
-- @tparam number cp1x x coordinate of curve's first control point (P1)
-- @tparam number cp1y y coordinate of curve's first control point (P1)
-- @tparam number cp2x x coordinate of curve's second control point (P2)
-- @tparam number cp2y y coordinate of curve's second control point (P2)
-- @tparam number x x coordinate of curve's end position (P3)
-- @tparam number y y coordinate of curve's end position (P3)

function Path:curveTo(...)
	lib.SubpathCurveTo(self:beginSubpath(), ...)
end

--- Get fill color.
-- @treturn Paint current fill color, or nil if there is no fill

function Path:getFillColor()
	return Paint._fromRef(lib.PathGetFillColor(self))
end

--- Get line color.
-- @treturn Paint current line color, or nil if there is no line

function Path:getLineColor()
	return Paint._fromRef(lib.PathGetLineColor(self))
end

--- Set fill color.
-- @usage
-- g:setFillColor(0.8, 0, 0, 0.5)  -- transparent reddish
-- g:setFillColor("#00ff00")  -- opaque green
-- g:setFillColor(nil)  -- no fill
-- @tparam number|string|Paint|nil r red
-- @tparam[optchain] number g green
-- @tparam[optchain] number b blue
-- @tparam[optchain=1] number a alpha

function Path:setFillColor(r, g, b, a)
	lib.PathSetFillColor(self, Paint._wrap(r, g, b, a))
end

--- Set line color.
-- @see setFillColor
-- @tparam number|string|Paint|nil r red
-- @tparam[optchain] number g green
-- @tparam[optchain] number b blue
-- @tparam[optchain=1] number a alpha

function Path:setLineColor(r, g, b, a)
	lib.PathSetLineColor(self, Paint._wrap(r, g, b, a))
end

--- Animate between two @{Path}s.
-- Makes this @{Path} to an interpolated inbetween.
-- @tparam Path a @{Path} at t == 0
-- @tparam Path b @{Path} at t == 1
-- @tparam number t interpolation (0 <= t <= 1)
-- @see Graphics:animate

function Path:animate(a, b, t)
	lib.PathAnimate(self, a, b, t)
end

--- Warp points.
-- @tparam function f takes (x, y, curvature) and returns the same
-- @see Graphics:warp

function Path:warp(f)
	local subpaths = self.subpaths
	for i = 1, subpaths.count do
		subpaths[i]:warp(f)
	end
end

--- Rotate elements.
-- Will recusively left rotate elements in this @{Graphics} such that items
-- found at index k will get moved to index 0.
-- @tparam string w what to rotate: either "subpath", "point" or "curve"
-- @tparam int k how much to rotate
-- @see Graphics:rotate

function Path:rotate(w, k)
	lib.PathRotate(self, tove.elements[w], k)
end

--- Find nearest point.
-- @tparam number x x component of point to search against
-- @tparam number y y component of point to search against
-- @tparam[opt] number max ignore curve points above this distance
-- @tparam[opt] number min return as soon as a distance smaller than this is found
-- @treturn number distance to nearest @{Subpath}
-- @treturn number t parameter describing nearest point on @{Subpath}
-- @treturn Subpath nearest @{Subpath}
-- @see Subpath:nearest

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

-- @set no_summary=true
--- Transform this @{Path}.
-- Also see <a href="https://love2d.org/wiki/love.math.newTransform">love.math.newTransform</a>.
-- @usage
-- g:transform(0, 0, 0, sx, sy)  -- scale by (sx, sy)
-- @tparam number|Translation x move by x in x-axis, or a LÖVE <a href="https://love2d.org/wiki/Transform">Transform</a>
-- @tparam[opt=0] number y move by y in y-axis
-- @tparam[optchain=0] number angle applied rotation in radians
-- @tparam[optchain=1] number sy scale factor in x
-- @tparam[optchain=1] number sy scale factor in y
-- @tparam[optchain=0] number ox x coordinate of origin
-- @tparam[optchain=0] number oy y coordinate of origin
-- @tparam[optchain=0] number kx skew in x-axis
-- @tparam[optchain=0] number ky skew in y-axis
-- @see Graphics:transform
-- @set no_summary=false

function Path:transform(...)
	self:set(tove.transformed(self, ...))
end

--- Get line join.
-- @treturn string one of the names in @{tove.LineJoin}
-- @see setLineJoin

function Path:getLineJoin()
	return findEnum(tove.lineJoins, lib.PathGetLineJoin(self))
end

--- Set line join.
-- @tparam string join one of the names in @{tove.LineJoin}
-- @see getLineJoin

function Path:setLineJoin(join)
	lib.PathSetLineJoin(self, tove.lineJoins[join])
end

--- Get line cap.
-- @treturn string one of the names in @{tove.LineCap}
-- @see setLineCap

function Path:getLineCap()
	return findEnum(tove.lineCaps, lib.PathGetLineCap(self))
end

--- Set line cap.
-- @tparam string join one of the names in @{tove.LineCap}
-- @see getLineCap

function Path:setLineCap(cap)
	lib.PathSetLineCap(self, tove.lineCaps[cap])
end

--- Get fill rule.
-- @treturn string one of the names in @{tove.FillRule}
-- @see setFillRule

function Path:getFillRule()
	return findEnum(tove.fillRules, lib.PathGetFillRule(self))
end

--- Set fill rule.
-- Defines the rule by which TÖVE knows which parts of a shape
-- are solid and which are not.
-- @tparam string rule one of the names in @{tove.FillRule}
-- @see getFillRule

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
