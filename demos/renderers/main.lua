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
	for _, mode in ipairs {"texture", "mesh", "gpux"} do
		local graphics = newGraphics()
		local quality = {}
		if mode == "mesh" then
			quality = {200}
		end
		graphics:setDisplay(mode, unpack(quality))
		flow:add(mode, graphics)
	end
end

load(love.filesystem.read("assets/rabbit.svg"))

function love.draw()
	tovedemo.draw("Renderers.")
	flow:draw()
end

function love.update(dt)
	flow:update(dt)
end

function love.filedropped(file)
	file:open("r")
	local svg = file:read()
	file:close()
	load(svg)
end
