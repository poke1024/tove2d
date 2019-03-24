-- TÖVE Demo: renderers.
-- (C) 2018 Bernhard Liebl, MIT license.

local tove = require "tove"
require "assets/tovedemo"

local flow

local function load()
	-- just some glue code for presentation.
	flow = tovedemo.newCoverFlow()

	local graphics
	
	local svg1 = love.filesystem.read("tree.svg")
	graphics = tove.newGraphics(svg1, 200)
	graphics:setDisplay("texture", "retro")
	flow:add("retro", graphics)

	local svg2 = love.filesystem.read("grad.svg")
	graphics = tove.newGraphics(svg2, 200)
	graphics:setDisplay("texture", "retro")
	flow:add("retro", graphics)

	graphics = tove.newGraphics(svg2, 200)
	graphics:setDisplay("texture", "retro", {{1, 1, 1}, {0, 0, 0}})
	flow:add("retro", graphics)
end

load()

function love.draw()
	tovedemo.draw("Retro.")
	flow:draw()
end

function love.update(dt)
	flow:update(dt)
end
