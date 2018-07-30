-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local boxy = require "boxy"
local class = require "boxy.class"
local utils = require "ui.utils"

local Toolbar = class("Toolbar", boxy.Panel)

function Toolbar:select(name)
    self.buttons[name]:setChecked(true)
end

function Toolbar:current()
    return self._current
end

function Toolbar:__init()
    boxy.Panel.__init(self)

    self.tool = boxy.Signal()
    self.operation = boxy.Signal()

    local tools = {
        "thirdparty/cursor",
        "thirdparty/location",
        "thirdparty/pencil",
        "gradient",
        "rectangle",
        "circle"
    }

    local operations = {
        {
            "add",
            "subtract"
        },
        {
            "thirdparty/up_arrow",
            "thirdparty/down_arrow"
        },
        {
            "thirdparty/eye",
            "thirdparty/download"
        }
    }

    self.buttons = {}
    local buttons = utils.makeButtons(
        tools, "monochrome",
        function(name, button)
            button:setMode("radio")
            button:onChecked(function()
                self._current = name
                self.tool:emit(name)
            end)
        self.buttons[name] = button
    end)
    buttons[1]:setChecked(true)

    for _, o in ipairs(operations) do
        table.insert(buttons,
            boxy.Spacer(8, "absolute"))

        utils.makeButtons(
            o, "monochrome",
            function(name, button)
                table.insert(buttons, button)
                button:onClicked(function()
                    self.operation:emit(name)
                end)
            end)
    end

    local groupBox = boxy.GroupBox()
    groupBox:setLayout(boxy.VBox(buttons):setMargins(4, 4, 4, 4))
    groupBox:setFrame(false)

    self:setLayout(boxy.VBox{ groupBox }:removeMargins())
end

return Toolbar
