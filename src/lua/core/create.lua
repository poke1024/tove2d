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

local function createDrawMesh(mesh, x0, y0, s)
	if mesh == nil then
		return function (x, y, r, sx, sy)
		end
	else
		local draw = love.graphics.draw
		return function (x, y, r, sx, sy)
			sx = sx or 1
			sy = sy or 1
			x = (x or 0) + x0 * sx
			y = (y or 0) + y0 * sy
			draw(mesh, x, y, r or 0, s * sx, s * sy)
		end
	end
end

local function createDrawShaders(shaders, s)
	local g = love.graphics
	return function(x, y, r, sx, sy)
		g.push("transform")
		g.translate(x or 0, y or 0)
		g.rotate(r or 0)
		g.scale((sx or 1) * s, (sy or 1) * s)
		for _, s in ipairs(shaders) do
			s:draw()
		end
		g.pop()
		g.setShader()
	end
end

local function _updateBitmap(graphics)
	return graphics:fetchChanges(lib.CHANGED_ANYTHING) ~= 0
end

local function _updateFlatMesh(graphics)
	local flags = graphics:fetchChanges(lib.CHANGED_ANYTHING)
	if flags >= lib.CHANGED_GEOMETRY then
		return true
	end

	local update = 0
	if bit.band(flags, lib.CHANGED_FILL_STYLE + lib.CHANGED_LINE_STYLE) ~= 0 then
		update = bit.bor(update, lib.UPDATE_MESH_COLORS)
	end

	if bit.band(flags, lib.CHANGED_POINTS) ~= 0 then
		local mesh = graphics._cache.mesh
		if mesh:getUsage("points") == "dynamic" then
			update = bit.bor(update, lib.UPDATE_MESH_VERTICES)
		else
			tove.warn(graphics, "static mesh points changed.")
			return true
		end
	end

	if update ~= 0 then
		local mesh = graphics._cache.mesh
		mesh:retesselate(update)
	end

	return false
end

local function _updateShaders(graphics)
	if graphics:fetchChanges(lib.CHANGED_GEOMETRY) ~= 0 then
		return true
	end
	-- update here to support Graphics:cache().
	for _, s in ipairs(graphics._cache.shaders) do
		s:update()
	end
	return false
end

local create = {}

local function _noCache()
end

create.bitmap = function(self)
	local resolution = self._resolution
	local x0, y0, x1, y1 = self:computeAABB()

	x0 = math.floor(x0)
	y0 = math.floor(y0)
	x1 = math.ceil(x1)
	y1 = math.ceil(y1)

	if x1 - x0 < 1 or y1 - y0 < 1 then
		return {
			draw = function() end,
			update = _updateBitmap,
			cache = _noCache
		}
	end

	local imageData = self:rasterize(
		resolution * (x1 - x0),
		resolution * (y1 - y0), -x0 * resolution, -y0 * resolution,
		resolution)
	local image = love.graphics.newImage(imageData)
	image:setFilter("linear", "linear")
	return {
		mesh = image,
		draw = createDrawMesh(image, x0, y0, 1 / resolution),
		update = _updateBitmap,
		cache = _noCache
	}
end

local function _cacheFlatMesh(data, ...)
	data.mesh:cache(...)
end

local function _cacheShadedMesh(data, ...)
	for _, s in ipairs(data.shaders) do
		s.linkdata.fillMesh:cache(...)
	end
end

create.mesh = function(self)
	local resolution = self._resolution
	local display = self._display
	local cquality, holes = unpack(display.cquality)
	local usage = self._usage

	if usage["gradients"] == "fast" then
		local gref = self._ref
		local mesh = tove.newColorMesh(usage, function (cmesh, flags)
			local res = lib.GraphicsTesselate(
				gref, cmesh, resolution, cquality, holes, flags)
			return res.update
		end)
		local x0, y0, x1, y1 = self:computeAABB()
		return {
			mesh = mesh,
			draw = createDrawMesh(
				mesh:getMesh(), 0, 0, 1 / resolution),
			update = _updateFlatMesh,
			cache = _cacheFlatMesh
		}
	else
		local tess = function(path, fill, line, flags)
			local res = lib.PathTesselate(
				path, fill, line, resolution or 1, cquality, holes, flags)
			return res.update
		end
		local shaders = self:shaders(function (path)
			return _shaders.newMeshShader(path, tess, usage)
		end)
		return {
			shaders = shaders,
			draw = createDrawShaders(shaders, 1 / resolution),
			update = _updateShaders,
			cache = _cacheShadedMesh
		}
	end
end

create.curves = function(self)
	local shaders = self:shaders(_shaders.newPathShader)
	return {
		shaders = shaders,
		draw = createDrawShaders(shaders, 1),
		update = _updateShaders,
		cache = _noCache
	}
end

return function(self)
	local cache = self._cache
	if cache ~= nil then
		if not cache.update(self) then
			return cache
		end
		self._cache = nil
		cache = nil
	end

	local mode = self._display.mode
	local f = create[mode]
	if f ~= nil then
		self._cache = f(self)
	else
		error("invalid tove display mode: " .. (mode or "nil"))
	end
	self:fetchChanges(lib.CHANGED_ANYTHING) -- clear all changes
	return self._cache
end
