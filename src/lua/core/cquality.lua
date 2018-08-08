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

tove.quality = {}

local fixed = {}
tove.quality.fixed = function(depth, holes)
	return setmetatable({depth = depth,
		holes = _holes[holes] or lib.HOLES_CW}, fixed)
end

local maxerror = {}
tove.quality.maxerror = function(quality)
	return setmetatable(quality, maxerror)
end

local antigrain = {}
tove.quality.antigrain = function(quality)
	return setmetatable(quality, antigrain)
end

tove.quality.quasifixed = function(depth)
	return tove.stop.maxerror {
		recursionLimit = depth
	}
end

local function createAdaptiveQuality(quality)
	local resolution = quality * 2
	local record = ffi.new("ToveTesselationQuality")
	record.stopCriterion = lib.TOVE_MAX_ERROR
	record.recursionLimit = 8
	record.maximumError = 1.0 / resolution
	record.arcTolerance = 1.0 / resolution
	return record
end

local function createAntigrainQuality(quality)
	local record = ffi.new("ToveTesselationQuality")
	record.stopCriterion = lib.TOVE_ANTIGRAIN

 	if type(quality) == "number" then
		-- deprecated. this code is awful.
		record.recursionLimit = 8
		-- arc tolerance for rounded line joins (used inside ClipperLib).
		record.arcTolerance = 20.0 / quality

		-- tricky stuff: some configurations make higher qualities produce
		-- less triangles due to colinearityEpsilon being too high.
		local ag = record.antigrain
		ag.distanceTolerance = 100 * 1000.0 / quality
		ag.colinearityEpsilon = 100 * 1000.0 / (quality * quality * 10)
		ag.angleTolerance = math.pi / (2 + math.min(1, quality) * 16)

		ag.angleEpsilon = 0.0
		ag.cuspLimit = 0.0
	else
		record.recursionLimit = quality.recursionLimit or 8
		record.arcTolerance = quality.arcTolerance or 0.0

		local antigrain = record.antigrain
		antigrain.distanceTolerance = quality.distanceTolerance or 0.0
		antigrain.colinearityEpsilon = quality.colinearityEpsilon or 0.0
		antigrain.angleTolerance = quality.angleTolerance or 0.0
		antigrain.angleEpsilon = quality.angleEpsilon or 0.0
		antigrain.cuspLimit = quality.cuspLimit or 0.0
	end

	return record
end

return function (quality, usage)
	local t = type(quality)
	local holes = lib.HOLES_CW  -- default for adaptive.
	if t == "number" then
		if usage["points"] == "dynamic" then
			local record = ffi.new("ToveTesselationQuality")
			record.stopCriterion = lib.TOVE_REC_DEPTH
			record.recursionLimit = math.floor(quality * 4)
			quality = record
		else
			quality = createAdaptiveQuality(quality)
		end
	elseif t == "nil" then
		if usage["points"] == "dynamic" then
			local record = ffi.new("ToveTesselationQuality")
			record.stopCriterion = lib.TOVE_REC_DEPTH
			record.recursionLimit = 5
			quality = record
		else
			quality = createAdaptiveQuality(1)
		end
	elseif t == "table" and getmetatable(quality) == fixed then
		holes = quality.holes
		local record = ffi.new("ToveTesselationQuality")
		record.stopCriterion = lib.TOVE_REC_DEPTH
		record.recursionLimit = quality.depth
		quality = record
	elseif t == "table" and getmetatable(quality) == maxerror then
		local record = ffi.new("ToveTesselationQuality")
		record.stopCriterion = lib.TOVE_MAX_ERROR
		record.recursionLimit = quality.recursionLimit or 8
		record.arcTolerance = quality.arcTolerance or 0
		record.maximumError = quality.maximumError or 0
	elseif getmetatable(quality) == antigrain then
		quality = createAntigrainQuality(quality)
	end
	return {quality, holes}
end
