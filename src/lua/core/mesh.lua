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

local floatSize = ffi.sizeof("float")
local indexSize = ffi.sizeof("ToveVertexIndex")

local function getTrianglesMode(cmesh)
	return lib.MeshGetIndexMode(cmesh) == lib.TRIANGLES_LIST
		and "triangles" or "strip"
end


local AbstractMesh = {}
AbstractMesh.__index = AbstractMesh

function AbstractMesh:getVertexMap()
	return self._mesh:getVertexMap()
end

function AbstractMesh:getUsage(what)
	return self._usage[what]
end

function AbstractMesh:updateVertices()
	local mesh = self._mesh
	if mesh ~= nil then
		local vdata = self._vdata
		lib.MeshCopyVertexData(
			self._tovemesh, vdata:getPointer(), vdata:getSize())
		mesh:setVertices(vdata)
	end
end

function AbstractMesh:updateTriangles()
	local mesh = self._mesh
	if mesh ~= nil then
		local indexCount = lib.MeshGetIndexCount(self._tovemesh)
		if indexCount < 1 then
			return
		end
		local size = indexCount * indexSize

		if size ~= self._idatasize then
			-- this is not great - yet better than the old solution.
			self._idata = love.data.newByteData(size)
			self._idatasize = size
		end

		local idata = self._idata
		lib.MeshCopyIndexData(
			self._tovemesh, idata:getPointer(), idata:getSize())

		mesh:setVertexMap(idata, indexSize == 2 and "uint16" or "uint32")
	else
		self:getMesh()
	end
end

function AbstractMesh:cache(mode)
	lib.MeshCache(self._tovemesh, mode == "keyframe")
end

function AbstractMesh:getMesh()
	if self._mesh ~= nil then
		return self._mesh
	end

	local n = lib.MeshGetVertexCount(self._tovemesh)
	if n < 1 then
		return nil
	end

	local usage = self._dynamic and "dynamic" or "static"
	local mesh = love.graphics.newMesh(
		self._attributes, n, getTrianglesMode(self._tovemesh), usage)
	self._mesh = mesh
	self._vdata = love.data.newByteData(n * self._vertexByteSize)

	self:updateTriangles()
	self:updateVertices()

	return mesh
end


local PositionMesh = {}
PositionMesh.__index = PositionMesh
setmetatable(PositionMesh, {__index = AbstractMesh})
PositionMesh._attributes = {{"VertexPosition", "float", 2}}
PositionMesh._vertexByteSize = 2 * floatSize

tove.newPositionMesh = function(name, usage)
	return setmetatable({
		_name = name,
		_tovemesh = ffi.gc(lib.NewMesh(), lib.ReleaseMesh),
		_mesh = nil,
		_usage = usage or {},
		_dynamic = usage["points"] == "dynamic",
		_vdata = nil}, PositionMesh)
end


local PaintMesh = {}
PaintMesh.__index = PaintMesh
setmetatable(PaintMesh, {__index = AbstractMesh})
PaintMesh._attributes = {
	{"VertexPosition", "float", 2},
	{"VertexPaint", "float", 1}}
PaintMesh._vertexByteSize = 3 * floatSize

tove.newPaintMesh = function(name, usage)
	return setmetatable({
		_name = name,
		_tovemesh = ffi.gc(lib.NewPaintMesh(), lib.ReleaseMesh),
		_mesh = nil,
		_usage = usage or {},
		_dynamic = usage["points"] == "dynamic",
		_vdata = nil}, PaintMesh)
end


local ColorMesh = {}
ColorMesh.__index = ColorMesh
setmetatable(ColorMesh, {__index = AbstractMesh})
ColorMesh._attributes = {
	{"VertexPosition", "float", 2},
	{"VertexColor", "byte", 4}}
ColorMesh._vertexByteSize = 2 * floatSize + 4

tove.newColorMesh = function(name, usage, tess)
	local cmesh = ffi.gc(lib.NewColorMesh(), lib.ReleaseMesh)
	tess(cmesh, -1)
	return setmetatable({
		_name = name, _tovemesh = cmesh, _mesh = nil,
		_tess = tess, _usage = usage,
		_dynamic = usage["points"] == "dynamic" or usage["colors"] == "dynamic",
		_vdata = nil}, ColorMesh)
end

function ColorMesh:retesselate(flags)
	local updated = self._tess(self._tovemesh, flags)
	if bit.band(updated, lib.UPDATE_MESH_GEOMETRY) ~= 0 then
		-- we have a change of vertex count, i.e. the mesh must be
		-- recreated. happens if we're using adapative tesselation.
		self._mesh = nil
		return true  -- the underlying mesh changed
	end

	if bit.band(updated,
		lib.UPDATE_MESH_VERTICES +
		lib.UPDATE_MESH_COLORS) ~= 0 then
		self:updateVertices()
	end
	if bit.band(updated, lib.UPDATE_MESH_TRIANGLES) ~= 0 then
		self:updateTriangles()
	end

	return false
end
