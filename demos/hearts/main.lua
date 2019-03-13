-- TÖVE Demo: Procedural form and color animation.
-- (C) 2018 Bernhard Liebl, MIT license.

local tove = require "tove"
require "assets/tovedemo"

local function newHeart()
	-- taken from: https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes
	local g = tove.newGraphics()
	g:setLineWidth(2)
	g:moveTo(75, 40)
	g:curveTo(75, 37, 70, 25, 50, 25)
	g:curveTo(20, 25, 20, 62.5, 20, 62.5)
	local animatedCurve = g:curveTo(20, 80, 40, 102, 75, 120) -- pick this curve for animation
	g:curveTo(110, 102, 130, 80, 130, 62.5)
	g:curveTo(130, 62.5, 130, 25, 100, 25)
	g:curveTo(85, 25, 75, 37, 75, 40)
	return g, animatedCurve
end

local function newLinearGradient(r)
	local c = math.cos(r)
	local s = math.sin(r)
	--local t = 1 + math.sin(r) * 0.2
	local ox, oy = 80, 80
	local gradient = tove.newLinearGradient(ox, oy, ox + c * 50, oy + s * 50)
	gradient:addColorStop(0, 0.859, 0.424, 0.631)
	gradient:addColorStop(1, 1, 1, 1) -- 0.231, 0.451, 0.710)
	return gradient
end

local hearts = {}
local modes = {"texture", "mesh", "mesh", "gpux"}
local names = {}

local function createHearts(stroke, filled)
	for i = 1, 4 do
		local heart, curve = newHeart()

		if filled then
			if i == 2 then
				heart:setFillColor(0.859, 0.424, 0.631)
			else
				heart:setFillColor(newLinearGradient(0))
			end
			heart:fill()
		end

		if stroke then
			heart:stroke()
		end

		-- cleanup for animated mesh rendering.
		heart:setOrientation("ccw")

		local quality = {}
		if modes[i] == "mesh" then
			quality = {"rigid", 4}
		end

		heart:setDisplay(modes[i], unpack(quality))
		heart:setUsage("points", "dynamic")
		heart:setUsage("colors", "dynamic")

		names[i] = modes[i]
		hearts[i] = {graphics = heart, animatedCurve = curve}
	end
end

local outlined = true
local filled = true
createHearts(outlined, filled)

-- remember initial point position.
local cx0 = hearts[1].animatedCurve.x
local cy0 = hearts[1].animatedCurve.y

-- dts for time measurements.
local dts = {}
for i = 1, 4 do
	dts[i] = 0
end

local function drawHeart(i, h, t)
	local column = 150
	local x0 = love.graphics.getWidth() / 2 - (column * 3 + 130) / 2

	-- use transforms to center the heart on the sceen (instead of using
	-- Graphics:transform, which would mess up our gradient's coordinate system).

	love.graphics.push("transform")
	love.graphics.translate(x0 + (i - 1) * column, love.graphics:getHeight() / 2 - 70)
	love.graphics.push("transform")
	love.graphics.rotate(math.sin(t * 1.32))
	local t0 = love.timer.getTime()

	h.graphics:draw()

	local dt = love.timer.getTime() - t0
	love.graphics.pop()
	dts[i] = 0.9 * dts[i] + 0.1 * dt
	love.graphics.setColor(0.3, 0.3, 0.3, 0.5)
	love.graphics.print(string.format("%s\n%.1f ms", names[i], dts[i] * 1000), 30, 200)
	love.graphics.setColor(1, 1, 1)
	love.graphics.pop()
end

function love.draw()
	tovedemo.draw("Hearts.", "You can toggle (s)troke and (f)ill.")

	local t = love.timer.getTime()
	for i = 1, 4 do
		local h = hearts[i]

		-- change point position.
		h.animatedCurve.x = cx0 + math.sin(t * 4) * 40
		h.animatedCurve.y = cy0 + (0.25 + math.sin(t * 1.2 + 0.3)) * 20

		-- change fill color.
		if filled and i ~= 2 then
			h.graphics.paths[1]:setFillColor(newLinearGradient(t))
		end

		-- now draw the heart.
		drawHeart(i, h, t)
	end
end

function love.keypressed(key)
	if key == "s" then
		outlined = not outlined
		createHearts(outlined, filled)
	end
	if key == "f" then
		filled = not filled
		createHearts(outlined, filled)
	end
end
