-- TÖVE Demo: renderers.
-- (C) 2018 Bernhard Liebl, MIT license.

local tove = require "tove"
require "assets/tovedemo"

local flow
local g = {}
local wobble = {}
local t = 0

local function load(svg)
	-- makes a new graphics, prescaled to 200 px
	local function newGraphics()
		return tove.newGraphics(svg, 200)
	end

	-- just some glue code for presentation.
	flow = tovedemo.newCoverFlow()
	for _, mode in ipairs {"mesh", "gpux"} do
		local graphics = newGraphics()
		local quality = {}
		if mode == "gpux" then
			quality = {"vertex"}
		end
		graphics:setDisplay(mode, unpack(quality))
		graphics.paths[1]:setOpacity(0.1)
		flow:add(mode, graphics)
		g[mode] = graphics
	end

	for i = 1, g["gpux"].paths[1].subpaths[1].curves.count do 
		local c = g["gpux"].paths[1].subpaths[1].curves[i]
		wobble[i] = c.cp1x
	end
end

load(love.filesystem.read("shape.svg"))

function love.draw()
	tovedemo.draw("Complex Transparent Paths.")
	flow:draw()
end

function love.update(dt)
	flow:update(dt)

	-- wobble a bit
	for i = 1, g["gpux"].paths[1].subpaths[1].curves.count - 1 do 
		local c = g["gpux"].paths[1].subpaths[1].curves[i]
		c.cp1x = wobble[i] + math.sin(t * (1 + i * 0.3)) * 50
		t = t + dt
	end
end
