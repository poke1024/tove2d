-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local Panel = require "ui/panel"
local VBox = require "ui/vbox"
local ImageButton = require "ui/imagebutton"
local RadioGroup = require "ui/radiogroup"

local Toolbar = {}
Toolbar.__index = Toolbar
setmetatable(Toolbar, {__index = Panel})

local items = {
    "thirdparty/cursor",
    "thirdparty/location",
    "thirdparty/pencil"
}

function Toolbar:select(name)
    self.group:select(name)
end

function Toolbar:current()
    return self.group:current()
end

Toolbar.new = function(callback)
    local toolbar = Panel.init(setmetatable({}, Toolbar))
    local buttons = ImageButton.group(toolbar, unpack(items))
    local group = RadioGroup.new(unpack(buttons))
    group:setCallback(callback)
    toolbar.group = group
    return toolbar
end

return Toolbar
