-- TÖVE Demo: Custom line and fill shaders.
-- (C) 2019 Bernhard Liebl, MIT license.

local tove = require "tove"
require "assets/tovedemo"

local function newHeart()
	-- taken from: https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes
	local g = tove.newGraphics()
	g:setLineWidth(2)
	g:moveTo(75, 40)
	g:curveTo(75, 37, 70, 25, 50, 25)
	g:curveTo(20, 25, 20, 62.5, 20, 62.5)
	g:curveTo(20, 80, 40, 102, 75, 120) -- pick this curve for animation
	g:curveTo(110, 102, 130, 80, 130, 62.5)
	g:curveTo(130, 62.5, 130, 25, 100, 25)
	g:curveTo(85, 25, 75, 37, 75, 40)
	return g
end

local fillShader
local lineShader

local function createHeart()
	local heart = newHeart()

	fillShader = tove.newShader([[
		uniform float time;
 
		vec4 COLOR(vec2 pos) {
			vec2 p = mod(pos, vec2(20));
			float r = 5 + sin(time * 2 + pos.x * 0.01 + cos(time + pos.y * 0.2)) * 2;
			return length(p - vec2(10)) < r ? vec4(0, 0, 0, 0) : vec4(0.8, 0.2, 0.2, 1);
		}
	]])

	heart:setFillColor(fillShader)
	heart:fill()
	
	lineShader = tove.newShader([[
		vec4 COLOR(vec2 pos) {
			vec2 p = mod(pos, vec2(20));
			return abs(p.y - 10) < 5 ? vec4(0.5, 0, 0, 1) : vec4(0.9, 0, 0, 1);
		}
	]])

	heart:setLineColor(lineShader)
	heart:stroke()

	heart:setDisplay("gpux")
	heart:rescale(500, true)
	return heart
end

local heart = createHeart()

function love.draw()
	tovedemo.draw("Custom Shaders.")

	fillShader:send("time", love.timer.getTime())

	heart:draw(
		love.graphics.getWidth() / 2,
		love.graphics.getHeight() / 2)
end
