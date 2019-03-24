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

local function fillGradientData(gradient)
	local n = gradient.numColors

	local matrixData = love.data.newByteData(env.matsize)
	gradient.matrix = matrixData:getPointer()
	gradient.arguments = nil

	local imageData = love.image.newImageData(1, n, "rgba8")
	gradient.colorsTexture = imageData:getPointer()
	gradient.colorsTextureRowBytes = imageData:getSize() / n
	gradient.colorsTextureHeight = n

	local s = 0.5 / n
	return {numColors = n, matrixData = matrixData,
		imageData = imageData, texture = nil, cscale = {s, 1 - 2 * s}}
end

local function newGradientTexture(d)
	d.texture = lg.newImage(d.imageData)
	d.texture:setFilter("nearest", "linear")
	d.texture:setWrap("clamp", "clamp")
end

local function reloadGradientTexture(d)
	d.texture:replacePixels(d.imageData)
end

local function sendColor(shader, uniform, rgba)
	shader:send(uniform, {rgba.r, rgba.g, rgba.b, rgba.a})
end

local function makeSendLUT(feed, shader)
	local bounds = feed.boundsByteData
	local lutX, lutY = unpack(feed.lookupTableByteData)
	local n = feed.data.lookupTableMeta.n
	local floatSize = ffi.sizeof("float[?]", 1)
	local meta = feed.lookupTableMetaByteData

	return function()
		shader:send("bounds", bounds)
		shader:send("lutX", lutX, 0, n[0] * floatSize)
		shader:send("lutY", lutY, 0, n[1] * floatSize)
		shader:send("tablemeta", meta)
	end
end

local function sendLineArgs(shader, data)
	shader:send("lineargs", {data.strokeWidth / 2, data.miterLimit})
end


local NullColorFeed = {}
NullColorFeed.__index = NullColorFeed

function NullColorFeed:beginInit()
end
function NullColorFeed:endInit(path)
end
function NullColorFeed:updateUniforms(chg1, path)
end


local ColorSend = {}
ColorSend.__index = ColorSend

local _flags = {
	fill = lib.CHANGED_FILL_STYLE,
	line = lib.CHANGED_LINE_STYLE
}

local _uniforms = {
	fill = {},
	line = {}
}

for _, name in ipairs({"color", "colors", "matrix", "cscale"}) do
	_uniforms.fill[name] = "fill" .. name
	_uniforms.line[name] = "line" .. name
end

local function newColorSend(shader, type, colorData)
	if shader == nil then
		return setmetatable({}, NullColorFeed)
	end
	return setmetatable({shader = shader, colorData = colorData,
		uniforms = _uniforms[type], flag = _flags[type]}, ColorSend)
end

function ColorSend:beginInit()
	local colorData = self.colorData
	if colorData.style >= 2 then
		self.gradientData = fillGradientData(colorData.gradient)
	end
end

function ColorSend:endInit(path)
	local gradientData = self.gradientData

	if gradientData ~= nil then
		newGradientTexture(gradientData)
	end

	local colorData = self.colorData
	local shader = self.shader
	local uniforms = self.uniforms

	if colorData.style == 1 then
		sendColor(shader, uniforms.color, colorData.rgba)
	elseif colorData.style >= 2 then
		shader:send(uniforms.colors, gradientData.texture)
		shader:send(uniforms.matrix, gradientData.matrixData)
		shader:send(uniforms.cscale, gradientData.cscale)
	end
end

function ColorSend:updateUniforms(chg1, path)
	if bit.band(chg1, self.flag) ~= 0 then
		local shader = self.shader
		local uniforms = self.uniforms
		local colorData = self.colorData
		if colorData.style == 1 then
			sendColor(shader, uniforms.color, colorData.rgba)
		elseif colorData.style >= 2 then
			local gradientData = self.gradientData
			reloadGradientTexture(gradientData)
			shader:send(uniforms.matrix, gradientData.matrixData)
			shader:send(uniforms.cscale, gradientData.cscale)
		end
	end
end



local MeshBands = {}
MeshBands.__index = MeshBands

-- 4 floats for each band vertex: (VertexPosition, VertexTexCoord)
MeshBands.FLOATS_PER_VERTEX = {4, 2, 4}

local function newMeshBands(data)
	return setmetatable({
		data = data,
		meshes = {nil, nil, nil},
		bandsVertexByteData = {nil, nil, nil},
		bandsVertexViewCache = {{}, {}, {}}
	}, MeshBands)
end

function MeshBands:initData()
	local data = self.data
	for i = 1, 3 do
		local bandDataSize = ffi.sizeof(
			"float[?]", data.maxBandsVertices * MeshBands.FLOATS_PER_VERTEX[i])
		local bandsByteData = love.data.newByteData(bandDataSize)
		self.bandsVertexByteData[i] = bandsByteData
		data.bandsVertices[i - 1] = bandsByteData:getPointer()
	end
end

function MeshBands:createMeshes(fillShader)
	local data = self.data

	self.meshes[1] = love.graphics.newMesh(
		{{"VertexPosition", "float", 2}, {"VertexTexCoord", "float", 2}},
		-- see FLOATS_PER_VERTEX
		data.maxBandsVertices,
		"triangles")

	self.meshes[2] = love.graphics.newMesh(
		{{"VertexPosition", "float", 2}},
		data.maxBandsVertices,
		"triangles")

	self.meshes[3] = love.graphics.newMesh(
		{{"VertexPosition", "float", 2}, {"VertexTexCoord", "float", 2}},
		-- see FLOATS_PER_VERTEX
		data.maxBandsVertices,
		"triangles")
	
	local lg = love.graphics

	local drawUnsafeBands = function(...)
		lg.setShader()
		lg.draw(self.meshes[2], ...)
	end

	return function(...)
		lg.stencil(drawUnsafeBands, "replace", 1)
	
		lg.setShader(fillShader)
		lg.draw(self.meshes[1], ...)
		lg.setStencilTest("greater", 0)
		lg.draw(self.meshes[3], ...)
		lg.setStencilTest()
	end
end

local function bandsGetVertexView(self, i)
	local size = self.data.numBandsVertices[i - 1]
	local cache = self.bandsVertexViewCache[i]
	local view = cache[size]
	if view == nil and size > 0 then
		view = love.data.newDataView(
			self.bandsVertexByteData[i], 0, ffi.sizeof(
				"float[?]", size * MeshBands.FLOATS_PER_VERTEX[i]))
		cache[size] = view
	end
	return view, size
end

function MeshBands:update()
	for i, mesh in ipairs(self.meshes) do
		local view, n = bandsGetVertexView(self, i)
		if n > 0 then
			mesh:setDrawRange(1, n)
			mesh:setVertices(view)
		end
	end
end


local GeometrySend = {}
GeometrySend.__index = GeometrySend

local function newGeometrySend(fillShader, lineShader, data, meshBand)
	return setmetatable({
		fillShader = fillShader,
		lineShader = lineShader,
		numLineSegments = 0,
		data = data,
		bands = meshBand and newMeshBands(data) or nil,
		boundsByteData = nil,
		listsImageData = nil,
		curvesImageData = nil,
		lookupTableByteData = {nil, nil},
		lookupTableMetaByteData = nil}, GeometrySend)
end

function GeometrySend:beginInit()
	local data = self.data

	-- note: everything we store as pointers into "data" (or some sub structure of
	-- it), needs to be referenced somewhere else; otherwise the gc will collect it.

	self.boundsByteData = love.data.newByteData(ffi.sizeof("ToveBounds"))
	data.bounds = self.boundsByteData:getPointer()

	local listsImageData = love.image.newImageData(
		data.listsTextureSize[0], data.listsTextureSize[1],
		ffi.string(data.listsTextureFormat))
	data.listsTextureRowBytes = listsImageData:getSize() / data.listsTextureSize[1]

	local curvesImageData = love.image.newImageData(
		data.curvesTextureSize[0], data.curvesTextureSize[1],
		ffi.string(data.curvesTextureFormat))
	data.curvesTextureRowBytes = curvesImageData:getSize() / data.curvesTextureSize[1]

	data.listsTexture = listsImageData:getPointer()
	data.curvesTexture = curvesImageData:getPointer()

	self.listsImageData = listsImageData
	self.curvesImageData = curvesImageData

	for i = 1, 2 do
		local lutData = love.data.newByteData(
			ffi.sizeof("float[?]", data.lookupTableSize))
		self.lookupTableByteData[i] = lutData
		data.lookupTable[i - 1] = lutData:getPointer()
	end

	self.lookupTableMetaByteData = love.data.newByteData(
		ffi.sizeof("ToveLookupTableMeta"))
	data.lookupTableMeta = self.lookupTableMetaByteData:getPointer()

	if self.bands ~= nil then
		self.bands:initData()
	end
end

local function makeDrawLine(feed)
	local lineShader = feed.lineShader
	local geometry = feed.data
	local lg = love.graphics

	local lineMesh = love.graphics.newMesh(
		{{0, -1}, {0, 1}, {1, -1}, {1, 1}}, "strip")
	local lineJoinMesh = love.graphics.newMesh(
		{{0, 0}, {1, -1}, {1, 1}, {-1, -1}, {-1, 1}}, "strip")

	local drawLine = function(...)
		local lineRuns = geometry.lineRuns
		if lineRuns == nil then
			return
		end

		lg.setShader(lineShader)

		local numSegments = feed.numLineSegments
		for i = 0, geometry.numSubPaths - 1 do
			local run = lineRuns[i]
			local numCurves = run.numCurves
			local numInstances = numSegments * numCurves

			lineShader:send("curve_index", run.curveIndex)
			lineShader:send("num_curves", numCurves)

			lineShader:send("draw_mode", 0)
			lg.drawInstanced(lineMesh, numInstances, ...)

			lineShader:send("draw_mode", 1)
			lg.drawInstanced(lineJoinMesh,
				numCurves - (run.isClosed and 0 or 1), ...)
		end
	end

	local bounds = ffi.cast("ToveBounds*", geometry.bounds)

	local drawTransparentLine = function(...)
		local border = geometry.strokeWidth + geometry.miterLimit * 2
		lg.stencil(drawLine, "replace", 1)
		lg.setStencilTest("greater", 0)
		local x0, y0 = bounds.x0, bounds.y0
		lineShader:send("draw_mode", 2)
		lg.rectangle("fill",
			x0 - border, y0 - border,
			bounds.x1 - x0 + 2 * border,
			bounds.y1 - y0 + 2 * border)
		lg.setStencilTest()
	end
	
	return function(...)
		if geometry.opaqueLine then
			drawLine(...)
		else
			drawTransparentLine()
		end
	end
end

function GeometrySend:endInit(lineStyle)
	local listsTexture = love.graphics.newImage(self.listsImageData)
	local curvesTexture = love.graphics.newImage(self.curvesImageData)
	listsTexture:setFilter("nearest", "nearest")
	listsTexture:setWrap("clamp", "clamp")
	curvesTexture:setFilter("nearest", "nearest")
	curvesTexture:setWrap("clamp", "clamp")
	self.listsTexture = listsTexture
	self.curvesTexture = curvesTexture

	local fillShader = self.fillShader
	local lineShader = self.lineShader
	local data = self.data

	if fillShader == nil then
		self.drawFill = function() end
	else
		if self.bands == nil then
			local mesh = love.graphics.newMesh(
				{{"VertexPosition", "float", 2}},
				{{0, 0}, {0, 1}, {1, 0}, {1, 1}}, "strip")

			local lg = love.graphics

			self.drawFill = function(...)
				lg.setShader(fillShader)
				lg.draw(mesh, ...)
			end
		else
			self.drawFill = self.bands:createMeshes(fillShader)
		end
	end

	if fillShader ~= lineShader and lineShader ~= nil then
		self.drawLine = makeDrawLine(self)
	else
		self.drawLine = function() end
	end

	if self.bands == nil then
		if fillShader ~= nil then
			self._sendUniforms = makeSendLUT(self, fillShader)
		else
			self._sendUniforms = function() end
		end
	else
		self._sendUniforms = function() self.bands:update() end
	end

	self._sendUniforms()

	if fillShader ~= nil then
		fillShader:send("lists", listsTexture)
		fillShader:send("curves", curvesTexture)
	end

	if lineShader ~= fillShader and lineShader ~= nil then
		lineShader:send("curves", curvesTexture)
	end

	if lineStyle >= 1 and lineShader ~= nil then
		sendLineArgs(lineShader, data)
	end
end

function GeometrySend:updateUniforms(flags)
	if bit.band(flags, lib.CHANGED_POINTS) ~= 0 then
		self._sendUniforms()

		if bit.band(flags, lib.CHANGED_LINE_ARGS) ~= 0 then
			local fillShader = self.fillShader
			local lineShader = self.lineShader

			if lineShader ~= nil then
				sendLineArgs(lineShader, self.data)
			elseif fillShader ~= nil then
				sendLineArgs(fillShader, self.data)
			end
		end

		self.listsTexture:replacePixels(self.listsImageData)
		self.curvesTexture:replacePixels(self.curvesImageData)
	end
end

return {
	newColorSend = newColorSend,
	newGeometrySend = newGeometrySend
}
