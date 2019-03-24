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
	-- , "mesh"
	for _, mode in ipairs {"texture", "mesh"} do
		local graphics = newGraphics()
		local quality = {}
		if mode == "mesh" then
			quality = {1000}
			-- work around a current TOVE limitation with uniforms
			-- and gradient shaders. use flat mesh for rendering.
			graphics:setUsage("shaders", "avoid")
		end
		graphics:setDisplay(mode, unpack(quality))
		flow:add(mode, graphics)
	end
end

load(love.filesystem.read("local/house.svg"))

function love.draw()
	tovedemo.draw("Complex Clip Paths.")
	flow:draw()
end

function love.update(dt)
	flow:update(dt)
end
