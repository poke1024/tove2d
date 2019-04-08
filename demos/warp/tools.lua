tools = {
	{
		name = "Move",
		newKernel = function(strength, dx, dy)
			return function(x, y)
				local s = strength(x, y)
				return x + dx * s, y + dy * s
			end
		end
	},
	{
		name = "Rounder",
		newKernel = function(strength, dx, dy)
			return function(x, y, curvature)
				local s = strength(x, y)
				return x, y, math.min(0.5, curvature * (1 + s * 0.1))
			end
		end
	},
	{
		name = "Sharper",
		newKernel = function(strength, dx, dy)
			return function(x, y, curvature)
				local s = strength(x, y)
				return x, y, math.max(0.01, curvature / (1 + s * 0.1))
			end
		end
	},
	{
		name = "Larger",
		newKernel = function(strength, dx, dy, mx, my)
			return function(x, y)
				local s = strength(x, y)
				local rx = x - mx
				local ry = y - my
				local rl = math.sqrt(rx * rx + ry * ry)
				s = (1 * s) / rl
				return x + rx * s, y + ry * s
			end
		end
	},
	{
		name = "Smaller",
		newKernel = function(strength, dx, dy, mx, my)
			return function(x, y)
				local s = strength(x, y)
				local rx = x - mx
				local ry = y - my
				local rl = math.sqrt(rx * rx + ry * ry)
				s = (1 * s) / rl
				return x - rx * s, y - ry * s
			end
		end
	}
}

function drawToolsMenu(tool)
	local width = love.graphics.getWidth()
	local height = love.graphics.getHeight()

	love.graphics.setFont(tovedemo.smallFont)

	local y = 10
	for i, t in ipairs(tools) do
		if i == tool then
			love.graphics.setColor(0.9, 0.5, 0.5, 0.7)
		else
			love.graphics.setColor(0.9, 0.9, 0.9, 0.7)
		end
		love.graphics.rectangle("fill", width - 100, y, 150 - 20, 20, 5, 5, 5)
		love.graphics.setColor(0, 0, 0, 0.7)
		love.graphics.print(t.name, width - 100 + 3, y - 3)
		y = y + 30
	end
end

function clickToolsMenu(mx, my)
	local width = love.graphics.getWidth()
	local height = love.graphics.getHeight()

	if mx >= width - 100 and mx <= width then
		local y = 10
		for i, t in ipairs(tools) do
			if my >= y and my <= y + 20 then
				return i
			end
			y = y + 30
		end
	end
end

function getToolRecord(tool)
    return tools[tool]
end
