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

local _attr = {r = 1, g = 2, b = 3, a = 4, rgba = 0}

local Paint = {}
Paint.__index = function (self, key)
	if _attr[key] ~= nil then
		local rgba = lib.ColorGet(self._ref, 1.0)
		if key == "rgba" then
			return {rgba.r, rgba.g, rgba.b, rgba.a}
		else
			return rgba[key]
		end
	else
		return Paint[key]
	end
end
Paint.__newindex = function(self, key, value)
	local i = _attr[key]
	if i ~= nil then
		local rgba = {self:get()}
		rgba[i] = value
		self:set(unpack(rgba))
	end
end

local function fromRef(ref)
	if ref.ptr == nil then
		return nil
	else
		return setmetatable({_ref = ffi.gc(ref, lib.ReleasePaint)}, Paint)
	end
end

Paint._fromRef = fromRef

local noColor = fromRef(lib.NewEmptyPaint())

local newColor = function(r, g, b, a)
	if r == nil then
		return noColor
	else
		return fromRef(lib.NewColor(r, g or 0, b or 0, a or 1))
	end
end

tove.newColor = newColor

tove.newLinearGradient = function(x0, y0, x1, y1)
	return fromRef(lib.NewLinearGradient(x0, y0, x1, y1))
end

tove.newRadialGradient = function(cx, cy, fx, fy, r)
	return fromRef(lib.NewRadialGradient(cx, cy, fx, fy, r))
end

local function unpackRGBA(rgba)
	return rgba.r, rgba.g, rgba.b, rgba.a
end


function Paint:get(opacity)
	return unpackRGBA(lib.ColorGet(self._ref, opacity or 1))
end

function Paint:set(r, g, b, a)
	lib.ColorSet(self._ref, r, g, b, a or 1)
end

local paintTypes = {
	none = lib.PAINT_NONE,
	solid = lib.PAINT_SOLID,
	linear = lib.PAINT_LINEAR_GRADIENT,
	radial = lib.PAINT_RADIAL_GRADIENT
}

function Paint:getType()
	local t = lib.PaintGetType(self._ref)
	for name, enum in pairs(paintTypes) do
		if t == enum then
			return name
		end
	end
end

function Paint:getNumColors()
	return lib.PaintGetNumColors(self._ref)
end

function Paint:getColorStop(i, opacity)
	return lib.PaintGetColorStop(self._ref, i - 1, opacity or 1)
end

function Paint:getGradientParameters()
	local t = lib.PaintGetType(self._ref)
	local v = lib.GradientGetParameters(self._ref).values
	if t == lib.PAINT_LINEAR_GRADIENT then
		return v[0], v[1], v[2], v[3]
	elseif t == lib.PAINT_RADIAL_GRADIENT then
		return v[0], v[1], v[2], v[3], v[4]
	end
end

function Paint:addColorStop(offset, r, g, b, a)
	lib.GradientAddColorStop(self._ref, offset, r, g, b, a or 1)
end

function Paint:clone()
	return lib.ClonePaint(self._ref)
end

local function exportGradientColors(paint)
	local n = paint:getNumColors()
	local colors = {}
	for i = 1, n do
		local stop = paint:getColorStop(i)
		table.insert(colors, {stop.offset, unpackRGBA(stop.rgba)})
	end
	return colors
end

local function importGradientColors(g, colors)
	for _, c in ipairs(colors) do
		g:addColorStop(unpack(c))
	end
end

function Paint:serialize()
	local t = lib.PaintGetType(self._ref)
	if t == lib.PAINT_SOLID then
		return {type = "solid", color = {self:get()}}
	elseif t == lib.PAINT_LINEAR_GRADIENT then
		local x0, y0, x1, y1 = self:getGradientParameters()
		return {type = "linear",
			x0 = x0, y0 = y0, x1 = x1, y1 = y1,
			colors = exportGradientColors(self)}
	elseif t == lib.PAINT_RADIAL_GRADIENT then
		local cx, cy, fx, fy, r = self:getGradientParameters()
		return {type = "radial",
			cx = cx, cy = cy, fx = fx, fy = fy, r = r,
			colors = exportGradientColors(self)}
	end
	return nil
end

tove.newPaint = function(p)
	if p.type == "solid" then
		return newColor(unpack(p.color))
	elseif p.type == "linear" then
		local g = tove.newLinearGradient(p.x0, p.y0, p.x1, p.y1)
		importGradientColors(g, p.colors)
		return g
	elseif p.type == "radial" then
		local g = tove.newRadialGradient(p.cx, p.cy, p.fx, p.fy, p.r)
		importGradientColors(g, p.colors)
		return g
	end
end

Paint._wrap = function(r, g, b, a)
	if getmetatable(r) == Paint then
		return r
	end
	local t = type(r)
	if t == "number" then
		return newColor(r, g, b, a)
	elseif t == "string" and r:sub(1, 1) == '#' then
		r = r:gsub("#","")
		return newColor(
			tonumber("0x" .. r:sub(1, 2)) / 255,
			tonumber("0x" .. r:sub(3, 4)) / 255,
			tonumber("0x" .. r:sub(5, 6)) / 255)
	elseif t == "nil" then
		return noColor
	else
		error("tove: cannot parse color: " .. tostring(r))
	end
end

return Paint
