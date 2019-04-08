-- TÖVE Demo: Warping.
-- (C) 2019 Bernhard Liebl, MIT license.

-- Morphing is TÖVE's term for animating if the point/curve
-- count of the animated paths/subpaths does not match. TÖVE
-- will automatically insert additional refinement points to
-- enable animation.

local tove = require "tove"
require "assets/tovedemo"
require "tools"

local flow
local attribution = "Graphics by Mike Mac."
local tool = 1
local toolRadius = 20

local function load(svg)
	local graphics = tove.newGraphics(svg, 400)

	graphics:setDisplay("texture", "fast")
	graphics:setResolution(500 / 400)

	-- texture works best for this. there are too many triangulation
	-- issues with mesh mode. animation mode is not working at all.
	--graphics:setDisplay("mesh", 1000)
	--graphics:setUsage("points", "dynamic")

	flow = tovedemo.newCoverFlow(0.5)
	local r = flow:add("", graphics)
	r.minsize = 500
	r.maxsize = 500
end

load(love.filesystem.read("assets/monster_2_by_mike_mac.svg"))


function love.draw()
	tovedemo.draw("Warp.")
	tovedemo.attribution(attribution)
	flow:draw()

	love.graphics.circle("line", love.mouse.getX(), love.mouse.getY(), toolRadius * 2)

	drawToolsMenu(tool)
end

function love.update(dt)
	flow:update(dt)
end

local mouse = nil

function love.mousepressed(x, y, button)
	if button == 1 then
		local t = clickToolsMenu(x, y)
		if t == nil then
			local mx, my = unpack(flow.items[1].mouse)
			mouse = {x = mx, y = my}
		else
			tool = t
		end
	end
end

function love.mousereleased(x, y, button, istouch, presses)
	if button == 1 then
		mouse = nil
	end
end

function love.mousemoved(x, y, dx, dy, istouch)
	if flow.items[1].mouse == nil then
		return
	end
	if not love.mouse.isDown(1) then
		return
	end
	if mouse == nil then
		return
	end

	local mx, my = unpack(flow.items[1].mouse)
	local dx = mx - mouse.x
	local dy = my - mouse.y
	mouse.x = mx
	mouse.y = my

	local graphics = flow.items[1].item

	local strength = function(x, y)
		local rx = x - mx
		local ry = y - my
		local l = math.sqrt(rx * rx + ry * ry)
		return math.min(1 / math.pow(l / toolRadius, 2), 1)
	end

	graphics:warp(tools[tool].newKernel(strength, dx, dy, mx, my))
end

function love.filedropped(file)
	file:open("r")
	local svg = file:read()
	file:close()
	load(svg)
	attribution = ""
end
