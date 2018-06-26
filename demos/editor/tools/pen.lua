-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local PathWidget = {}
PathWidget.__index = PathWidget

function PathWidget:create()
    if self.object ~= nil then
        return
    end

    local object = self.editor:newObjectWithPath("line")
    self.object = object
    self.subpath = object.graphics.paths[1].subpaths[1]
end

function PathWidget:draw(gtransform)
    if self.object == nil then
        return
    end

    local g = self.object.graphics
    local handles = self.handles

    local tfm = love.math.newTransform()
	tfm:apply(gtransform)
	tfm:apply(self.object.transform:get("full"))

    for i = 1, g.paths.count do
        local path = g.paths[i]
        for j = 1, path.subpaths.count do
            local traj = path.subpaths[j]
            for k = 1, traj.points.count, 3 do
                local pt = traj.points[k]
                local x, y = tfm:transformPoint(pt.x, pt.y)
                local type = traj:isLineAt(k) and "edge" or "smooth"
                handles.knots.normal[type]:draw(x, y)
            end
        end
    end

    local dx, dy = self.dx, self.dy
    if dx ~= 0 or dy ~= 0 then
        local subpath = self.subpath
        local n = subpath.points.count
        local p = subpath.points[n]
        local px, py = p.x, p.y

        local x0, y0 = px - dx, py - dy
        local x1, y1 = px + dx, py + dy
        love.graphics.line(x0, y0, x1, y1)
        self.handles.selected:draw(x0, y0, 0, 0.75, 0.75)
        self.handles.selected:draw(x1, y1, 0, 0.75, 0.75)
    end
end

local function lengthSqr(dx, dy)
	return dx * dx + dy * dy
end

function PathWidget:_add(x, y)
    local subpath = self.subpath
    if subpath.points.count < 1 then
        subpath:moveTo(x, y)
    else
        local dx, dy = self.dx, self.dy
        if dx ~= 0 or dy ~= 0 then
            local p = subpath.points[subpath.points.count]
            local x0, y0 = p.x + dx, p.y + dy
            subpath:curveTo(x0, y0, (x0 + x) / 2, (y0 + y) / 2, x, y)
        else
            subpath:lineTo(x, y)
        end
    end
end

function PathWidget:mousedown(gx, gy, gs, button)
    self:create()

    local subpath = self.subpath
    local points = subpath.points
    local n = points.count

    local clickRadiusSqr = self.handles.clickRadiusSqr * gs * gs
    for i = 1, n, 3 do
        local px, py = points[i].x, points[i].y
        if lengthSqr(gx - px, gy - py) < clickRadiusSqr then
            if i == 1 then
                self:_add(px, py)
                subpath.isClosed = true
                self.object:refresh()
                return
            end
            return function() end
        end
    end

    if self.selected ~= 0 and not subpath:isLineAt(self.selected) then
    end

    self.object:changePoints(function()
        self:_add(gx, gy)
    end)
    self.object:refresh()

    n = points.count

    local p = points[n]
    local px, py = p.x, p.y
    self.dx, self.dy = 0, 0
    self.selected = n

    local modify = function(x, y)
        local dx, dy = x - px, y - py
        self.dx, self.dy = dx, dy
        subpath:move(n - 1, px - dx, py - dy)
    end

    return function(x, y)
        self.object:changePoints(modify, x, y)
        self.object:refresh()
    end
end

function PathWidget:click(gx, gy, gs, button)
end

function PathWidget:mousereleased()
end

function PathWidget:keypressed(key)
    return false
end

return function(editor)
	return setmetatable({
		handles = editor.handles,
        editor = editor,
		object = nil,
        subpath = nil,
        selected = 0,
        dx = 0,
        dy = 0}, PathWidget)
end
