-- TÖVE Demo: tesselation.
-- (C) 2018 Bernhard Liebl, MIT license.

require "lib/tove"
require "assets/tovedemo"

local rabbit = love.filesystem.read("assets/rabbit.svg")

local function newRabbit()
	-- make a new rabbit graphics, prescaled to 200 px
	local graphics = tove.newGraphics(rabbit, 200)
	graphics:setResolution(2)
	graphics:setUsage("gradients", "fast")
	return graphics
end

local function newInfoText(rabbit, quality)
	return "quality " .. string.format("%.3f", quality) .. "\n" ..
		tostring(rabbit:getNumTriangles()) .. " triangles"
end

local flow = tovedemo.newCoverFlow()
local qualities = {0.01, 0.25, 1}
for i = 1, 3 do
	local rabbit = newRabbit()
	rabbit:setDisplay("mesh", qualities[i])
	flow:add(newInfoText(rabbit, qualities[i]), rabbit)
end

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
