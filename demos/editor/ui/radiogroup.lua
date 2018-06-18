-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local RadioGroup = {}

local RadioGroup = {}
RadioGroup.__index = RadioGroup

function RadioGroup:setCallback(callback)
    self.callback = callback
end

function RadioGroup:select(name)
    for _, button in ipairs(self.buttons) do
        local selected = button.name == name
        button.selected = selected
        if selected and self.callback ~= nil then
            self.callback(button)
        end
    end
end

function RadioGroup.new(...)
    local self = {buttons = {...}}
    local buttons = self.buttons
    buttons[1].selected = true
    local callback = function(clicked)
        for _, b in ipairs(buttons) do
            if b ~= clicked then
                b.selected = false
            end
        end
        if self.callback ~= nil then
            self.callback(clicked)
        end
    end
    for _, b in ipairs(buttons) do
        b:setCallback(callback)
    end
    return setmetatable(self, RadioGroup)
end

return RadioGroup
