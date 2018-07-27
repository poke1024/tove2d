-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local boxy = require "boxy"

local function loadImage(name, mode)
    local svg = love.filesystem.read("images/" .. name .. ".svg")
    local graphics = tove.newGraphics(svg, 16)
    if mode == "monochrome" then
        graphics:setMonochrome(1, 1, 1)
    end
    local imageData = graphics:rasterize("default")
    local image = love.graphics.newImage(imageData)
    image:setFilter("linear", "linear")
    return image
end

local function makeButtons(paths, mode, attach)
    local buttons = {}
    for _, path in ipairs(paths) do
        local image = loadImage(path, mode)
        local button = boxy.PushButton(
            image, 16 / math.max(image:getWidth(), image:getHeight()))
        button:configure(4, 16)
        table.insert(buttons, button)

        local name
        for n in string.gmatch(path, "[^/]+$") do
    		name = n
    	end

        attach(name, button)
    end

    return buttons
end

return {
    loadImage = loadImage,
    makeButtons = makeButtons
}
