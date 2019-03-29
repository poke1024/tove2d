All methods that take colors as parameters allow 5 different ways to specify colors.

# RGB

Use `r, g, b` to specify the RGB color `(r, g, b)`.

!!! tip ""
	`path:setFillColor(1, 0.5, 0.2)`

# RGBA

Use `r, g, b, a` to specify the RGB color `(r, g, b)` with alpha value `a`.

!!! tip ""
	`path:setFillColor(1, 0, 0.1, 0.5)`

# Hex String

!!! tip ""
	`path:setFillColor("#aabbcc")`

# Paint Object

use a preconstructed `Paint` object.

!!! tip ""
	```
	local p = tove.newPaint{type="solid", color={0.5, 0.2, 0.1}}
	path:setFillColor(p)
	```

# Nothing

`nil` indicates that no color (e.g. no fill) should be used.

!!! tip ""
	`path:setFillColor(nil)`
