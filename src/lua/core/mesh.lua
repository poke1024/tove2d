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

local PositionMesh = {}
PositionMesh.__index = PositionMesh

tove.newPositionMesh = function(name, usage)
	return setmetatable({_cmesh = ffi.gc(lib.NewMesh(), lib.ReleaseMesh),
		_mesh = nil, _usage = usage or {}}, PositionMesh)
end

function PositionMesh:getUsage(what)
	return self._usage[what]
end

function PositionMesh:updateVertices()
	local cvertices = lib.MeshGetVertices(self._cmesh)

	local vertices = {}
	local positions = cvertices.array
	local pi = 0
	for i = 1, cvertices.n do
		vertices[i] = {positions[pi + 0], positions[pi + 1],
			positions[pi + 0], positions[pi + 1]}
		pi = pi + 2
	end

	local mesh = self._mesh
	if mesh ~= nil then
		mesh:setVertices(vertices)
	end
end

function PositionMesh:updateTriangles()
	local mesh = self._mesh
	if mesh ~= nil then
		ctriangles = lib.MeshGetIndices(self._cmesh)
		local indices = totable(ctriangles.array, ctriangles.n * 3)
		mesh:setVertexMap(indices)
	else
		self:getMesh()
	end
end

function PositionMesh:getMesh()
	if self._mesh ~= nil then
		return self._mesh
	end

	local usage = "static"
	if self._usage["points"] == "dynamic" then
		usage = "dynamic"
	end

	local attributes = {{"VertexPosition", "float", 2}, {"VertexTexCoord", "float", 2}}
	local cvertices = lib.MeshGetVertices(self._cmesh)
	if cvertices.n < 1 then
		return nil
	end

	local mesh = love.graphics.newMesh(
		attributes, cvertices.n, "triangles", usage)
	self._mesh = mesh
	self:updateTriangles()
	self:updateVertices()
	return mesh
end

function PositionMesh:cache(mode)
	lib.MeshCache(self._cmesh, mode == "keyframe")
end


local ColorMesh = {}
ColorMesh.__index = ColorMesh

tove.newColorMesh = function(name, usage, tess)
	local cmesh = ffi.gc(lib.NewColorMesh(), lib.ReleaseMesh)
	tess(cmesh, -1)
	return setmetatable({_name = name, _cmesh = cmesh, _mesh = nil,
		_tess = tess, _usage = usage or {}}, ColorMesh)
end

function ColorMesh:getVertexMap()
	return self._mesh:getVertexMap()
end

function ColorMesh:getUsage(what)
	return self._usage[what]
end

function ColorMesh:retesselate(flags)
	local updated = self._tess(self._cmesh, flags)
	self:updateVertices()
	if bit.band(updated, lib.UPDATE_MESH_TRIANGLES) ~= 0 then
		self:updateTriangles()
	end
end

function ColorMesh:updateVertices()
	local mesh = self._mesh
	local cvertices = lib.MeshGetVertices(self._cmesh)

	if cvertices.n ~= mesh:getVertexCount() then
		self._mesh = nil
		self:getMesh() -- need a new mesh here.
		tove.warn("internal error: a mesh was recreated in " .. self._name)
		return
	end

	ccolors = lib.MeshGetColors(self._cmesh)

	local positions = cvertices.array
	local colors = ccolors.array

	local vertices = {}
	local pi = 0
	local ci = 0
	for i = 1, cvertices.n do
		vertices[i] = {
			positions[pi + 0], positions[pi + 1],
			colors[ci + 0] / 255, colors[ci + 1] / 255,
			colors[ci + 2] / 255, colors[ci + 3] / 255}
		pi = pi + 2
		ci = ci + 4
	end

	if mesh ~= nil then
		mesh:setVertices(vertices)
	end
end

function ColorMesh:updateTriangles()
	local mesh = self._mesh
	if mesh ~= nil then
		local ctriangles = lib.MeshGetIndices(self._cmesh)
		local indices = totable(ctriangles.array, ctriangles.n * 3)
		mesh:setVertexMap(indices)
	else
		self:getMesh()
	end
end

function ColorMesh:getMesh()
	if self._mesh ~= nil then
		return self._mesh
	end
	local attributes = {{"VertexPosition", "float", 2},
		{"VertexColor", "byte", 4}}

	local usage = "static"
	if self._usage["points"] == "dynamic" or self._usage["colors"] == "dynamic" then
		usage = "dynamic"
	end

	local cvertices = lib.MeshGetVertices(self._cmesh)
	if cvertices.n < 1 then
		return nil
	end

	local mesh = love.graphics.newMesh(
		attributes, cvertices.n, "triangles", usage)
	self._mesh = mesh
	self:updateTriangles()
	self:updateVertices()
	return mesh
end

function ColorMesh:cache(mode)
	lib.MeshCache(self._cmesh, mode == "lock")
end
