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

local love_graphics = love.graphics

local shaders = {
	line = [[
#if LINE_STYLE == 1
uniform vec4 linecolor;
#elif LINE_STYLE >= 2
uniform sampler2D linecolors;
uniform mat3 linematrix;
uniform vec2 linecscale;
#endif

#if LINE_STYLE > 0
vec4 computeLineColor(vec2 pos) {
#if LINE_STYLE == 1
	return linecolor;
#elif LINE_STYLE == 2
	float y = (linematrix * vec3(pos, 1)).y;
	y = linecscale.x + linecscale.y * y;
	return texture2D(linecolors, vec2(0.5, y));
#elif LINE_STYLE == 3
	float y = length((linematrix * vec3(pos, 1)).xy);
	y = linecscale.x + linecscale.y * y;
	return texture2D(linecolors, vec2(0.5, y));
#endif
}
#endif
]],
	lineGlue = [[
vec4 effect(vec4 c, Image t, vec2 tc, vec2 sc) {
	return computeLineColor(tc);
}
]],
			fill = [[
#if FILL_STYLE == 1
uniform vec4 fillcolor;
#elif FILL_STYLE >= 2
uniform sampler2D fillcolors;
uniform mat3 fillmatrix;
uniform vec2 fillcscale;
#endif

#if FILL_STYLE > 0
vec4 computeFillColor(vec2 pos) {
#if FILL_STYLE == 1
	return fillcolor;
#elif FILL_STYLE == 2
	float y = (fillmatrix * vec3(pos, 1)).y;
	y = fillcscale.x + fillcscale.y * y;
	return texture2D(fillcolors, vec2(0.5, y));
#elif FILL_STYLE == 3
	float y = length((fillmatrix * vec3(pos, 1)).xy);
	y = fillcscale.x + fillcscale.y * y;
	return texture2D(fillcolors, vec2(0.5, y));
#endif
}
#endif
]],
	fillGlue = [[
vec4 effect(vec4 c, Image t, vec2 tc, vec2 sc) {
	return computeFillColor(tc);
}
]]
}

shaders.code = [[
--!! include "tove.glsl"
]]

local max = math.max
local floor = math.floor

local _log2 = math.log(2)
local function log2(n)
	return math.log(n) / _log2
end

local function next2(n)
	return math.pow(2, math.ceil(log2(max(8, n))))
end

local function newShader(data)
	local geometry = data.geometry
	local lutN = geometry.lookupTableSize
	local f = string.format
	local code = {
		f("#define LUT_SIZE %d", lutN),
		f("#define NUM_CURVES %d", geometry.numCurves),
		f("#define LISTS_W %d", geometry.listsTextureSize[0]),
		f("#define LISTS_H %d", geometry.listsTextureSize[1]),
		f("#define LINE_STYLE %d", data.color.line.style),
		f("#define FILL_STYLE %d", data.color.fill.style),
		f("#define FILL_RULE %d", geometry.fillRule),
		shaders.line,
		shaders.fill,
		shaders.code
	}
	return love_graphics.newShader(table.concat(code, "\n"))
end

local function newLineShader(data)
	if data.style < 1 then
		return nil
	end
	local code = {
		string.format("#define LINE_STYLE %d", data.style),
		shaders.line,
		shaders.lineGlue
	}
	return love_graphics.newShader(table.concat(code, "\n"))
end

local function newFillShader(data)
	if data.style < 1 then
		return nil
	end
	local code = {
		string.format("#define FILL_STYLE %d", data.style),
		shaders.fill,
		shaders.fillGlue
	}
	return love_graphics.newShader(table.concat(code, "\n"))
end

--!! import "feed.lua" as feed

local MeshShader = {}
MeshShader.__index = MeshShader

local function newMeshShaderLinkData(path, tess, usage)
	local fillMesh = tove.newPositionMesh(usage)
	local lineMesh = tove.newPositionMesh(usage)

	tess(path, fillMesh._cmesh, lineMesh._cmesh, lib.UPDATE_MESH_EVERYTHING)

	local link = ffi.gc(lib.NewShaderLink(0), lib.ReleaseShaderLink)
	local data = lib.ShaderLinkGetColorData(link)
	lib.ShaderLinkBeginUpdate(link, path, true)

	--print("xx", data.line.style, data.fill.style)

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

local newMeshShader = function(path, tess, usage)
	return setmetatable({path = path, tess = tess, usage = usage,
		linkdata = newMeshShaderLinkData(path, tess, usage)}, MeshShader)
end

function MeshShader:update()
	local linkdata = self.linkdata
	local path = self.path

	local flags = path:fetchChanges(lib.CHANGED_POINTS)

	if bit.band(flags, lib.CHANGED_POINTS) ~= 0 then
		if self.usage["points"] ~= "dynamic" then
			tove.warn(path, "static mesh points changed.")
			self.linkdata = newMeshShaderLinkData(self.path, self.tess, self.usage)
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
		tove.warn(path, "mesh recreation triggered.")
		self.linkdata = newMeshShaderLinkData(self.path, self.tess, self.usage)
		return
	end

	linkdata.lineColorFeed:update(chg1, path)
	linkdata.fillColorFeed:update(chg1, path)

	local chg2 = lib.ShaderLinkEndUpdate(link, path, false)
end

function MeshShader:draw()
	self:update()
	local linkdata = self.linkdata
	if linkdata.fillShader ~= nil then
		love_graphics.setShader(linkdata.fillShader)
		local mesh = linkdata.fillMesh:getMesh()
		if mesh ~= nil then
			love_graphics.draw(mesh)
		end
	end
	if linkdata.lineShader ~= nil then
		love_graphics.setShader(linkdata.lineShader)
		local mesh = linkdata.lineMesh:getMesh()
		if mesh ~= nil then
			love_graphics.draw(mesh)
		end
	end
end


local PathShader = {}
PathShader.__index = PathShader

local function newPathShaderLinkData(path)
	local link = ffi.gc(lib.NewShaderLink(path:getNumCurves()), lib.ReleaseShaderLink)
	local data = lib.ShaderLinkGetData(link)

	lib.ShaderLinkBeginUpdate(link, path, true)
	local shader = newShader(data)

	local lineColorFeed = feed.newColorFeed(shader, "line", data.color.line)
	lineColorFeed:beginInit()
	local fillColorFeed = feed.newColorFeed(shader, "fill", data.color.fill)
	fillColorFeed:beginInit()

	local geometryFeed = feed.newGeometryFeed(shader, data.geometry)
	geometryFeed:beginInit()

	lib.ShaderLinkEndUpdate(link, path, true)

	lineColorFeed:endInit(path)
	fillColorFeed:endInit(path)
	geometryFeed:endInit(data.color.line.style)

	path:clearChanges()

	return {
		link = link,
		shader = shader,
		geometryFeed = geometryFeed,
		lineColorFeed = lineColorFeed,
		fillColorFeed = fillColorFeed
	}
end

local newPathShader = function(path)
	return setmetatable({
		path = path,
		linkdata = newPathShaderLinkData(path)
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

function PathShader:draw()
	self:update()
	local linkdata = self.linkdata
	love_graphics.setShader(linkdata.shader)
	love_graphics.draw(linkdata.geometryFeed.mesh)
end

return {
	newMeshShader = newMeshShader,
	newPathShader = newPathShader
}
