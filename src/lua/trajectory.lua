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
	return lib.TrajectoryGetValue(self.traj, self.i + _attributes[key])
end
Curve.__newindex = function (self, key, value)
	return lib.TrajectorySetValue(self.traj, self.i + _attributes[key], value)
end

local Curves = {}
Curves.__index = function (self, index)
	return setmetatable({traj = self.traj, i = 2 + (index - 1) * 6}, Curve)
end

local Points = {}
Points.__index = function (self, i)
	local n = lib.TrajectoryGetNumPoints(self.traj)
	if i < 1 or i > n then
		return nil
	end
	i = i - 1
	local p = lib.TrajectoryGetPoints(self.traj)
	return {x = p[2 * i + 0], y = p[2 * i + 1]}
end

local Trajectory = {}
Trajectory.__index = function (self, key)
	if key == "curves" then
		return setmetatable({traj = self}, Curves)
	elseif key == "ncurves" then
		return lib.TrajectoryGetNumCurves(self)
	elseif key == "points" then
		return setmetatable({traj = self}, Points)
	elseif key == "npoints" then
		return lib.TrajectoryGetNumPoints(self)
	else
		return Trajectory[key]
	end
end

function Trajectory:setPoints(t, n)
	lib.TrajectorySetPoints(self, t, n)
end

ffi.metatype("ToveTrajectoryRef", Trajectory)

tove.newTrajectory = function()
	return ffi.gc(lib.NewTrajectory(), lib.ReleaseTrajectory)
end

return Trajectory
