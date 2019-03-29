# Overview

`Graphics` is the main class for constructing and drawing things in TÖVE.

A `Graphics` object knows about

* the things it should draw, and
* the quality with which things should get drawn.

Each `Graphics` consists of a number of `Path`s.

# Construction

###	tove.newGraphics()

create an empty `Graphics` object for drawing into.

### tove.newGraphics(svg, [size:GraphicsSize])

create a `Graphics` from the given SVG string (the actual data, not the filename).

### tove.newGraphics({path_1, ..., path_n}, [size])

construct a `Graphics` from the given `Path` objects. `size` behaves as above.

# Methods

### clone() : Graphics

creates a deep copy of this `Graphics` object.

### clear()

remove all `Path`s from this `Graphics`.

### beginPath() : Path

start a new `Path` for drawing (without closing the current `Path` and making it a loop).

### closePath()

close the current `Path` (making it a loop). Ensuing drawing commands on this `Graphics` will open a new path.

### beginSubpath() : SubPath

inside the current `Path`, begin a new `Subpath` for drawing (without making the current `Subpath` a loop). Subsequent commands like `lineTo` will happen in that new `Subpath`.

### closeSubpath()

inside the current `Path`, close the current `Subpath` (making it a loop). Ensuing drawing commands on this `Graphics` will draw to the same `Path` as before, but in a new `Subpath`. Use this if you want to draw holes.

###  getCurrentPath() : Path

return the current `Path`.

### moveTo(x, y)

start a new `SubPath` and move the cursor to position `(x, y)`.

### lineTo(x, y)

starting at the current position, draw a line to `(x, y)`.

### curveTo(cpx1, cpy1, cpx2, cpy2, x, y)

starting at the current position, draw a cubic bezier curve to `(x, y)`, using `(cpx1, cpy1)` and `(cpx2, cpy2)` as control points.

### arc(x, y, r, startAngle, endAngle, counterclockwise)

draw an arc with radius `r` from `startAngle` to `endAngle` going clockwise or counterclockwise (if `counterclockwise` is `true`).

### addPath(path:Path)

append the given `path` to this `Graphics`.

### drawRect(x, y, w, h, [rx], [ry])

draw a rectangle with `(x, y)` as the top left corner and with width `w` and height `w`. `rx` and `ry` specify the roundedness of the corners.

### drawCircle(x, y, r)

draw a circle centered at `(x, y)` with radius `r`.

### drawEllipse(x, y, rx, ry)

draw an ellipse centered at `(x, y)` with semi axes `rx` and `ry`.

###  setFillColor([color...])

set the fill to the given [color](colors.md).

### setLineColor([color...])

set the line color to the given [color](colors.md).

### setLineDash(width_1, ..., width_n)

set the line dash to widths `width_1`, ..., `width_n`.

### setLineDashOffset(offset)

set the line dash offset to `offset`.

### setLineWidth(width)

set the line width to `width`.

### setMiterLimit(limit)

set the miter limit (length at which miters are turned into bezels) to `limit`.

### fill()

add a fill to the current path.

### stroke()

add a stroke (line) to the current path.

### computeAABB([precision:AABBPrecision]):x0, y0, x1, y1

compute axis-aligned bounding box.

### getWidth([precision:AABBPrecision])

get `Graphics`'s width (how wide it is when drawn with scale 1).

### getHeight([precision:AABBPrecision])

get `Graphics`'s height (how high it is when drawn with scale 1).

### setDisplay(mode:DisplayMode, [quality])

general form of the `setDisplay` method. see specific forms below.

### setDisplay("texture", [quality])

when drawing this `Graphics`, use a texture drawn through a single quad. note that this display mode does not take a quality parameter. if you want to change the texture resolution, use `setResolution`.

`quality` is either `"best"` (default) or `"fast"`.

### setDisplay("mesh", [tesselator...])

when rendering this `Graphics`, use a a mesh constructed using the specified [tesselator](tesselators.md). If no `tesselator` is given, a default one is picked based on the `Graphics` usage (see `setUsage`). If instead of a tesselator, a number is given, this number will specify a quality using the default tesselator for the configured usage.

### setDisplay("gpux", [lineShader], [lineQuality])

when rendering this `Graphics`, use an experimental GPU-based approach (rendering implicit surfaces using shaders). `lineShader` is either `"fragment"` or `"vertex"`. `lineQuality` is a number between 0 and 1.

### setUsage(what:UsageTarget, usage:Usage)

### setResolution(resolution)

indicates up to which scale this `Graphics` should look crisp when displaying it through a scale transform. 

For example, a `resolution` of `1` indicates that you intend to apply no scaling. A `resolution` of `3` indicates, that you intend to display the `Graphics` up to three times its normal size.

In case of `texture` displays, this will simply scale the underlying image by the given factor, giving you an unpixelated image at the desired scale factor. For `mesh`, the tesselation is applied to the blown up vector data.

Note that increasing `resolution` does not affect the output size when drawing. It only affects the underlying display quality.

### setOrientation(orientation:Orientation)

Ensure that the orientation of all `Subpath`s in this `Graphics` is the given `orientation`.

# Attributes

`paths`

gives you the `Path`s in this `Graphics` as a [collection](collections.md).

# Types

## GraphicsSize

* `"copy"`
* `"auto"` (default)
* `number`

## AABBPrecision

* `"low"`
* `"high"`

The latter computation will exactly estimate certain line extensions like miters, but it will be much slower.

## DisplayMode

* `"texture"`
* `"mesh"`
* `"gpux"`

## Orientation

* `"cw"` specifies clockwise orientation.
* `"ccw"` specifies counterclockwise orientation.



