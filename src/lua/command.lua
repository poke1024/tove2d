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
-- @usage
-- 
-- local graphics = tove.newGraphics()
-- graphics:setFillColor(1, 1, 1)
-- local rect = graphics:drawRect(0, 0, 10, 10)
-- graphics:fill()
-- rect.w = 20 -- change/animate width
-- @table w

--- height
-- @table h

--- x coordinate of center
-- @usage
-- local circle = graphics:drawCircle(0, 0, 10)
-- circle.cx = 5 -- change/animate x position
-- @table cx

--- y coordinate of center
-- @table cy

--- x radius
-- @usage
-- local circle = graphics:drawCircle(0, 0, 10)
-- circle.rx = 5 -- change/animate x radius
-- @table rx

--- y radius
-- @table ry

--- radius
-- @usage
-- local circle = graphics:drawCircle(0, 0, 10)
-- circle.r = 5 -- change/animate radius
-- @table r

--- @section Curves

--- x coordinate of curve point P0
-- @table x0

--- y coordinate of curve point P0
-- @table y0

--- x coordinate of first control point P1
-- @table cp1x

--- y coordinate of first control point P1
-- @table cp1y

--- x coordinate of second control point P2
-- @table cp2x

--- y coordinate of second control point P2
-- @table cp2y

--- x coordinate of curve point P3
-- @table x

--- y coordinate of curve point P3
-- @table y

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
