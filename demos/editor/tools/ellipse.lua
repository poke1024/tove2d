-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local EllipseTool = {}
EllipseTool.__index = EllipseTool

function EllipseTool:_create()
    local object = self.editor:newObjectWithPath("fill")
    self._object = object
    self._subpath = object.graphics.paths[1].subpaths[1]
end

function EllipseTool:draw(gtransform)
end

local function isCommandDown()
    return love.keyboard.isDown("lgui") or
        love.keyboard.isDown("rgui")
end

function EllipseTool:mousedown(gx, gy, gs, button)
    self:_create()

    local subpath = self._subpath
    local x0, y0 = gx, gy
    self._ellipse = subpath:drawEllipse(0, 0, 0, 0)

    local modify = function(rx, ry)
        self._ellipse.rx = rx
        self._ellipse.ry = ry
        self._ellipse:commit()
    end

    return function(x, y)
        local cx, cy = (x + x0) / 2, (y + y0) / 2
        local rx, ry = math.abs(x - cx), math.abs(y - cy)
        local tx, ty = cx, cy
        if isCommandDown() then
            tx, ty = cx - rx, cy - ry
            rx, ry = rx * 2, ry * 2
        end
        self._object:changePoints(modify, rx, ry)
        self._object.transform.tx = tx
        self._object.transform.ty = ty
        self._object.transform:update()
        self._object:refresh()
    end
end

function EllipseTool:click(gx, gy, gs, button)
end

function EllipseTool:mousereleased()
    local ellipse = self._ellipse
    if ellipse ~= nil and ellipse.rx == 0 and ellipse.ry == 0 then
        self.editor:removeObject(self._object)
    end
end

function EllipseTool:keypressed(key)
    return false
end

return function(editor)
	return setmetatable({
		handles = editor.handles,
        editor = editor,
		_object = nil,
        _subpath = nil,
        _ellipse = nil}, EllipseTool)
end
