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
	none = lib.HOLES_NONE,
	cw = lib.HOLES_CW,
	ccw = lib.HOLES_CCW
}

local fixed = {}
tove.fixed = function(depth, holes)
	return setmetatable({depth = depth,
		holes = _holes[holes] or lib.HOLES_CW}, fixed)
end

local adaptive = {}
tove.adaptive = function(quality)
	return setmetatable(quality, adaptive)
end

tove.quasifixed = function(depth)
	return tove.adaptive {
		recursionLimit = depth,
		distanceTolerance = 1e-8,
		colinearityEpsilon = 0,
		angleTolerance = 0,
		angleEpsilon = 0,
		cuspLimit = 0
	}
end

local function setAdaptiveQuality(record, quality)
	record.recursionLimit = 8
	local adaptive = record.adaptive
	adaptive.valid = true

	-- tricky stuff: some configurations make higher qualities produce
	-- less triangles due to colinearityEpsilon being too high.

	if quality < 0.1 then -- special low quality tier
		adaptive.distanceTolerance = 100.0
		adaptive.colinearityEpsilon = 1.0 / quality
		adaptive.angleTolerance = (math.pi / 16) * (0.5 / quality)
	else
		adaptive.distanceTolerance = 100.0 / (1 + 10 * quality)
		adaptive.colinearityEpsilon = 0.1 / (1 + 10 * quality)
		adaptive.angleTolerance = math.pi / (2 + math.min(1, quality) * 16)
	end
	adaptive.angleEpsilon = 0.0
	adaptive.cuspLimit = 0.0
end

return function (quality, usage)
	local t = type(quality)
	local holes = lib.HOLES_CW  -- default for adaptive.
	if t == "number" then
		local record = ffi.new("ToveTesselationQuality")
		if usage["points"] == "dynamic" then
			record.adaptive.valid = false
			record.recursionLimit = math.floor(quality * 4)
		else
			setAdaptiveQuality(record, quality)
		end
		quality = record
	elseif t == "nil" then
		local record = ffi.new("ToveTesselationQuality")
		if usage["points"] == "dynamic" then
			record.adaptive.valid = false
			record.recursionLimit = 5
		else
			setAdaptiveQuality(record, 1)
		end
		quality = record
	elseif t == "table" and getmetatable(quality) == fixed then
		holes = quality.holes
		local record = ffi.new("ToveTesselationQuality")
		record.adaptive.valid = false
		record.recursionLimit = quality.depth
		quality = record
	elseif t == "table" and getmetatable(quality) == adaptive then
		local record = ffi.new("ToveTesselationQuality")
		record.recursionLimit = quality.recursionLimit or 8
		local adaptive = record.adaptive
		adaptive.valid = true
		adaptive.distanceTolerance = quality.distanceTolerance or 100.0
		adaptive.colinearityEpsilon = quality.colinearityEpsilon or 1.0
		adaptive.angleTolerance = quality.angleTolerance or (math.pi / 16)
		adaptive.angleEpsilon = quality.angleEpsilon or 0.0
		adaptive.cuspLimit = quality.cuspLimit or 0.0
		quality = record
	end
	return {quality, holes}
end
