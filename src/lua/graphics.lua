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

--!! import "core/tesselator.lua" as createTesselator

local function gcpath(p)
	return p and ffi.gc(p, lib.ReleasePath)
end

local Paths = {}
Paths.__index = function (self, i)
	if i == "count" then
		return lib.GraphicsGetNumPaths(self._ref)
	else
		local t = type(i)
		if t == "number" then
			return gcpath(lib.GraphicsGetPath(self._ref, i))
		elseif t == "string" then
			return gcpath(lib.GraphicsGetPathByName(self._ref, i))
		end
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

local function newUsage()
	return {points = "static", colors = "static"}
end

tove.newGraphics = function(data, size)
	local svg = nil
	if type(data) == "string" then
		svg = data
	end
	local name = "unnamed"
	if tove.getReportLevel() == lib.TOVE_REPORT_DEBUG then
		name = "Graphics originally created at " .. debug.traceback()
	end
	local ref = ffi.gc(lib.NewGraphics(svg, "px", 72), lib.ReleaseGraphics)
	local graphics = setmetatable({
		_ref = ref,
		_cache = nil,
		_display = {mode = "texture", quality = {}},
		_resolution = 1,
		_usage = newUsage(),
		_name = name,
		paths = setmetatable({_ref = ref}, Paths)}, Graphics)
	if type(data) == "table" then
		for _, p in ipairs(data.paths) do
			graphics:addPath(tove.newPath(p))
		end
		graphics:setDisplay(unpack(data.display))
		for k, v in pairs(data.usage) do
			graphics:setUsage(k, v)
		end
	end
	if data then
		if size == "copy" then
			-- nothing
		elseif type(size) == "number" then
			graphics:rescale(size)
		elseif size == nil or size == "auto" then
			local x0, y0, x1, y1 = graphics:computeAABB()
			graphics:rescale(math.min(1024, math.max(x1 - x0, y1 - y0)))
		end
	end
	return graphics
end

function Graphics:clear()
	lib.GraphicsClear(self._ref)
end

local function attachTesselator(display, usage)
	if display.mode == "mesh" then
		display.tesselator = createTesselator(usage, unpack(display.quality))
	end
	return display
end

local function makeDisplay(mode, quality, usage)
	local clonedQuality = quality
	if type(quality) == "table" then
		clonedQuality = deepcopy(quality)
	else
		quality = quality or 1
	end
	return attachTesselator({mode = mode, quality = clonedQuality,
		tesselator = nil}, usage)
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
		_usage = newUsage(),
		_name = self._name,
		paths = setmetatable({_ref = ref}, Paths)}, Graphics)
	for k, v in pairs(self._usage) do
		g._usage[k] = v
	end
	return g
end

function Graphics:beginPath()
	return ffi.gc(lib.GraphicsBeginPath(self._ref), lib.ReleasePath)
end
bind("closePath", "GraphicsClosePath")

function Graphics:beginSubpath()
	return ffi.gc(lib.GraphicsBeginSubpath(self._ref), lib.ReleaseSubpath)
end
function Graphics:closeSubpath()
	lib.GraphicsCloseSubpath(self._ref, true)
end
bind("invertSubpath", "GraphicsInvertSubpath")

function Graphics:getCurrentPath()
	return gcpath(lib.GraphicsGetCurrentPath(self._ref))
end

bind("addPath", "GraphicsAddPath")

function Graphics:fetchChanges(flags)
	return lib.GraphicsFetchChanges(self._ref, flags)
end

function Graphics:moveTo(x, y)
	lib.GraphicsCloseSubpath(self._ref, false)
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

function Graphics:computeAABB(prec)
	local bounds = lib.GraphicsGetBounds(self._ref, prec == "high")
	return bounds.x0, bounds.y0, bounds.x1, bounds.y1
end

function Graphics:getWidth(prec)
	local bounds = lib.GraphicsGetBounds(self._ref, prec == "high")
	return bounds.x1 - bounds.x0
end

function Graphics:getHeight(prec)
	local bounds = lib.GraphicsGetBounds(self._ref, prec == "high")
	return bounds.y1 - bounds.y0
end

function Graphics:setDisplay(mode, ...)
	local quality = {...}
	if mode.__index == Graphics then
		local g = mode
		mode = g._display.mode
		quality = g._display.quality
	end
	if self._cache ~= nil and mode == self._display.mode then
		if self._cache.updateQuality(quality) then
			self._display = makeDisplay(mode, quality, self._usage)
			return
		end
	end
	self._cache = nil
	self._display = makeDisplay(mode, quality, self._usage)
	self:setResolution(1)
end

function Graphics:getDisplay()
	return self._display.mode, unpack(self._display.quality)
end

function Graphics:getQuality()
	return unpack(self._display.quality)
end

function Graphics:getUsage()
	return self._usage
end

function Graphics:setResolution(resolution)
	if resolution ~= self._resolution then
		self._cache = nil
		self._resolution = resolution
	end
end

function Graphics:setUsage(what, usage)
	if what.__index == Graphics then
		for k, v in pairs(what._usage) do
			self:setUsage(k, v)
		end
	end
	if usage ~= self._usage[what] then
		self._cache = nil
		self._usage[what] = usage
		if what == "points" then
			self._usage["triangles"] = usage
		end
		attachTesselator(self._display, self._usage)
	end
end

function Graphics:clearCache()
	self._cache = nil
end

function Graphics:cache(...)
	self:_create()
	self._cache.cache(self._cache, ...)
end

function Graphics:set(arg, swl)
	if getmetatable(arg) == tove.Transform then
		lib.GraphicsSet(
			self._ref,
			arg.s._ref,
			swl or false,
			unpack(arg.args))
	else
		lib.GraphicsSet(
			self._ref,
			arg._ref,
			false, 1, 0, 0, 0, 1, 0)
	end
end

function Graphics:transform(t)
	self:set(tove.transformed(self, t))
end

function Graphics:rescale(size, center)
	local x0, y0, x1, y1 = self:computeAABB()
	local r = math.max(x1 - x0, y1 - y0)
	if math.abs(r) > 1e-8 then
		local s = size / r
		if center or true then
			self:set(tove.transformed(
				self, 0, 0, 0, s, s, (x0 + x1) / 2, (y0 + y1) / 2), true)
		else
			self:set(tove.transformed(self, 0, 0, 0, s, s), true)
		end
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

function Graphics:rasterize(width, height, tx, ty, scale, settings)
	if width == "default" then
		settings = height -- second arg

		local resolution = self._resolution * tove._highdpi
		local x0, y0, x1, y1 = self:computeAABB("high")

		x0 = math.floor(x0)
		y0 = math.floor(y0)
		x1 = math.ceil(x1)
		y1 = math.ceil(y1)

		if x1 - x0 < 1 or y1 - y0 < 1 then
			return nil
		end

 		return self:rasterize(
			resolution * (x1 - x0),
			resolution * (y1 - y0),
			-x0 * resolution,
			-y0 * resolution,
			resolution, settings), x0, y0, x1, y1, resolution
	end

	width = math.ceil(width)
	height = math.ceil(height)

	local imageData = love.image.newImageData(width, height, "rgba8")
	local stride = imageData:getSize() / height
	lib.GraphicsRasterize(
		self._ref, imageData:getPointer(), width, height, stride,
		tx or 0, ty or 0, scale or 1, settings)

	return imageData
end

function Graphics:animate(a, b, t)
	lib.GraphicsAnimate(self._ref, a._ref, b._ref, t or 0)
end

local orientations = {
	cw = lib.TOVE_ORIENTATION_CW,
	ccw = lib.TOVE_ORIENTATION_CCW
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
		return gcpath(path)
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

function Graphics:setMonochrome(r, g, b)
	local paths = self.paths
	r = r or 1
	g = g or 1
	b = b or 1
    for i = 1, paths.count do
        local p = paths[i]
        if p:getFillColor() ~= nil then
            p:setFillColor(r, g, b)
        end
        p:setLineColor(r, g, b)
    end
end

function Graphics:setAnchor(dx, dy, mode)
	dx = dx or 0
	dy = dy or 0
	local x0, y0, x1, y1 = self:computeAABB(mode)
	local ox = ((dx <= 0 and x0 or x1) + (dx < 0 and x0 or x1)) / 2
	local oy = ((dy <= 0 and y0 or y1) + (dy < 0 and y0 or y1)) / 2
	self:set(tove.transformed(self, -ox, -oy))
end

function Graphics:serialize()
	local paths = self.paths
	local p = {}
	for i = 1, paths.count do
		p[i] = paths[i]:serialize()
	end
	local d = self._display
	return {version = 1,
		paths = p,
		display = {d.mode, d.quality},
		usage = self._usage}
end

function Graphics:debug(...)
	if self._cache ~= nil and self._cache.debug ~= nil then
		self._cache.debug(...)
	end
end

--!! import "core/create.lua" as create
Graphics._create = create
