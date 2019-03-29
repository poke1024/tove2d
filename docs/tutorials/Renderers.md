# Setting Renderers
This section should give you some idea of why there are three different renderers in TÖVE, what they do, and when you want to change things.

##  The renderers
TÖVE provides three renderers to bring vector graphics to the screen: `texture`, `mesh` and `shader`:

* `texture` is the default renderer. It gives high quality and high performance for static images that don't need animation or scaling. Internally, this is NanoSVG rendering to a texture.
*  `mesh` uses a triangle mesh to produce a tessellated version of the graphics, which is then drawn using a LÖVE Mesh. Scaling will not produce pixelation, but crisp straight edges. Using this renderer,  curves and colors can be animated efficiently.
* `shader` is a purely shader-based renderer that gives high quality at different resolutions. It's a very complex shader and highly experimental. It allows for efficient animation of curves and colors.

You can tell TÖVE which renderer to use by calling `Graphics:setDisplay(mode, quality)` on your `Graphics` instance, e.g. `myDrawing:setDisplay("mesh")`. The next call to `Graphics:draw()` will then honor that setting. The optional `quality` parameter is a render-dependent number that allows you to configure the level of detail for some renderers.

Let's try this out. With the curved triangle from the Getting Started section, let's set a mesh renderer before drawing:

```
 myDrawing:setDisplay("mesh")

function love.draw()
	myDrawing:draw()
end
```

![](images/tutorial/triangle-mesh-fine.jpg)

You might notice that the bottom right edge looks a bit edgy now. That's due to the mesh's default quality setting. To see that we're really dealing with a mesh, let's increase that effect by telling TÖVE to use even less triangles:

```
 myDrawing:setDisplay("mesh", 100) -- specify the target resolution in pixels to define tesselation detail
```

![](images/tutorial/mesh-low.jpg)

One of the strengths of TÖVE is that choosing the display renderer is independent of building your geometry and colors.

##  Which renderer is right for me?
Here are some hints:

`texture` gives great quality, but is slow for re-rendering and scaling via transforms will produce pixelation, unless you re-render for a higher resolution. You can tell TÖVE at which resolution to render internally by using `Graphics:setResolution` (this won't affect the display size, but you'll see that a resolution of `2` will allow you to scale the image by a factor of 2 without seeing pixelation).

`mesh` is the best all-round solution if you need to dynamically scale your graphics. Once you determine a detail level that matches your requirements (in terms of scaling and zoom), you're basically dealing with a LÖVE Mesh, which is efficient to draw, scale and animate.

`shader` can give excellent results in terms of quality and performance in some situations. Then again, it can be very expensive in terms of shader performance and it has some issues with numerical stability (computing cubic roots in a shader using 16 bit floating point numbers is not only potty but indeed has its limits).

## Setting the mesh renderer quality
The quality setting in `Graphics:setDisplay`  lets you change the level of the detail the mesh renderer tessellates its mesh. There are two meanings of this values, depending on whether your points are `static` or `dynamic` (see the section on Animating Things).

* If your points are `static` (default), the quality is a number between 0 and 1, where 1 is the highest suggested quality (though you can go above 1). The renderer will produce an adaptive mesh tessellation, where some curves get many points and others (e.g. straight lines) few. Values below 0.1 trigger a special low quality tier that tries to produce a minimum of triangles.
* If your points are `dynamic` (for animation), the quality is also a number between 0 and 1, but 0 now corresponds to 0 fixed subdivisions per curve, whereas 1 corresponds to a fixed subdivision of 16 segments per curve. In short: your quality is no longer adaptive.
* In addition, you can always force fixed subdivision by specifying `tove.fixed(n)` as quality: each curve will then be segmented into exactly 2^n parts. Use this if you don't want adaptive subdivision for static point meshes.

The second factor to mention here is the value set via `Graphics:setResolution`. TÖVE internally scales meshes before applying the quality settings. That means that setting a higher resolution will yield a higher quality with the same quality number. Imagine rendering the same image at a higher resolution and then scaling it down again for display. This is the effect you get by calling `setResolution` on meshes.

## The mesh renderer and gradients
The mesh renderer automatically detects if you are using gradients or not. If not, it internally uses a flat mesh of vertices and colors. If you use gradients, it will use a shader implementation for the coloring.
