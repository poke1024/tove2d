-- *****************************************************************
-- TÖVE - Animated vector graphics for LÖVE.
-- https://github.com/poke1024/tove2d
--
-- Copyright (c) 2018, Bernhard Liebl
--
-- Distributed under the MIT license. See LICENSE file for details.
--
-- All rights reserved.
-- *****************************************************************

-- a basic retained API - inspired by EaselJS.

local g = love.graphics

local Container = {}
Container.__index = Container

tove.newContainer = function()
	return setmetatable({x = 0, y = 0, r = 0, sx = 1, sy = 1,
		_children = {}}, Container)
end

function Container:draw()
	if next(self._children) ~= nil then
		g.push("transform")

		g.translate(self.x, self.y)
		g.rotate(self.r)
		g.scale(self.sx, self.sy)

		for _, child in ipairs(self._children) do
			child:draw()
		end

		g.pop()
	end
end

function Container:addChild(...)
	for _, child in ipairs({...}) do
		table.insert(self._children, child)
	end
end

function Container:addChildAt(...)
	local args = {...}
	local at = args[#p]
	args[#p] = nil
	for i, child in ipairs(args) do
		table.insert(self._children, at + i - 1, child)
	end
end

local function remove(t, x)
	for i, y in ipairs(t) do
		if x == y then
			table.remove(t, i)
			break
		end
	end
end

function Container:removeChild(...)
	for _, child in ipairs({...}) do
		remove(self._children, child)
	end
end

function Container:removeChildAt(...)
	for _, i in ipairs({...}) do
		table.remove(self._children, i)
	end
end

function Container:setChildIndex(child, index)
	self:removeChild(child, index)
	self:addChildAt(child, index)
end

tove.newStage = function()
	return tove.newContainer()
end


local Shape = {}
Shape.__index = Shape

tove.newShape = function(graphics)
	if graphics == nil then
		graphics = tove.newGraphics()
	end
	return setmetatable({graphics = graphics, x = 0, y = 0,
		r = 0, sx = 1, sy = 1, animation = nil}, Shape)
end

function Shape:animate(t)
	if self.animation ~= nil then
		if not self.animation:animate(self, t) then
			self.animation = nil
		end
	end

	for _, child in ipairs(self.children) do
		child:animate(t)
	end
end

function Shape:draw()
	self.graphics:draw(self.x, self.y, self.r, self.sx, self.sy)
end

function Shape:setDisplay(...)
	self.graphics:setDisplay(...)
end

function Shape:setFillColor(...)
	for _, path in self._graphics.paths do
		path:setFillColor(...)
	end
end

return Shape
