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

--- Colors and gradients used in fills and lines.
-- @classmod Paint
-- @set sort=true

local Paint = {}
Paint.__index = function (self, key)
	if _attr[key] ~= nil then
		local rgba = lib.ColorGet(self, 1.0)
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
		return ffi.gc(ref, lib.ReleasePaint)
	end
end

Paint._fromRef = fromRef

ffi.metatype("TovePaintRef", Paint)

local noColor = fromRef(lib.NewEmptyPaint())

local newColor = function(r, g, b, a)
	if r == nil then
		return noColor
	else
		return fromRef(lib.NewColor(r, g or 0, b or 0, a or 1))
	end
end

--- Create a new color.
-- @tparam number r red
-- @tparam number g green
-- @tparam number b blue
-- @tparam number a alpha

tove.newColor = newColor

--- Create a linear gradient.
-- Initially the gradient will not contain any colors.
-- Also see <a href="https://developer.mozilla.org/en-US/docs/Web/SVG/Element/linearGradient">SVG documentation by Mozilla</a>.
-- @tparam number x0 x coordinate of start point of gradient
-- @tparam number y0 y coordinate of start point of gradient
-- @tparam number x1 x coordinate of end point of gradient
-- @tparam number y1 y coordinate of end point of gradient
-- @see addColorStop

tove.newLinearGradient = function(x0, y0, x1, y1)
	return fromRef(lib.NewLinearGradient(x0, y0, x1, y1))
end

--- Create a radial gradient.
-- Initially the gradient will not contain any colors.
-- Also see <a href="https://developer.mozilla.org/en-US/docs/Web/SVG/Element/radialGradient">SVG documentation by Mozilla</a>.
-- @tparam number cx x coordinate of the end circle
-- @tparam number cy y coordinate of the end circle
-- @tparam number fx x coordinate of the start circle
-- @tparam number fy y coordinate of the start circle
-- @tparam number r radius of end circle
-- @see addColorStop

tove.newRadialGradient = function(cx, cy, fx, fy, r)
	return fromRef(lib.NewRadialGradient(cx, cy, fx, fy, r))
end

local function unpackRGBA(rgba)
	return rgba.r, rgba.g, rgba.b, rgba.a
end

--- Get RGBA components.
-- If called on a gradient, this will not yield useful results.
-- @tparam[opt=1] opacity opacity to apply on returned color
-- @treturn number red
-- @treturn number green
-- @treturn number blue
-- @treturn number alpha

function Paint:get(opacity)
	return unpackRGBA(lib.ColorGet(self, opacity or 1))
end

--- Set to RGBA.
-- @tparam number r red
-- @tparam number g green
-- @tparam number b blue
-- @tparam[opt=1] number a alpha

function Paint:set(r, g, b, a)
	lib.ColorSet(self, r, g, b, a or 1)
end

--- types of paint
-- @table PaintType
-- @field none empty paint (transparent)
-- @field solid solid RGBA color
-- @field linear linear gradient
-- @field radial radial gradient
-- @field shader custom shader

local paintTypes = {
	none = lib.PAINT_NONE,
	solid = lib.PAINT_SOLID,
	linear = lib.PAINT_LINEAR_GRADIENT,
	radial = lib.PAINT_RADIAL_GRADIENT,
	shader = lib.PAINT_SHADER
}

--- Query paint type.
-- @treturn string one of the names in @{Paint.PaintType}

function Paint:getType()
	local t = lib.PaintGetType(self)
	for name, enum in pairs(paintTypes) do
		if t == enum then
			return name
		end
	end
end

--- Get number of colors.
-- Interesting for gradients only.
-- @treturn int number of colors in this paint.

function Paint:getNumColors()
	return lib.PaintGetNumColors(self)
end

--- Get i-th color stop.
-- Use this to retrieve gradient's colors.
-- @tparam int i 1-based index of color
-- @tparam[opt=1] number opacity to apply on returned color
-- @treturn number offset (between 0 and 1)
-- @treturn number red
-- @treturn number green
-- @treturn number blue
-- @treturn number alpha
-- @see Paint:addColorStop

function Paint:getColorStop(i, opacity)
	local s = lib.PaintGetColorStop(self, i - 1, opacity or 1)
	return s.offset, unpackRGBA(s.rgba)
end

function Paint:getGradientParameters()
	local t = lib.PaintGetType(self)
	local v = lib.GradientGetParameters(self).values
	if t == lib.PAINT_LINEAR_GRADIENT then
		return v[0], v[1], v[2], v[3]
	elseif t == lib.PAINT_RADIAL_GRADIENT then
		return v[0], v[1], v[2], v[3], v[4]
	end
end

--- Add new color stop.
-- @tparam number offset where to add this new color (0 <= offset <= 1)
-- @tparam number r red
-- @tparam number g green
-- @tparam number b blue
-- @tparam[opt=1] number a alpha
-- @see Paint:getColorStop

function Paint:addColorStop(offset, r, g, b, a)
	lib.GradientAddColorStop(self, offset, r, g, b, a or 1)
end

--- Clone.
-- @return Paint cloned @{Paint}

function Paint:clone()
	return lib.ClonePaint(self)
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
	local t = lib.PaintGetType(self)
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


local _sent = {}
tove._sent = _sent

--- Send data to custom shader.
-- Also see LÖVE's <a href="https://love2d.org/wiki/Shader:send">Shader:send</a>.
-- @usage
-- shader:send("t", love.timer.getTime())
-- @tparam string k name of uniform
-- @tparam ... data data to send

function Paint:send(k, ...)
	local args = lib.ShaderNextSendArgs(self)
	_sent[args.id][k] = {args.version, {...}}
end

--- Create new custom shader.
-- This shader can be used as a paint in fill and line colors. Your code needs to contain
-- a function called `COLOR` that takes a `vec2` position and returns a `vec4` color.
-- Currently, custom shaders are only supported for the `gpux` display mode.
-- Also see LÖVE's <a href="https://love2d.org/wiki/love.graphics.newShader">newShader</a>.
-- @tparam string source shader source code
-- @treturn Paint new shader
-- @usage
-- fillShader = tove.newShader([[
--	  uniform float time; 
--	  vec4 COLOR(vec2 pos) {
--        return vec4(1, 0, sin(t), 1);
--	   }
-- ]])
-- @see Graphics:setDisplay
-- @see Graphics:warmup

tove.newShader = function(source)
	local function releaseShader(self)
		local args = lib.ShaderNextSendArgs(self)
		_sent[args.id] = nil
		lib.ReleasePaint(self)
	end
	
	local shader = lib.NewShaderPaint(source)
	local args = lib.ShaderNextSendArgs(shader)
	_sent[args.id] = {}
	return ffi.gc(shader, releaseShader)
end


Paint._wrap = function(r, g, b, a)
	if ffi.istype("TovePaintRef", r) then
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
