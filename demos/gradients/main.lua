-- TÖVE Demo: renderers.
-- (C) 2018 Bernhard Liebl, MIT license.

local tove = require "tove"
require "assets/tovedemo"

local flow

local function load(svg)
	local function newGraphics()
		return tove.newGraphics(svg, 750)
	end

	-- just some glue code for presentation.
	flow = tovedemo.newCoverFlow(0.5)
	for i, mode in ipairs {"texture", "texture", "mesh"} do
		local graphics = newGraphics()
		local quality = {}
		if i == 1 then
			quality = {"fast"}
			desc = "tex fast"
		elseif i == 2 then
			quality = {"best"}
			desc = "tex best (default)"
		else
			desc ="mesh"
		end
		graphics:setDisplay(mode, unpack(quality))
		local r = flow:add(desc, graphics)
		r.minsize = 100
		r.maxsize = 500
	end
end

load(love.filesystem.read("gradients.svg"))

function love.draw()
	tovedemo.draw("Gradients.")
	flow:draw()
end

function love.update(dt)
	flow:update(dt)
end
