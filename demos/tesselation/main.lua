-- TÖVE Demo: tesselation.
-- (C) 2018 Bernhard Liebl, MIT license.

local tove = require "tove"
require "assets/tovedemo"

local function newInfoText(graphics, quality)
	return "resolution " .. string.format("%d", quality) .. "\n" ..
		tostring(graphics:getNumTriangles()) .. " triangles"
end

local flow

local function load(svg)
	-- makes a new graphics, prescaled to 200 px
	local function newGraphics()
		return tove.newGraphics(svg, 200)
	end

	-- just some glue code for presentation.
	flow = tovedemo.newCoverFlow()
	local qualities = {50, 100, 200}
	for i = 1, 3 do
		local graphics = newGraphics()
		graphics:setDisplay("mesh", qualities[i])
		local record = flow:add(newInfoText(graphics, qualities[i]), graphics)
		record.minsize = math.max(10, qualities[i])
	end
end

load(love.filesystem.read("assets/rabbit.svg"))

function love.draw()
	tovedemo.draw("Tesselation.")
	flow:draw()
end

function love.update(dt)
	flow:update(dt)

	local focus = flow:getFocus()
	if focus ~= 0 then
		local d = 0
		if love.keyboard.isDown("+") then
			d = 1
		elseif love.keyboard.isDown("-") then
			d = -1
		end
		if d ~= 0 then
			qualities[focus] = math.max(0, qualities[focus] + 0.001 * d)
			local rabbit = flow:getItem(focus)
			rabbit:setDisplay("mesh", qualities[focus])
			flow:setName(focus, newInfoText(rabbit, qualities[focus]))
		end
	end
end

function love.filedropped(file)
	file:open("r")
	local svg = file:read()
	file:close()
	load(svg)
end
