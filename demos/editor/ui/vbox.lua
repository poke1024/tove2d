-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local Control = require "ui/control"

local VBox = {}
VBox.__index = VBox
setmetatable(VBox, {__index = Control})

function VBox:empty()
	self.items = {}
	self.layoutDirty = true
end

function VBox:add(item)
	table.insert(self.items, item)
end

function VBox:layout()
	if not self.layoutDirty then
		return
	end
	local x, y, w = self.x, self.y, self.w
	for _, item in ipairs(self.items) do
		local h = item:getOptimalHeight() + 2 * item.padding
		item:setBounds(x, y, w, h)
		y = y + h
	end
	self.actualHeight = y - self.y
	self.layoutDirty = false
end

function VBox:draw()
	self:layout()
	if self.frame then
		love.graphics.setColor(0.5, 0.5, 0.5)
		love.graphics.rectangle(
			"line", self.x, self.y, self.w, self.actualHeight)
		love.graphics.setColor(1, 1, 1)
	end
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

function VBox:getOptimalHeight()
	local h = 0
	for _, item in ipairs(self.items) do
		h = h + item:getOptimalHeight()
	end
	return h
end

function VBox:init()
	self.items = {}
	self.frame = false
	self.layoutDirty = true
    return Control.init(self)
end

function VBox.new()
    return VBox.init(setmetatable({}, VBox))
end

return VBox
