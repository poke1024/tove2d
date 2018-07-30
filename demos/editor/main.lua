-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

tove = require "tove"

local Object = require "object"

-- msaa = 4
love.window.setMode(800, 600, {highdpi = true})
love.window.setTitle("TÖVE miniedit")

tove.configure{
	highdpi = true,
	performancewarnings = false
}

local editor = require "editor"
local demo = require "demo"
local app = {
	module = editor
}

local screenwidth = love.graphics.getWidth()
local screenheight = love.graphics.getHeight()

local somegraphics = tove.newGraphics()
somegraphics:drawCircle(0, 0, screenwidth / 4)
somegraphics:setFillColor(0.8, 0.1, 0.1)
somegraphics:fill()
somegraphics:setLineColor(0, 0, 0)
somegraphics:stroke()

editor:addObject(Object.new(screenwidth / 2, screenheight / 2, somegraphics))

function love.load()
	editor:startup()
end

function love.draw()
	love.graphics.setBackgroundColor(0.7, 0.7, 0.7)
	love.graphics.setColor(1, 1, 1)

	app.module:draw()
end

function love.update(dt)
	app.module:update(dt)
end

function love.mousepressed(x, y, button, isTouch, clickCount)
	if app.module == demo then
		app.module = editor
	else
		app.module:mousepressed(x, y, button, clickCount)
	end
end

function love.mousereleased(x, y, button)
	app.module:mousereleased(x, y, button)
end

function enterDemoMode()
	demo:setObjects(editor.objects)
	app.module = demo
end

local function toggleDemoMode()
	if app.module == editor then
		enterDemoMode()
	else
		app.module = editor
	end
end

function love.keypressed(key, scancode, isrepeat)
	if app.module == demo and key == "escape" then
		app.module = editor
	elseif key == "d" then
		toggleDemoMode()
	end
	app.module:keypressed(key, scancode, isrepeat)
end

function love.wheelmoved(x, y)
	app.module:wheelmoved(x, y)
end

function love.filedropped(file)
	app.module:loadfile(file)
end

function love.quit()
	editor:save()
end
