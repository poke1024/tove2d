-- TÖVE Demo: Morphing.
-- (C) 2019 Bernhard Liebl, MIT license.

-- Morphing is TÖVE's term for animating if the point/curve
-- count of the animated paths/subpaths does not match. TÖVE
-- will automatically insert additional refinement points to
-- enable animation.

local tove = require "tove"
require "assets/tovedemo"

-- load both shapes.
local svg1 = love.filesystem.read("shape1.svg")
local svg2 = love.filesystem.read("shape2.svg")
svg1 = tove.newGraphics(svg1, 300)
svg2 = tove.newGraphics(svg2, 300)

-- you can tweak some of the visuals by choosing which curves
-- align by rotating curves - might make a morph look better.
svg2.paths["body"]:rotate("curve", 1)
-- now create a morphing tween
local tween = tove.newMorph(svg1):to(svg2, 1)

-- standard demo code for displaying animated assets.
local animations = {}

table.insert(animations, tove.newFlipbook(8, tween, "texture"))
table.insert(animations, tove.newAnimation(tween, "mesh", "rigid", 2, "none"))
table.insert(animations, tove.newAnimation(tween, "gpux"))

local flow = tovedemo.newCoverFlow(0.5)
flow:add("flipbook", animations[1])
flow:add("mesh", animations[2])
flow:add("gpux", animations[3])

function love.draw()
	tovedemo.draw("Morph.")

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
