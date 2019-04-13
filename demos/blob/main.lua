-- TÖVE Demo: Flipbooks and animations.
-- (C) 2018 Bernhard Liebl, MIT license.

local tove = require "tove"
require "assets/tovedemo"

-- enable to see some stats on triangulation cache.
--tove.setReportLevel("debug")

local svg1 = love.filesystem.read("gradient-circle/gradient-circle_00001.svg")
local svg2 = love.filesystem.read("gradient-circle/gradient-circle_00240.svg")

local tween = tove.newTween(svg1):to(svg2, 1)

local animations = {}

local meshAnimation =  tove.newAnimation(tween, "mesh", "rigid", 2, "none")
meshAnimation:setName("mesh")

table.insert(animations, tove.newFlipbook(8, tween, "texture"))
table.insert(animations, meshAnimation)
table.insert(animations, tove.newAnimation(tween, "gpux"))

local flow = tovedemo.newCoverFlow(0.5)
flow:add("flipbook", animations[1])
flow:add("mesh", animations[2])
flow:add("gpux", animations[3])

function love.draw()
	tovedemo.draw("Blob.")

	local speed = 1
	local t = math.abs(math.sin(love.timer.getTime() * speed))

	for i, animation in ipairs(animations) do
		animation.t = t
	end

	flow:draw()
end

function love.update(dt)
	flow:update(dt)
end
