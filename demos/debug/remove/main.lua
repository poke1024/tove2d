local tove = require "tove"
require "assets/tovedemo"

font = tovedemo.newFont(30)

local g = tove.newGraphics()
g:setDisplay("mesh")

local flow = tovedemo.newCoverFlow(0.5)
flow:add("demo", g)

function love.draw()
	tovedemo.draw("Path Add/Remove.")

	love.graphics.setFont(font)

	flow:draw()
end

function love.update(dt)
	flow:update(dt)
end

function love.keypressed(key, unicode)
	if unicode == "x" then
		g:removePath(g.paths[1])
	elseif unicode == "a" then
		local path = tove.newPath()

		path:beginSubpath():drawCircle(
			(math.random() - 0.5) * 100,
			(math.random() - 0.5) * 100,
			(1 + math.random()) * 10)
		path:setFillColor(
			tove.newColor(math.random(), math.random(), math.random()))
		path:setLineColor(0, 0, 0)
		path:setLineWidth(2)

		g:addPath(path)
	end
end
