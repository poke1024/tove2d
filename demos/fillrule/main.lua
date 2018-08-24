-- Tove2D Demo: Graphics display renderers.
-- (C) 2018 Bernhard Liebl, MIT license.

local tove = require "tove"
require "assets/tovedemo"

-- fill rule example taken from http://tutorials.jenkov.com/svg/fill.html
local samples = {}
for i, display in ipairs({"texture", "mesh", "gpux"}) do
	samples[i] = {label = display, row = {}}
	for j, fillRule in ipairs({"evenodd", "nonzero"}) do
		local g = tove.newGraphics()
		local path = tove.newPath("M50,20 l40,40 l-40,40 l-40,-40 l40,-40 M50,40 l20,20 l-20,20 l-20,-20 l20,-20")
		path:setFillColor("#6666ff")
		path:setLineColor(0, 0, 0)
		path:setLineWidth(2)
		path:setFillRule(fillRule)
		g:addPath(path)
		g:setDisplay(display)
		samples[i].row[j] = g
	end
end

font = tovedemo.newFont(30)

function love.draw()
	tovedemo.draw("Fill Rule Support.")

	love.graphics.setFont(font)

	x0 = 0
	for i = 1, #samples do
		x0 = math.max(x0, font:getWidth(samples[i].label))
	end

	local dx = 100

	for j, rulename in ipairs({"evenodd", "nonzero"}) do
		love.graphics.setColor(0.2, 0.3, 0.2)
		love.graphics.print(rulename, x0 + j * dx + 20, 100)
	end

	for i = 1, #samples do
		love.graphics.push()
		love.graphics.translate(0, i * 100 + 20)
		love.graphics.setColor(0.2, 0.3, 0.2)
		love.graphics.print(samples[i].label, 100, 50)
		love.graphics.setColor(1, 1, 1)
		for j = 1, #samples[i].row do
			love.graphics.push()
			love.graphics.translate(x0 + j * dx, 10)
			samples[i].row[j]:draw()
			love.graphics.pop()
		end
		love.graphics.pop()
	end
end
