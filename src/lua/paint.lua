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

local _attr = {r = 1, g = 2, b = 3, a = 4}

local Paint = {}
Paint.__index = function (self, key)
	if _attr[key] ~= nil then
		local rgba = lib.ColorGet(self._ref, 1)
		return rgba[key]
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

local newColor = function(r, g, b, a)
	return fromRef(lib.NewColor(r or 0, g or 0, b or 0, a or 1))
end

tove.newColor = newColor

tove.newLinearGradient = function(x0, y0, x1, y1)
	return fromRef(lib.NewLinearGradient(x0, y0, x1, y1))
end

tove.newRadialGradient = function(cx, cy, fx, fy, r)
	return fromRef(lib.NewRadialGradient(cx, cy, fx, fy, r))
end

function Paint:get(opacity)
	local rgba = lib.ColorGet(self._ref, opacity or 1)
	return rgba.r, rgba.g, rgba.b, rgba.a
end

function Paint:set(r, g, b, a)
	lib.ColorSet(self._ref, r, g, b, a or 1)
end

function Paint:addColorStop(offset, r, g, b, a)
	lib.GradientAddColorStop(self._ref, offset, r, g, b, a or 1)
end

function Paint:clone()
	return lib.ClonePaint(self._ref)
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
	else
		error("tove: cannot parse color: " .. tostring(r))
	end
end

return Paint
