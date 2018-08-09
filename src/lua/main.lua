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

local basepath = (...) .. "/"
local ffi = require 'ffi'

ffi.cdef [[
--!! include "../cpp/_interface.h"
]]

local tove = {}

tove.init = function(path)
	local libName = {
		["OS X"] = "libTove.dylib",
		["Windows"] = "libTove.dll",
		["Linux"] = "libTove.so"
	}

	local lib = ffi.load(basepath .. libName[love.system.getOS()])
	tove.lib = lib
	tove.getVersion = function()
		return ffi.string(lib.GetVersion())
	end

	tove._debug = false

	tove._warnverbose = 1
	tove.warn = function(s)
		if tove._warnverbose == 0 then
			return
		end
		print("TÖVE warning: " .. s)
		if tove._warnverbose >= 2 then
			print(debug.traceback())
		end
	end
	tove.slow = tove.warn

	lib.SetWarningFunction(ffi.cast("ToveWarningFunction", function(s)
		tove.warn(ffi.string(s))
  	end))

	tove._highdpi = 1

	tove.configure = function(config)
		if config.debug ~= nil then
			tove_debug = config.debug
			tove.slow = tove.warn
		end
		if config.performancewarnings ~= nil then
			tove.slow = enabled and tove.warn or function() end
		end
		if config.highdpi ~= nil then
			tove._highdpi = config.highdpi and 2 or 1
		end
	end

	local env = {
		graphics = love.graphics.getSupported(),
		rgba16f = love.graphics.getCanvasFormats()["rgba16f"],
		mat3 = {
			glsl = "mat3",
			size = ffi.sizeof("ToveMatrix3x3"),
			type = lib.MATRIX_MAT3x3
		}
	}

	-- work around crashing Parallels drivers with mat3
	do
		local _, _, _, device = love.graphics.getRendererInfo()
		if string.find(device, "Parallels") ~= nil then
			env.mat3.glsl = "mat3x4"
			env.mat3.size = ffi.sizeof("float[?]", 12)
			env.mat3.type = lib.MATRIX_MAT3x4
		end
	end

	lib.ConfigureShaderCode(
		env.graphics.glsl3 and lib.TOVE_GLSL3 or lib.TOVE_GLSL2,
		env.mat3.type == lib.MATRIX_MAT3x4)

	-- deepcopy: taken from http://lua-users.org/wiki/CopyTable
	function deepcopy(orig)
	    local orig_type = type(orig)
	    local copy
	    if orig_type == 'table' then
	        copy = {}
	        for orig_key, orig_value in next, orig, nil do
	            copy[deepcopy(orig_key)] = deepcopy(orig_value)
	        end
	        setmetatable(copy, deepcopy(getmetatable(orig)))
	    else -- number, string, boolean, etc
	        copy = orig
	    end
	    return copy
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

	tove.Transform = {}

	tove.transformed = function(source, ...)
		local args = {...}
		local t
		if type(args[1]) == "number" then
			t = love.math.newTransform(...)
		else
			t = args[1]
		end
		local a, b, _, c, d, e, _, f = t:getMatrix()
		return setmetatable({
			s = source, args = {a, b, c, d, e, f}}, tove.Transform)
	end

	tove.ipairs = function(a)
		local i = 1
		local n = a.count
		return function()
			local j = i
			if j <= n then
				i = i + 1
				return j, a[j]
			else
				return nil
			end
		end
	end

	local bit = require("bit")

	tove.lineJoins = {
		miter = lib.LINEJOIN_MITER,
		round = lib.LINEJOIN_ROUND,
		bevel = lib.LINEJOIN_BEVEL
	}

	tove.fillRules = {
		nonzero = lib.FILLRULE_NON_ZERO,
		evenodd = lib.FILLRULE_EVEN_ODD
	}

	local function findEnum(enums, value)
		for k, v in pairs(enums) do
			if v == value then
				return k
			end
		end
		error("illegal enum value")
	end

	--!! import "paint.lua" as Paint
	--!! import "command.lua" as newCommand
	--!! import "subpath.lua" as Subpath
	--!! import "path.lua" as Path

	--!! import "core/mesh.lua" as _mesh
	--!! import "core/shader.lua" as _shaders
	--!! import "graphics.lua" as Graphics
	--!! import "shape.lua" as Shape

	--!! import "animation.lua" as Animation
end

tove.init()

return tove
