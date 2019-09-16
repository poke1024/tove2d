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

local lg = love.graphics

local function createSendSync(shader, code, sync)
	local embedded = code.embedded
	local sent = {}
	for i = 0, 1 do
		local k = embedded[i]
		if k ~= 0 then
			table.insert(sent, tove._sent[k])
		end
	end
	if #sent == 0 then
		return sync
	else
		local version = 0
		local max = math.max
		return function()
			local nversion = version
			for _, s in ipairs(sent) do
				for name, values in pairs(s) do
					local v = values[1]
					if v > version then
						nversion = max(nversion, v)
						shader:send(name, unpack(values[2]))
					end
				end
			end
			version = nversion
			if sync ~= nil then
				sync()
			end
		end
	end
end

local function newGeometryFillShader(data, sync, fragLine, meshBand, debug)
	local geometry = data.geometry

	local code = lib.GetGPUXFillShaderCode(
		data, fragLine, meshBand, debug)

	local shader = lg.newShader(ffi.string(code.code))

	shader:send("constants", {geometry.maxCurves,
		geometry.listsTextureSize[0], geometry.listsTextureSize[1]})

	return shader, createSendSync(shader, code, sync)
end

local function newGeometryLineShader(data, sync)
	local style = data.color.line.style
	if style < 1 then
		return nil
	end

	local code = lib.GetGPUXLineShaderCode(data)

	local shader = lg.newShader(ffi.string(code.code))

	shader:send("max_curves", data.geometry.maxCurves)

	return shader, createSendSync(shader, code, sync)
end

local function newPaintShader(numPaints, numGradients)
	return lg.newShader(ffi.string(lib.GetPaintShaderCode(numPaints, numGradients)))
end

--!! import "feed.lua" as feed

local MeshShader = {}
MeshShader.__index = MeshShader

local function newMeshFeedData(name, graphics, tess, usage, resolution)
	local mesh = tove.newPaintMesh(name, usage)
	tess(mesh._tovemesh, lib.UPDATE_MESH_EVERYTHING)

	local link = ffi.gc(
		lib.NewColorFeed(graphics._ref, resolution), lib.ReleaseFeed)

	local alloc = lib.FeedGetColorAllocation(link)
	local matrixData = love.data.newByteData(
		(1 + alloc.numGradients) * env.matsize)
	local imageData = love.image.newImageData(
		alloc.numPaints, alloc.numColors, "rgba8")
	local colorsTexture = lg.newImage(imageData)
	local gradientData = ffi.new("ToveGradientData")
	gradientData.numColors = alloc.numColors
	gradientData.numGradients = alloc.numGradients
	gradientData.matrix = matrixData:getPointer()
	gradientData.matrixRows = env.matrows
	gradientData.colorsTexture = imageData:getPointer()
	gradientData.colorsTextureRowBytes = imageData:getSize() / alloc.numColors
	gradientData.colorsTextureHeight = alloc.numColors
	lib.FeedBindPaintIndices(link, gradientData)
	local paintShader = newPaintShader(alloc.numPaints, alloc.numGradients)
	colorsTexture:setFilter("nearest", "linear")
	colorsTexture:setWrap("clamp", "clamp")

	lib.FeedEndUpdate(link)

	colorsTexture:replacePixels(imageData)
	paintShader:send("matrix", matrixData)
	paintShader:send("colors", colorsTexture)
	paintShader:send("cstep", 0.5 / gradientData.colorsTextureHeight)

	lib.GraphicsClearChanges(graphics._ref)

	return {
		link = link,
		mesh = mesh,
		shader = paintShader,

		matrixData = matrixData,
		imageData = imageData,
		colorsTexture = colorsTexture
	}
end

local newMeshShader = function(name, graphics, tess, usage, resolution)
	return setmetatable({
		graphics = graphics, tess = tess, usage = usage, _name = name,
		resolution = resolution,
		linkdata = newMeshFeedData(
			name, graphics, tess, usage, resolution)}, MeshShader)
end

function MeshShader:getMesh()
	return self.linkdata.mesh
end

function MeshShader:recreate()
	tove.warn("full mesh recreation triggered in " .. tove._str(self._name))
	self.linkdata = newMeshFeedData(
		self.name, self.graphics, self.tess, self.usage, self.resolution)
end

function MeshShader:update()
	local linkdata = self.linkdata
	local graphics = self.graphics

	local flags = graphics:fetchChanges(lib.CHANGED_POINTS + lib.CHANGED_GEOMETRY)

	if bit.band(flags, lib.CHANGED_GEOMETRY) ~= 0 then
		self:recreate()
		return
	elseif bit.band(flags, lib.CHANGED_POINTS) ~= 0 then
		if self.usage["points"] == "static" then
			tove.slow("static mesh points changed in " .. tove._str(self._name))
			self:recreate()
			return
		end

		local tessFlags = lib.UPDATE_MESH_VERTICES
		if self.usage["triangles"] ~= "static" then
			tessFlags = bit.bor(tessFlags, lib.UPDATE_MESH_AUTO_TRIANGLES)
		end
		local updated = self.tess(linkdata.mesh._tovemesh, tessFlags)

		linkdata.mesh:updateVertices()
		if bit.band(updated, lib.UPDATE_MESH_TRIANGLES) ~= 0 then
			linkdata.mesh:updateTriangles()
		end
	end

	local link = linkdata.link
	local chg1 = lib.FeedBeginUpdate(link)

	if chg1 ~= 0 then
		if bit.band(chg1, lib.CHANGED_RECREATE) ~= 0 then
			self:recreate()
			return
		end

		lib.FeedEndUpdate(link)
		linkdata.colorsTexture:replacePixels(linkdata.imageData)
		local shader = linkdata.shader
		shader:send("matrix", linkdata.matrixData)
	end
end

function MeshShader:draw(...)
	local linkdata = self.linkdata
	local mesh = linkdata.mesh:getMesh()
	if mesh ~= nil then
		lg.setShader(linkdata.shader)
		lg.draw(mesh, ...)
		lg.setShader(nil)
	end
end

function MeshShader:warmup(...)
	warmupShader(self.linkdata.shader)
end

local function setLineQuality(linkData, lineQuality)
	linkData.lineQuality = lineQuality
	if linkData.data.color.line.style > 0 then
		if linkData.lineShader == linkData.fillShader then
			linkData.fillShader:send("line_quality", lineQuality)
		else
			local numSegments = math.max(2, math.ceil(50 * lineQuality))
			linkData.lineShader:send("segments_per_curve", numSegments)
			linkData.geometryFeed.numLineSegments = numSegments
		end
	end
end

local ComputeShader = {}
ComputeShader.__index = ComputeShader

local function parseQuality(q)
	local lineType = "fragment"
	local lineQuality = 1.0
	if type(q) == "table" then
		lineType = q[1] or "fragment"
		lineQuality = q[2] or 1.0
		if lineType == "vertex" then
			if not env.graphics.instancing then
				tove.warn("falling back on fragment line mode.")
				lineType = "fragment"
				lineQuality = 1
			end
		elseif lineType ~= "fragment" then
			error("line type must be \"fragment\" or \"vertex\".")
		end
	end
	return lineType, lineQuality
end

local function newComputeFeedData(path, quality, debug)
	local lineType, lineQuality = parseQuality(quality)
	local fragLine = (lineType == "fragment")
	local syncShader = nil

	-- enables slower mesh-based drawing instead of
	-- in-shader binary search in lookup tables.
	local meshBand = false

	local link = ffi.gc(
		lib.NewGeometryFeed(path, fragLine), lib.ReleaseFeed)
	local data = lib.FeedGetData(link)

	lib.FeedBeginUpdate(link)

	local fillShader = nil
	if fragLine or data.color.fill.style > 0 then
		fillShader, syncShader = newGeometryFillShader(
			data, nil, fragLine, meshBand, debug or false)
	end

	local lineShader
	if fragLine then
		lineShader = fillShader
	else
		lineShader, syncShader = newGeometryLineShader(data, syncShader)
	end

	local lineColorSend = feed.newColorSend(
		lineShader, "line", data.color.line)
	lineColorSend:beginInit()
	local fillColorSend = feed.newColorSend(
		fillShader, "fill", data.color.fill)
	fillColorSend:beginInit()

	local geometryFeed = feed.newGeometrySend(
		fillShader, lineShader, data.geometry, meshBand)
	geometryFeed:beginInit()

	lib.FeedEndUpdate(link)

	lineColorSend:endInit(path)
	fillColorSend:endInit(path)
	geometryFeed:endInit(data.color.line.style)

	local linkData = {
		link = link,
		data = data,
		fillShader = fillShader,
		lineShader = lineShader,
		syncShader = syncShader,
		lineType = lineType,
		quality = quality,
		lineQuality = lineQuality,
		geometryFeed = geometryFeed,
		lineColorSend = lineColorSend,
		fillColorSend = fillColorSend,
		paintOrder = lib.PathHasNormalFillStrokeOrder(path)
	}
	setLineQuality(linkData, lineQuality)
	return linkData
end

local newComputeShader = function(path, quality)
	return setmetatable({
		path = path,
		linkdata = newComputeFeedData(path, quality)
	}, ComputeShader)
end

function ComputeShader:update()
	local path = self.path
	local linkdata = self.linkdata
	local link = linkdata.link

	local sync = linkdata.syncShader
	if sync ~= nil then sync() end

	local chg1 = lib.FeedBeginUpdate(link)

	if bit.band(chg1, lib.CHANGED_RECREATE) ~= 0 then
		self.linkdata = newComputeFeedData(path, linkdata.quality)
		return
	end

	local chg2 = lib.FeedEndUpdate(link)

	linkdata.lineColorSend:updateUniforms(chg1, path)
	linkdata.fillColorSend:updateUniforms(chg1, path)

	linkdata.geometryFeed:updateUniforms(chg2)
end

function ComputeShader:updateQuality(quality)
	local linkdata = self.linkdata
	linkdata.quality = quality
	local lineType, lineQuality = parseQuality(quality)
	if lineType == linkdata.lineType then
		setLineQuality(linkdata, lineQuality)
		return true
	else
		return false
	end
end

function ComputeShader:draw(...)
	local linkdata = self.linkdata
	local feed = linkdata.geometryFeed
	if linkdata.paintOrder then
		feed.drawFill(...)
		feed.drawLine(...)
	else
		feed.drawLine(...)
		feed.drawFill(...)
	end
end

function ComputeShader:warmup(...)
	return self.linkdata.geometryFeed:warmup(...)
end

function ComputeShader:debug(curve)
	if curve == "off" then
		self.linkdata = newComputeFeedData(
			self.path, self.linkdata.quality, false)
		self.debugging = nil
		return
	end
	if self.debugging == nil then
		self.debugging = true

		self.linkdata = newComputeFeedData(
			self.path, self.linkdata.quality, true)
	end
	self.linkdata.fillShader:send("debug_curve", curve)	
end

return {
	newMeshShader = newMeshShader,
	newComputeShader = newComputeShader
}
