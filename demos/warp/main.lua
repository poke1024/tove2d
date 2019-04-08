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

local currentTool = 1
local toolRadius = 20

local frames = {}
local currentFrame = 1
local animation = nil

local function load(svg)
	local graphics = tove.newGraphics(svg, 400)

	if true then
		graphics:setDisplay("texture", "fast")
		graphics:setResolution(500 / 400)
	else
		graphics:setDisplay("mesh", "rigid", 3, "none")
		graphics:setUsage("points", "dynamic")
	end

	frames[currentFrame] = graphics

	flow = tovedemo.newCoverFlow(0.5)
	local r = flow:add("", graphics)
	r.minsize = 500
	r.maxsize = 500
end

load(love.filesystem.read("assets/monster_2_by_mike_mac.svg"))

function play()
	if animation ~= nil then
		animation = nil
		flow.items[1].item = frames[currentFrame]
		return
	end

	local tween

	local n = 0
	for k, frame in pairs(frames) do
		n = math.max(n, k)
	end

	if n > 0 then
		for k, frame in pairs(frames) do
			if tween == nil then
				tween = tove.newTween(frame)
			else
				tween:to(frame, k / n)
			end
		end
	end

	animation = tove.newAnimation(tween, "mesh", "rigid", 3, "none")
	flow.items[1].item = animation
end

local function gotoFrame(i)
	local graphics = flow.items[1].item

	if frames[i] ~= nil then
		flow.items[1].item = frames[i]
	else
		graphics = graphics:clone()
		frames[i] = graphics
		flow.items[1].item = graphics	
	end

	currentFrame = i
end

function love.draw()
	tovedemo.draw("Warp.")
	tovedemo.attribution(attribution)
	flow:draw()

	if animation ~= nil then
		local speed = 3
		local t = math.abs(math.sin(love.timer.getTime() * speed))	
		animation.t = t
	else
		love.graphics.circle("line", love.mouse.getX(), love.mouse.getY(), toolRadius * 2)
		drawToolsMenu(currentTool, frames, currentFrame)
	end
end

function love.update(dt)
	flow:update(dt)
end

local mouse = nil

function love.mousepressed(x, y, button)
	if button == 1 then
		local what = clickToolsMenu(x, y)
		if what == nil then
			local mx, my = unpack(flow.items[1].mouse)
			mouse = {x = mx, y = my}
		elseif what[1] == "tool" then
			currentTool = what[2]
		elseif what[1] == "frame" then
			gotoFrame(what[2])
		elseif what[1] == "play" then
			play()
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

	graphics:warp(tools[currentTool].newKernel(strength, dx, dy, mx, my))
end

function love.keypressed(key, scancode, isrepeat)
	local i = string.byte(key) - string.byte("0")
	if i > 0 and i <= 9 then
		gotoFrame(i)
	end

	if key == "p" or key == "escape" then
		play()
	end
end

function love.filedropped(file)
	file:open("r")
	local svg = file:read()
	file:close()
	load(svg)
	attribution = ""
end
