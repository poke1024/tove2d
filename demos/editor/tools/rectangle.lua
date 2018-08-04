-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local RectangleTool = {}
RectangleTool.__index = RectangleTool

function RectangleTool:_create()
    local object = self.editor:newObjectWithPath("fill")
    self._object = object
    self._subpath = object.graphics.paths[1].subpaths[1]
end

function RectangleTool:draw(gtransform)
end

local function isCommandDown()
    return love.keyboard.isDown("lgui") or
        love.keyboard.isDown("rgui")
end

function RectangleTool:mousedown(gx, gy, gs, button)
    self:_create()

    local subpath = self._subpath
    local x0, y0 = gx, gy
    self._rectangle = subpath:drawRect(0, 0, 1, 1)
    self._rectangle.w = 0
    self._rectangle.h = 0

    local modify = function(rx, ry)
        self._rectangle.w = rx * 2
        self._rectangle.h = ry * 2
        self._rectangle:commit()
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

function RectangleTool:click(gx, gy, gs, button)
end

function RectangleTool:mousereleased()
    local rectangle = self._rectangle
    if rectangle ~= nil and rectangle.w == 0 and rectangle.h == 0 then
        self.editor:removeObject(self._object)
    end
end

function RectangleTool:keypressed(key)
    return false
end

return function(editor)
	return setmetatable({
		handles = editor.handles,
        editor = editor,
		_object = nil,
        _subpath = nil,
        _rectangle = nil}, RectangleTool)
end
