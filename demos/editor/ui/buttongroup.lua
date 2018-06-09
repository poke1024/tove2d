-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local Control = require "ui/control"
local ImageButton = require "ui/imagebutton"

local ButtonGroup = {}
ButtonGroup.__index = ButtonGroup
setmetatable(ButtonGroup, {__index = Control})

function ButtonGroup:draw()
	for _, button in ipairs(self.buttons) do
		button:draw()
	end
end

function ButtonGroup:click(x, y)
	local sel = 0
	for i, button in ipairs(self.buttons) do
		if button:inside(x, y) then
			sel = i
			break
		end
	end
	if sel > 0 then
		for i, button in ipairs(self.buttons) do
			button.selected = i == sel
		end
		self.callback(self.names[sel])
        return function() end
	end
	return nil
end

function ButtonGroup:select(name)
	for i, n in ipairs(self.names) do
		self.buttons[i].selected = n == name
	end
end

function ButtonGroup:layout()
	local x, y, w = self.x, self.y, self.w
    x = x + self.padding
	for _, button in ipairs(self.buttons) do
		button:setBounds(x, y, 32, 32)
		x = x + 40
	end
end

function ButtonGroup:getOptimalHeight()
    local h = 0
    for _, button in ipairs(self.buttons) do
        h = math.max(h, button:getOptimalHeight())
    end
    return h
end

function ButtonGroup:setCallback(callback)
    self.callback = callback
end

ButtonGroup.new = function(...)
	local buttons = {}
	for _, name in ipairs {...} do
		local button = ImageButton.new(name)
		table.insert(buttons, button)
	end
	return Control.init(setmetatable(
		{callback = function() end, buttons = buttons,
        names = {...}}, ButtonGroup))
end

return ButtonGroup
