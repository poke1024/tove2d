-- TÖVE Demo: Code for debugging GPUX.
-- (C) 2018 Bernhard Liebl, MIT license.

local tove = require "tove"
require "assets/tovedemo"

local svg1 = love.filesystem.read("gradient-circle_00001.svg")
local svg2 = love.filesystem.read("gradient-circle_00002.svg")

local tween = tove.newTween(svg1):to(svg2, 1)

local animation = tove.newAnimation(tween, "gpux")

local flow = tovedemo.newCoverFlow(0.5)
flow:add("gpux", animation)

function love.draw()
	tovedemo.draw("GPUX Debug Tool.")

	local speed = 1
	local t = math.abs(math.sin(love.timer.getTime() * speed))
	animation.t = t

	flow:draw()
end

function love.update(dt)
	flow:update(dt)
end

local debugCurve = 0

local function debugNext()
	local c = 0
	for i = 1, animation.paths.count do
		for j = 1, animation.paths[i].subpaths.count do
			c = c + animation.paths[i].subpaths[j].curves.count
		end
	end
	if debugCurve >= c then
		animation:debug(1, "off")
		debugCurve = 0
	else
		animation:debug(1, debugCurve)
		debugCurve = debugCurve + 1
	end
end

function love.keypressed(key, unicode)
	if key == "#" then
		debugNext()
	end
end
