-- TÖVE demo helpers.
-- (C) 2018 Bernhard Liebl, MIT license.

local function lerp(a, b, t)
	return a * (1 - t) + b * t
end

local font = love.graphics.newFont("assets/amatic/Amatic-Bold.ttf", 48)
local smallFont = love.graphics.newFont("assets/amatic/Amatic-Bold.ttf", 20)
local logo = love.graphics.newImage("assets/tovelogo.png", {mipmaps = true})
logo:setFilter("linear", "linear", 1)
logo:setMipmapFilter("linear")

love.window.setTitle("")

tovedemo = {}

function tovedemo.draw(title, help)
	local width = love.graphics.getWidth()
	local height = love.graphics.getHeight()
	love.graphics.setBackgroundColor(0.722, 0.898, 0.988)
	love.graphics.setColor(0, 0, 0, 0.2)
	love.graphics.rectangle("fill", 0, 0, width, height / 2)
	love.graphics.setColor(0, 0, 0, 0.5)
	love.graphics.setFont(font)
	local x0 = logo:getWidth() * 0.25 + 20
	love.graphics.print(title, x0, 20)

	if help ~= nil then
		love.graphics.setFont(smallFont)
		love.graphics.print(help, x0, 20 + 48)
		love.graphics.setFont(font)
	end

	love.graphics.setColor(1, 1, 1, 0.5)
	love.graphics.draw(logo, 10, 10, 0, 0.25, 0.25)
	love.graphics.setColor(1, 1, 1)
end

function tovedemo.newFont(size)
	return love.graphics.newFont("assets/amatic/Amatic-Bold.ttf", size)
end

-- CoverFlow

local CoverFlow = {}
CoverFlow.__index = CoverFlow

function tovedemo.newCoverFlow(scale)
	return setmetatable({items = {}, focus = 0, padding = 15, basescale = scale or 1}, CoverFlow)
end

function CoverFlow:add(name, item)
	local rec = {
		zoom = 0,
		minsize = 200,
		maxsize = 400,
		scale = 1,
		name = name,
		item = item
	}
	table.insert(self.items, rec)
	return rec
end

function CoverFlow:getFocus()
	return self.focus
end

function CoverFlow:getItem(i)
	return self.items[i].item
end

function CoverFlow:setName(i, name)
	self.items[i].name = name
end

function CoverFlow:draw()
	local mouseX = love.mouse.getX()
	local mouseY = love.mouse.getY()

	self.focus = 0

	local mid = love.graphics.getHeight() / 2

	local width = (#self.items - 1) * self.padding
	for _, item in ipairs(self.items) do
		width = width + item.scale * 200
	end

	local x = love.graphics.getWidth() / 2 - width / 2
	for i, item in ipairs(self.items) do
		local scale = item.scale
		local size = 200 * scale

		x = x + size / 2

		if mouseX > x - size / 2 and mouseX < x + size / 2 then
			if mouseY >= mid - size / 2 and mouseY <= mid + size / 2 then
				self.focus = i
			end
		end

		love.graphics.setColor(1, 1, 1)
		love.graphics.push()
		love.graphics.translate(x, love.graphics.getHeight() / 2)
		local s = (item.minsize + (item.maxsize - item.minsize) * (scale - 1)) / 200
		love.graphics.scale(s * self.basescale)
		item.item:draw(0, 0)
		love.graphics.pop()

		love.graphics.setColor(0.4, 0.6, 0.7)
		local name = item.name
		local textWidth = font:getWidth(name)
		love.graphics.print(name, x - textWidth / 2, mid + size / 2)

		x = x + size / 2
		x = x + self.padding
	end
end

function CoverFlow:update(dt)
	for i, item in ipairs(self.items) do
		if i == self.focus then
			item.zoom = lerp(item.zoom, 1, dt * 2)
		else
			item.zoom = lerp(item.zoom, 0, dt * 10)
		end
		item.scale = 1 + math.sin(item.zoom * math.pi / 2)
	end
end
