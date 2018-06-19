-- TÖVE Demo: renderers.
-- (C) 2018 Bernhard Liebl, MIT license.

require "lib/tove"
require "assets/tovedemo"

local rabbit = love.filesystem.read("assets/rabbit.svg")

local function newRabbit()
	-- make a new rabbit graphics, prescaled to 200 px
	local graphics = tove.newGraphics(rabbit, 200)
	return graphics
end

local bitmapRabbit = newRabbit()
bitmapRabbit:setDisplay("bitmap")

local meshRabbit = newRabbit()
meshRabbit:setDisplay("mesh")

local flatmeshRabbit = newRabbit()
flatmeshRabbit:setDisplay("flatmesh")

local curveRabbit = newRabbit()
curveRabbit:setDisplay("curves")

-- just some glue code for presentation.
local flow = tovedemo.newCoverFlow()
flow:add("bitmap", bitmapRabbit)
flow:add("mesh", meshRabbit)
flow:add("flatmesh", flatmeshRabbit)
flow:add("curves", curveRabbit)

function love.draw()
	tovedemo.draw("Renderers.")
	flow:draw()
end

function love.update(dt)
	flow:update(dt)
end
