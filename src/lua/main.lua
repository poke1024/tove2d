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

--- @module tove

--!! include "license.lua"

local ffi = require 'ffi'

ffi.cdef [[
--!! include "../cpp/interface/types.h"
--!! include "../cpp/interface/api.h"
]]

local tove = {}

tove.init = function(path)

	local dynamicLibraryPathResolver = function()

		local getSystemPathSeparator = function()
			if love.system.getOS() == "Windows" then
				return "\\"
			else
				return "/"
			end
		end

		local libName = {
			["OS X"] = "libTove.dylib",
			["Windows"] = "libTove.dll",
			["Linux"] = "libTove.so"
		}

		local envPath = os.getenv("TOVE_DYNAMIC_LIB_PREFIX")
		local fileSep = getSystemPathSeparator()

		if envPath == nil then
			-- We expect tove2d's lib to live inside a folder called "tove" in the game's main
			-- source folder. Should fix https://github.com/poke1024/tove2d/issues/21
			local projectPrefix = love.filesystem.getSource()

			return projectPrefix .. fileSep .. "tove".. fileSep .. libName[love.system.getOS()]
		else
			-- Unless a environmental variable is defined and contains
			-- the absolute path up to, but not including, the dynamic library file
			return envPath .. fileSep .. libName[love.system.getOS()]
		end
	end

	libPath = dynamicLibraryPathResolver()
	local lib = ffi.load(libPath)
	tove.lib = lib
	tove.getVersion = function()
		return ffi.string(lib.GetVersion())
	end

	do
		local reportLevel

		local levels = {"debug", "slow", "warn", "err", "fatal"}
		local ilevels = {}
		local outputs = {}

		for i, n in ipairs(levels) do
			ilevels[n] = i
		end

		tove.setReportLevel = function(level)
			local l = ilevels[level]
			if l ~= nil then
				reportLevel = l
				lib.SetReportLevel(l)
			else
				print("illegal report level " .. level)
			end
		end

		tove.getReportLevel = function()
			return reportLevel
		end

		local report = function(s, l)
			l = tonumber(l)
			if l >= reportLevel then
				if outputs[l] ~= s then  -- dry
					outputs[l] = s
					print("TÖVE [" .. levels[l] .. "] " .. s)
					if l >= math.min(reportLevel + 1, lib.TOVE_REPORT_ERR) then
						print("     " .. debug.traceback())
					end
				end
			end
		end

		tove.report = report
		tove.warn = function(s)
			report(s, lib.TOVE_REPORT_WARN)
		end
		tove.slow = function(s)
			report(s, lib.TOVE_REPORT_SLOW)
		end

		lib.SetReportFunction(ffi.cast("ToveReportFunction", function(s, level)
			report(ffi.string(s), level)
		end))

		tove.setReportLevel("slow")
	end

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
		matrows = 2 -- number of mat3x2 rows
	}

	-- work around crashing Parallels drivers with mat3
	do
		local _, _, _, device = love.graphics.getRendererInfo()
		if string.find(device, "Parallels") ~= nil then
			env.matrows = 4
		end
	end

	env.matsize = ffi.sizeof("float[?]", 3 * env.matrows)

	lib.ConfigureShaderCode(
		env.graphics.glsl3 and lib.TOVE_GLSL3 or lib.TOVE_GLSL2,
		env.matrows)

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

	--- The visual approach with which lines are joined.
	-- Also see <a href="https://developer.mozilla.org/en-US/docs/Web/SVG/Attribute/stroke-linejoin">Mozilla SVG documentation</a>.
	-- @table LineJoin
	-- @field miter Mitered (sharp) line joins
	-- @field round Round line joins
	-- @field bevel Bevel (flat) line joins

	tove.lineJoins = {
		miter = lib.TOVE_LINEJOIN_MITER,
		round = lib.TOVE_LINEJOIN_ROUND,
		bevel = lib.TOVE_LINEJOIN_BEVEL
	}

	--- The shape used to draw the end points of lines.
	-- Also see <a href="https://developer.mozilla.org/en-US/docs/Web/SVG/Attribute/stroke-linecap">Mozilla SVG documentation</a>.
	-- @table LineCap
	-- @field butt Butt line caps
	-- @field round Round line caps
	-- @field square Square line caps

	tove.lineCaps = {
		butt = lib.TOVE_LINECAP_BUTT,
		round = lib.TOVE_LINECAP_ROUND,
		square = lib.TOVE_LINECAP_SQUARE
	}

	--- The rule by which solidness in paths is defined.
	-- Also see <a href="https://developer.mozilla.org/en-US/docs/Web/SVG/Attribute/fill-rule">Mozilla SVG documentation</a>
	-- @table FillRule
	-- @field nonzero Non-zero number of path crossings defines insideness
	-- @field evenodd Odd number of path crossings defines insideness

	tove.fillRules = {
		nonzero = lib.TOVE_FILLRULE_NON_ZERO,
		evenodd = lib.TOVE_FILLRULE_EVEN_ODD
	}

	tove.elements = {
		curve = lib.TOVE_CURVE,
		point = lib.TOVE_POINT,
		subpath = lib.TOVE_SUBPATH,
		path = lib.TOVE_PATH
	}

	local function findEnum(enums, value)
		for k, v in pairs(enums) do
			if v == value then
				return k
			end
		end
		error("illegal enum value")
	end

	tove.createBlueNoise = function(s)
		local d = love.data.newByteData(ffi.sizeof("float[?]", s * s))
		if not lib.GenerateBlueNoise(s, d:getPointer()) then d = nil end
		return d
	end

	local function warmupShader(shader)
		love.graphics.setShader(shader)
		-- the following is a bit of a hack to make love2d actually load
		-- and compile the shader. it won't without the following line.
		love.graphics.rectangle("fill", 0, 0, 0, 0)
	end

	tove._str = function(name)
		return ffi.string(lib.NameCStr(name))
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

--- A list of elements, such as e.g. @{Path}s.
-- @type List

--- Total number of elements in this list.
-- @tparam number count number of elements in this list
-- @usage
-- graphics.paths.count -- number of paths
-- graphics.paths[1].subpaths.count -- number of subpaths in path 1
-- graphics.paths[1].subpaths[1].curves.count -- number of curves in subpath 1 in path 1

--- Get element at index.
-- @tparam number index 1-based index of element to retrieve
-- @return retrieved element
-- @usage
-- graphics.paths[1]  -- retrieve first path
-- @function at

--- Get element with name.
-- @tparam string name name of element to retrieve
-- @return retrieved element
-- @usage
-- graphics.paths["circle"] -- retrieve the path named "circle"
-- @function at
