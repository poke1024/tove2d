-- TÖVE Demo: Zoom.
-- (C) 2018 Bernhard Liebl, MIT license.

local tove = require "tove"
require "assets/tovedemo"

local rabbit = love.filesystem.read("assets/rabbit.svg")

local function newRabbit()
	-- make a new rabbit graphics, prescaled to 200 px
	local graphics = tove.newGraphics(rabbit)
	graphics:rescale(200)
	return graphics
end

local textureRabbit = newRabbit()
textureRabbit:setDisplay("texture")

local meshRabbit = newRabbit()
meshRabbit:setDisplay("mesh", "rigid", 0)

local shaderRabbit = newRabbit()
shaderRabbit:setDisplay("gpux")

function love.draw()
	tovedemo.draw("Zoom.")

	for i = 1, 3 do
		love.graphics.stencil(function()
			love.graphics.rectangle("fill", 50 + (i - 1) * 250, 100, 200, 200)
		end, "replace", 1)
		love.graphics.setStencilTest("greater", 0)

		local name

		love.graphics.push("transform")
		love.graphics.scale(5, 5)
		if i == 1 then
			textureRabbit:draw(20, 20)
			name = "texture"
		elseif i == 2 then
			meshRabbit:draw(70, 20)
			name = "mesh"
		elseif i == 3 then
			shaderRabbit:draw(110, 20)
			name = "gpux"
		end
		love.graphics.pop()

		love.graphics.setStencilTest()

		love.graphics.setColor(0.2, 0.2, 0.2)
		love.graphics.print(name, 50 + (i - 1) * 250, 310)
		love.graphics.setColor(1, 1, 1)
	end
end
