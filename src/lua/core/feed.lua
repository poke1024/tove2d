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

	local matrixData = love.data.newByteData(env.mat3.size)
	gradient.matrix = matrixData:getPointer()
	gradient.arguments = nil

	local imageData = love.image.newImageData(1, n, "rgba8")
	gradient.colorsTexture = imageData:getPointer()
	gradient.colorsTextureRowBytes = imageData:getSize() / n

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

local function sendLUT(feed, shader)
	shader:send("lut", feed.lookupTableByteData)
	shader:send("tablemeta", feed.lookupTableMetaByteData)
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
function NullColorFeed:update(chg1, path)
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


local GeometrySend = {}
GeometrySend.__index = GeometrySend

local function newGeometrySend(fillShader, lineShader, data)
	return setmetatable({
		fillShader = fillShader,
		lineShader = lineShader,
		data = data,
		boundsByteData = nil,
		listsImageData = nil,
		curvesImageData = nil,
		lookupTableByteData = nil,
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

	self.lookupTableByteData = love.data.newByteData(
		ffi.sizeof("float[?]", data.lookupTableSize * 2))
	data.lookupTable = self.lookupTableByteData:getPointer()

	self.lookupTableMetaByteData = love.data.newByteData(
		ffi.sizeof("ToveLookupTableMeta"))
	data.lookupTableMeta = self.lookupTableMetaByteData:getPointer()
end

function GeometrySend:endInit(lineStyle)
	local listsTexture = love.graphics.newImage(self.listsImageData)
	local curvesTexture = love.graphics.newImage(self.curvesImageData)
	listsTexture:setFilter("nearest", "nearest")
	curvesTexture:setFilter("nearest", "nearest")
	self.listsTexture = listsTexture
	self.curvesTexture = curvesTexture

	-- create mesh

	local fillShader = self.fillShader
	local lineShader = self.lineShader
	local data = self.data

	-- note: since bezier curves stay in the convex hull of their control points, we
	-- could triangulate the mesh here. with strokes, it gets difficult though.

	self.mesh = love.graphics.newMesh(
		{{"VertexPosition", "float", 2}},
		{{0, 0}, {0, 1}, {1, 0}, {1, 1}}, "strip")

	if fillShader ~= lineShader and lineShader ~= nil then
		self.lineMesh = love.graphics.newMesh(
			{{0, -1}, {0, 1}, {1, -1}, {1, 1}}, "strip")
		self.lineJoinMesh = love.graphics.newMesh(
			{{0, 0}, {1, -1}, {1, 1}, {-1, -1}, {-1, 1}}, "strip")
	end

	sendLUT(self, fillShader)
	fillShader:send("bounds", self.boundsByteData)

	fillShader:send("lists", listsTexture)
	fillShader:send("curves", curvesTexture)

	if lineShader ~= fillShader and lineShader ~= nil then
		lineShader:send("curves", curvesTexture)
	end

	if lineStyle >= 1 and lineShader ~= nil then
		sendLineArgs(lineShader, data)
	end
end

function GeometrySend:updateUniforms(flags)
	if bit.band(flags, lib.CHANGED_POINTS) ~= 0 then
		local fillShader = self.fillShader
		local lineShader = self.lineShader

		sendLUT(self, fillShader)
		fillShader:send("bounds", self.boundsByteData)

		if bit.band(flags, lib.CHANGED_LINE_ARGS) ~= 0 then
			if lineShader ~= nil then
				sendLineArgs(lineShader, self.data)
			else
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
