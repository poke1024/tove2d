-- TÖVE Demo: Zoom.
-- (C) 2018 Bernhard Liebl, MIT license.

local tove = require "tove"
require "assets/tovedemo"

local rabbit = love.filesystem.read("assets/monster_by_mike_mac.svg")

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
	tovedemo.attribution("Graphics by Mike Mac.")

	if not tovedemo.warmup(shaderRabbit) then
		return
	end

	local t = love.timer.getTime() * 0.5
	local radius = 250
	local shiftX = math.cos(t) * radius
	local shiftY = math.sin(t) * radius

	for i = 1, 3 do
		local x0 = 50 + (i - 1) * 250

		love.graphics.stencil(function()
			love.graphics.rectangle("fill", x0, 100, 200, 400)
		end, "replace", 1)
		love.graphics.setStencilTest("greater", 0)

		local name

		love.graphics.push("transform")
		love.graphics.translate(x0 + shiftX, shiftY)
		love.graphics.scale(5, 5)
		if i == 1 then
			textureRabbit:draw(0, 0)
			name = "texture"
		elseif i == 2 then
			meshRabbit:draw(0, 0)
			name = "mesh"
		elseif i == 3 then
			shaderRabbit:draw(0, 0)
			name = "gpux"
		end
		love.graphics.pop()

		love.graphics.setStencilTest()

		love.graphics.setColor(0, 0, 0)
		love.graphics.rectangle("line", x0, 100, 200, 400)

		love.graphics.setColor(0.2, 0.2, 0.2)
		love.graphics.print(name, 50 + (i - 1) * 250, 510)
		love.graphics.setColor(1, 1, 1)
	end
end
