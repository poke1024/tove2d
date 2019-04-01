-- TÖVE debugging for curve refinement.
-- (C) 2019 Bernhard Liebl, MIT license.

local tove = require "tove"
graphics = nil

function load(factor)
	svgData = love.filesystem.read("shape.svg")
	graphics = tove.newGraphics(svgData, 400)
	graphics:setDisplay("mesh", 400)
	for i = 1, graphics.paths.count do
		graphics.paths[i]:refine(factor)
	end
end

load(1)

function love.draw()
	love.graphics.setBackgroundColor(.8, .8, .8)
	
	local x = love.graphics.getWidth() / 2
	local y = love.graphics.getHeight() / 2
	love.graphics.setColor(1, 1, 1)
	graphics:draw(x, y)

	love.graphics.setColor(1, 0, 0)
	for i = 1, graphics.paths.count do
		local p = graphics.paths[i]
		for j = 1, p.subpaths.count do
			local sp = p.subpaths[j]
			for k = 1, sp.curves.count do
				local c = sp.curves[k]
				love.graphics.circle(
					"fill", x + c.x0, y + c.y0, 5)
			end
		end
	end
end

function love.keypressed(key, scancode, isrepeat)
	local i = string.byte(key) - string.byte("0")
	if i >= 1 and i <= 9 then
		load(i)
	end
end


