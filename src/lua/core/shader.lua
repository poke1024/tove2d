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
]]
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

--!! import "feed.lua" as feed

local MeshShader = {}
MeshShader.__index = MeshShader

local function newMeshShaderLinkData(name, path, tess, usage, resolution)
	local fillMesh = tove.newPositionMesh(name, usage)
	local lineMesh = tove.newPositionMesh(name, usage)

	tess(path, fillMesh._cmesh, lineMesh._cmesh, lib.UPDATE_MESH_EVERYTHING)

	local link = ffi.gc(
		lib.NewColorShaderLink(resolution), lib.ReleaseShaderLink)
	local data = lib.ShaderLinkGetColorData(link)
	lib.ShaderLinkBeginUpdate(link, path, true)

	local lineShader = newLineShader(data.line)
	local fillShader = newFillShader(data.fill)

	local lineColorFeed = feed.newColorFeed(lineShader, "line", data.line)
	lineColorFeed:beginInit()
	local fillColorFeed = feed.newColorFeed(fillShader, "fill", data.fill)
	fillColorFeed:beginInit()

	lib.ShaderLinkEndUpdate(link, path, true)
	lineColorFeed:endInit(path)
	fillColorFeed:endInit(path)

	path:clearChanges()

	return {
		link = link,
		data = data,
		lineShader = lineShader,
		fillShader = fillShader,
		lineColorFeed = lineColorFeed,
		fillColorFeed = fillColorFeed,
		lineMesh = lineMesh,
		fillMesh = fillMesh
	}
end

local newMeshShader = function(name, path, tess, usage, resolution)
	return setmetatable({
		path = path, tess = tess, usage = usage, _name = name,
		linkdata = newMeshShaderLinkData(
			name, path, tess, usage, resolution)}, MeshShader)
end

function MeshShader:update()
	local linkdata = self.linkdata
	local path = self.path

	local flags = path:fetchChanges(lib.CHANGED_POINTS)

	if bit.band(flags, lib.CHANGED_POINTS) ~= 0 then
		if self.usage["points"] ~= "dynamic" then
			tove.slow("static mesh points changed in " .. self._name)
			self.linkdata = newMeshShaderLinkData(
				self.name, self.path, self.tess, self.usage)
			return
		end

		local tessFlags = lib.UPDATE_MESH_VERTICES

		if self.usage["triangles"] == "dynamic" then
			tessFlags = bit.bor(tessFlags, lib.UPDATE_MESH_AUTO_TRIANGLES)
		end

		local updated = self.tess(self.path, linkdata.fillMesh._cmesh,
			linkdata.lineMesh._cmesh, tessFlags)

		if bit.band(updated, lib.UPDATE_MESH_TRIANGLES) ~= 0 then
			linkdata.fillMesh:updateTriangles()
		end

		if linkdata.fillShader ~= nil then
			linkdata.fillMesh:updateVertices()
		end
		if linkdata.lineShader ~= nil then
			linkdata.lineMesh:updateVertices()
		end
	end

	local link = linkdata.link
	local chg1 = lib.ShaderLinkBeginUpdate(link, path, false)

	if bit.band(chg1, lib.CHANGED_RECREATE) ~= 0 then
		tove.warn("mesh recreation triggered in " .. self._name)
		self.linkdata = newMeshShaderLinkData(
			self.name, self.path, self.tess, self.usage)
		return
	end

	linkdata.lineColorFeed:update(chg1, path)
	linkdata.fillColorFeed:update(chg1, path)

	lib.ShaderLinkEndUpdate(link, path, false)
end

function MeshShader:draw()
	local linkdata = self.linkdata
	if linkdata.fillShader ~= nil then
		lg.setShader(linkdata.fillShader)
		local mesh = linkdata.fillMesh:getMesh()
		if mesh ~= nil then
			lg.draw(mesh)
		end
	end
	if linkdata.lineShader ~= nil then
		lg.setShader(linkdata.lineShader)
		local mesh = linkdata.lineMesh:getMesh()
		if mesh ~= nil then
			lg.draw(mesh)
		end
	end
end


local PathShader = {}
PathShader.__index = PathShader

local function parseQuality(q)
	local lineType = "fragment"
	local lineQuality = 1.0
	if type(q) == "table" and type(q.line) == "table" then
		lineType = q.line.type or "fragment"
		lineQuality = q.line.quality or 1.0
	end
	return lineType, lineQuality
end

local function newPathShaderLinkData(path, quality)
	local lineType, lineQuality = parseQuality(quality)
	local fragLine = (lineType == "fragment")

	local link = ffi.gc(
		lib.NewGeometryShaderLink(path, fragLine), lib.ReleaseShaderLink)
	local data = lib.ShaderLinkGetData(link)

	lib.ShaderLinkBeginUpdate(link, path, true)
	local fillShader = newGeometryFillShader(data, fragLine)
	local lineShader
	if fragLine then
		lineShader = fillShader
		if data.color.line.style > 0 then
			fillShader:send("line_quality", lineQuality)
		end
	else
		lineShader = newGeometryLineShader(data)
	end

	local lineColorFeed = feed.newColorFeed(
		lineShader, "line", data.color.line)
	lineColorFeed:beginInit()
	local fillColorFeed = feed.newColorFeed(
		fillShader, "fill", data.color.fill)
	fillColorFeed:beginInit()

	local geometryFeed = feed.newGeometryFeed(
		fillShader, lineShader, data.geometry)
	geometryFeed:beginInit()

	lib.ShaderLinkEndUpdate(link, path, true)

	lineColorFeed:endInit(path)
	fillColorFeed:endInit(path)
	geometryFeed:endInit(data.color.line.style)

	path:clearChanges()

	return {
		link = link,
		data = data,
		fillShader = fillShader,
		lineShader = lineShader,
		lineType = lineType,
		lineQuality = lineQuality,
		geometryFeed = geometryFeed,
		lineColorFeed = lineColorFeed,
		fillColorFeed = fillColorFeed
	}
end

local newPathShader = function(path, quality)
	return setmetatable({
		path = path,
		linkdata = newPathShaderLinkData(path, quality)
	}, PathShader)
end

function PathShader:update()
	local path = self.path
	local linkdata = self.linkdata
	local link = linkdata.link

	local chg1 = lib.ShaderLinkBeginUpdate(link, path, false)

	if bit.band(chg1, lib.CHANGED_RECREATE) ~= 0 then
		self.linkdata = newPathShaderLinkData(path)
		return
	end

	linkdata.lineColorFeed:update(chg1, path)
	linkdata.fillColorFeed:update(chg1, path)

	local chg2 = lib.ShaderLinkEndUpdate(link, path, false)

	linkdata.geometryFeed:update(chg2)
end

function PathShader:updateQuality(quality)
	local linkdata = self.linkdata
	local lineType, lineQuality = parseQuality(quality)
	if lineType == linkdata.lineType then
		linkdata.lineQuality = lineQuality
		if lineType == "fragment" and linkdata.data.color.line.style > 0 then
			linkdata.fillShader:send("line_quality", lineQuality)
		end
		return true
	else
		return false
	end
end

function PathShader:draw()
	local linkdata = self.linkdata

	local fillShader = linkdata.fillShader
	local lineShader = linkdata.lineShader
	local lineQuality = linkdata.lineQuality

	lg.setShader(fillShader)
	lg.draw(linkdata.geometryFeed.mesh)

	if fillShader ~= lineShader and lineShader ~= nil then
		lg.setShader(lineShader)
		local numSegments = math.max(2, math.ceil(50 * lineQuality))
		lineShader:send("segments_per_curve", numSegments)
		local geometry = linkdata.data.geometry
		local lineRuns = geometry.lineRuns
		if lineRuns ~= nil then
			local feed = linkdata.geometryFeed
			local lineMesh = feed.lineMesh
			local lineJoinMesh = feed.lineJoinMesh
			for i = 0, geometry.numSubPaths - 1 do
				local run = lineRuns[i]
				local numCurves = run.numCurves
				local numInstances = numSegments * numCurves
				lineShader:send("curve_index", run.curveIndex)
				lineShader:send("num_curves", numCurves)
				lineShader:send("draw_joins", 0)
				lg.drawInstanced(lineMesh, numInstances)
				lineShader:send("draw_joins", 1)
				lg.drawInstanced(lineJoinMesh,
					numCurves - (run.isClosed and 0 or 1))
			end
		end
	end
end

return {
	newMeshShader = newMeshShader,
	newPathShader = newPathShader
}
