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

local function getMeshBounds(data)
	local x1 = data.bounds[0]
	local y1 = data.bounds[1]
	local x2 = data.bounds[2]
	local y2 = data.bounds[3]
	return x1, y1, x2, y2
end


local function newGradientData(gradient)
	local n = gradient.numColors

	local data = ffi.new("ToveShaderGradientData")
	gradient.data = data

	local imageData = love.image.newImageData(1, n, "rgba8")
	data.colorsTexture = imageData:getPointer()
	data.colorsTextureRowBytes = imageData:getSize() / n

	return {data = data, imageData = imageData, texture = nil}
end

local function newGradientTexture(d)
	d.texture = love_graphics.newImage(d.imageData)
	d.texture:setFilter("nearest", "linear")
	d.texture:setWrap("clamp", "clamp")
end

local function reloadGradientTexture(d)
	d.texture:replacePixels(d.imageData)
end

local function sendColor(shader, uniform, rgba)
	shader:send(uniform, {rgba.r, rgba.g, rgba.b, rgba.a})
end

local function sendGradientMatrix(shader, uniform, gradient)
	shader:send(uniform, totable(gradient.data.matrix, 9))
end


local NullColorFeed = {}
NullColorFeed.__index = NullColorFeed

function NullColorFeed:beginInit()
end
function NullColorFeed:endInit(path)
end
function NullColorFeed:update(chg1, path)
end


local ColorFeed = {}
ColorFeed.__index = ColorFeed

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

local function newColorFeed(shader, type, colorData)
	if shader == nil then
		return setmetatable({}, NullColorFeed)
	end
	return setmetatable({shader = shader, colorData = colorData,
		uniforms = _uniforms[type], flag = _flags[type]}, ColorFeed)
end

function ColorFeed:beginInit()
	local colorData = self.colorData
	if colorData.style >= 2 then
		self.gradientData = newGradientData(colorData.gradient)
	end
end

function ColorFeed:endInit(path)
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
		local gradient = colorData.gradient
		sendGradientMatrix(shader, uniforms.matrix, gradient)
		local s = 0.5 / gradient.numColors
		shader:send(uniforms.cscale, {s, 1 - 2 * s})
	end
end

function ColorFeed:update(chg1, path)
	if bit.band(chg1, self.flag) ~= 0 then
		local shader = self.shader
		local uniforms = self.uniforms
		local colorData = self.colorData
		if colorData.style == 1 then
			sendColor(shader, uniforms.color, colorData.rgba)
		elseif colorData.style >= 2 then
			reloadGradientTexture(self.gradientData)
			local gradient = colorData.gradient
			sendGradientMatrix(shader, uniforms.matrix, gradient)
			local s = 0.5 / gradient.numColors
			shader:send(uniforms.cscale, {s, 1 - 2 * s})
		end
	end
end


local GeometryFeed = {}
GeometryFeed.__index = GeometryFeed

local function newGeometryFeed(shader, data)
	return setmetatable({shader = shader, data = data}, GeometryFeed)
end

function GeometryFeed:beginInit()
	local data = self.data

	-- note: everything we store as pointers into "data" (or some sub structure of
	-- it), needs to be referenced somewhere else; otherwise the gc will collect it.

	local listsImageData = love.image.newImageData(
		data.listsTextureSize[0], data.listsTextureSize[1],
		ffi.string(data.listsTextureFormat))
	data.listsTextureRowBytes = listsImageData:getSize() / data.listsTextureSize[1]

	local curvesImageData = love.image.newImageData(
		data.curvesTextureSize[0], data.curvesTextureSize[1],
		ffi.string(data.curvesTextureFormat))
	data.curvesTextureRowBytes = curvesImageData:getSize() / data.curvesTextureSize[1]

	--print("data.curvesTextureRowBytes", data.curvesTextureRowBytes)

	data.listsTexture = listsImageData:getPointer()
	data.curvesTexture = curvesImageData:getPointer()

	self.listsImageData = listsImageData
	self.curvesImageData = curvesImageData
end

function GeometryFeed:endInit(lineStyle)
	local listsTexture = love.graphics.newImage(self.listsImageData)
	local curvesTexture = love.graphics.newImage(self.curvesImageData)
	listsTexture:setFilter("nearest", "nearest")
	curvesTexture:setFilter("nearest", "nearest")
	self.listsTexture = listsTexture
	self.curvesTexture = curvesTexture

	-- create mesh

	local shader = self.shader
	local data = self.data

	-- note: since bezier curves stay in the convex hull of their control points, we
	-- could triangulate the mesh here. with strokes, it gets difficult though.

	local x1, y1, x2, y2 = getMeshBounds(data)
	local vertices = {{x1, y1, x1, y1}, {x2, y1, x2, y1},
		{x2, y2, x2, y2}, {x1, y2, x1, y2}}
	self.mesh = love.graphics.newMesh(
		{{"VertexPosition", "float", 2}, {"VertexTexCoord", "float", 2}}, vertices)

	sendLUT(shader, data)

	shader:send("lists", listsTexture)
	shader:send("curves", curvesTexture)

	if lineStyle >= 1 then
		shader:send("linewidth", data.strokeWidth / 2)
	end
end

function GeometryFeed:update(chg2)
	local data = self.data

	if bit.band(chg2, lib.CHANGED_POINTS) ~= 0 and data.lookupTableFill[1] > 1 then
		local shader = self.shader
		sendLUT(shader, data)

		self.listsTexture:replacePixels(self.listsImageData)
		self.curvesTexture:replacePixels(self.curvesImageData)

		local x1, y1, x2, y2 = getMeshBounds(data)
		self.mesh:setVertices({{x1, y1, x1, y1}, {x2, y1, x2, y1}, {x2, y2, x2, y2}, {x1, y2, x1, y2}})
	end
end

return {
	newColorFeed = newColorFeed,
	newGeometryFeed = newGeometryFeed
}
