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

--- A vector graphics.
-- @classmod Graphics
-- @set sort=true

--!! import "core/tesselator.lua" as createTesselator

local function gcpath(p)
	return p and ffi.gc(p, lib.ReleasePath)
end

---@{tove.List} of @{Path}s in this @{Graphics}
-- @table[readonly] paths

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

--- Create a new Graphics.
-- @usage
-- local svg = love.filesystem.read("MyGraphics.svg")
-- g = tove.newGraphics(svg, 200)  -- scale to 200 pixels
-- @tparam string|{Path,...} data either an SVG string or a table of @{Path}s
-- @tparam[opt="auto"] int|string size the size to scale the @{Graphics} to.
-- Use `"auto"` (scale longest side to 1024),
-- `"copy"` (do not scale and use original size)
-- or a number (the number of pixels to scale to).
-- @treturn Graphics a new Graphics
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
		_name = ffi.gc(lib.NewName(name), lib.ReleaseName),
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

--- Remove all @{Path}s.
-- Effectively empties this @{Graphics} of all drawable content.
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

--- Set name.
-- That name will show up in error messages concering this @{Graphics}.
-- @tparam string name new name of this @{Graphics}
function Graphics:setName(name)
	lib.NameSet(self._name, name)
end

--- Create a deep copy.
-- This cloned copy will include new copies of all @{Path}s.
-- @treturn Graphics cloned @{Graphics}.
function Graphics:clone()
	local ref = lib.CloneGraphics(self._ref, true)
	local d = self._display
	local g = setmetatable({
		_ref = ref,
		_cache = nil,
		_display = makeDisplay(d.mode, d.quality, self._usage),
		_resolution = self._resolution,
		_usage = newUsage(),
		_name = ffi.gc(lib.CloneName(self._name), lib.ReleaseName),
		paths = setmetatable({_ref = ref}, Paths)}, Graphics)
	for k, v in pairs(self._usage) do
		g._usage[k] = v
	end
	return g
end

--- Start a fresh @{Path} for drawing.
-- The previously active @{Path} will get closed but won't
-- be made into loop. Subsequent drawing commands like @{Graphics:lineTo}
-- will draw into the newly started @{Path}.
-- @treturn Path new active @{Path} in this @{Graphics}
-- @usage
-- g:moveTo(10, 10)
-- g:lineTo(20, 15)
-- g:lineTo(5, 0)
-- g:beginPath()  -- do not connect lines above into a loop
-- g:moveTo(50, 50)
-- g:lineTo(60, 55)
function Graphics:beginPath()
	return ffi.gc(lib.GraphicsBeginPath(self._ref), lib.ReleasePath)
end

--- Close the current @{Path}, thereby creating a loop.
-- Subsequent drawing commands like @{Graphics:lineTo} will automatically
-- start a fresh @{Path}.
-- @function closePath
-- @usage
-- g:moveTo(10, 10)
-- g:lineTo(20, 15)
-- g:lineTo(5, 0)
-- g:closePath() -- connect lines above into a loop

bind("closePath", "GraphicsClosePath")

--- Inside the current @{Path}, start a fresh @{Subpath}.
-- The previously active @{Subpath} will not be made into a loop.
-- Subsequent drawing commands like @{Graphics:lineTo} will draw into the
-- new @{Subpath} inside the same @{Path}.

function Graphics:beginSubpath()
	return ffi.gc(lib.GraphicsBeginSubpath(self._ref), lib.ReleaseSubpath)
end

--- Inside the current @{Path}, close the current @{Subpath}, thereby creating a loop.
-- Subsequent drawing commands like @{Graphics:lineTo} will automatically
-- start a fresh @{Subpath} inside the same @{Path}. Use this
-- method if you want to draw holes.

function Graphics:closeSubpath()
	lib.GraphicsCloseSubpath(self._ref, true)
end
bind("invertSubpath", "GraphicsInvertSubpath")

--- Get the current @{Path} for drawing.
-- @treturn Path the currently active @{Path} that gets drawn into.

function Graphics:getCurrentPath()
	return gcpath(lib.GraphicsGetCurrentPath(self._ref))
end

--- Add a @{Path}.
-- The new @{Path} will be appended after other existing @{Path}s.
-- @tparam Path path @{Path} to append
-- @function addPath

bind("addPath", "GraphicsAddPath")

function Graphics:fetchChanges(flags)
	return lib.GraphicsFetchChanges(self._ref, flags)
end

--- Move to position (x, y).
-- Automatically starts a fresh @{Subpath} if necessary. 
-- @tparam number x new x coordinate
-- @tparam number y new y coordinate
-- @treturn Command move command

function Graphics:moveTo(x, y)
	lib.GraphicsCloseSubpath(self._ref, false)
	local t = self:beginSubpath()
	return newCommand(t, lib.SubpathMoveTo(t, x, y))
end

--- Draw a line to (x, y).
-- @usage
-- g:moveTo(10, 20)
-- g:lineTo(5, 30)
-- @tparam number x x coordinate of line's end position
-- @tparam number y y coordinate of line's end position
-- @treturn Command line command

function Graphics:lineTo(x, y)
	local t = self:beginSubpath()
	return newCommand(t, lib.SubpathLineTo(t, x, y))
end

--- Draw a cubic Bézier curve to (x, y).
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
-- @treturn Command curve command

function Graphics:curveTo(...)
	local t = self:beginSubpath()
	return newCommand(t, lib.SubpathCurveTo(t, ...))
end

--- Draw an arc.
-- @tparam number x x coordinate of centre of arc
-- @tparam number y y coordinate of centre of arc
-- @tparam number r radius of arc
-- @tparam number startAngle angle at which to start drawing
-- @tparam number endAngle angle at which to stop drawing
-- @tparam bool ccw true if drawing between given angles is counterclockwise

function Graphics:arc(x, y, r, startAngle, endAngle, ccw)
	lib.SubpathArc(self:beginSubpath(), x, y, r, startAngle, endAngle, ccw or false)
end

--- Draw a rectangle.
-- @tparam number x x coordinate of top left corner
-- @tparam number y y coordinate of top left corner
-- @tparam number w width
-- @tparam number h height
-- @tparam number rx horizontal roundness of corners
-- @tparam number ry vertical roundness of corners
-- @treturn Command rectangle command

function Graphics:drawRect(x, y, w, h, rx, ry)
	local t = self:beginSubpath()
	return newCommand(t, lib.SubpathDrawRect(t, x, y, w, h or w, rx or 0, ry or 0))
end

--- Draw a circle.
-- @tparam number x x coordinate of centre
-- @tparam number y y coordinate of centre
-- @tparam number r radius
-- @treturn Command circle command

function Graphics:drawCircle(x, y, r)
	local t = self:beginSubpath()
	return newCommand(t, lib.SubpathDrawEllipse(t, x, y, r, r))
end

--- Draw an ellipse.
-- @tparam number x x coordinate of centre
-- @tparam number y y coordinate of centre
-- @tparam number rx horizontal radius
-- @tparam number ry vertical radius
-- @treturn Command ellipse command

function Graphics:drawEllipse(x, y, rx, ry)
	local t = self:beginSubpath()
	return newCommand(t, lib.SubpathDrawEllipse(t, x, y, rx, ry or rx))
end

--- Set fill color.
-- Will affect subsequent calls to @{Graphics:fill}.
-- @usage
-- g:setFillColor(0.8, 0, 0, 0.5)  -- transparent reddish
-- g:setFillColor("#00ff00")  -- opaque green
-- @tparam number|string|Paint|nil r red
-- @tparam[optchain] number g green
-- @tparam[optchain] number b blue
-- @tparam[optchain=0] number a alpha

function Graphics:setFillColor(r, g, b, a)
	local color = Paint._wrap(r, g, b, a)
	lib.GraphicsSetFillColor(self._ref, color)
	return color
end

--- Set line dash pattern.
-- Takes a list of dash lengths. Will affect lines drawn with subsequent calls to @{Graphics:stroke}.
-- @usage
-- g:setLineDash(0.5, 5.0, 1.5, 5.0)
-- @tparam number... widths lengths of dashes

function Graphics:setLineDash(...)
	local dashes = {...}
	local n = #dashes
	local data = ffi.new("float[" .. tostring(n) .. "]", dashes)
	lib.GraphicsSetLineDash(self._ref, data, n)
	return self
end

--- Set line width.
-- Will affect lines drawn with subsequent calls to @{Graphics:stroke}.
-- @tparam number width new line width
-- @function setLineWidth

bind("setLineWidth", "GraphicsSetLineWidth")

--- Set miter limit.
-- Will affect lines drawn with calls to @{Graphics:stroke}.
-- See Mozilla for a good <a href="https://developer.mozilla.org/en-US/docs/Web/SVG/Attribute/stroke-miterlimit">
-- description of miter limits</a>.
-- @tparam number miterLimit new miter limit
-- @function setMiterLimit

bind("setMiterLimit", "GraphicsSetMiterLimit")

--- Set line dash offset.
-- Will affect lines drawn with calls to @{Graphics:stroke}.
-- See Mozilla for a good <a href="https://developer.mozilla.org/en-US/docs/Web/SVG/Attribute/stroke-dashoffset">
-- description of line dash offsets</a>.
-- @tparam number offset new line dash offset
-- @function setLineDashOffset

bind("setLineDashOffset", "GraphicsSetLineDashOffset")

--- Rotate elements.
-- Will recusively left rotate elements in this @{Graphics} such that items
-- found at index k will get moved to index 0.
-- @tparam string w what to rotate: either "path", "subpath", "point" or "curve"
-- @tparam int k how much to rotate
-- @see Path:rotate

function Graphics:rotate(w, k)
	lib.GraphicsRotate(tove.elements[w], k)
end

--- Set line color.
-- Will affect subsequent calls to @{Graphics:stroke}.
-- @see setFillColor
-- @tparam number|string|Paint|nil r red
-- @tparam[optchain] number g green
-- @tparam[optchain] number b blue
-- @tparam[optchain=0] number a alpha

function Graphics:setLineColor(r, g, b, a)
	local color = Paint._wrap(r, g, b, a)
	lib.GraphicsSetLineColor(self._ref, color)
	return color
end

--- Add fill to shape.
-- @usage
-- g:moveTo(10, 20)
-- g:curveTo(7, 5, 10, 3, 50, 30)
-- g:fill()
-- @see setFillColor
-- @function fill

bind("fill", "GraphicsFill")

--- Add stroke to shape.
-- g:moveTo(10, 20)
-- g:curveTo(7, 5, 10, 3, 50, 30)
-- g:stroke()
-- @see setLineColor
-- @function stroke

bind("stroke", "GraphicsStroke")

--- Compute bounding box.
-- `"low"` precision will not correctly compute certain line boundaries (miters),
-- however it will be exact for fills and much faster than `"high"`.
-- @tparam[opt="high"] string prec precision of bounding box: `"high"` (i.e. exact)  or `"low"` (faster)
-- @treturn number x0 (left-most x)
-- @treturn number y0 (top-most y)
-- @treturn number x1 (right-most x)
-- @treturn number y1 (bottom-most y)

function Graphics:computeAABB(prec)
	local bounds = lib.GraphicsGetBounds(self._ref, prec == "high")
	return bounds.x0, bounds.y0, bounds.x1, bounds.y1
end

--- Get width.
-- @tparam[opt="high"] string prec precision: `"high"` (i.e. exact) or `"low"` (faster)
-- @treturn number width of contents in this @{Graphics}
-- @see computeAABB

function Graphics:getWidth(prec)
	local bounds = lib.GraphicsGetBounds(self._ref, prec == "high")
	return bounds.x1 - bounds.x0
end

--- Get height.
-- @tparam[opt="high"] string prec precision: `"high"` (i.e. exact)  or `"low"` (faster)
-- @treturn number height of contents in this @{Graphics}
-- @see computeAABB

function Graphics:getHeight(prec)
	local bounds = lib.GraphicsGetBounds(self._ref, prec == "high")
	return bounds.y1 - bounds.y0
end

--- Set display mode.
-- Configures in detail how this @{Graphics} gets rendered to the
-- screen when you call @{Graphics:draw}. Note that this is a very expensive
-- operation. You should only call it once for each @{Graphics} and
-- never in your draw loop.
-- @tparam string mode either one of `"texture"`, `"mesh"` or `"gpux"`, see @{display}
-- @tparam[opt] ... args detailed quality configuration for specified mode, see @{display}
-- @see Graphics:getDisplay
-- @see Graphics:getQuality
-- @see display

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

--- Get display mode.
-- @treturn string mode the current mode as set with @{Graphics:setDisplay}.
-- @treturn ... the detailed quality configuration as set with @{Graphics:setDisplay}.
-- @see Graphics:setDisplay
-- @see Graphics:getQuality

function Graphics:getDisplay()
	return self._display.mode, unpack(self._display.quality)
end

--- Get display quality.
-- @treturn ... the detailed quality configuration as set with @{Graphics:setDisplay}.
-- @see Graphics:setDisplay
-- @see Graphics:getDisplay

function Graphics:getQuality()
	return unpack(self._display.quality)
end

--- Get usage.
-- @treturn table a table containing the configured usage for each dimension as key,
-- as set with @{Graphics:setUsage}.
-- @see Graphics:setUsage

function Graphics:getUsage()
	return self._usage
end

--- Set resolution.
-- Determines how much you can scale this @{Graphics} when drawing without it looking bad. Affects the "texture"
-- and "mesh" display modes. Note that this will not affect the size of the @{Graphics} when drawing with scale 1.
-- @usage
-- g:setDisplay("texture")
-- g:draw(0, 0) -- looks crisp
-- g:draw(0, 0, 2) -- at scale 2, things get fuzzy
-- g:setResolution(2)
-- g:draw(0, 0) -- same size as before
-- g:draw(0, 0, 2) -- at scale 2, things still look good
-- g:draw(0, 0, 3) -- now things get fuzzy
-- @tparam number resolution scale factor up to which @{Graphics} will look crisp when scaled in @{Graphics:draw}
-- (or through a Transform)
-- @see Graphics:draw

function Graphics:setResolution(resolution)
	if resolution ~= self._resolution then
		self._cache = nil
		self._resolution = resolution
	end
end

--- Set usage.
-- Indicates which elements of the @{Graphics} you want to change (i.e. animate) at runtime.
-- Currently, this method only has an effect for display mode "mesh".
-- Hint: to force meshes to not use shaders, use g:setUsage("shaders", "avoid").
-- @usage
-- g:setUsage("points", "stream") -- animate points on each frame
-- g:setUsage("colors", "stream") -- animate colors on each frame
-- @tparam string what either one of "points" or "colors"
-- @tparam string usage usually either one of "static", "dynamic" or "stream" (see <a href="https://love2d.org/wiki/SpriteBatchUsage">love2d docs on mesh usage</a>)
-- @see Graphics:setDisplay
-- @see Graphics:getUsage

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

--- Clear internal drawing cache.
-- Recreates internal drawing objects (e.g. meshes). You shouldn't need this.
-- Please note that this is NOT related to @{Graphics:cacheKeyFrame}.

function Graphics:clearCache()
	self._cache = nil
end

--- Cache the current shape as key frame.
-- In "mesh" display mode, indicate to TÖVE that the current shape of this
-- @{Graphics} will recur in your animations as key frame. This allows TÖVE
-- to precompute a triangulation for the current shape and reuse that later
-- on. It also tells TÖVE to never evict this specific triangulation from
-- its internal cache.
-- @usage
-- setKeyFrame1(g)
-- g:cacheKeyFrame()
-- setKeyFrame2(g)
-- g:cacheKeyFrame()
-- @see setCacheSize

function Graphics:cacheKeyFrame()
	self:_create()
	self._cache.cacheKeyFrame(self._cache)
end

--- Set internal key frame cache size.
-- Tell TÖVE how many triangulations to cache in "mesh" mode. This number
-- should be high enough to allow TÖVE to never cache all necessary
-- triangulations during an animation cycle. Obviously, this number
-- depends on the shape and kind of your animation. Note that a
-- high number will not automatically improve performance, as cache
-- entries need to be checked and evaluated on each frame.
-- @see cacheKeyFrame

function Graphics:setCacheSize(size)
	self:_create()
	self._cache.setCacheSize(self._cache, size)
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

--  @set no_summary=true
--- Transform this @{Graphics}.
-- Use this to transform all points in a @{Graphics} with an affine matrix. Don't use this for
-- drawing a rotated or scaled version of your @{Graphics}, use @{Graphics:draw} instead. 
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
-- @see Path:transform
-- @see Subpath:transform
--  @set no_summary=false

function Graphics:transform(...)
	self:set(tove.transformed(self, ...))
end

--- Rescale to given size.
-- @tparam number size target size to scale to
-- @tparam[opt] bool center center the @{Graphics} so that calling @{Graphics:draw}
-- will always place it around the given draw position (defaults to true)
-- @see Graphics:draw

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

--- Get number of triangles.
-- @treturn int number of triangles involved in drawing this @{Graphics}.

function Graphics:getNumTriangles()
	self:_create()
	if self._cache ~= nil and self._cache.mesh ~= nil then
		return self._cache.mesh:getNumTriangles()
	else
		return 2
	end
end

--- Draw to screen.
-- The details of how the rendering happens can get configured through
-- @{Graphics:setDisplay}.
-- @usage
-- g:draw(120, 150, 0.1)  -- draw at (120, 150) with some rotation
-- @tparam number x the x-axis position to draw at
-- @tparam number y the y-axis position to draw at
-- @tparam[opt=0] number r orientation in radians
-- @tparam[opt=1] number sx scale factor in x
-- @tparam[opt=1] number sy scale factor in y
-- @see Graphics:setDisplay
-- @see Graphics:setResolution
-- @see Graphics:rescale

function Graphics:draw(x, y, r, sx, sy)
	self:_create().draw(x, y, r, sx, sy)
end

--- Warm-up shaders.
-- Instructs TÖVE to compile all shaders involved in drawing this
-- @{Graphics} with the current display settings. On some platforms,
-- compiling `gpux` shaders can take a long time. @{Graphics:warmup} will quit
-- after a certain time and allow you to display some progress info.
-- In these cases you need to call it again until it returns nil.
-- @treturn number|nil nil if all shaders were compiled, otherwise
-- a number between 0 and 1 indicating percentage of shaders compiled
-- so far.
-- @see Graphics:setDisplay

function Graphics:warmup()
	local r = self:_create().warmup()
	love.graphics.setShader()
	return r
end

--  @set no_summary=true
--- Rasterize as bitmap.
-- @tparam number|string width desired width in pixels, or "default" to use @{Graphics} internal size
-- @tparam number height desired height in pixels
-- @tparam[opt=0] number tx x-translation of content
-- @tparam[optchain=0] number ty y-translation of content
-- @tparam[optchain=1] number scale scale factor of content
-- @tparam[optchain=nil] nil settings internal quality settings, not yet public
-- @treturn ImageData the rasterized image.
--  @set no_summary=false

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

--- Animate between two @{Graphics}.
-- Makes this @{Graphics} to an interpolated inbetween.
-- @tparam Graphics a @{Graphics} at t == 0
-- @tparam Graphics b @{Graphics} at t == 1
-- @tparam number t interpolation (0 <= t <= 1)
-- @see Path:animate

function Graphics:animate(a, b, t)
	lib.GraphicsAnimate(self._ref, a._ref, b._ref, t or 0)
end

--- Warp points.
-- Allows you to change the shape of a vector graphics in a way
-- that animates well.
-- The specified function applies a space mapping: it gets
-- called for specific positions and returns new ones.
-- TÖVE will call the function for non-control-points only
-- and decide where to put control points itself based on
-- an estimation of previous curve form. The curvature argument
-- lets you estimate and change how curvy the shape is at a
-- given (non-control) point.
-- @tparam function f takes (x, y, curvature) and returns the same
-- @see Path:warp

function Graphics:warp(f)
	local paths = self.paths
    for i = 1, paths.count do
		paths[i]:warp(f)
	end
end

local orientations = {
	cw = lib.TOVE_ORIENTATION_CW,
	ccw = lib.TOVE_ORIENTATION_CCW
}

--- Set orientation of all @{Subpath}s.
-- @tparam string orientation "cw" for clockwise, "ccw" for counterclockwise

function Graphics:setOrientation(orientation)
	lib.GraphicsSetOrientation(self._ref, orientations[orientation])
end

--- Clean paths.
-- Removes duplicate or collinear points which can cause problems
-- with various algorithms (e.g. triangulation) in TÖVE. If you
-- have messy vector input, this can be a good first step after loading.
-- @tparam[opt=1e-2] number eps triangle area at which triangles get collapsed

function Graphics:clean(eps)
	lib.GraphicsClean(self._ref, eps or 1e-2)
end

--- Check if inside.
-- Returns true if the given point is inside any of the @{Graphics}'s @{Path}s.
-- @tparam number x x coordinate of tested point
-- @tparam number y y coordinate of tested point

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

--- Set all colors to one.
-- Will change all fills and line colors to the given color,
-- making the @{Graphics} appear monochrome.
-- @tparam number r red
-- @tparam number g green
-- @tparam number b blue

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

function Graphics:setAnchor(dx, dy, prec)
	dx = dx or 0
	dy = dy or 0
	local x0, y0, x1, y1 = self:computeAABB(prec)
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
