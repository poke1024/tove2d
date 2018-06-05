-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

require "lib/tove"

local newObject = require "object"
local ui = require "ui"

-- msaa = 4
love.window.setMode(800, 600, {highdpi = true})

local editor = require "editor"

local screenwidth = love.graphics.getWidth()
local screenheight = love.graphics.getHeight()

local somegraphics = tove.newGraphics()
somegraphics:drawCircle(0, 0, screenwidth / 4)
somegraphics:setFillColor(0.8, 0.1, 0.1)
somegraphics:fill()
somegraphics:setLineColor(0, 0, 0)
somegraphics:stroke()


local radios = ui.newImageRadioButtonGroup(8, screenheight - 8 - 16,
	"bitmap", "mesh", "curves")
radios:select("mesh")


editor:addObject(newObject(screenwidth / 2, screenheight / 2, somegraphics))

function love.load()
	editor:load()
end

function love.draw()
	love.graphics.setBackgroundColor(0.7, 0.7, 0.7)
	love.graphics.setColor(1, 1, 1)

	editor:draw()

	if editor.widget ~= nil then
		radios:draw()
	end
end

function love.update()
	editor:update()
end

function love.mousepressed(x, y, button, isTouch, clickCount)
	local mode = radios:click(x, y)
	if mode ~= nil then
		editor:setDisplay(mode)
		return
	end

	editor:mousedown(x, y, button, clickCount)
end

function love.mousereleased(x, y, button)
	editor:mousereleased(x, y, button)
end

function love.keypressed(key, scancode, isrepeat)
	editor:keypressed(key, scancode, isrepeat)
end
