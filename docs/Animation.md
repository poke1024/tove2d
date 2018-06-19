# Animating Things
TÖVE is designed for animating stuff. Its focus is not on static vector graphics rendering, but on animated vector graphics for games.

## Animating with the Stage API
Many animations can be achieved by moving, rotating and scaling certain fixed parts of a graphics. TÖVE offers the retained `Stage` API for simplifying these kind of animations.

At the core is the `Shape` object. Each `Shape` holds a `Graphics` you can access. Each `Shape` knows about its own position, rotation and scale. You can change these at any time and  they will stay this way across frames until you change them again. So if you have objects on the screen and only want to move some of them, `Shape` might simplify your code.

For drawing, `Shapes` need to added to a `Stage`. Here's an example:

```
local stage = tove.newStage()

local shape = tove.newShape()
shape.graphics:drawCircle(0, 0, 100)
shape.graphics:setFillColor(0.7, 0.2, 0.3)
shape.graphics:fill()

-- add this shape to the stage. from now on
-- it will get rendered with the stage.
stage:addChild(shape)

function love.draw()
	-- change the position of our shape.
	shape.x = love.mouse.getX()
	shape.y = love.mouse.getY()

	stage:draw()
end
```

## Flipbooks and Animations
Let's say you have a couple of SVGs that you want to play back like some sort of sprite sheet. TÖVE has two kinds of API to support you with this.

First, there are flipbooks. These are prerendered distinct frames. Think of a sprite atlas being played back.

Seconds, there are animations. These are interpolated in real time, i.e. they are not prerendered. Note that for these, TÖVE expects all frames to have exactly the same path layout (same number of trajectories and points across all frames).

The "blob" demo has code for both animation styles.

### Tweens

For both flipbooks and animations, the first thing to do is to create a tween that gathers all the keyframes and durations, much like a timeline in an animation program.

Tweens can be created from `Graphics` or from SVG text data like this:

```
svg1 = love.filesystem.read("frame1.svg")
svg2 = love.filesystem.read("frame2.svg")

tween = tove.newTween(svg1):to(svg2, 1)
```

The code above creates a tween of two frames, where the second frame happens one second after the other. You can add more frames by calling `to`. So,  `tove.newTween(svg1):to(svg2, 1):to(svg3, 5)` would create a tween that adds an additional frame 5 seconds after the second.

`to` takes an optional third parameter which allows you to ease the transition. It expects a one-parameter Lua function that maps (0, 1).

### Flipbooks

Flipbooks consist of distinct prerendered frames. That means flipbooks have high quality but only a discrete time resolution.

Creating a flipbook is easy:

`f = tove.newFlipbook(8, tween)`

This creates a flipbook with 8 frames per second, i.e. it prerenders the tween at this frame rate. You can animate this flipbook by calling `f:draw()` and by setting `f.t` for its current time (in tween time, i.e. this is not a frame number).

 You can also specify what kind of renderer you want to use for the distinct frames. `bitmap` is the default, but you can also choose `mesh` which will prerender your frames as adaptive meshes. You can also specify quality settings as optional follow-up parameters.

### Animations

Animations are frames interpolated and rendered in realtime. That means animations have a continuous time resolution but can also be expensive in terms of rendering.

Animations are created like this:

`a = tove.newAnimation(tween, "mesh")`

As with flipbooks, you can use `a:draw()` to draw the current frame, and `a.t` to set the time.

### Looping

By default, animations and flipbooks interpolate as if they bounce forth and back. If you want to look your animation, make sure to transition to the first frame at the end of the tween. This indicates to TÖVE that you want a continuous interpolation from first to last:

```
tween = tove.newTween(svg1):to(svg2, 1):to(svg1, 1)
```

### A Technical Note

If you're using animations with a mesh renderer, TÖVE will use a fixed resolution flattening and try to precalculate all triangulations. That means  that the quality will be lower than for adaptive meshes, but things should be fast.

## Procedural Animation Features
Sometimes you want even more flexibility for some kind of procedural animation:

* freely animate colors and gradients of a path in realtime
* freely animate a path by modifying its control points in realtime

TÖVE can do this. Take a look at the "hearts" demo to see working code. Here are some hints:

The first step to animate colors or points is to tell your `Graphics` instance about it: call `Graphics:setUsage` to animate `points` (the shape of your drawing) or `colors` (the fill or stroke color of your drawing) or both. 

```
myDrawing:setUsage("points", "dynamic") -- animate points
myDrawing:setUsage("colors", "dynamic") -- animate colors
```

Without doing this, TÖVE will not be able to animate your objects efficiently and things will be slow.

Please note that TÖVE does not efficiently support animations that add or delete points.

## Animating colors
Animating colors is easy. Here are two examples:

### Animating by re-setting the color

To animate the triangle from the Getting Started section, you could do this:

```
function love.draw()
	local t = love.timer.getTime() -- current time
	local r, g, b = math.abs(math.sin(t)), 0.4, 0.2
	myDrawing.paths[1]:setFillColor(tove.newColor(r, g, b))
end
```

Note that we do not call `setFillColor` on the `Graphics` as that would only change the next new path we fill using `Graphics:fill()`.  As we actively want to change the color of an already existing  path, we need to grab that path using `myDrawing.paths[1]`.

Of course, if you'd like the line color to change, you'd call `setLineColor` instead.

### Animating by changing the color

A slightly different approach is changing the color itself:

```
local myColor = tove.newColor()
myDrawing.paths[1]:setFillColor(myColor)

function love.draw()
	local r = math.abs(math.sin(love.timer.getTime()))
	myColor:set(r, 0.4, 0.2)
```

RGB colors can be read and changed by either `set(r, g, b, a)` and `r, g, b, a = get()` or directly via attributes, e.g. `someColor.r = 0.3`.

So the code above could also be written as:

```
local myColor = tove.newColor(0.0, 0.4, 0.2)
myDrawing.paths[1]:setFillColor(myColor)

function love.draw()
	myColor.r = math.abs(math.sin(love.timer.getTime()))
```

### A note on dependencies

TÖVE internally assumes that each object only has exactly one owner. For example, a color can only belong to one path; you cannot assign the same color to two paths, and then change it for both paths.

As soon as you assign the same color to two paths, TÖVE will internally clone one of the colors.

So what will happen in the case above is that only one path (the one with the uncloned color) would change its color.

The same holds true for trajectories and paths, and paths and graphics. Keep that in mind when working with many different objects.

## Animating shapes with Commands
Most drawing calls come return a command, that allows you to animate what you have just drawn.

An example:

```
local shape = tove.newGraphics()
shape.moveTo(10, 10)
local myLineTo = shape.lineTo(100, 100)
shape.moveTo(-50, 200)
```

At any point later in your code, you can now modify the `lineTo` command:

```
myLineTo.x = 200
```

This has the same effect as if you'd recreated the whole graphics with the modified `lineTo`,  but it's much faster - using mutable commands, TÖVE will animate stuff very efficiently (for example, for the mesh renderer, this boils down to a `Mesh:setVertices` call in LÖVE).

The attribute names are `x` and `y` for `moveTo` and `lineTo`. For `curveTo` you also have `cp1x`, `cp1y`, `cp2x`, `cp2y`. You can read and write those properties.

##  Directly Accessing Paths and Curves
Using  Command  is what you want to animate one path most of the time. Sometimes though want to have a broader access to your curve data. For example you might want to morph whole parts of a path. TÖVE let's you do this.

Let's first talk about how TÖVE stores your curves and how it names things:

* A `Graphics` is a collection of `Path`s
* A `Path` is a collection of `Subpath`s (plus fill and stroke styles etc.)
* A `Subpath` consists of `Curve`s
* A `Curve` is a cubic bezier curve and consists of four control points

With this scheme, you can access paths, subpaths and curves (all indices are 1-based):

```
local myCurve = myGraphics.paths[1].subpaths[2].curves[5]
```

Once you have picked a curve, you can modify its points by accessing its attributes `x0`, `y0`,  `cp1x`, `cp1y`, `cp2x`, `cp2y`, `x`, `y` (the latter 6 correspond to the parameters you'd pass to `Graphics:curveTo`). This works for reading and writing.

For example, to animate the x target point of curve 1 of some graphics using your mouse, you might do this:

```
function love.draw()
	myDrawing.paths[1].subpaths[1].curves[1].x = love.mouse.getX()
	myDrawing:Draw()
end
```

If you don't want to access curves, but the underlying points of all the subpath's curves as one continuous array, you can do it like this:

```
 myGraphics.paths[1].subpaths[2].points[3].x
 myGraphics.paths[1].subpaths[2].points[3].y
```

To find out how many paths, subpaths, curves or points you have, you can use the `count` attribute, e.g.:

```
myDrawing.paths.count -- number of paths
myDrawing.paths[1].subpaths.count -- number of subpaths in path 1
myDrawing.paths[1].subpaths[1].points.count -- number of points in subpath 1 in path 1
``` 

## Mesh Animation Caveats
If you're using the `mesh` renderer for animation, there are two specific things that can cause problems:

* Clean, oriented curves. Usually, things should just work. If not, here are some hints.

TÖVE's automatically deals with obvious cases of point duplication (also during animation). Furthermore in dynamic points mesh mode, the default is to ignore orientation (i.e. no holes), which means your curve can be oriented in the wrong way and you will still see it.

There might be situations where you want to clean or reorient your curves. TÖVE offers two functions. Note that cleaning will get you into problems when animating paths that need to have exactly the same point layout.

```
myGraphics:clean()
myGraphics:setOrientation("ccw")
```

* Triangulations.  TÖVE's animation system has a dynamic triangulation monitor that check whether it's current triangulation is still good or needs updating (which will usually happen due to sub shapes changing from convex to concave forms or vice versa). TÖVE also caches triangulations and tries to reuse them. Still, with procedural animation and complete freedom, you might end up in situations where many retriangulations happen, which is expensive on the CPU.

You can disable automatic retriangulation by calling `myDrawing:setUsage("triangles", "static")`  after calling `myDrawing:setUsage("points", "dynamic")` (as the latter automatically always sets `triangles` to `dynamic`). This means TÖVE will stick with one triangulation (which can result in visual errors).  If you do this, be sure to use `graphics:cache()` to pick one good triangulation at the beginning that you define as suitable for all animation frames.
