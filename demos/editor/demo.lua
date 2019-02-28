-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local Demo = {}
Demo.__index = Demo

function Demo:draw()
    love.graphics.push("transform")
	love.graphics.applyTransform(self.transform)
    for _, o in ipairs(self.items) do
        o.graphics:draw(o.x, o.y, o.r, o.s, o.s)
    end
    love.graphics.pop()

    if self.measureDraw == 1 then
        -- some things liker shaders are only compiled on the first draw
        -- in some drivers; measure that time here as startup time.
        self.startupTime = love.timer.getTime() - self.startupTime0
        self.measureDraw = 2
    elseif self.measureDraw < 1 then
        self.measureDraw = self.measureDraw + 1
    end

    local status = "num objects:" .. tostring(#self.items) ..
        "\nfps: " .. tostring(love.timer.getFPS()) ..
        "\nstartup time: " .. string.format("%.2f", self.startupTime) .. "s" ..
        "\nanimation: " .. tostring(self.animation) ..
        "\n" ..
        "\n+: more items" ..
        "\n-: less items" ..
        "\na: toggle animation" ..
        "\nmousewheel: zoom"

    love.graphics.print(status, 10, 10)
end

function Demo:update(dt)
    local w = love.graphics.getWidth()
	local h = love.graphics.getHeight()
    local t = love.timer.getTime()

    local animation = self.animation
    dt = math.min(dt, 0.1)

    local anim1 = math.sin(t) * 100
    local anim2 = math.sin(t * 0.4) * 100

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
        if animation then
            local pt = o.graphics.paths[1].subpaths[1].points[1]
            pt.x = o.points[1].x + anim1
            pt.y = o.points[1].y + anim2
        end
    end
end

function Demo:mousepressed(x, y, button, clickCount)
end

function Demo:mousereleased(x, y, button)
end

function Demo:keypressed(key, scancode, isrepeat)
    if key == "a" then
        self.animation = not self.animation
    end
    if key == "+" or key == "kp+" then
        self:addObjects()
    end
    if key == "-" or key == "kp-" then
        self:removeObjects()
    end
end

function Demo:wheelmoved(wx, wy)
    local x = love.mouse.getX()
	local y = love.mouse.getY()
	local lx, ly = self.transform:inverseTransformPoint(x, y)
	self.transform:translate(lx, ly)
	self.transform:scale(1 + wy / 10)
	self.transform:translate(-lx, -ly)
end

function Demo:filedropped(file)
end

function Demo:addObjects()
    local items = self.items
    local w = love.graphics.getWidth()
	local h = love.graphics.getHeight()

    for _, o in ipairs(self.prototypes) do
        for i = 1, self.duplicates do
            local graphics = o:clone()

            local traj = graphics.paths[1].subpaths[1]
            local pts = traj.points
            local newpts = {}
            for j = 1, traj.points.count do
                local pt = pts[j]
                newpts[j] = {x = pt.x, y = pt.y}
            end

            table.insert(items, {
                graphics = graphics,
                x = math.random() * w,
                y = math.random() * h,
                r = math.random(),
                s = 0.1 + 0.9 * math.random(),
                vx = (math.random() - 0.5) * 1000,
                vy = (math.random() - 0.5) * 1000,
                vr = math.random() * 0.5,
                vs = math.random() * 0.01,
                points = newpts
            })
            graphics:cache()
        end
    end
end

function Demo:removeObjects()
    if #self.items > self.duplicates then
        for i = 1, self.duplicates do
            table.remove(self.items, #self.items)
        end
    end
end

function Demo:setObjects(objects)
    local g = nil
    for _, o in ipairs(objects) do
        local og = o.scaledGraphics
        if g == nil then
            g = og:clone() -- clone display settings
            g:clear()
        end
        for i = 1, og.paths.count do
            local path = tove.newPath()
            local t = o.transform
            path:set(tove.transformed(
                og.paths[i], t.tx, t.ty, t.r, t.sx, t.sy, t.ox, o.oy), true)
            g:addPath(path)
        end
    end

    self.prototypes = {}
    table.insert(self.prototypes, g)

    self.startupTime0 = love.timer.getTime()

    self.items = {}
    self:addObjects()

    self.measureDraw = 0
end

return setmetatable({
    transform = love.math.newTransform(),
    items = {},
    duplicates = 10,
    startupTime = 0,
    animation = false,
    startupTime0 = 0,
    measureDraw = 2}, Demo)
