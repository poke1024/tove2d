-- TÖVE Editor.
-- (C) 2018 Bernhard Liebl, MIT license.

local Transform = {}
Transform.__index = Transform

function Transform:update()
	local transform = self._transform
	local sx, sy = self.sx, self.sy

	local stretch = transform.stretch
	stretch:reset()
	stretch:scale(sx, sy)
	stretch:apply(transform.rest)

	local q1, q2 = stretch:transformPoint(-self.ox, -self.oy)
	local draw = transform.draw
	draw:reset()
	draw:translate(self.tx, self.ty)
	draw:rotate(self.r)
	draw:translate(q1, q2)

	local full = transform.full
	full:reset()
	full:translate(self.tx, self.ty)
	full:rotate(self.r)
	full:apply(stretch)
	full:translate(-self.ox, -self.oy)
end

function Transform:inverseTransformPoint(x, y)
	return self._transform.full:inverseTransformPoint(x, y)
end

function Transform:inverseUnscaledTransformPoint(x, y)
	return self._transform.draw:inverseTransformPoint(x, y)
end

function Transform:get(type)
	return self._transform[type]
end

function Transform:apply()
end

function Transform:values()
	return {
		tx = self.tx,
		ty = self.ty,
		r = self.r,
		sx = self.sx,
		sy = self.sy,
		ox = self.ox,
		oy = self.oy,
		rest = self._transform.rest:clone()
	}
end

local pi = math.pi

local function mod_distance(x, y)
	local z = math.fmod(math.fmod(x, y) + y, y)
	return math.min(math.abs(z - y), math.abs(z))
end

local function mod_near(x, y)
	return mod_distance(math.abs(x - y), 2 * pi) < 0.01
end

local function decompose2x2(phi0, rest, a, b, c, d)
	local sqrt, pow = math.sqrt, math.pow

	local q = sqrt(pow(b - c, 2) + pow(a + d, 2))

	local pa = (c*(-b + c) + a*(a + d)) / q
	local pb = (a*b + c*d) / q
	local pc = (a*b + c*d) / q
	local pd = (b*b - b*c + d*(a + d)) / q

	local va = (a + d) / q
	--local vb = (b - c) / q
	local vc = (-b + c) / q
	--local vd = (a + d) / q

	local phi = math.atan2(vc, va)
	local sx = sqrt(pa * pa + pb * pb)
	local sy = sqrt(pc * pc + pd * pd)

	if mod_near(phi, phi0 + pi) or mod_near(phi, phi0 - pi) then
		sx, sy = -sx, -sy
		phi = phi0
	end

	local cosphi = math.cos(phi)
	local sinphi = math.sin(phi)

	local ra = (a*cosphi + c*sinphi)/sx
	local rb = (b*cosphi + d*sinphi)/sx
	local rc = (c*cosphi - a*sinphi)/sy
	local rd = (d*cosphi - b*sinphi)/sy

	if ra < 1e-5 and rb < 1e-5 then
		sx, ra, rb = -sx, -ra, -rb
	end
	if rc < 1e-5 and rd < 1e-5 then
		sy, rc, rd = -sy, -rc, -rd
	end

	rest:setMatrix(
		ra, rb, 0, 0,
		rc, rd, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1)

	return phi, sx, sy
end

function Transform:setMatrix(a, b, c, d, e, f, ref)
	ref = ref or self
	local rest = self._transform.rest
	local phi, sx, sy = decompose2x2(ref.r, rest, a, b, d, e)

	local ox, oy = ref.ox, ref.oy
	local tx = c + a * ox + b * oy
	local ty = f + d * ox + e * oy

	self.tx = tx
	self.ty = ty
	self.r = phi
	self.sx = sx
	self.sy = sy
	self.ox = ox
	self.oy = oy
end

function Transform:set(t)
	self.tx = t.tx
	self.ty = t.ty
	self.r = t.r
	self.sx = t.sx
	self.sy = t.sy
	self.ox = t.ox
	self.oy = t.oy

	for _, name in ipairs({"full", "draw", "rest", "stretch"}) do
		self._transform[name]:setMatrix(
			t._transform[name]:getMatrix())
	end
end

function Transform:compose(t)
	local combined = self:get("full")

	combined:translate(t.tx, t.ty)
	combined:rotate(t.r)
	combined:scale(t.sx, t.sy)
	combined:apply(t.rest or t._transform.rest)
	combined:translate(-t.ox, -t.oy)

	local a, b, _, c, d, e, _, f = combined:getMatrix()
	self:setMatrix(a, b, c, d, e, f, t)
end

function Transform:serialize()
	return {
		tx = self.tx, ty = self.ty,
		r = self.r,
		sx = self.sx, sy = self.sy,
		ox = self.ox, oy = self.oy,
		rest = {self._transform.rest:getMatrix()}}
end

local function newTransform(arg1, arg2)
	local tx, ty

	if type(arg1) == "table" then
		tx = 0
		ty = 0
	else
		tx = arg1
		ty = arg2
	end

	local t = setmetatable({
		tx = tx,
		ty = ty,
		r = 0,
		sx = 1,
		sy = 1,
		ox = 0,
		oy = 0,
		_transform = {
			full = love.math.newTransform(),
			draw = love.math.newTransform(),
			rest = love.math.newTransform(),
			stretch = love.math.newTransform()
		}
	}, Transform)

	if type(arg1) == "table" then
		t.tx = arg1.tx
		t.ty = arg1.ty
		t.r = arg1.r
		t.sx = arg1.sx
		t.sy = arg1.sy
		t.ox = arg1.ox
		t.oy = arg1.oy
		t._transform.rest:setMatrix(unpack(arg1.rest))
	end

	t:update()
	return t
end

return newTransform
