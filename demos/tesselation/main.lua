-- TÖVE Demo: tesselation.
-- (C) 2018 Bernhard Liebl, MIT license.

local tove = require "tove"
require "assets/tovedemo"

local function newInfoText(graphics, quality)
	return "r " .. string.format("%d", quality) .. " / " ..
		tostring(graphics:getNumTriangles()) .. " tris"
end

local flow
local qualities = {50, 100, 200}

local function load(svg)
	-- makes a new graphics, prescaled to 200 px
	local function newGraphics()
		return tove.newGraphics(svg, 200)
	end

	-- just some glue code for presentation.
	flow = tovedemo.newCoverFlow()
	for i = 1, 3 do
		local graphics = newGraphics()
		graphics:setDisplay("mesh", qualities[i])
		local record = flow:add(newInfoText(graphics, qualities[i]), graphics)
		record.minsize = math.max(10, qualities[i])
	end
end

load(love.filesystem.read("assets/monster_3_by_mike_mac.svg"))

function love.draw()
	tovedemo.draw("Tesselation.", "Use + / - to modify.")
	tovedemo.attribution("Graphics by Mike Mac.")
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
			qualities[focus] = math.max(0, qualities[focus] + d)
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
