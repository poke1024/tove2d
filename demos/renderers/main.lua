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

local textureRabbit = newRabbit()
textureRabbit:setDisplay("texture")

local meshRabbit = newRabbit()
meshRabbit:setDisplay("mesh")

local flatmeshRabbit = newRabbit()
flatmeshRabbit:setDisplay("flatmesh")

local shaderRabbit = newRabbit()
shaderRabbit:setDisplay("shader")

-- just some glue code for presentation.
local flow = tovedemo.newCoverFlow()
flow:add("texture", textureRabbit)
flow:add("mesh", meshRabbit)
flow:add("flatmesh", flatmeshRabbit)
flow:add("shader", shaderRabbit)

function love.draw()
	tovedemo.draw("Renderers.")
	flow:draw()
end

function love.update(dt)
	flow:update(dt)
end
