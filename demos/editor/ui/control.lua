-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local Control = {}
Control.__index = Control

function Control:setBounds(x, y, w, h)
    local p = self.padding
	self.x, self.y, self.w, self.h = x + p, y + p, w - 2 * p, h - 2 * p
	self:layout()
end

function Control:click(x, y)
    return nil
end

function Control:layout()
end

function Control:init()
    self.padding = 8
    return self
end

function Control.new()
    return Control.init(setmetatable({}, Control))
end

return Control
