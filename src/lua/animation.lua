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

-- Helpers for animating things.

--- @module animation

--- A tween.
-- @type Tween
-- @set sort=true

local Tween = {}
Tween.__index = Tween

--- Create new tween.
-- @tparam Graphics graphics @{Graphics} which will be at frame 0 of the new tween
-- @treturn Tween new empty tween

tove.newTween = function(graphics)
	return setmetatable({_graphics0 = graphics, _to = {}, _duration = 0, _morph = false}, Tween)
end

tove.newMorph = function(graphics)
	local t = tove.newTween(graphics)
	t._morph = true
	return t
end

--- Add new tween step.
-- @usage
-- tween = tove.newTween(svg1):to(svg2, 0.3):to(svg3, 0.1)
-- @tparam Graphics graphics @{Graphics} which will be at the new step
-- @tparam number duration duration from current end of tween to step
-- @tparam[opt] function ease easing function
-- @treturn Tween the modified tween

function Tween:to(graphics, duration, ease)
	table.insert(self._to, {graphics = graphics, duration = duration, ease = ease or _linear})
	self._duration = self._duration + duration
	return self
end

local function morphify(graphics)
	local n = #graphics
	local refs = ffi.new("ToveGraphicsRef[?]", n)
	for i, g in ipairs(graphics) do
		refs[i - 1] = g._ref
	end
	lib.GraphicsMorphify(refs, n)
end

local function createGraphics(graphics)
	if type(graphics) == "string" then
		graphics = tove.newGraphics(graphics)
	end
	return graphics
end


--- A flipbook.
-- @type Flipbook
-- @set sort=true

local Flipbook = {}

--- Create new flipbook.
-- A flipbook consists of prerendered discrete frames. Performance
-- will be high, but memory requirements might be high as well.
-- @tparam number fps playback rate as frames per second
-- @tparam Tween tween describes the animation to play
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

		-- for flipbooks, we can morph between pairs of
		-- graphics (we don't need a global morph).
		local ag0 = g0
		local ag1 = g1
		if j0 <= duration - 1 and tween._morph then
			ag0 = ag0:clone()
			ag1 = ag1:clone()
			morphify({ag0, ag1})
		end

		for j = j0, duration - 1 do
			if ease == "none" then
				table.insert(frames, g0)
			else		
				local inbetween = tove.newGraphics()
				inbetween:animate(ag0, ag1, ease(j / duration))
				inbetween:setDisplay(unpack(display))

				-- you can create flipbooks with meshes. in this case,
				-- this will precache all triangulations that occur.
				inbetween:cacheKeyFrame()

				table.insert(frames, inbetween)
			end
		end

		table.insert(frames, g1)
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

--- Draw.
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
	graphics:setUsage("points", "stream")
	graphics:setUsage("colors", "stream")
	graphics:setDisplay(unpack(display))
	local offset = 0

	do
		local g = createGraphics(tween._graphics0)
		graphics:animate(g, g, 0)
		graphics:cacheKeyFrame()
		table.insert(keyframes, {graphics = g, offset = 0, duration = 0, ease = _linear})
	end

	for i, keyframe in ipairs(tween._to) do
		local g = createGraphics(keyframe.graphics)
		keyframe.graphics = g
		offset = offset + keyframe.duration
		keyframe.offset = offset
		graphics:animate(g, g, 0)
		graphics:cacheKeyFrame()
		table.insert(keyframes, keyframe)
	end

	if tween._morph then
		local g = {}
		for i, f in ipairs(keyframes) do
			table.insert(g, f.graphics)
		end
		morphify(g)
	end

	return setmetatable({_keyframes = keyframes, _graphics = graphics,
		_t = 0, _i = 1, _duration = tween._duration}, Animation)
end

Animation.__newindex = function(self, key, value)
	if key == "t" then
		local t = math.max(0, math.min(value, self._duration))
		self._t = t
		local f = self._keyframes
		local n = #f
		if n > 1 then
			local lo = 0
			local hi = n
			while lo < hi do
				local mid = math.floor((lo + hi) / 2)
				if f[mid + 1].offset < t then
					lo = mid + 1
				else
					hi = mid
				end
			end
			lo = math.min(math.max(lo, 1), n)
			local f0 = f[lo]
			local f1 = f[lo + 1]
			self._graphics:animate(f0.graphics, f1.graphics,
				f1.ease((t - f0.offset) / f1.duration))
		end
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

function Animation:setCacheSize(s)
	self._graphics:setCacheSize(s)
end

function Animation:setName(n)
	self._graphics:setName(n)
end
