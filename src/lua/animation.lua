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

local function _linear(x)
	return x
end


local Tween = {}
Tween.__index = Tween

tove.newTween = function(graphics)
	return setmetatable({_graphics0 = graphics, _to = {}, _duration = 0}, Tween)
end

function Tween:to(graphics, duration, ease)
	table.insert(self._to, {graphics = graphics, duration = duration, ease = ease or _linear})
	self._duration = self._duration + duration

	return self
end

local function createGraphics(graphics)
	if type(graphics) == "string" then
		graphics = tove.newGraphics(graphics)
	end
	return graphics
end

local Flipbook = {}

tove.newFlipbook = function(fps, tween, ...)
	display = {...}
	if next(display) == nil then
		display = {"texture"}
	end
	local frames = {}
	local looping = tween._graphics0 == tween._to[#tween._to].graphics
	for i, keyframe in ipairs(tween._to) do
		duration = math.ceil(keyframe.duration * fps)
		local g0 = frames[#frames]
		local j0 = 1
		if i == 1 then
			g0 = createGraphics(tween._graphics0)
			if not looping then
				j0 = 0
				duration = duration - 1
			end
		end
		local g1 = createGraphics(keyframe.graphics)
		local ease = keyframe.ease
		for j = j0, duration do
			if ease == "none" then
				table.insert(frames, j < duration and g0 or g1)
			else
				local inbetween = tove.newGraphics()
				inbetween:animate(g0, g1, ease(j / duration))
				inbetween:setDisplay(unpack(display))
				inbetween:cache()
				table.insert(frames, inbetween)
			end
		end
	end
	return setmetatable({_fps = fps, _frames = frames, _duration = tween._duration, _t = 0, _i = 1}, Flipbook)
end

Flipbook.__index = function(self, key)
	if key == "t" then
		return self._t
	else
		return Flipbook[key]
	end
end

Flipbook.__newindex = function(self, key, value)
	if key == "t" then
		self._t = math.max(0, math.min(value, self._duration))
		self._i = math.min(math.max(1,
			1 + math.floor(value * self._fps)), #self._frames)
	end
end

function Flipbook:draw(...)
	self._frames[self._i]:draw(...)
end


local Animation = {}

tove.newAnimation = function(tween, ...)
	display = {...}
	if next(display) == nil then
		display = {"mesh", 0.5} -- tove.fixed(2)
	end
	local keyframes = {}
	local graphics = tove.newGraphics()
	graphics:setUsage("points", "dynamic")
	graphics:setUsage("colors", "dynamic")
	graphics:setDisplay(unpack(display))
	local offset = 0

	do
		local g = createGraphics(tween._graphics0)
		graphics:animate(g, g, 0)
		graphics:cache("keyframe")
		table.insert(keyframes, {graphics = g, offset = 0, duration = 0, ease = _linear})
	end

	for i, keyframe in ipairs(tween._to) do
		local g = createGraphics(keyframe.graphics)
		keyframe.graphics = g
		offset = offset + keyframe.duration
		keyframe.offset = offset
		graphics:animate(g, g, 0)
		graphics:cache("keyframe")
		table.insert(keyframes, keyframe)
	end

	return setmetatable({_keyframes = keyframes, _graphics = graphics,
		_t = 0, _i = 1, _duration = tween._duration}, Animation)
end

Animation.__newindex = function(self, key, value)
	if key == "t" then
		local t = math.max(0, math.min(value, self._duration))
		self._t = t
		local f = self._keyframes
		local lo = 0
		local hi = #f
		while lo < hi do
			local mid = math.floor((lo + hi) / 2)
			if f[mid + 1].offset < t then
				lo = mid + 1
			else
				hi = mid
			end
		end
		lo = math.min(math.max(lo, 1), #f - 1)
		local f0 = f[lo]
		local f1 = f[lo + 1]
		self._graphics:animate(f0.graphics, f1.graphics,
			f1.ease((t - f0.offset) / f1.duration))
	end
end

Animation.__index = function(self, key)
	if key == "t" then
		return self._t
	elseif key == "paths" then
		return self._graphics.paths
	else
		return Animation[key]
	end
end

function Animation:draw(...)
	self._graphics:draw(...)
end

function Animation:debug(...)
	self._graphics:debug(...)
end
