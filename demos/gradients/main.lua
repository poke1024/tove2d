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
	for i, mode in ipairs {"texture", "texture"} do
		local graphics = newGraphics()
		local quality = {}
		if i == 1 then
			quality = {"fast"}
			desc = "fast"
		else
			quality = {"best"}
			desc = "best (default)"
		end
		graphics:setDisplay(mode, unpack(quality))
		local r = flow:add(desc, graphics)
		r.minsize = 100
		r.maxsize = 500
	end
end

load(love.filesystem.read("gradients.svg"))

function love.draw()
	tovedemo.draw("Texture Gradients.")
	flow:draw()
end

function love.update(dt)
	flow:update(dt)
end
