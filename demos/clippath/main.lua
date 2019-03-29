-- TÖVE Demo: renderers.
-- (C) 2018 Bernhard Liebl, MIT license.

local tove = require "tove"
require "assets/tovedemo"

local flow

local function load(svg)
	local function newGraphics()
		return tove.newGraphics(svg, 200)
	end

	-- just some glue code for presentation.
	flow = tovedemo.newCoverFlow()

	for _, mode in ipairs {"texture", "mesh"} do
		local graphics = newGraphics()
		local quality = {}
		if mode == "mesh" then
			quality = {1000}
		end
		graphics:setDisplay(mode, unpack(quality))
		flow:add(mode, graphics)
	end
end

load(love.filesystem.read("local/house.svg"))

function love.draw()
	tovedemo.draw("Complex Clipping.")
	flow:draw()
end

function love.update(dt)
	flow:update(dt)
end
