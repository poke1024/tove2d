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
	for i, mode in ipairs {"texture", "mesh", "gpux"} do
		local graphics = newGraphics(i)
		local quality = {}
		if mode == "mesh" then
			quality = {200}
		end
		graphics:setDisplay(mode, unpack(quality))
		flow:add(mode, graphics)
	end
end

load(love.filesystem.read("assets/monster_by_mike_mac.svg"))

function love.draw()
	tovedemo.draw("Renderers.")

	-- on the first runs, display a progress on shader compilation.
	if not tovedemo.warmup(flow) then
		return
	end
	
	tovedemo.attribution("Graphics by Mike Mac.")
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
