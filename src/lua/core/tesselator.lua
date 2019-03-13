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

local _holes = {
	none = lib.TOVE_HOLES_NONE,
	cw = lib.TOVE_HOLES_CW,
	ccw = lib.TOVE_HOLES_CCW
}

tove.newAdaptiveTesselator = function(resolution, recursionLimit)
	return ffi.gc(lib.NewAdaptiveTesselator(
		resolution or 128, recursionLimit or 8), lib.ReleaseTesselator)
end

tove.newRigidTesselator = function(subdivisions, holes)
	return ffi.gc(lib.NewRigidTesselator(
		subdivisions, _holes[holes or "none"]), lib.ReleaseTesselator)
end

return function (usage, quality, ...)
	local t = type(quality)
	if t == "string" then
		if quality == "rigid" then
			return tove.newRigidTesselator(...)
		elseif quality == "adaptive" then
			return tove.newAdaptiveTesselator(...)
		else
			error("tesselator must be \"rigid\" or \"adaptive\".")			
		end
	elseif t == "number" then
		if usage["points"] == "dynamic" then
			return tove.newRigidTesselator(
				math.log(quality) / math.log(2), "cw")
		else
			return tove.newAdaptiveTesselator(quality)
		end
	elseif t == "nil" then
		if usage["points"] == "dynamic" then
			return tove.newRigidTesselator(4, "cw")
		else
			return tove.newAdaptiveTesselator(128)
		end
	else
		return quality
	end
end
