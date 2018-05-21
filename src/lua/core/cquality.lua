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

local fixed = {}
tove.fixed = function(depth)
	return setmetatable({depth = depth}, fixed)
end

local adaptive = {}
tove.adaptive = function(quality)
	return setmetatable(quality, adaptive)
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
		adaptive.distanceTolerance = 100.0 / (1 + quality)
		adaptive.colinearityEpsilon = 0.1 / (1 + 10 * quality)
		adaptive.angleTolerance = math.pi / (8 + math.min(1, quality) * 8)
	end
	adaptive.angleEpsilon = 0.0
	adaptive.cuspLimit = 0.0
end

return function (quality, usage)
	local t = type(quality)
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
	return quality
end
