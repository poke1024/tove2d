-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local Demo = {}
Demo.__index = Demo

function Demo:draw()
    for _, o in ipairs(self.items) do
        o.graphics:draw(o.x, o.y, o.r, o.s, o.s)
    end

    local status = "fps: " .. tostring(love.timer.getFPS()) ..
        "\nstartup time: " .. string.format("%.2f", self.startupTime) .. "s"

    love.graphics.print(status, 10, 10)
end

function Demo:update(dt)
    local w = love.graphics.getWidth()
	local h = love.graphics.getHeight()
    for _, o in ipairs(self.items) do
        o.x = o.x + dt * o.vx
        o.y = o.y + dt * o.vy
        if o.x < 0 and o.vx < 0 then
            o.vx = -o.vx
        elseif o.x > w and o.vx > 0 then
            o.vx = -o.vx
        end
        if o.y < 0 and o.vy < 0 then
            o.vy = -o.vy
        elseif o.y > h and o.vy > 0 then
            o.vy = -o.vy
        end
        o.s = o.s + o.vs
        if o.s < 0.1 and o.vs < 0 then
            o.vs = -o.vs
        end
        if o.s > 1 and o.vs > 0 then
            o.vs = -o.vs
        end
    end
end

function Demo:mousepressed(x, y, button, clickCount)
end

function Demo:mousereleased(x, y, button)
end

function Demo:keypressed(key, scancode, isrepeat)
    if key == "a" then
        self.animateShapes = not self.animateShapes
    end
end

function Demo:wheelmoved(x, y)
end

function Demo:filedropped(file)
end

function Demo:setObjects(objects)
    local t0 = love.timer.getTime()

    local items = {}
    local w = love.graphics.getWidth()
	local h = love.graphics.getHeight()
    local ndup = 10
    for _, o in ipairs(objects) do
        for i = 1, ndup do
            local graphics = o.scaledgraphics:clone()
            table.insert(items, {
                graphics = graphics,
                x = math.random() * w,
                y = math.random() * h,
                r = math.random(),
                s = 0.1 + 0.9 * math.random(),
                vx = (math.random() - 0.5) * 1000,
                vy = (math.random() - 0.5) * 1000,
                vr = math.random() * 0.5,
                vs = math.random() * 0.01
            })
            graphics:cache()
        end
    end

    self.startupTime = love.timer.getTime() - t0

    self.items = items
end

return setmetatable({
    items = {},
    startupTime = 0,
    animateShapes = false}, Demo)
