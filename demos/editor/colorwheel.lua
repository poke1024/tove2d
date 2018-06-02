return function(callback)
	-- https://love2d.org/wiki/HSV_color
	local function hsv(h, s, v)
	    if s <= 0 then return v,v,v end
	    if h < 0 then
	    	h = h + 1
	    end
	    h, s, v = h*6, s, v
	    local c = v*s
	    local x = (1-math.abs((h%2)-1))*c
	    local m,r,g,b = (v-c), 0,0,0
	    if h < 1     then r,g,b = c,x,0
	    elseif h < 2 then r,g,b = x,c,0
	    elseif h < 3 then r,g,b = 0,c,x
	    elseif h < 4 then r,g,b = 0,x,c
	    elseif h < 5 then r,g,b = x,0,c
	    else              r,g,b = c,0,x
	    end return (r+m),(g+m),(b+m)
	end

	local function barycentric(px, py, x1, y1, x2, y2, x3, y3)
		local dx = px - x3
		local dy = py - y3
		local denom = (y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3)
		local u = ((y2 - y3) * dx + (x3 - x2) * dy) / denom
		local v = ((y3 - y1) * dx + (x1 - x3) * dy) / denom
		local w = 1.0 - u - v
		return u, v, w
	end

	local oversample = 1

	local code = [[
	#define M_PI 3.1415926535897932384626433832795

	uniform float radius;
	uniform float thickness;

	// https://github.com/hughsk/glsl-hsv2rgb
	vec3 hsv2rgb(vec3 c) {
		vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
		vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
		return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
	}

	vec4 effect(vec4 _, Image texture, vec2 texture_coords, vec2 screen_coords){
		float d = length(texture_coords);

		float phi = atan(texture_coords.y, texture_coords.x) / (M_PI * 2);
		vec3 color = hsv2rgb(vec3(phi, 1, 1));
		
		float alpha = smoothstep(radius - 1, radius + 1, d) *
			(1.0f - smoothstep(radius + thickness - 1, radius + thickness + 1, d));

		return vec4(color, alpha);
	}
	]]

	local radius = 60
	local thickness = 15
	local halfsize = radius + thickness
	local size = halfsize * 2

	local shader = love.graphics.newShader(code)
	shader:send("radius", radius)
	shader:send("thickness", thickness)

	local attributes = {{"VertexPosition", "float", 2}, {"VertexTexCoord", "float", 2}}

	local pixelsize = size * oversample

	local vertices = {
		{ 0, 0, -halfsize, -halfsize },
		{ pixelsize, 0, halfsize, -halfsize },
		{ pixelsize, pixelsize, halfsize, halfsize },
		{ 0, pixelsize, -halfsize, halfsize }
	}

	local mesh = love.graphics.newMesh(attributes, vertices, "fan")

	canvas = love.graphics.newCanvas(pixelsize, pixelsize)
	love.graphics.setCanvas(canvas)
    love.graphics.clear()
	love.graphics.setShader(shader)
	love.graphics.draw(mesh)
	love.graphics.setShader()
	love.graphics.setCanvas()

	canvas:setFilter("linear", "linear")

	local scale = 1 / oversample
	local pi2 = 2 * math.pi

	local currentHue = 0

	local function computeTriangleVertices()
		local phi = currentHue
		local phi1 = phi + 2 * math.pi * (1 / 3)
		local phi2 = phi + 2 * math.pi * (2 / 3)

		return {
			{ radius * math.cos(phi), radius * math.sin(phi), hsv(phi / pi2, 1, 1) },
			{ radius * math.cos(phi1), radius * math.sin(phi1), 1, 1, 1 },
			{ radius * math.cos(phi2), radius * math.sin(phi2), 0, 0, 0 }
		}
	end

	local triangle
	local triangleVertices
	local triangleSpot = {1, 0, 0}
	do
		local attributes = {{"VertexPosition", "float", 2}, {"VertexColor", "float", 3}}
		triangleVertices = computeTriangleVertices()
		triangle = love.graphics.newMesh(attributes, triangleVertices, "fan", "dynamic")
	end

	circle = tove.newGraphics()
	circle:drawCircle(0, 0, thickness / 3)
	circle:setLineColor(0, 0, 0)
	circle:setLineWidth(0.9)
	circle:stroke()

	local x, y = love.graphics.getWidth() - halfsize, halfsize

	local function colorChanged()
		local color = {}
		for j = 1, 3 do
			local v = 0
			for i = 1, 3 do
				v = v + triangleSpot[i] * triangleVertices[i][3 + j - 1]
			end
			color[j] = v
		end

		callback(unpack(color))
	end

	local function updateHue(mx, my)
		local dx = mx - x
		local dy = my - y
		currentHue = math.atan2(dy, dx)
		triangleVertices = computeTriangleVertices()
		triangle:setVertices(triangleVertices)
		colorChanged()
	end

	local function updateSL(mx, my, mode)
		local ax, ay = unpack(triangleVertices[1])
		local bx, by = unpack(triangleVertices[2])
		local cx, cy = unpack(triangleVertices[3])
		local u, v, w = barycentric(mx - x, my - y, ax, ay, bx, by, cx, cy)
		if mode ~= "check" then
			u = math.max(u, 0)
			v = math.max(v, 0)
			w = math.max(w, 0)
			local s = u + v + w
			u = u / s
			v = v / s
			w = w / s
		end
		if u < 0 or v < 0 or w < 0 then
			return false
		end
		triangleSpot = {u, v, w}
		colorChanged()
		return true
	end

	colorChanged()

	return {
		draw = function()
			love.graphics.setColor(1, 1, 1, 1)
			love.graphics.setBlendMode("alpha", "premultiplied")
			love.graphics.draw(canvas, x - halfsize, y - halfsize, 0, scale, scale)
			love.graphics.setBlendMode("alpha")
			love.graphics.draw(triangle, x, y)

			local r = radius + thickness / 2
			circle:draw(x + math.cos(currentHue) * r, y + math.sin(currentHue) * r)

			local ax, ay = unpack(triangleVertices[1])
			local bx, by = unpack(triangleVertices[2])
			local cx, cy = unpack(triangleVertices[3])
			local u, v, w = unpack(triangleSpot)
			circle:draw(x + ax * u + bx * v + cx * w, y + ay * u + by * v + cy * w)
		end,
		click = function(mx, my)
			local dx = mx - x
			local dy = my - y
			local d = math.sqrt(dx * dx + dy * dy)
			local r = radius
			if d >= r and d <= r + thickness then
				updateHue(mx, my)
				return updateHue
			else
				if updateSL(mx, my, "check") then
					return updateSL
				end
				return nil
			end
		end
	}
end
