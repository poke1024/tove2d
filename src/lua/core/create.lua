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

local function createDrawShaders(shaders)
	local setShader = love.graphics.setShader
	return function(...)
		for _, s in ipairs(shaders) do
			s:draw(...)
		end
		setShader()
	end
end

local function _updateBitmap(graphics)
	return graphics:fetchChanges(lib.CHANGED_ANYTHING) ~= 0
end

local function _makeDrawFlatMesh(mesh)
	return createDrawMesh(mesh:getMesh(), 0, 0, 1)
end

local function _updateFlatMesh(graphics)
	local flags = graphics:fetchChanges(lib.CHANGED_ANYTHING)
	if flags >= lib.CHANGED_GEOMETRY then
		return true -- recreate from scratch
	end

	if bit.band(flags, lib.CHANGED_FILL_STYLE + lib.CHANGED_LINE_STYLE) ~= 0 then
		return true -- recreate from scratch (might no longer be flat)
	end

	if bit.band(flags, lib.CHANGED_POINTS) ~= 0 then
		local mesh = graphics._cache.mesh
		if mesh:getUsage("points") ~= "dynamic" then
			tove.slow("static mesh points changed in " .. graphics._name)
		end
		local tessFlags = lib.UPDATE_MESH_VERTICES
		if graphics._usage["triangles"] == "dynamic" then
			tessFlags = bit.bor(tessFlags, lib.UPDATE_MESH_AUTO_TRIANGLES)
		end
		if mesh:retesselate(tessFlags) then
			-- retesselate indicated that the underlying mesh change. we need
			-- to update the draw function that is bound to the love mesh.
			graphics._cache.draw = _makeDrawFlatMesh(mesh)
		end
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

create.texture = function(self)
	local quality = self._display.quality
	local settings = ffi.new("ToveRasterizeSettings")
	lib.DefaultRasterizeSettings(settings)

	if quality[1] == "fast" then
		settings.quality = 0
	elseif (quality[1] or "best") == "best" then
		settings.quality = 1
	else
		error("illegal texture quality " .. tostring(quality[1]))
	end

	local imageData, x0, y0, x1, y1, resolution =
		self:rasterize("default", settings)

	if imageData == nil then
		return {
			draw = function() end,
			update = _updateBitmap,
			cache = _noCache
		}
	end

	local image = love.graphics.newImage(imageData)
	image:setFilter("linear", "linear")

	return {
		mesh = image,
		draw = createDrawMesh(image, x0, y0, 1 / resolution),
		update = _updateBitmap,
		updateQuality = function() return false end,
		cache = _noCache
	}
end

local function _cacheMesh(data, ...)
	data.mesh:cache(...)
end

create.mesh = function(self)
	local display = self._display
	local tsref = display.tesselator
	local usage = self._usage
	local name = self._name or "unnamed"

	local gref = self._ref
	local tess = function(cmesh, flags)
		return lib.TesselatorTessGraphics(tsref, gref, cmesh, flags)
	end

	if lib.GraphicsAreColorsSolid(self._ref) or
		usage["shaders"] == "avoid" then

		local mesh = tove.newColorMesh(name, usage, tess)
		local x0, y0, x1, y1 = self:computeAABB()
		return {
			mesh = mesh,
			draw = _makeDrawFlatMesh(mesh),
			update = _updateFlatMesh,
			updateQuality = function() return false end,
			cache = _cacheMesh
		}
	else
		local shader = _shaders.newMeshShader(
			name, self, tess, usage, 1)

		return {
			mesh = shader:getMesh(),
			draw = function(...) shader:draw(...) end,
			update = function() shader:update() end,
			updateQuality = function() return false end,
			cache = _cacheMesh
		}
	end
end

create.gpux = function(self)
	local quality = self._display.quality
	local shaders = self:shaders(function(path)
		return _shaders.newComputeShader(path, quality)
	end)
	return {
		shaders = shaders,
		draw = createDrawShaders(shaders),
		update = _updateShaders,
		updateQuality = function(quality)
			for _, s in ipairs(shaders) do
				if not s:updateQuality(quality) then
					return false
				end
			end
			return true
		end,
		debug = function(i, ...)
			shaders[i]:debug(...)
		end,
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
