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

local Command = {}
Command.__index = function (self, key)
	return lib.SubpathGetCommandValue(self._t, self._c, _attributes[key])
end
Command.__newindex = function (self, key, value)
	lib.SubpathSetCommandValue(rawget(self, "_t"), rawget(self, "_c"), _attributes[key], value)
end

return function(trajectory, command)
	return setmetatable({_t = trajectory, _c = command}, Command)
end
