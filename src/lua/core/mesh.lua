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

local function getTrianglesMode(cmesh)
	local m = lib.MeshGetTriangles(cmesh).mode
	return m == lib.TRIANGLES_LIST and "triangles" or "strip"
end


local PositionMesh = {}
PositionMesh.__index = PositionMesh

tove.newPositionMesh = function(name, usage)
	return setmetatable({_cmesh = ffi.gc(lib.NewMesh(), lib.ReleaseMesh),
		_mesh = nil, _usage = usage or {}, _vdata = nil}, PositionMesh)
end

function PositionMesh:getUsage(what)
	return self._usage[what]
end

function PositionMesh:updateVertices()
	local mesh = self._mesh
	if mesh ~= nil then
		local vdata = self._vdata
		lib.MeshCopyVertexData(
			self._cmesh, vdata:getPointer(), vdata:getSize())
		mesh:setVertices(vdata)
	end
end

function PositionMesh:updateTriangles()
	local mesh = self._mesh
	if mesh ~= nil then
		ctriangles = lib.MeshGetTriangles(self._cmesh)
		local indices = totable(ctriangles.array, ctriangles.size)
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

	local attributes = {{"VertexPosition", "float", 2}}
	local n = lib.MeshGetVertexCount(self._cmesh)
	if n < 1 then
		return nil
	end

	local mesh = love.graphics.newMesh(
		attributes, n, getTrianglesMode(self._cmesh), usage)
	self._mesh = mesh
	self._vdata = love.data.newByteData(n * 2 * floatSize)

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
		_tess = tess, _usage = usage or {}, _vdata = nil}, ColorMesh)
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
	if mesh ~= nil then
		local vdata = self._vdata
		lib.MeshCopyVertexData(
			self._cmesh, vdata:getPointer(), vdata:getSize())
		mesh:setVertices(vdata)
	end
end

function ColorMesh:updateTriangles()
	local mesh = self._mesh
	if mesh ~= nil then
		local ctriangles = lib.MeshGetTriangles(self._cmesh)
		local indices = totable(ctriangles.array, ctriangles.size)
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
	if self._usage["points"] == "dynamic" or
		self._usage["colors"] == "dynamic" then
		usage = "dynamic"
	end

	local n = lib.MeshGetVertexCount(self._cmesh)
	if n < 1 then
		return nil
	end

	local mesh = love.graphics.newMesh(
		attributes, n, getTrianglesMode(self._cmesh), usage)
	self._mesh = mesh
	self._vdata = love.data.newByteData(n * (2 * floatSize + 4))
	self:updateTriangles()
	self:updateVertices()
	return mesh
end

function ColorMesh:cache(mode)
	lib.MeshCache(self._cmesh, mode == "lock")
end
