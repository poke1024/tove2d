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

--- Represents a graphics primitive, for example a circle or a curve.
-- Commands allow reading and writing properties of primitives after
-- they have been created. Only certain properties are available for
-- each type of primitive. For example, a circle knows cx, cy and r.
-- @classmod Command

--- width
-- @table[readonly] w

--- height
-- @table[readonly] h

--- x coordinate of center
-- @usage
-- local circle = graphics:drawCircle(0, 0, 10)
-- circle.cx = 5
-- @table[readonly] cx

--- y coordinate of center
-- @table[readonly] cy

--- x radius
-- @table[readonly] rx

--- y radius
-- @table[readonly] ry

--- radius
-- @table[readonly] r

--- @section Curves

--- x coordinate of curve point P0
-- @table[readonly] x0

--- y coordinate of curve point P0
-- @table[readonly] y0

--- x coordinate of first control point P1
-- @table[readonly] cp1x

--- y coordinate of first control point P1
-- @table[readonly] cp1y

--- x coordinate of second control point P2
-- @table[readonly] cp2x

--- y coordinate of second control point P2
-- @table[readonly] cp2y

--- x coordinate of curve point P3
-- @table[readonly] x

--- y coordinate of curve point P3
-- @table[readonly] y

local Command = {}
Command.__index = function(self, key)
	local a = _attributes[key]
	if a ~= nil then
		return lib.SubpathGetCommandValue(self._t, self._c, a)
	else
		return Command[key]
	end
end
Command.__newindex = function(self, key, value)
	lib.SubpathSetCommandValue(
		rawget(self, "_t"), rawget(self, "_c"), _attributes[key], value)
end

function Command:commit()
	lib.SubpathCommit(self._t)
end

return function(trajectory, command)
	return setmetatable({_t = trajectory, _c = command}, Command)
end
