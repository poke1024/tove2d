-- TÖVE Demo: renderers.
-- (C) 2018 Bernhard Liebl, MIT license.

local tove = require "tove"

-- load graphics from svg file. TÖVE also supports
-- constructing vector graphics on the fly.

svgData = love.filesystem.read("player_vector.svg")
graphics = tove.newGraphics(svgData, 1000)

-- for rendering, choose among three renderers:
-- * "texture" will render into a bitmap
-- * "mesh" will tesselate into a mesh
-- * "gpux" will use a shader implementation

graphics:setDisplay("mesh", 200)

function love.draw()
	-- render svg at mouse position.

	x, y = love.mouse.getPosition()
	graphics:draw(x, y)
end



