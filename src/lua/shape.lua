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

-- A thin API for retained drawing.
-- Inspired by <a href="EaselJS">https://www.createjs.com/docs/easeljs/modules/EaselJS.html</a>.

--- @module stage

--- A container which allows you to draw a couple of things with one draw call.
-- @type Container
-- @set sort=true

local g = love.graphics

local Container = {}
Container.__index = Container

--- x position
-- @number x

--- y position
-- @number y

--- rotation in radians
-- @number r

--- x scale factor
-- @number sx

--- y scale factor
-- @number sy

--- Create new container.
-- @treturn Container new empty container
tove.newContainer = function()
	return setmetatable({x = 0, y = 0, r = 0, sx = 1, sy = 1,
		_children = {}}, Container)
end

--- Draw container and its children.
-- @usage
-- container.x = 20
-- container.y = 30
-- container:draw()
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

--- Add child(ren).
-- @usage
-- c:addChild(someGraphics)
-- @tparam Drawable c1 first child to add
-- @tparam[opt] Drawable... c more children to add
function Container:addChild(...)
	for _, child in ipairs({...}) do
		table.insert(self._children, child)
	end
end

--- Add child(ren) at position.
-- @usage
-- c:addChildAt(otherContainer, someGraphics, 7)
-- @tparam Drawable c1 first child to add
-- @tparam[opt] Drawable... c more children to add
-- @tparam int index where to add children
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

--- Remove child(ren).
-- @usage
-- c:removeChild(someGraphics)
-- @tparam Drawable c1 first child to remove
-- @tparam[opt] Drawable... c more children to remove
function Container:removeChild(...)
	for _, child in ipairs({...}) do
		remove(self._children, child)
	end
end

--- Remove child(ren) at position.
-- @usage
-- c:removeChildAt(2, 7)
-- @tparam int i1 index of first child to remove
-- @tparam[opt] int... c indices of more children to remove

function Container:removeChildAt(...)
	local j = {...}
	table.sort(j, function(a, b) return a > b end)
	for _, i in ipairs(j) do
		table.remove(self._children, i)
	end
end

--- Move child to position.
-- @tparam Drawable child child to move
-- @tparam int index index to move the child to
function Container:setChildIndex(child, index)
	self:removeChild(child)
	self:addChildAt(child, index)
end

--- @type Stage

--- Create new stage.
-- @treturn Container empty stage

tove.newStage = function()
	return tove.newContainer()
end


--- @type Shape

local Shape = {}
Shape.__index = Shape

---@number x x position

---@number y y position

---@number rotation rotation in radians

---@number sx x scale factor

---@number sy y scale factor

--- Create new shape.
-- @tparam Graphics graphics for drawing this shape
-- @treturn Shape new shape

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

--- Draw.
-- @usage
-- shape = tove.newShape(someGraphics)
-- shape.x = 20
-- shape.y = 30
-- shape.r = 0.1 -- rotation
-- shape:draw() -- no need to track x, y and r here
-- @see Graphics:draw

function Shape:draw()
	self.graphics:draw(self.x, self.y, self.r, self.sx, self.sy)
end

--- Set display mode.
-- Change display mode of underlying @{Graphics}.
-- @tparam string mode display moede
-- @tparam[opt] ... args additional args
-- @see Graphics:setDisplay

function Shape:setDisplay(...)
	self.graphics:setDisplay(...)
end

function Shape:setFillColor(...)
	for _, path in self._graphics.paths do
		path:setFillColor(...)
	end
end

return Shape
