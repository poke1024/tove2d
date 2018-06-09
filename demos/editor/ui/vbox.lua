-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local Control = require "ui/control"

local VBox = {}
VBox.__index = VBox
setmetatable(VBox, {__index = Control})

function VBox:add(item)
	table.insert(self.items, item)
end

function VBox:layout()
	local x, y, w = self.x, self.y, self.w
	for _, item in ipairs(self.items) do
		local h = item:getOptimalHeight() + 2 * item.padding
		item:setBounds(x, y, w, h)
		y = y + h
	end
end

function VBox:draw()
	for _, item in ipairs(self.items) do
		item:draw()
	end
end

function VBox:click(x, y)
    for _, item in ipairs(self.items) do
		local dragfunc = item:click(x, y)
        if dragfunc ~= nil then
            return dragfunc
        end
	end
    return nil
end

function VBox:init()
    Control.init(self)
    self.items = {}
    return self
end

return VBox
