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

local Curve = {}
Curve.__index = function (self, key)
	return lib.TrajectoryGetCurveValue(self.traj, self.curve, _attributes[key])
end
Curve.__newindex = function (self, key, value)
	return lib.TrajectorySetCurveValue(self.traj, self.curve, _attributes[key], value)
end

local Curves = {}
Curves.__index = function (self, curve)
	return setmetatable({traj = self.traj, curve = curve - 1}, Curve)
end

local _pt = {
	x = 0,
	y = 1
}

local Point = {}
Point.__index = function (self, key)
	return lib.TrajectoryGetPtValue(self.traj, self.i, _pt[key] or -1)
end
Point.__newindex = function (self, key, value)
	lib.TrajectorySetPtValue(self.traj, self.i, _pt[key] or -1, value)
end

local Points = {}
Points.__index = function (self, i)
	return setmetatable({traj = self.traj, i = i - 1}, Point)
end


local Trajectory = {}
Trajectory.__index = function (self, key)
	if key == "curves" then
		return setmetatable({traj = self}, Curves)
	elseif key == "ncurves" then
		return lib.TrajectoryGetNumCurves(self)
	elseif key == "pts" then
		return setmetatable({traj = self}, Points)
	elseif key == "npts" then
		return lib.TrajectoryGetNumPoints(self)
	elseif key == "closed" then
		return lib.TrajectoryIsClosed(self)
	else
		return Trajectory[key]
	end
end

function Trajectory:position(t)
	local v = lib.TrajectoryGetPosition(self, t)
	return v.x, v.y
end

function Trajectory:normal(t)
	local v = lib.TrajectoryGetNormal(self, t)
	return v.x, v.y
end

function Trajectory:closest(x, y, dmax, dmin)
	return lib.TrajectoryClosest(self, x, y, dmin or 1e-4, dmax or 1e50)
end

Trajectory.insertCurveAt = lib.TrajectoryInsertCurveAt
Trajectory.removeCurve = lib.TrajectoryRemoveCurve
Trajectory.mould = lib.TrajectoryMould
Trajectory.setPoints = lib.TrajectorySetPoints
Trajectory.moveTo = lib.TrajectoryMoveTo
Trajectory.lineTo = lib.TrajectoryLineTo
Trajectory.curveTo = lib.TrajectoryCurveTo

ffi.metatype("ToveTrajectoryRef", Trajectory)

tove.newTrajectory = function()
	return ffi.gc(lib.NewTrajectory(), lib.ReleaseTrajectory)
end

return Trajectory
