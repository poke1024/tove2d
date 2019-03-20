-- TÖVE Demo: renderers.
-- (C) 2018 Bernhard Liebl, MIT license.

local tove = require "tove"
require "assets/tovedemo"

local flow

local function load(svg)
	-- makes a new graphics, prescaled to 200 px
	local function newGraphics()
		return tove.newGraphics(svg, 200)
	end

	-- just some glue code for presentation.
	flow = tovedemo.newCoverFlow()
	for _, mode in ipairs {"texture", "mesh"} do
		local graphics = newGraphics()
		local quality = {}
		if mode == "mesh" then
			quality = {200}
		end
		graphics:setDisplay(mode, unpack(quality))
		flow:add(mode, graphics)
	end
end

load(love.filesystem.read("clippath.svg"))

function love.draw()
	tovedemo.draw("Clip Paths.")
	flow:draw()
end

function love.update(dt)
	flow:update(dt)
end
