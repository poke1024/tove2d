-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local Control = require "ui/control"

local HBox = {}
HBox.__index = HBox
setmetatable(HBox, {__index = Control})

function HBox:empty()
	self.items = {}
	self.layoutDirty = true
end

function HBox:add(item)
	table.insert(self.items, item)
end

function HBox:layout()
	if not self.layoutDirty then
		return
	end
	local x, y, h = self.x, self.y, self.h
	for _, item in ipairs(self.items) do
		local w = item:getOptimalWidth() + 2 * item.xpad
		item:setBounds(x, y, w, h)
		x = x + w
	end
	self.actualWidth = x - self.x
	self.layoutDirty = false
end

function HBox:draw()
	self:layout()
	if self.frame then
		love.graphics.setColor(0.5, 0.5, 0.5)
		love.graphics.rectangle(
			"line", self.x, self.y, self.actualWidth, self.h)
		love.graphics.setColor(1, 1, 1)
	end
	for _, item in ipairs(self.items) do
		item:draw()
	end
end

function HBox:click(x, y)
    for _, item in ipairs(self.items) do
		local dragfunc = item:click(x, y)
        if dragfunc ~= nil then
            return dragfunc
        end
	end
    return nil
end

function HBox:getOptimalWidth()
	local w = 0
	for _, item in ipairs(self.items) do
		w = w + item:getOptimalWidth()
	end
	return w
end

function HBox:getOptimalHeight()
    local h = 0
	for _, item in ipairs(self.items) do
		h = math.max(h, item:getOptimalHeight())
	end
	return h
end

function HBox:init()
	self.items = {}
	self.frame = false
	self.layoutDirty = true
    return Control.init(self)
end

function HBox.new()
    return HBox.init(setmetatable({}, HBox))
end

return HBox
