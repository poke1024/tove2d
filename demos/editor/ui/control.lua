-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local Control = {}
Control.__index = Control

function Control:setBounds(x, y, w, h)
    local px, py = self.xpad, self.ypad
	self.x, self.y, self.w, self.h = x + px, y + py, w - 2 * px, h - 2 * py
	self:layout()
end

function Control:click(x, y)
    return nil
end

function Control:layout()
end

function Control:init()
    self.xpad = 8
    self.ypad = 8
    return self
end

function Control.new()
    return Control.init(setmetatable({}, Control))
end

return Control
