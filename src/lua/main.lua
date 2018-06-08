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

--!! include "license.lua"

local ffi = require 'ffi'

ffi.cdef [[
--!! include "../cpp/_interface.h"
]]

tove = {}

tove.init = function(path)
	local libName = {
		["OS X"] = "libTove.dylib",
		["Windows"] = "libTove.dll",
		["Linux"] = "libTove.so"
	}

	-- https://stackoverflow.com/questions/6380820/get-containing-path-of-lua-file
	local basepath = debug.getinfo(2, "S").source:sub(2):match("(.*/)")

	local lib = ffi.load(basepath .. libName[love.system.getOS()])
	tove.lib = lib
	tove.getVersion = function()
		return ffi.string(lib.GetVersion())
	end

	lib.SetWarningFunction(ffi.cast("ToveWarningFunction", function(s)
		print("TÖVE warning: " .. ffi.string(s))
		print(debug.traceback())
  	end))

	local env = {
		graphics = love.graphics.getSupported(),
		rgba16f = love.graphics.getCanvasFormats()["rgba16f"],
		mat3 = {
			glsl = "mat3",
			size = ffi.sizeof("ToveMatrix3x3")
		}
	}

	-- work around crashing Parallels drivers with mat3
	do
		local _, _, _, device = love.graphics.getRendererInfo()
		if string.find(device, "Parallels") ~= nil then
			env.mat3.glsl = "mat3x4"
			env.mat3.size = ffi.sizeof("float[?]", 12)
		end
	end

	-- common attributes used by Command and Curve.
	local _attributes = {
		cp1x = 0,
		cp1y = 1,
		cp2x = 2,
		cp2y = 3,
		x = 4,
		y = 5,
		x0 = -2,
		y0 = -1,

		w = 6,
		h = 7,

		cx = 100,
		cy = 101,
		rx = 102,
		ry = 103,
		r = 104,
	}

	local function totable(u, n)
		local t = {}
		for i = 1, n do
			t[i] = u[i - 1]
		end
		return t
	end

	local bit = require("bit")

	--!! import "paint.lua" as Paint
	--!! import "command.lua" as newCommand
	--!! import "trajectory.lua" as Trajectory
	--!! import "path.lua" as Path

	--!! import "core/mesh.lua" as _mesh
	--!! import "core/shader.lua" as _shaders
	--!! import "graphics.lua" as Graphics
	--!! import "shape.lua" as Shape

	--!! import "animation.lua" as Animation
end

tove.init()
