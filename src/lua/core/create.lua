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
		if mesh:getUsage("points") == "static" then
			tove.slow("static mesh points changed in " .. tove._str(graphics._name))
		end
		local tessFlags = lib.UPDATE_MESH_VERTICES
		if graphics._usage["triangles"] ~= "static" then
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
	local palette  -- keep until rasterized
	local mode = quality[1]

	if mode == "fast" then
		lib.SetRasterizeSettings(settings, "fast", lib.NoPalette(), 1, 0, nil, 0)
	elseif (mode or "best") == "best" then
		local _, noise, spread, algo, palette = unpack(quality)
		local noiseData, noiseDataSize = nil,  0
		if type(noise) == "table" then
			local data, amount = unpack(noise)
			noise = amount
			noiseData = data:getPointer()
			noiseDataSize = math.floor(math.sqrt(data:getSize() / ffi.sizeof("float")))
		end
		if not lib.SetRasterizeSettings(
			settings, algo or "jarvis", palette or lib.NoPalette(), spread or 1,
			noise or 0.01, noiseData, noiseDataSize) then
			error(table.concat({"illegal rasterize settings:", unpack(quality)}, " "))
		end
	elseif mode == "retro" then
		local _, ipal, algo, spread, noise = unpack(quality)
		local size
		if ipal == nil then
			palette = lib.NoPalette()
		elseif type(ipal) == "string" then
			palette = lib.DefaultPalette(ipal)
		elseif type(ipal) == "table" then
			size = #ipal
			local colors = ffi.new("uint8_t[?]", size * 3)
			local i = 0
			for _, c in ipairs(ipal) do
				if type(c) == "table" then
					colors[i] = c[1] * 255
					colors[i + 1] = c[2] * 255
					colors[i + 2] = c[3] * 255
				elseif c:sub(1, 1) == '#' then
					colors[i] = tonumber("0x" .. c:sub(2, 3))
					colors[i + 1] = tonumber("0x" .. c:sub(4, 5))
					colors[i + 2] = tonumber("0x" .. c:sub(6, 7))
				end
				i = i + 3
			end
			palette = ffi.gc(lib.NewPalette(colors, size), lib.ReleasePalette)
		else
			palette = ipal
		end

		if not lib.SetRasterizeSettings(
			settings, algo or "stucki", palette, spread or 1, noise or 0, nil, 0) then
			error(table.concat({"illegal rasterize settings:", unpack(quality)}, " "))
		end
	else
		error("illegal texture quality " .. tostring(quality[1]))
	end

	local imageData, x0, y0, x1, y1, resolution =
		self:rasterize("default", settings)

	if imageData == nil then
		return {
			draw = function() end,
			update = _updateBitmap,
			cacheKeyFrame = _noCache,
			setCacheSize = _noCache
		}
	end

	local image = love.graphics.newImage(imageData)
	image:setFilter("linear", "linear")

	return {
		mesh = image,
		draw = createDrawMesh(image, x0, y0, 1 / resolution),
		warmup = function() end,
		update = _updateBitmap,
		updateQuality = function() return false end,
		cacheKeyFrame = _noCache,
		setCacheSize = _noCache
	}
end

local function _cacheMeshKeyFrame(data)
	data.mesh:cacheKeyFrame()
end

local function _setMeshCashSize(data, size)
	data.mesh:setCacheSize(size)
end

create.mesh = function(self)
	local display = self._display
	local tsref = display.tesselator
	local usage = self._usage
	local name = self._name

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
			warmup = function() end,
			update = _updateFlatMesh,
			updateQuality = function() return false end,
			cacheKeyFrame = _cacheMeshKeyFrame,
			setCacheSize = _setMeshCashSize
		}
	else
		local shader = _shaders.newMeshShader(
			name, self, tess, usage, 1)

		return {
			mesh = shader:getMesh(),
			draw = function(...) shader:draw(...) end,
			warmup = function()
				if shader:warmup() then
					return 1
				end
			end,
			update = function() shader:update() end,
			updateQuality = function() return false end,
			cacheKeyFrame = _cacheMeshKeyFrame,
			setCacheSize = _setMeshCashSize
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
		warmup = function(...)
			for i, s in ipairs(shaders) do
				if s:warmup(...) then
					return i / #shaders
				end
			end
		end,
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
		cacheKeyFrame = _noCache,
		setCacheSize = _noCache
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
