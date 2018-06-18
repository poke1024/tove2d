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

--!! import "core/cquality.lua" as cquality

local Paths = {}
Paths.__index = function (self, i)
	if i == "count" then
		return lib.GraphicsGetNumPaths(self._ref)
	else
		return ffi.gc(lib.GraphicsGetPath(self._ref, i), lib.ReleasePath)
	end
end

local Graphics = {}
Graphics.__index = Graphics

local function bind(methodName, libFuncName)
	Graphics[methodName] = function(self, ...)
		lib[libFuncName](self._ref, ...)
		return self
	end
end

tove.newGraphics = function(svg, size)
	local name = "unnamed"
	if tove.debug then
		name = "Graphics originally created at " .. debug.traceback()
	end
	local ref = ffi.gc(lib.NewGraphics(svg, "px", 72), lib.ReleaseGraphics)
	local graphics = setmetatable({
		_ref = ref,
		_cache = nil,
		_display = {mode = "bitmap"},
		_resolution = 1,
		_usage = {},
		_name = name,
		paths = setmetatable({_ref = ref}, Paths)}, Graphics)
	if type(size) == "number" then
		graphics:rescale(size)
	elseif size == nil then -- auto
		local x0, y0, x1, y1 = graphics:computeAABB()
		graphics:rescale(math.min(1024, math.max(x1 - x0, y1 - y0)))
	end
	return graphics
end

local function makeDisplay(mode, quality, usage)
	local clonedQuality = quality
	if type(quality) == "table" then
		clonedQuality = deepcopy(quality)
	else
		quality = quality or 1
	end
	return {mode = mode, quality = clonedQuality,
		cquality = cquality(quality, usage)}
end

function Graphics:setName(name)
	self._name = name
end

function Graphics:clone()
	local ref = lib.CloneGraphics(self._ref)
	local d = self._display
	local g = setmetatable({
		_ref = ref,
		_cache = nil,
		_display = makeDisplay(d.mode, d.quality, self._usage),
		_resolution = self._resolution,
		_usage = {},
		paths = setmetatable({_ref = ref}, Paths)}, Graphics)
	for k, v in pairs(self._usage) do
		g._usage[k] = v
	end
	return g
end

function Graphics:beginSubpath()
	return ffi.gc(lib.GraphicsBeginSubpath(self._ref), lib.ReleaseSubpath)
end
bind("closeSubpath", "GraphicsCloseSubpath")
bind("invertSubpath", "GraphicsInvertSubpath")

function Graphics:getCurrentPath()
	return ffi.gc(lib.GraphicsGetCurrentPath(self._ref), lib.ReleasePath)
end

bind("addPath", "GraphicsAddPath")

function Graphics:fetchChanges(flags)
	return lib.GraphicsFetchChanges(self._ref, flags)
end

function Graphics:moveTo(x, y)
	local t = self:beginSubpath()
	return newCommand(t, lib.SubpathMoveTo(t, x, y))
end

function Graphics:lineTo(x, y)
	local t = self:beginSubpath()
	return newCommand(t, lib.SubpathLineTo(t, x, y))
end

function Graphics:curveTo(...)
	local t = self:beginSubpath()
	return newCommand(t, lib.SubpathCurveTo(t, ...))
end

function Graphics:arc(x, y, r, a1, a2, ccw)
	lib.SubpathArc(self:beginSubpath(), x, y, r, a1, a2, ccw or false)
end

function Graphics:drawRect(x, y, w, h, rx, ry)
	local t = self:beginSubpath()
	return newCommand(t, lib.SubpathDrawRect(t, x, y, w, h or w, rx or 0, ry or 0))
end

function Graphics:drawCircle(x, y, r)
	local t = self:beginSubpath()
	return newCommand(t, lib.SubpathDrawEllipse(t, x, y, r, r))
end

function Graphics:drawEllipse(x, y, rx, ry)
	local t = self:beginSubpath()
	return newCommand(t, lib.SubpathDrawEllipse(t, x, y, rx, ry or rx))
end

function Graphics:setFillColor(r, g, b, a)
	local color = Paint._wrap(r, g, b, a)
	lib.GraphicsSetFillColor(self._ref, color._ref)
	return color
end

function Graphics:setLineDash(...)
	local dashes = {...}
	local n = #dashes
	local data = ffi.new("float[" .. tostring(n) .. "]", dashes)
	lib.GraphicsSetLineDash(self._ref, data, n)
	return self
end

bind("setLineWidth", "GraphicsSetLineWidth")
bind("setMiterLimit", "GraphicsSetMiterLimit")
bind("setLineDashOffset", "GraphicsSetLineDashOffset")

function Graphics:setLineColor(r, g, b, a)
	local color = Paint._wrap(r, g, b, a)
	lib.GraphicsSetLineColor(self._ref, color._ref)
	return color
end

bind("fill", "GraphicsFill")
bind("stroke", "GraphicsStroke")

function Graphics:computeAABB(mode)
	local bounds = lib.GraphicsGetBounds(self._ref, mode == "exact")
	return bounds.x0, bounds.y0, bounds.x1, bounds.y1
end

function Graphics:setDisplay(mode, quality)
	if self._cache ~= nil and mode == self._display.mode then
		if self._cache.updateQuality(quality) then
			self._display = makeDisplay(mode, quality, self._usage)
			return
		end
	end
	if mode == "flatmesh" then
		mode = "mesh"
		self._usage["gradients"] = "fast"
	end
	self._cache = nil
	self._display = makeDisplay(mode, quality, self._usage)
end

function Graphics:getDisplay()
	return self._display.mode, self._display.quality
end

function Graphics:getQuality()
	return self._display.quality
end

function Graphics:setResolution(resolution)
	if resolution ~= self._resolution then
		self._cache = nil
		self._resolution = resolution
	end
end

function Graphics:setUsage(what, usage)
	if usage ~= self._usage[what] then
		self._cache = nil
		self._usage[what] = usage
		if what == "points" then
			self._usage["triangles"] = usage
		end
		self._display.cquality = cquality(
			self._display.quality, self._usage)
	end
end

function Graphics:clearCache()
	self._cache = nil
end

function Graphics:cache(...)
	self:_create()
	self._cache.cache(self._cache, ...)
end


tove.transformed = function(graphics, ...)
	return {g = graphics, args = {...}}
end


function Graphics:set(arg, swl)
	local mt = getmetatable(arg)
	if mt == Graphics then
		lib.GraphicsSet(
			self._ref,
			arg._ref,
			false, 0, 0, 0, 1, 1, 0, 0, 0, 0)
	else
		local tx, ty, r, sx, sy, ox, oy, kx, ky = unpack(arg.args)
		sx = sx or 1
		sy = sy or sx

		lib.GraphicsSet(
			self._ref,
			arg.g._ref,
			swl or false,
			tx or 0, ty or 0,
			r or 0,
			sx, sy,
			ox or 0, oy or 0,
			kx or 0, ky or 0)
	end
end

function Graphics:rescale(size, center)
	local x0, y0, x1, y1 = self:computeAABB()
	local s = size / math.max(x1 - x0, y1 - y0)
	if center or true then
		self:set(tove.transformed(self, 0, 0, 0, s, s, (x0 + x1) / 2, (y0 + y1) / 2), true)
	else
		self:set(tove.transformed(self, 0, 0, 0, s, s), true)
	end
end

function Graphics:getNumTriangles()
	self:_create()
	if self._cache ~= nil and self._cache.mesh ~= nil then
		return #self._cache.mesh:getVertexMap() / 3
	else
		return 2
	end
end

function Graphics:draw(x, y, r, sx, sy)
	self:_create().draw(x, y, r, sx, sy)
end

function Graphics:rasterize(width, height, tx, ty, scale, quality)
	width = math.ceil(width)
	height = math.ceil(height)
	local data = lib.GraphicsRasterize(
		self._ref, width, height, tx or 0, ty or 0, scale or 1, quality)
	if data == nil then
		error(string.format("could rasterize image %d x %d", width, height))
	end
	local loveImageData = love.image.newImageData(
		width, height, "rgba8", ffi.string(data.pixels, width * height * 4))
	lib.DeleteImage(data)
	return loveImageData
end

function Graphics:animate(a, b, t)
	lib.GraphicsAnimate(self._ref, a._ref, b._ref, t or 0)
end

local orientations = {
	cw = lib.ORIENTATION_CW,
	ccw = lib.ORIENTATION_CCW
}

function Graphics:setOrientation(orientation)
	lib.GraphicsSetOrientation(self._ref, orientations[orientation])
end

function Graphics:clean(eps)
	lib.GraphicsClean(self._ref, eps or 0.0)
end

function Graphics:hit(x, y)
	local path = lib.GraphicsHit(self._ref, x, y)
	if path.ptr ~= nil then
		return ffi.gc(path, lib.ReleasePath)
	else
		return nil
	end
end

function Graphics:shaders(gen)
	local npaths = lib.GraphicsGetNumPaths(self._ref)
	local shaders = {}
	for i = 1, npaths do
		shaders[i] = gen(lib.GraphicsGetPath(self._ref, i))
	end
	return shaders
end

--!! import "core/create.lua" as create
Graphics._create = create
