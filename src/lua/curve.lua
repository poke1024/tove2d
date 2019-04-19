--- A cubic Bézier curve.
-- For details on Bézier curves, see <a href="https://en.wikipedia.org/wiki/B%C3%A9zier_curve">Bézier curves on Wikipedia</a>.
-- @classmod Curve

--- x coordinate of curve point P0
-- @table x0

--- y coordinate of curve point P0
-- @table y0

--- x coordinate of first control point P1
-- @usage
-- graphics.paths[1].subpaths[1].curves[1].cp1x = 10
-- @table cp1x

--- y coordinate of first control point P1
-- @usage
-- print(graphics.paths[1].subpaths[1].curves[1].cp1y)
-- @table cp1y

--- x coordinate of second control point P2
-- @table cp2x

--- y coordinate of second control point P2
-- @table cp2y

--- x coordinate of curve point P3
-- @table x

--- y coordinate of curve point P3
-- @table y

--- table of curve points.
-- Use this to access points in a curve by index.
-- Note that for consistency with Lua, the indices here
-- are one-based, even though traditional Bézier terminology
-- is 0-based. So keep in mind that
-- P0 = `p[1]`, P1 = `p[2]`, P2 = `p[3]`, P3 = `p[4]`.
-- @usage
-- print(graphics.paths[1].subpaths[1].curves[1].p[1].x) -- get P0.x
-- graphics.paths[1].subpaths[1].curves[1].p[4].y = 7 -- set P3.y
-- @table p

--- Remove curve from subpath.
-- @function remove

--- Split curve into two curves.
-- @tparam number t location of split, 0 <= t <= 1
-- @function split
