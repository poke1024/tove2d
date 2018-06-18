-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local PathWidget = {}
PathWidget.__index = PathWidget

function PathWidget:create()
    if self.object ~= nil then
        return
    end

    local object = Object.new(0, 0, tove.newGraphics())
	table.insert(self.editor.objects, object)

    local traj = tove.newTrajectory()
    local path = tove.newPath()
    path:addTrajectory(traj)
    object.graphics:addPath(path)

    object.graphics.paths[1]:setLineColor(0, 0, 0)
    object.graphics.paths[1]:setLineWidth(1)

    self.object = object
    self.trajectory = object.graphics.paths[1].subpaths[1]
end

function PathWidget:draw(gtransform)
    if self.object == nil then
        return
    end

    local g = self.object.graphics
    local handles = self.handles

    local tfm = love.math.newTransform()
	tfm:apply(gtransform)
	tfm:apply(self.object.transform.mousetransform)

    for i = 1, g.paths.count do
        local path = g.paths[i]
        for j = 1, path.subpaths.count do
            local traj = path.subpaths[j]
            for k = 1, traj.points.count, 3 do
                local pt = traj.points[k]
                local x, y = tfm:transformPoint(pt.x, pt.y)
                local type = traj:isEdgeAt(k) and "edge" or "smooth"
                handles.knots.normal[type]:draw(x, y)
            end
        end
    end
end

function PathWidget:mousedown(gx, gy, gs, button)
    self:create()

    for i = 1, self.trajectory.points.count do
        local p = self.trajectory.points[i]
        --print(i, p.x, p.y, self.trajectory:isEdgeAt(i))
    end

    self.object:changePoints(function()
        if self.trajectory.points.count < 1 then
            self.trajectory:moveTo(gx, gy)
        else
            self.trajectory:lineTo(gx, gy)
        end
    end)

    self.object:refresh()

    return function() end
end

function PathWidget:click(gx, gy, gs, button)
end

function PathWidget:mousereleased()
end

function PathWidget:keypressed(key)
end

return function(editor)
	return setmetatable({
		handles = editor.handles,
        editor = editor,
		object = nil,
        trajectory = nil}, PathWidget)
end
