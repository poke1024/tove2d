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
	tovedemo.drawBackground()

	if not tovedemo.warmup(shaderRabbit) then
		tovedemo.draw("Zoom.")
		return
	end

	local t = love.timer.getTime() * 0.5
	local radius = 250
	local shiftX = math.cos(t) * radius
	local shiftY = math.sin(t) * radius

	local mx = love.mouse.getX()
	local my = love.mouse.getY()

	local show = "all"
	for i = 1, 3 do
		local x0 = 50 + (i - 1) * 250
		if mx >= x0 and mx < x0 + 200 and my >= 100 and my <= 100 + 400 then
			show = i
			break
		end
	end

	for i = 1, 3 do
		if show == "all" or show == i then
			local x0 = 50 + (i - 1) * 250

			if show == "all" then
				love.graphics.stencil(function()
					love.graphics.rectangle("fill", x0, 100, 200, 400)
				end, "replace", 1)
				love.graphics.setStencilTest("greater", 0)
			end

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

			if show == "all" then
				love.graphics.setColor(0, 0, 0)
				love.graphics.rectangle("line", x0, 100, 200, 400)

				love.graphics.setColor(0.2, 0.2, 0.2)
				love.graphics.print(name, 50 + (i - 1) * 250, 510)
				love.graphics.setColor(1, 1, 1)
			end
		end
	end

	tovedemo.drawForeground("Zoom.")
	tovedemo.attribution("Graphics by Mike Mac.")
end
