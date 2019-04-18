--- A cubic bezier curve.
-- @classmod Curve

--- x coordinate of curve point P0
-- @table x0

--- y coordinate of curve point P0
-- @table y0

--- x coordinate of first control point P1
-- @table cp1x

--- y coordinate of first control point P1
-- @table cp1y

--- x coordinate of second control point P2
-- @table cp2x

--- y coordinate of second control point P2
-- @table cp2y

--- x coordinate of curve point P3
-- @table x

--- y coordinate of curve point P3
-- @table y

--- Remove curve from subpath.
-- @function remove

--- Split curve into two curves.
-- @tparam number t location of split, 0 <= t <= 1
-- @function split
