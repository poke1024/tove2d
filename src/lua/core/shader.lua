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

local shaders = {
	prolog = env.graphics.glsl3
		and "#pragma language glsl3\n#define GLSL3 1\n"
		or "",
	line = [[
#if LINE_STYLE == 1
uniform vec4 linecolor;
#elif LINE_STYLE >= 2
uniform sampler2D linecolors;
uniform ]] .. env.mat3.glsl .. [[ linematrix;
uniform vec2 linecscale;
#endif

#if LINE_STYLE > 0
vec4 computeLineColor(vec2 pos) {
#if LINE_STYLE == 1
	return linecolor;
#elif LINE_STYLE == 2
	float y = (linematrix * vec3(pos, 1)).y;
	y = linecscale.x + linecscale.y * y;
	return Texel(linecolors, vec2(0.5, y));
#elif LINE_STYLE == 3
	float y = length((linematrix * vec3(pos, 1)).xy);
	y = linecscale.x + linecscale.y * y;
	return Texel(linecolors, vec2(0.5, y));
#endif
}
#endif
]],
	lineGlue = [[
varying vec2 raw_vertex_pos;
vec4 effect(vec4 _1, Image _2, vec2 _3, vec2 _4) {
	return computeLineColor(raw_vertex_pos);
}
]],
	fill = [[
#if FILL_STYLE == 1
uniform vec4 fillcolor;
#elif FILL_STYLE >= 2
uniform sampler2D fillcolors;
uniform ]] .. env.mat3.glsl .. [[ fillmatrix;
uniform vec2 fillcscale;
#endif

#if FILL_STYLE > 0
vec4 computeFillColor(vec2 pos) {
#if FILL_STYLE == 1
	return fillcolor;
#elif FILL_STYLE == 2
	float y = (fillmatrix * vec3(pos, 1)).y;
	y = fillcscale.x + fillcscale.y * y;
	return Texel(fillcolors, vec2(0.5, y));
#elif FILL_STYLE == 3
	float y = length((fillmatrix * vec3(pos, 1)).xy);
	y = fillcscale.x + fillcscale.y * y;
	return Texel(fillcolors, vec2(0.5, y));
#endif
}
#endif
]],
	fillGlue = [[
varying vec2 raw_vertex_pos;
vec4 effect(vec4 _1, Image _2, vec2 _3, vec2 _4) {
	return computeFillColor(raw_vertex_pos);
}
]],
	vertexGlue = [[
varying vec2 raw_vertex_pos;
vec4 position(mat4 transform_projection, vec4 vertex_pos) {
	raw_vertex_pos = vertex_pos.xy;
	return transform_projection * vertex_pos;
}
]],
	vertex = [[
	uniform vec4 bounds;
	varying vec4 raw_vertex_pos;

	vec4 position(mat4 transform_projection, vec4 vertex_pos) {
		raw_vertex_pos = vec4(mix(bounds.xy, bounds.zw, vertex_pos.xy), vertex_pos.zw);
	    return transform_projection * raw_vertex_pos;
	}
]],
	fragment = [[
--!! include "tove.glsl"
]],
	paintVertex = [[
attribute float VertexPaint;
uniform ]] .. env.mat3.glsl .. [[ matrix[NUM_PAINTS];
uniform vec4 arguments[NUM_PAINTS];

varying vec2 gradient_pos;
varying vec3 gradient_scale;
varying vec2 texture_pos;

vec4 position(mat4 transform_projection, vec4 vertex_pos) {
	int i = int(VertexPaint);
	vec4 vertex_arg = arguments[i];
	gradient_pos = (matrix[i] * vec3(vertex_pos.xy, 1)).xy;
	gradient_scale = vertex_arg.zwx;

	float y = mix(gradient_pos.y, length(gradient_pos), gradient_scale.z);
	y = gradient_scale.x + gradient_scale.y * y;

	texture_pos = vec2((i + 0.5) / NUM_PAINTS, y);
	return transform_projection * vertex_pos;
}
]],
	paintFragment = [[
uniform sampler2D colors;
varying vec2 gradient_pos;
varying vec3 gradient_scale;
varying vec2 texture_pos;

vec4 effect(vec4 _1, Image _2, vec2 _3, vec2 _4) {
	float y = mix(gradient_pos.y, length(gradient_pos), gradient_scale.z);
	y = gradient_scale.x + gradient_scale.y * y;

	vec2 texture_pos_exact = vec2(texture_pos.x, y);
	]] ..
	-- ensure texture prefetch via texture_pos; for solid colors and
	-- many linear gradients, texture_pos == texture_pos_exact.
	[[
	vec4 c = Texel(colors, texture_pos);

	return texture_pos_exact == texture_pos ?
		c : Texel(colors, texture_pos_exact);
}
]],
}

local max = math.max
local floor = math.floor

local _log2 = math.log(2)
local function log2(n)
	return math.log(n) / _log2
end

local function next2(n)
	return math.pow(2, math.ceil(log2(max(16, n))))
end


local function newGeometryFillShader(data, fragLine)
	local geometry = data.geometry
	local f = string.format

	-- encourage shader caching by trying to reduce
	-- code changing states.
	local lutN = next2(geometry.lookupTableSize)

	local lineStyle = data.color.line.style
	if not fragLine then
		lineStyle = 0
	end

	local code = {
		shaders.prolog,
		f("#define LUT_SIZE %d", lutN),
		f("#define LINE_STYLE %d", lineStyle),
		f("#define FILL_STYLE %d", data.color.fill.style),
		f("#define FILL_RULE %d", geometry.fillRule),
		f("#define CURVE_DATA_SIZE %d", geometry.curvesTextureSize[0]),
		shaders.line,
		shaders.fill,
		shaders.fragment
	}

	local shader = lg.newShader(
		table.concat(code, "\n"), shaders.prolog .. shaders.vertex)

	shader:send("constants", {geometry.maxCurves,
		geometry.listsTextureSize[0], geometry.listsTextureSize[1]})
	return shader
end

local function newGeometryLineShader(data)
	local style = data.color.line.style
	if style < 1 then
		return nil
	end

	local geometry = data.geometry
	local f = string.format

	local fragcode = {
		shaders.prolog,
		f("#define LINE_STYLE %d", style),
		shaders.line,
		shaders.lineGlue
	}

	local vertcode = {
		shaders.prolog,
		f("#define CURVE_DATA_SIZE %d", geometry.curvesTextureSize[0]),
		[[
		--!! include "line.glsl"
		]]
	}

	local shader = lg.newShader(
		table.concat(fragcode, "\n"),
		table.concat(vertcode, "\n"))

	shader:send("max_curves", data.geometry.maxCurves)

	return shader
end

local function newLineShader(data)
	if data.style < 1 then
		return nil
	end
	local code = {
		shaders.prolog,
		string.format("#define LINE_STYLE %d", data.style),
		shaders.line,
		shaders.lineGlue
	}
	return lg.newShader(table.concat(code, "\n"),
		shaders.prolog .. shaders.vertexGlue)
end

local function newFillShader(data)
	if data.style < 1 then
		return nil
	end
	local code = {
		shaders.prolog,
		string.format("#define FILL_STYLE %d", data.style),
		shaders.fill,
		shaders.fillGlue
	}
	return lg.newShader(table.concat(code, "\n"),
		shaders.prolog .. shaders.vertexGlue)
end

local function newPaintShader(numPaints)
	local fragmentCode = {
		shaders.prolog,
		string.format("#define NUM_PAINTS %d", numPaints),
		shaders.paintFragment
	}
	local vertexCode = {
		shaders.prolog,
		string.format("#define NUM_PAINTS %d", numPaints),
		shaders.paintVertex
	}
	return lg.newShader(
		table.concat(fragmentCode, "\n"),
		table.concat(vertexCode, "\n"))
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
		alloc.numPaints * env.mat3.size)
	local imageData = love.image.newImageData(
		alloc.numPaints, alloc.numColors, "rgba8")
	local colorsTexture = lg.newImage(imageData)
	local argumentsData = love.data.newByteData(
		alloc.numPaints * ffi.sizeof("ToveVec4"))
	local gradientData = ffi.new("ToveGradientData")
	gradientData.matrix = matrixData:getPointer()
	gradientData.arguments = argumentsData:getPointer()
	gradientData.colorsTexture = imageData:getPointer()
	gradientData.colorsTextureRowBytes = imageData:getSize() / alloc.numColors
	gradientData.colorTextureHeight = imageData:getHeight()
	lib.FeedBind(link, gradientData)
	local paintShader = newPaintShader(alloc.numPaints)
	colorsTexture:setFilter("nearest", "linear")
	colorsTexture:setWrap("clamp", "clamp")

	lib.FeedEndUpdate(link)

	colorsTexture:replacePixels(imageData)
	paintShader:send("matrix", matrixData)
	paintShader:send("arguments", argumentsData)
	paintShader:send("colors", colorsTexture)

	lib.GraphicsClearChanges(graphics._ref)

	return {
		link = link,
		mesh = mesh,
		shader = paintShader,

		matrixData = matrixData,
		argumentsData = argumentsData,
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

function MeshShader:update()
	local linkdata = self.linkdata
	local graphics = self.graphics

	local flags = graphics:fetchChanges(lib.CHANGED_POINTS)
	if bit.band(flags, lib.CHANGED_POINTS) ~= 0 then
		if self.usage["points"] ~= "dynamic" then
			tove.slow("static mesh points changed in " .. self._name)
			self.linkdata = newMeshFeedData(
				self.name, self.graphics, self.tess, self.usage, self.resolution)
			return
		end

		local tessFlags = lib.UPDATE_MESH_VERTICES
		if self.usage["triangles"] == "dynamic" then
			tessFlags = bit.bor(tessFlags, lib.UPDATE_MESH_AUTO_TRIANGLES)
		end
		local updated = self.tess(linkdata.mesh._tovemesh, tessFlags)

		if bit.band(updated, lib.UPDATE_MESH_TRIANGLES) ~= 0 then
			linkdata.mesh:updateTriangles()
		end
		linkdata.mesh:updateVertices()
	end

	local link = linkdata.link
	local chg1 = lib.FeedBeginUpdate(link)

	if chg1 ~= 0 then
		if bit.band(chg1, lib.CHANGED_RECREATE) ~= 0 then
			tove.warn("mesh recreation triggered in " .. self._name)
			self.linkdata = newMeshFeedData(
				self.name, self.graphics, self.tess, self.usage)
			return
		end

		lib.FeedEndUpdate(link)
		linkdata.colorsTexture:replacePixels(linkdata.imageData)
		local shader = linkdata.shader
		shader:send("matrix", linkdata.matrixData)
		shader:send("arguments", linkdata.argumentsData)
	end
end

function MeshShader:draw(...)
	local linkdata = self.linkdata

	lg.setShader(linkdata.shader)
	local mesh = linkdata.mesh:getMesh()
	if mesh ~= nil then
		lg.draw(mesh, ...)
	end
	lg.setShader(nil)
end


local function setLineQuality(linkData, lineQuality)
	linkData.lineQuality = lineQuality
	if linkData.data.color.line.style > 0 then
		if linkData.lineShader == linkData.fillShader then
			linkData.fillShader:send("line_quality", lineQuality)
		else
			local numSegments = math.max(2, math.ceil(50 * lineQuality))
			linkData.lineShader:send("segments_per_curve", numSegments)
			linkData.numSegments = numSegments
		end
	end
end

local ComputeShader = {}
ComputeShader.__index = ComputeShader

local function parseQuality(q)
	local lineType = "fragment"
	local lineQuality = 1.0
	if type(q) == "table" and type(q.line) == "table" then
		lineType = q.line.type or "fragment"
		lineQuality = q.line.quality or 1.0
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

local function newComputeFeedData(path, quality)
	local lineType, lineQuality = parseQuality(quality)
	local fragLine = (lineType == "fragment")

	local link = ffi.gc(
		lib.NewGeometryFeed(path, fragLine), lib.ReleaseFeed)
	local data = lib.FeedGetData(link)

	lib.FeedBeginUpdate(link)
	local fillShader = newGeometryFillShader(data, fragLine)
	local lineShader
	if fragLine then
		lineShader = fillShader
	else
		lineShader = newGeometryLineShader(data)
	end

	local lineColorSend = feed.newColorSend(
		lineShader, "line", data.color.line)
	lineColorSend:beginInit()
	local fillColorSend = feed.newColorSend(
		fillShader, "fill", data.color.fill)
	fillColorSend:beginInit()

	local geometryFeed = feed.newGeometrySend(
		fillShader, lineShader, data.geometry)
	geometryFeed:beginInit()

	lib.FeedEndUpdate(link)

	lineColorSend:endInit(path)
	fillColorSend:endInit(path)
	geometryFeed:endInit(data.color.line.style)

	path:clearChanges()

	local linkData = {
		link = link,
		data = data,
		fillShader = fillShader,
		lineShader = lineShader,
		lineType = lineType,
		lineQuality = lineQuality,
		geometryFeed = geometryFeed,
		lineColorSend = lineColorSend,
		fillColorSend = fillColorSend
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

	local chg1 = lib.FeedBeginUpdate(link)

	if bit.band(chg1, lib.CHANGED_RECREATE) ~= 0 then
		self.linkdata = newComputeFeedData(path)
		return
	end

	linkdata.lineColorSend:updateUniforms(chg1, path)
	linkdata.fillColorSend:updateUniforms(chg1, path)

	local chg2 = lib.FeedEndUpdate(link)
	linkdata.geometryFeed:updateUniforms(chg2)
end

function ComputeShader:updateQuality(quality)
	local linkdata = self.linkdata
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

	local fillShader = linkdata.fillShader
	local lineShader = linkdata.lineShader
	local lineQuality = linkdata.lineQuality
	local feed = linkdata.geometryFeed

	lg.setShader(fillShader)
	lg.draw(feed.mesh, ...)

	if fillShader == lineShader or lineShader == nil then
		return
	end

	local geometry = linkdata.data.geometry
	local lineRuns = geometry.lineRuns
	if lineRuns == nil then
		return
	end

	lg.setShader(lineShader)

	local lineMesh = feed.lineMesh
	local lineJoinMesh = feed.lineJoinMesh
	local numSegments = linkdata.numSegments
	for i = 0, geometry.numSubPaths - 1 do
		local run = lineRuns[i]
		local numCurves = run.numCurves
		local numInstances = numSegments * numCurves
		lineShader:send("curve_index", run.curveIndex)
		lineShader:send("num_curves", numCurves)
		lineShader:send("draw_joins", 0)
		lg.drawInstanced(lineMesh, numInstances, ...)
		lineShader:send("draw_joins", 1)
		lg.drawInstanced(lineJoinMesh,
			numCurves - (run.isClosed and 0 or 1), ...)
	end
end

return {
	newMeshShader = newMeshShader,
	newComputeShader = newComputeShader
}
